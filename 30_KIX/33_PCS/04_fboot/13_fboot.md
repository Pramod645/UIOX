50_UIX/13_fboot/
├── include/
│   ├── uiox_fboot_types.h      # Error codes, phase IDs, timing structs,
│   │                           #   snapshot header, deferred-init descriptor
│   ├── uiox_fboot_timer.h      # High-resolution boot timer (ARM64 / RISC-V)
│   ├── uiox_fboot_snapshot.h   # Suspend-to-disk save / restore API
│   ├── uiox_fboot_defer.h      # Deferred / lazy driver init registry
│   └── uiox_fboot.h            # Master include + top-level pipeline API
└── src/
    ├── uiox_fboot_timer.c      # Counter init, ticks→µs, busy-wait
    ├── uiox_fboot_snapshot.c   # Probe, restore, capture, invalidate
    ├── uiox_fboot_defer.c      # Register, sort-by-priority, run-all
    └── uiox_fboot.c            # Phase begin/end/skip, report, syscalls
======================================================
Boot pipeline — cold vs. snapshot
Power-on
   │
   ├─ uiox_fb_init()             timer latched at reset
   │
   ├─[SNAPSHOT path]
   │   uiox_fb_snap_probe()  ──── valid? ──► uiox_fb_snap_restore()
   │                                              │ (does not return)
   │                                              └─► shell resumes
   │
   └─[COLD path]
       RESET → CLK_PLL → DDR_INIT → FW_VERIFY (→ 12_ksign)
            → DECOMPRESS → DEVTREE → EARLY_DRIVERS
            → FS_MOUNT → INIT_SPAWN → SHELL_READY
                                           │
                                 uiox_fb_report()   ← timing table
                                 uiox_fb_defer_run_all()  ← background
------------------------------------------------------------------------
13_fboot — Fast Boot Timing & Snapshot
Must be kernel — no question.

uiox_fb_init() is called at reset, latching a hardware timer — userspace doesn't exist yet
Manages boot snapshots (hibernate-style fast resume) — requires direct memory/storage access with no filesystem or process model yet running
uiox_fb_shell_ready() is a milestone marker called from kernel_main() — the call chain is entirely pre-userspace
Deferred-init scheduling runs inside the kernel boot path
Where it belongs: already correctly in 30_KIX build.
==========================================================


What fast boot and cold boot are
Two different things, and the 04_fboot module only fully owns one of them.

Cold boot is the full path from power-on: hardware runs, firmware runs, DDR is trained, the kernel image is decompressed and relocated, drivers probe, the filesystem mounts, PID 1 spawns, and eventually a shell appears. Every phase the module tracks — RESET → CLK_PLL → DDR_INIT → FW_VERIFY → DECOMPRESS → DEVTREE → EARLY_DRIVERS → FS_MOUNT → INIT_SPAWN → SHELL_READY — is a cold-boot phase.

Fast boot, in this design, is not a faster cold boot. It's avoiding it. On clean shutdown the kernel writes its own RAM image to a snapshot partition; on next power-on, Stage 0d finds a valid header, skips DDR training and decompression, copies the image straight back into RAM, and jumps to the saved resume vector. The snapshot header's own comment names the saving: DDR training ~300 ms + decompress ~150 ms + driver init ~400 ms ≈ 850 ms per boot.

So the relationship is: cold boot is the fallback, snapshot resume is the fast path, and uiox_fb_snap_probe() decides which one you take.

Is it for both warm and cold boot?
The code answers this more precisely than the docs do. There are four modes:

c


typedef enum {
    UIOX_FB_MODE_COLD        = 0,
    UIOX_FB_MODE_RESUME      = 1,
    UIOX_FB_MODE_SNAPSHOT    = 2,
    UIOX_FB_MODE_WARMRESET   = 3,
} uiox_fb_mode_t;
But only two of them have a code path. uiox_fb_init() takes a mode and stores it in ctx->timing.mode; the report prints it; nothing branches on it. The actual fork in the pipeline is snap_probe() at the call site in the docs — not the mode field.

Mode	Path in code	Notes
COLD	full phase sequence	the only path that populates the timing table
SNAPSHOT	probe → restore → never returns	no phases run, so no timings recorded
WARMRESET	none	declared, never distinguished
RESUME	none	declared, never distinguished
So: it covers cold boot and snapshot resume. Warm reset and (S3-style) resume are named but unbuilt — a warm reset in practice would fall through to the cold path with mostly-correct behaviour, since the phase functions check completed and some phases would legitimately skip. Worth knowing that's incidental rather than designed.

A second gap worth flagging: snapshot resume never calls the phase functions, so uiox_fb_report() after a resume prints a table of all-zero rows. That's not a bug exactly — recording ~0 ms is arguably the point — but the report makes no distinction between "we resumed, nothing ran" and "we crashed before anything ran", since both show ---- statuses.

The analysis you asked for
Sized and structured well. Four sources, five headers, clean layering: timer is a leaf, snapshot and defer both depend only on types, and the master aggregates. That's the same shape 05_sec ended at.

The three weak platform hooks are honest and clearly marked. uiox_fb_plat_read_counter has real ARM64 (cntpct_el0) and RISC-V (rdtime) implementations with a software fallback for host builds; the snapshot hooks are stubs with a comment saying what to replace them with.

But the timer's tick→µs conversion is worth a look:

c


return (ticks / t->freq_hz) * 1000000u
     + (ticks % t->freq_hz) * 1000000u / t->freq_hz;
The comment says "avoid 64-bit overflow", and it does avoid overflowing ticks * 1000000. But the middle term (ticks % freq) * 1000000 can itself overflow: with freq_hz = 24_000_000, the remainder is up to 24 million, times a million is 2.4×10¹³ — fine for 64-bit. With a 1 GHz counter it's 10¹⁵, still fine. It only breaks above ~18 THz. So the guard is sound for every plausible counter; the comment slightly oversells it, since the real risk was the first form and this correctly handles it.

The bigger design issue is uiox_fb_snap_restore():

c


/* 1. Read compressed image from partition */
uint8_t *comp_buf = (uint8_t *)(ctx->snap_part_base + sizeof(*h));

/* 2. Verify hash before touching RAM */
uiox_fb_plat_sha256(comp_buf, (size_t)h->image_size, actual_hash);
It computes the SHA-256 of image_size bytes starting at snap_part_base + 64 — with no check that image_size fits inside snap_part_size. A corrupt or hostile header with a large image_size reads past the partition, and the hash comparison that follows is over whatever memory happened to be there. That's the same class as the bounds guard I fixed in 05_sec's policy loader: the header is trusted for its own extent. Two lines fix it:

c


if (h->image_size > ctx->snap_part_size - sizeof(*h)) return UIOX_FB_ERR_IO;
if (h->raw_size   > ctx->ram_size)                    return UIOX_FB_ERR_IO;
The second matters equally — uiox_fb_plat_decompress is called with dst_cap = ctx->ram_size, and the stub checks src_len > dst_cap, but a real LZ4 decompressor writes according to the stored raw size, so raw_size needs validating against ram_size before the call, not after.

And the hash is FNV-1a, same as 05_sec had. uiox_fb_plat_sha256 is a 32-bit FNV-1a written into 4 of 32 bytes, with a comment saying "reuses uiox_ksign SHA-256 when linked together" — the identical phrasing uiox_mac_plat_sha256 carried. Now that 03_ksign lives at 33_PCS/03_ksign, both should delegate to uiox_ks_sha256. And the snapshot hash is doing real work here: it's what stands between a corrupt snapshot and an uncontrolled jump to *(resume_fn_t *)ram_base.

Minor: uiox_fb_phase_str returns names from a static array indexed by the enum, and checks p < UIOX_FB_PHASE__COUNT. Good. But uiox_fb_report uses a separate local mode_str[] array for modes and checks t->mode < 4u — two conventions for the same job, and the mode one duplicates what a uiox_fb_mode_str() helper would centralise. Not a defect; just the tree's recurring pattern.

Where it belongs
Your note says 30_KIX/33_PCS/04_fboot, and the directory listing at the bottom says 50_UIX/13_fboot — two different paths in the same message. The rationale you gave (runs at reset, before userspace; needs direct storage and memory access; called from kernel_main()) settles it: 30_KIX/33_PCS/04_fboot is right, and the 50_UIX/13_fboot header in your listing looks like a stale copy. Worth checking which one the umbrella Makefile actually builds.

What I'd want before proposing edits
Four of the nine files came through in full — uiox_fboot.c, uiox_fboot_snapshot.c, uiox_fboot_defer.c, uiox_fboot_types.h, plus uiox_fboot_timer.c and the five headers. What I have not seen is uiox_fboot_timer.h's companion pieces if any, and there's one thing the files assume that I can't verify:

c


#include "uiox_fw_types.h"   /* the fboot types header pulls this in */
04_fboot uses uiox_fw_types.h while 05_sec uses uiox_base_types.h — two type authorities under 10_BSP. Given what happened to the arm32 build when a module mixed UIOX_BASETYPES_COMPAT with <stdint.h>, that's worth confirming before anything is compiled:



ls /Users/pramodkumar/Hack/WS/UIOX/10_BSP/03_SoC/include/ | grep -i 'types'
grep -n 'uint8_t\|UIOX_BASETYPES_COMPAT\|stdint' /Users/pramodkumar/Hack/WS/UIOX/10_BSP/03_SoC/include/uiox_fw_types.h 2>/dev/null | head
If uiox_fw_types.h is a second, independently-written type header, then 05_sec and 04_fboot are using different vocabularies for the same primitives — the exact defect the handoff §6 describes, and it will bite the moment both are linked into one image.

Say the word and I'll write the two bounds checks and the SHA-256 delegation into uiox_fboot_snapshot.c — it's a self-contained file I've read in full, and the fixes are the same two I applied to the policy loader.



======
cp the five headers  → 30_KIX/33_PCS/04_fboot/include/
cp the four sources  → 30_KIX/33_PCS/04_fboot/src/
cp Makefile_fboot    → 30_KIX/33_PCS/
cd /Users/pramodkumar/Hack/WS/UIOX/30_KIX/33_PCS
make -f Makefile_fboot info
make -f Makefile_fboot arm64

