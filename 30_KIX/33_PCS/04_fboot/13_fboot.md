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