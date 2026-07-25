Key difference between the two builds:



                        Static	                                    Dynamic
BSP binary	            Linked into uiox_kernel.elf	                Standalone uiox_bsp.elf / .bin
Primary BL target	    Jumps directly to uiox_kernel_main	        Jumps to bsp_entry.S stub
Kernel loading	        Already in flash/RAM at link address	    BSP reads kernel ELF, copies segments to DRAM
BSP entry               point	uiox_bsp_init() called by kernel	uiox_bsp_entry_c() runs before kernel exists
Use case	            Tightly integrated SoC, small flash	        Separate bootloader partition, OTA updates
=================================================================================================================
── STATIC (current) ─────────────────────────────────────────────────
Primary BL (01_uBoot)
    │  jumps directly to
    ▼
uiox_kernel_main()          ← kernel ELF entry
    │  calls
    ▼
uiox_bsp_init()             ← BSP linked inside kernel
    ├─► arch_init()
    └─► uiox_soc_init()
    │
    ▼
kernel subsystems start
(33_PCS, 30_DeviceDrivers, …)


── DYNAMIC (needs wiring) ───────────────────────────────────────────
Primary BL (01_uBoot)
    │  loads uiox_bsp.bin to 0x48000000
    │  jumps to uiox_bsp_entry   ← bsp_entry.S
    ▼
uiox_bsp_entry_c(dtb_pa, args_pa)
    ├─► arch_init()
    ├─► uiox_soc_init()
    ├─► load_kernel_elf()        ← reads kernel ELF from flash/storage
    │       copies segments to DRAM
    │       parses ELF entry point
    └─► uiox_bsp_jump_to_kernel(entry, dtb_pa, args_pa)
            │  bare branch / jr / jmp
            ▼
        uiox_kernel_main()       ← kernel starts, BSP gone from memory
            │  NOW skips uiox_bsp_init() — hardware already set up
            ▼
        kernel subsystems start
============================================================================
Let me read the BSP structure first.
Let me read the key BSP files to give you a complete answer.
Now I have the full picture from the repo. Here's a thorough answer.

What the BSP already has

The BSP is already fully designed for both modes — the code is there, it's just not wired into the kernel build system yet. Here's what exists:

| Component | File | Purpose |
|---|---|---|
| BUILD=static → libbsp.a | BSP Makefile | Kernel links BSP as a static library |
| BUILD=dynamic → uioxbsp.elf + .bin | BSP Makefile | BSP is a standalone secondary bootloader binary |
| bspentry.S | Assembly stub | Sets stack, zeros BSS, calls uioxbspentryc() — all 4 arches |
| uioxbspentryc() | uioxbspmain.c | archinit → socinit → loadkernelelf → jump to kernel |
| uioxbspjumptokernel() | uioxbspmain.c | Final bare-metal branch to kernel entry — all 4 arches |
| bspdynamic.ld | Linker script | Loads BSP at 0x48000000, entry = uioxbspentry |
| uioxbootargst | uioxbsp.h | Struct the primary BL passes: kernel PA, DTB PA, memory map |

So the BSP itself knows exactly what to do in both modes. The gap is only in 30KIX/Makefile — it currently only consumes BSP in static mode and doesn't know how to build or depend on a dynamic BSP.

The two boot flows side by side

``
── STATIC (current) ─────────────────────────────────────────────────
Primary BL (01uBoot)
    │  jumps directly to
    ▼
uioxkernelmain()          ← kernel ELF entry
    │  calls
    ▼
uioxbspinit()             ← BSP linked inside kernel
    ├─► archinit()
    └─► uioxsocinit()
    │
    ▼
kernel subsystems start
(33PCS, 30DeviceDrivers, …)

── DYNAMIC (needs wiring) ───────────────────────────────────────────
Primary BL (01uBoot)
    │  loads uioxbsp.bin to 0x48000000
    │  jumps to uioxbspentry   ← bspentry.S
    ▼
uioxbspentryc(dtbpa, argspa)
    ├─► archinit()
    ├─► uioxsocinit()
    ├─► loadkernelelf()        ← reads kernel ELF from flash/storage
    │       copies segments to DRAM
    │       parses ELF entry point
    └─► uioxbspjumptokernel(entry, dtbpa, argspa)
            │  bare branch / jr / jmp
            ▼
        uioxkernelmain()       ← kernel starts, BSP gone from memory
            │  NOW skips uioxbspinit() — hardware already set up
            ▼
        kernel subsystems start
`

What needs to change in the kernel build system
30KIX/Makefile — detect and handle dynamic BSP

The kernel Makefile currently always passes BSPLIB=<path>/libbsp.a at link time. When BSP is dynamic, the kernel must not link libbsp.a and must not call uioxbspinit(). Add a BSPMODE variable:

`makefile
30KIX/Makefile  — add near the top
BSPMODE ?= static    # override with: make arm64 BSPMODE=dynamic

ifeq ($(BSPMODE),dynamic)
  # Dynamic: BSP is a separate binary, kernel gets no BSP lib
  BSPLIBARG  :=
  BSPMODEDEF := -DUIOXBSPDYNAMICBOOT
else
  # Static: BSP linked into kernel as before
  BSPLIBARG  := $(BSPDIR)/build/$(ARCH)/libbsp.a
  BSPMODEDEF :=
endif
`

Then in buildkernelmain pass $(BSPMODEDEF) to the compiler and use $(BSPLIBARG) at link time:

`makefile
buildkernelmain:
    @mkdir -p $(BUILDDIR)
    $(CC) $(KERNELCFLAGSBASE) $(ARCHCFLAGS) $(ARCHCPUDEF) \
        $(BSPMODEDEF) \
        -I$(ARCHINC) \
        -c $(MFDIR)uioxkernelmain.c \
        -o $(BUILDDIR)/uioxkernelmain.o

linkkernel:
    $(LD) -m $(ARCHLDEMUL) \
        -T $(MFDIR)linker/kernel$(ARCH).ld \
        --gc-sections \
        $(BUILDDIR)/uioxkernelmain.o \
        $(wildcard $(MFDIR)33PCS/build/$(ARCH)/.a) \
        ...other subsystem .a files... \
        $(BSPLIBARG) \          # ← empty when dynamic
        -o $(BUILDDIR)/uioxkernel.elf
`

uioxkernelmain.c — skip BSP init when booted dynamically

`c
int uioxkernelmain(void)
{
#if defined(UIOXBSPDYNAMICBOOT)
    /
      Dynamic boot: BSP already ran as secondary bootloader.
      archinit() and uioxsocinit() are already done.
      Hardware is ready — go straight to subsystem init.
     /
#else
    / Static boot: BSP is linked in, init it now /
    uioxbspconfigt cfg = {
        .flags = 0,
    };
    int rc = uioxbspinit(&cfg);
    if (rc != UIOXBSPOK) { / halt / }
#endif

    / Common path — subsystems always start here /
    uioxmminit(DRAMBASE, DRAMSIZE);
    uioxprocinit();
    / … rest of kernel init … /
}
`

Add a BSP dynamic build step in the umbrella Makefile

When BSPMODE=dynamic, build the BSP as a binary first, then build the kernel separately:

`makefile
30KIX/Makefile — add dynamic BSP build target
.PHONY: bsp-dynamic-arm64
bsp-dynamic-arm64:
    $(MAKE) -C $(BSPDIR) arm64 BUILD=dynamic

.PHONY: kernel-dynamic-arm64
kernel-dynamic-arm64: bsp-dynamic-arm64
    $(MAKE) -f $(MFDIR)Makefile doarm64 BSPMODE=dynamic
`

The outputs are then two separate binaries:
• 10BSP/build/arm64/uioxbsp.bin — loaded by primary BL to 0x48000000
• 30KIX/build/arm64/uioxkernel.elf — loaded by the BSP from flash to DRAM

Kernel linker script — entry point differs between modes

In static mode the kernel linker script entry is uioxkernelmain. In dynamic mode it's the same — but the load address must match what loadkernelelf() in the BSP copies the segments to. Make sure linker/kernelarm64.ld (and equivalents) use a KERNELLOADADDR that matches cfg.kernelloadpa in the BSP config:

`ld
/ linker/kernelarm64.ld /
KERNELLOADADDR = 0x40200000;   / must match uioxbspconfigt.kernelloadpa /
. = KERNELLOADADDR;
`

What is complete vs what is missing

| | Status |
|---|---|
| BSP static build (libbsp.a) | ✅ Complete and working |
| BSP dynamic build (uioxbsp.elf/.bin) | ✅ BSP side complete |
| bspentry.S for all 4 arches | ✅ Complete |
| uioxbspjumptokernel() for all 4 arches | ✅ Complete |
| loadkernelelf() in BSP | ✅ Present in uioxbspmain.c |
| 30KIX/Makefile BSPMODE variable | ❌ Missing — needs adding |
| uioxkernelmain.c UIOXBSPDYNAMICBOOT guard | ❌ Missing — needs adding |
| Kernel linker script load address alignment | ⚠️ Needs verification against BSP's kernelloadpa |
| uioxbootargst populated by primary BL (01uBoot) | ⚠️ Needs verification — primary BL must fill the struct before jumping to BSP |

The BSP is the most complete piece. The three-line Makefile change and the #ifdef guard in uioxkernel_main.c` are the only things standing between you and a working dynamic boot path.
------------    

The Makefile has the BSPMODE logic pasted at the top as a fragment but the actual buildkernelmain and linkkernel targets below still use the old $(BSPLIB) variable and missing $(BSPMODEDEF). It needs to be unified into one clean file.

uioxkernelmain.c has the #if defined(UIOXBSPDYNAMICBOOT) block pasted as a dead stub before the real #if defined(aarch64) chain — it references uioxbspinit() and uioxmminit() which aren't declared, and it sits outside any arch guard. The real fix is to weave the dynamic/static guard inside each arch's entry function, wrapping only the archinit() call.

Let me write both corrected files now.
Now writing the updated uioxkernelmain.c:
Here are both updated files, ready to drop in:

Makefile

uioxkernelmain.c

What changed and why
30KIX/Makefile — v2.1.0 → v2.2.0

Problem with the attached version: the BSPMODE logic and the patched buildkernelmain/linkkernel snippets were pasted as a separate block at the top — but the real targets lower in the file still used the old $(BSPLIB) variable (not $(BSPLIBARG)) and never passed $(BSPMODEDEF) to the compiler. There were also hard-tab vs space mixing issues that would cause make to error on the stubs.

Key fixes:

| | Before | After |
|---|---|---|
| BSPLIBARG / BSPMODEDEF | Defined at top but never used in real targets | Lazily expanded (= not :=) so $(ARCH) resolves correctly at use-time |
| buildkernelmain | Did not pass $(BSPMODEDEF) | Passes -DUIOXBSPDYNAMICBOOT when BSPMODE=dynamic |
| linkkernel | Used $(BSPLIB) (old variable) | Uses $(BSPLIBARG) — empty when dynamic, libbsp.a path when static |
| BSPMODE forwarding | Not passed to sub-makes | Every doxxx target forwards BSPMODE=$(BSPMODE) |
| Dynamic targets | Partial stubs with wrong indentation | Full bsp-dynamic-xxx + kernel-dynamic-xxx targets for all 4 arches |
| all-dynamic | Missing | Added — builds all 4 arches in dynamic mode |

uioxkernelmain.c — v1.0.0 → v1.1.0

Problem with the attached version: the #if defined(UIOXBSPDYNAMICBOOT) block was pasted as a dead stub before the real #if defined(aarch64) chain — it referenced uioxbspinit() and uioxmminit() which aren't declared in this TU, and it defined uioxkernelmain() twice (a redefinition error).

Key fixes:

| | Before | After |
|---|---|---|
| Duplicate uioxkernelmain() | Stub + real definition = redefinition error | Single definition per arch |
| uioxbspinit() call | In dead stub, undeclared | #include "uioxbsp.h" guarded by #if !defined(UIOXBSPDYNAMICBOOT) |
| archinit() skip | Not implemented | New kernelarchinit() wrapper — no-op with log message in dynamic mode, calls real archinit() in static mode |
| Repeated init sequence | Copy-pasted across all 4 arch blocks | Extracted into kernelcommoninit() — one place to maintain |
| Boot mode log | Silent | Each arch entry prints boot = dynamic / static so you can see which path ran |

Usage summary

``bash
Static build (default — BSP linked into kernel):
make arm64 LINK=yes

Dynamic build (BSP as secondary bootloader, kernel loaded separately):
make kernel-dynamic-arm64          # builds BSP binary + kernel ELF
make kernel-dynamic-riscv64

All arches, both modes:
make all                           # static
make all-dynamic                   # dynamic
`

Flash layout for dynamic mode:
`
0x48000000  →  10BSP/build/arm64/uioxbsp.bin     (primary BL loads this)
0x40200000  →  30KIX/build/arm64/uiox_kernel.elf  (BSP loads this from flash)
``