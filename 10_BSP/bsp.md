the secondary bootloader lives entirely inside 10_BSP.

10_BSP/soc/src/uiox_soc_main.c already is the secondary bootloader. Its 9-stage init pipeline is exactly what a secondary bootloader does.
=====================================================================================================================
Primary bootloader (SoC BootROM — NOT in your repo)
    └── loads → secondary bootloader → jumps to it
                        │
                        ▼
        10_BSP/soc/src/uiox_soc_main.c   ← secondary bootloader

The secondary bootloader adds three things that neither arch/ nor soc/ individually cover:



Responsibility	Where in 10_BSP	Not covered by
Boot pipeline sequencing — orchestrate all stages in the correct order	soc/src/uiox_soc_main.c	arch/ only does ISA init; soc/ files do individual subsystems
Kernel hand-off — load kernel image, pass DTB, jump to entry point	uiox_arch_main.c + uiox_soc_main.c	Neither arch nor soc individually transfer control to the kernel
Boot-time verification — POST, secure boot chain, PCR extension	soc/src/uiox_soc_post.c, uiox_soc_secboot.c	These exist in soc/ but are orchestrated by the pipeline

=====================================================================================================================
The complete secondary bootloader pipeline already in 10_BSP:
uiox_kernel_main(dtb_pa)         ← kernel calls this
    │
    └── uiox_arch_main(dtb_pa)   ← 10_BSP/uiox_arch_main.c
            │
            ├── arch_init()      ← 10_BSP/arch/<arch>/src/arch_init.c
            │       • CPU identify
            │       • cache enable
            │       • GIC/APIC/PLIC
            │       • VBAR/stvec/IDT
            │       • generic timer
            │       • IRQ handlers
            │
            └── uiox_soc_init()  ← 10_BSP/soc/src/uiox_soc_main.c
                    │
                    ├── Stage 0a: TrustZone / EL3 setup
                    ├── Stage 0b: PSCI registration
                    ├── Stage 0c: POST (memory walk, CPU sanity)
                    ├── Stage 0d: Secure boot verification
                    ├── Stage 0e: I2C init
                    ├── Stage 0f: SPI init
                    ├── Stage 0g: Watchdog init
                    ├── Stage 0h: DMA init
                    ├── Stage 0i: PCIe ECAM scan
                    ├── Stage 1:  SoC detect + chip-specific init
                    ├── Stage 2:  Memory map (DTB probe)
                    ├── Stage 3:  IRQ manager
                    ├── Stage 4:  System timer
                    ├── Stage 5:  GPIO
                    ├── Stage 6:  Storage (ramdisk / eMMC)
                    ├── Stage 7:  Device switch table
                    └── Stage 8:  Hand-off → kernel
=====================================================================================================================
What a traditional secondary bootloader (e.g. U-Boot) adds that UIOX does NOT need


Traditional U-Boot feature	UIOX equivalent / decision
FAT/ext4 filesystem read to load kernel from SD	Not needed — UIOX kernel is linked with the BSP or loaded by the SoC BootROM
TFTP / network boot	Not needed for embedded target
U-Boot shell / environment variables	Replaced by UIOX shell in 50_UIX/01_shell
Device tree overlay patching	DTB passed as-is via dtb_pa
Splash screen / logo	Not in scope
bootcmd script execution	Replaced by uiox_soc_main.c pipeline

UIOX does not need those because the kernel and BSP are compiled together — there is no dynamic kernel loading step.

Final directory picture:
10_BSP/
├── uiox_arch_main.c     ← secondary bootloader entry + hand-off
├── uiox_arch_main.h
├── arch/                ← ISA layer (was 10_Arch)
│   └── <arch>/src/arch_init.c
└── soc/                 ← SoC + FwHal layer (was 03_SoC + 02_FwHal)
    └── src/
        └── uiox_soc_main.c  ← 9-stage secondary bootloader pipeline
The secondary bootloader is not a separate directory — it is the combination of:

10_BSP/uiox_arch_main.c — orchestration entry
10_BSP/arch/<arch>/src/arch_init.c — ISA bring-up
10_BSP/soc/src/uiox_soc_main.c — full 9-stage boot pipeline
Everything a secondary bootloader needs to do is already covered. No new directory is required.
=====================================================================================================================
if the kernel and BSP are not compiled together, UIOX needs a dynamic kernel loader. Here is exactly what changes and what new code is required.

The two models compared


Model A — compiled together (current)        Model B — separate (dynamic load)
─────────────────────────────────────        ──────────────────────────────────
10_BSP + kernel = one ELF image              10_BSP = one ELF/binary (bootloader)
                                             kernel  = separate ELF/binary
uiox_soc_main.c Stage 8:                     uiox_soc_main.c Stage 8:
  extern void uiox_kernel_main();              uiox_kernel_load()    ← NEW
  uiox_kernel_main(dtb_pa);                    uiox_kernel_verify()  ← NEW
                                               uiox_kernel_jump()    ← NEW

What needs to be added to 10_BSP
Five new components are needed:



10_BSP/
└── soc/
    ├── include/
    │   └── uiox_kernel_loader.h    ← NEW
    └── src/
        └── uiox_kernel_loader.c    ← NEW

=====================================================================================================================
Static kernel (BSP + kernel compiled together)
One binary image containing both 10_BSP and the kernel.

Advantages
Advantage	Why it matters for UIOX
Simplest build	One make all produces one flashable image — no separate kernel image to manage
No loader code	uiox_kernel_loader.c is not needed — reduces code size and attack surface
Fastest boot	No storage read, no CRC compute, no image parse — Stage 8 is a direct function call
Compiler can optimize across layers	LTO (Link-Time Optimization) can inline BSP calls into kernel and vice versa
No ABI contract needed	BSP and kernel share types, headers, and stack — no calling convention boundary
Easier debugging	One symbol table, one ELF, one objdump output — GDB loads one file
Deterministic size	Flash layout is fixed at build time — no risk of kernel image being too large
No TOCTOU risk	Storage is not read at boot — flash corruption after programming cannot cause a partial load
Disadvantages


Disadvantage	Impact
Must reflash everything for a kernel update	Changing one kernel line requires reflashing the entire BSP+kernel image
Longer build cycles	Every kernel change recompiles and relinks the full image
Harder to support multiple kernels	Cannot boot a different kernel without a complete rebuild
Binary grows with both layers	BSP init code that runs once is permanently in flash alongside the kernel
Harder to share BSP across products	Same BSP image cannot run a different OS or kernel variant
Cannot update kernel OTA independently	Over-the-air update must replace the entire image — higher risk, more data

=====================================================================================================================
Dynamic kernel (BSP loads kernel at runtime)
Two separate binaries — BSP loads the kernel from storage and jumps to it.

Advantages
Advantage	Why it matters for UIOX
Independent kernel updates	Flash only the kernel partition — BSP stays untouched
OTA (Over-The-Air) updates	Smaller update payload — update just the kernel image
Multiple kernel slots	A/B boot: slot A = current kernel, slot B = new kernel — safe rollback on failure
BSP reuse	One BSP binary boots different kernel versions or different OS variants
Faster iteration on kernel	Only rebuild and flash the kernel image during development
Production flexibility	Factory flashes BSP once; field updates only touch the kernel partition
Clean separation	BSP and kernel have a defined interface — each team can develop independently
Signature verification	Kernel image signature checked at boot — tampered images rejected before execution
Disadvantages


Disadvantage	Impact
More code to write and maintain	uiox_kernel_loader.c + image format (UIF) + verify logic
Storage driver required before kernel load	eMMC / SD / SPI flash driver must work at BSP stage — increases BSP complexity
Slower boot	Storage read + CRC + optional decompression adds time (50–500 ms depending on storage speed)
ABI contract required	BSP and kernel must agree on calling convention, register layout, DTB location — a breaking change in one breaks the other
Image format versioning	UIF header must be versioned — old BSP must handle new kernel headers gracefully
Additional flash partitions	Flash layout needs separate BSP partition + kernel partition + optionally A/B slots
Harder to debug	Two separate ELF files — GDB needs both symbol tables loaded
More attack surface	Loader code + signature verification code = more code that can have bugs
Memory layout complexity	Load address, entry point, DTB address must all be coordinated at build time
=====================================================================================================================
Criterion	                        Static                  Dynamic
Boot speed	                        ✅ Fastest	            ⚠️ Slower (storage read)
Update granularity	                ❌ Full image	        ✅ Kernel only
OTA support	                        ❌ Hard	                ✅ Natural fit
A/B slot support	                ❌ Not possible	        ✅ Built-in
Code complexity	                    ✅ Simple	            ❌ More code
Debug simplicity	                ✅ One image	            ❌ Two images
BSP reuse across kernels            ❌ No	                ✅ Yes
Storage driver needed at BSP stage	✅ No	                ❌ Yes
ABI contract needed	                ✅ No	                ❌ Yes
Signature verification	            ⚠️ Optional	             ✅ Natural integration
Flash partition complexity	        ✅ Simple	            ❌ Needs layout planning
LTO / cross-layer optimization	    ✅ Possible	            ❌ Not possible
Production update risk	            ❌ High (full reflash)	✅ Low (kernel only)

=====================================================================================================================

Recommendation for UIOX
Phase	Recommendation
Development / bring-up	        Use static — fastest iteration, simplest debug, no loader complexity
Product / field deployment	    Use dynamic — OTA updates, A/B slots, independent BSP and kernel teams
Both supported simultaneously	Use the #ifdef UIOX_DYNAMIC_KERNEL_LOAD flag already in uiox_soc_main.c — same codebase, different build flag

The cleanest approach for UIOX is exactly what was implemented: the #ifdef switch in Stage 8 of uiox_soc_main.c that selects static or dynamic at compile time — zero runtime cost for static builds, full loader capability for dynamic builds, from the same source tree.
=====================================================================================================================
Change 1 — 10_BSP/soc/Makefile — produce an ELF not just .a

Makefile

Change 2 — Linker scripts (one per architecture)

Change 3 — uiox_arch_main.c — add BSS zero and early stack setup
When the BSP is a separate ELF the C runtime is not available — BSS must be zeroed manually and the stack must be set before any C code runs. Add a small assembly entry stub:

Change 4 — uiox_arch_main.c — add __attribute__((weak)) stubs for kernel symbols
When building the BSP as a separate ELF the kernel symbols (uiox_kernel_main, syscall_dispatch etc.) are not present. Add weak stubs:

/* 10_BSP/uiox_arch_main.c — add at bottom */

/*
 * Weak stubs for kernel symbols that are absent when BSP is
 * built as a separate ELF with UIOX_DYNAMIC_KERNEL_LOAD.
 * The dynamic loader never calls these — it jumps directly
 * to the loaded kernel image's entry point.
 */
#if defined(UIOX_DYNAMIC_KERNEL_LOAD)

void __attribute__((weak, noreturn)) uiox_kernel_main(unsigned long dtb_pa)
{
    (void)dtb_pa;
    uiox_soc_puts("[bsp] uiox_kernel_main stub — should not be called\n");
    for (;;) uiox_soc_hw_barrier();
}

long __attribute__((weak))
syscall_dispatch(unsigned long nr,
                  unsigned long a0, unsigned long a1,
                  unsigned long a2, unsigned long a3,
                  unsigned long a4, unsigned long a5)
{
    (void)nr; (void)a0; (void)a1;
    (void)a2; (void)a3; (void)a4; (void)a5;
    return -1L;
}

void __attribute__((weak))
exception_dispatch(unsigned long cause,
                    unsigned long tval,
                    void *frame)
{
    (void)cause; (void)tval; (void)frame;
}

unsigned long __attribute__((weak)) phys_alloc_page(void)
{
    return 0UL;   /* stub — mm not available in BSP-only image */
}

#endif /* UIOX_DYNAMIC_KERNEL_LOAD */


Change 5 — Flash layout additions
When producing separate ELFs the flash partition table needs updating:



Flash layout (example — QEMU ARM64 virt):
───────────────────────────────────────────────────────
0x00000000  +──────────────────+  4 MB
            │  SoC BootROM     │  (primary bootloader — NOT our code)
0x00400000  +──────────────────+
            │  uiox_bsp.bin    │  ← 10_BSP separate ELF, stripped to .bin
            │  (max 4 MB)      │    DRAM load: 0x40000000
0x00800000  +──────────────────+
            │  kernel.bin      │  ← kernel separate ELF, stripped to .bin
            │  (max 16 MB)     │    DRAM load: 0x40400000
0x01800000  +──────────────────+
            │  DTB             │  ← Device Tree Blob
            │  (max 64 KB)     │    DRAM load: 0x42000000
0x01810000  +──────────────────+
            │  User data /     │
            │  Filesystem      │
───────────────────────────────────────────────────────

Summary of all changes


#	What changes	File	Why
1	New Makefile that produces .elf + .bin	10_BSP/soc/Makefile	Links with -T linker/bsp_<arch>.ld to produce standalone ELF
2	Four linker scripts	linker/bsp_arm64/32/x86_64/riscv64.ld	Define BSP memory regions, stack, BSS boundaries
3	Entry stub zeros BSS and sets stack	src/bsp_entry.S	C runtime not available when BootROM jumps into BSP
4	Weak stubs for absent kernel symbols	uiox_arch_main.c	Linker needs definitions even though they are never called
5	-DUIOX_DYNAMIC_KERNEL_LOAD flag	Makefile CFLAGS	Activates Stage 8 dynamic load path in uiox_soc_main.c
6	Flash partition table	10_BSP/soc/linker/flash_layout.md	Documents where BSP and kernel binaries live in flash

-------------------------------------------------------
How it works


make ARCH=arm64                           → static  (default)
make ARCH=arm64 DYNAMIC_KERNEL=1          → dynamic loader
=====================================================================================================================
How the #ifdef switch works — zero overhead summary


Static build (DYNAMIC_KERNEL not set):
  CFLAGS: no -DUIOX_DYNAMIC_KERNEL_LOAD
  DYNAMIC_SRC: empty → uiox_kernel_loader.c NOT compiled
  Stage 8: extern uiox_kernel_main() called directly
  Result: loader code = 0 bytes in the image
Dynamic build (DYNAMIC_KERNEL=1):
  CFLAGS: -DUIOX_DYNAMIC_KERNEL_LOAD
  DYNAMIC_SRC: soc/src/uiox_kernel_loader.c compiled in
  Stage 8: uiox_kernel_load() → verify() → jump()
  Result: full loader present, kernel loaded from storage

=====================================================================================================================

  File	Purpose
10_BSP/include/uiox_bsp.h	Public API — both build modes, all return codes, config struct
10_BSP/src/uiox_bsp_main.c	Main BSP source — static uiox_bsp_init() + dynamic uiox_bsp_entry_c() + ELF64 loader + uiox_bsp_jump_to_kernel()
10_BSP/src/bsp_entry.S	Assembly stub (dynamic only) — sets stack, zeros BSS, calls C, all 4 arches
10_BSP/linker/bsp_static.ld	Linker fragment for static build (included by kernel LD script)
10_BSP/linker/bsp_dynamic.ld	Full standalone LD script for secondary-bootloader binary
10_BSP/Makefile	Master Makefile — all 4 arches, both modes, all-arches target
10_BSP/BSP_integration_notes.md	Integration guide + call chains + build commands
=====================================================================================================================
Two call chains at a glance:

Static build (BSP linked into kernel):

uiox_boot_arch_jump()  →  uiox_kernel_main()  →  uiox_bsp_init()
                                                       ├─ arch_init()
                                                       └─ uiox_soc_init()
Dynamic build (BSP is secondary bootloader):


uiox_boot_arch_jump()  →  bsp_entry.S  →  uiox_bsp_entry_c()
                                               ├─ arch_init()
                                               ├─ uiox_soc_init()
                                               ├─ load_kernel_elf()
                                               └─ uiox_bsp_jump_to_kernel()
                                                        └─ uiox_kernel_main()
=====================================================================================================================
# UIOX BSP Integration Notes
**Date:** 2026-07-21  
**Version:** 1.0.0

---

## Static Build — kernel integration

In `33_ProcessControlSubsystem/uiox_kernel_main.c`, add the BSP init call
**before** `arch_init()` is called independently.  The BSP init internally
calls both `arch_init()` and `uiox_soc_init()`, so the kernel should
delegate to `uiox_bsp_init()` rather than calling `arch_init()` directly.

```c
#include "../../10_BSP/include/uiox_bsp.h"

// Inside uiox_kernel_main(), replace the bare arch_init() call with:

uiox_bsp_config_t bsp_cfg = {
    .dtb_pa  = g_dtb_pa,
    .args_pa = (uint64_t)(uintptr_t)g_boot_args,
    .flags   = 0,
};

int rc = uiox_bsp_init(&bsp_cfg);
if (rc != UIOX_BSP_OK) {
    early_puts("[kernel] FATAL: uiox_bsp_init failed\r\n");
    for (;;) __asm__ volatile("wfi");
}
// arch_init() and uiox_soc_init() are now done — continue with ksign, etc.
```

### Static build call chain

```
Primary BL (uiox_boot_arch_jump)
  └─► uiox_kernel_main()          [33_ProcessControlSubsystem]
        └─► uiox_bsp_init()       [10_BSP/src/uiox_bsp_main.c]
              ├─► arch_init()     [10_BSP/10_Arch/<arch>/src/arch_init.c]
              └─► uiox_soc_init() [10_BSP/03_SoC/src/uiox_soc_main.c]
```

---

## Dynamic Build — secondary bootloader

The BSP binary is loaded by the primary bootloader into SRAM at
`BSP_LOAD_ADDR` (default `0x48000000` — tune in `bsp_dynamic.ld`).

### Dynamic build call chain

```
Primary BL (uiox_boot_arch_jump)
  └─► uiox_bsp_entry()           [10_BSP/src/bsp_entry.S]
        ├── zeros BSS, sets stack
        └─► uiox_bsp_entry_c()   [10_BSP/src/uiox_bsp_main.c]
              ├─► arch_init()
              ├─► uiox_soc_init()
              ├─► load_kernel_elf()   (ELF loader built into BSP)
              └─► uiox_bsp_jump_to_kernel()
                    └─► uiox_kernel_main()
```

### Build commands

```bash
# Static  (default) — produces 10_BSP/build/<arch>/libbsp.a
make -C 10_BSP ARCH=arm64
make -C 10_BSP ARCH=arm32
make -C 10_BSP ARCH=riscv64
make -C 10_BSP ARCH=x86_64

# Dynamic — produces uiox_bsp.elf + uiox_bsp.bin
make -C 10_BSP ARCH=arm64   BUILD=dynamic
make -C 10_BSP ARCH=arm32   BUILD=dynamic
make -C 10_BSP ARCH=riscv64 BUILD=dynamic
make -C 10_BSP ARCH=x86_64  BUILD=dynamic

# All architectures in one shot
make -C 10_BSP all-arches
make -C 10_BSP all-arches BUILD=dynamic
```

---

## File layout

```
10_BSP/
├── include/
│   └── uiox_bsp.h              ← public API (kernel + BSP consumers)
├── src/
│   ├── uiox_bsp_main.c         ← main BSP source (static + dynamic paths)
│   └── bsp_entry.S             ← assembly stub (dynamic build only)
├── linker/
│   ├── bsp_static.ld           ← linker fragment (included by kernel LD script)
│   └── bsp_dynamic.ld          ← full LD script for standalone BSP binary
├── Makefile                    ← master Makefile, all 4 architectures
├── 10_Arch/                    ← existing arch layer (arch_init per ISA)
│   ├── arm32/
│   ├── arm64/
│   ├── riscv64/
│   └── x86_64/
└── 03_SoC/                     ← existing SoC layer (uiox_soc_init)
    └── src/
        └── uiox_soc_main.c
```
=====================================================================================================================
=================================================== END HERE ========================================================
=====================================================================================================================

# 10_BSP — Architecture

**Document:** `00_Docs/10_BSP_ARCHITECTURE.md`
**Companions:** `00_Docs/01_uBoot_CALLFLOW.md` · `00_Docs/01_uBoot_PORTABILITY.md`
**Last reviewed:** 2026-09-20
**Status:** Baseline — update on any structural change to the BSP

---

## Purpose

`10_BSP` is the **secondary bootloader + board bring-up** layer. It sits between
the primary loader (`01_uBoot`) and the kernel (`30_KIX`), and it owns the
architecture and SoC initialisation that must happen exactly once, in one place.

This document records what the layer contains, the two build modes, the layer
boundary it enforces, and the two concrete gaps left for real silicon.

---

## Layout

```
10_BSP/
├── src/            bsp_entry.S · uiox_bsp_main.c · uiox_bsp_stubs.c
├── include/        uiox_bsp.h
├── 10_Arch/        uiox_arch_main.c/.h        ← the dispatcher
│   └── <arch>/src/ arch_init.c  (ISA-level init per arch)
├── 03_SoC/         uiox_soc_main.c (Stage 0a–8)
│                   uiox_soc_clk.c · uiox_soc_power.c · uiox_soc_pm.c
│                   uiox_kernel_loader.c · uiox_soc_secboot.c · …
├── linker/         bsp_static.ld · bsp_dynamic.ld
├── Makefile        v1.8.0
└── bsp.md
```

---

## The two build modes

| Mode | Flag | Entry | Kernel |
|------|------|-------|--------|
| **static** (default) | — | `uiox_bsp_init()` | linked into the image |
| **dynamic** | `BUILD=dynamic`, `-DUIOX_DYNAMIC_KERNEL_LOAD` | `uiox_bsp_entry()` (from `bsp_entry.S`) | standalone ELF, loaded at runtime |

Only the dynamic build compiles `bsp_entry.S`. The Makefile gates it:

```make
ifeq ($(BUILD),dynamic)
  BSP_SRCS_S := $(BSP_DIR)/src/bsp_entry.S
else
  BSP_SRCS_S :=
endif
```

---

## Call flow

### Static build

```
uiox_kernel_main()                 [30_KIX]
  └─► uiox_bsp_init()              [src/uiox_bsp_main.c]
        ├─► arch_init()            [10_Arch/<arch>/src/arch_init.c]
        └─► uiox_soc_init()        [03_SoC/src/uiox_soc_main.c]
              └─► Stage 8: call uiox_kernel_main() (already linked)
```

### Dynamic build

```
uiox_boot_arch_jump()              [01_uBoot/src/uiox_boot_handoff.c]
  └─► uiox_bsp_entry             [src/bsp_entry.S]
        ├─ set stack from _stack_top
        ├─ zero _bss_start .. _bss_end
        └─► uiox_bsp_entry_c(dtb_pa, args_pa)
              └─► uiox_arch_main(dtb_pa)      [10_Arch/uiox_arch_main.c]
                    ├─► arch_init()                       (Stage 1 — ISA)
                    ├─► uiox_soc_init()                   (Stage 2 — SoC 0a–8)
                    │     └─► Stage 8: load → verify → jump
                    └─► uiox_arch_dtb_pa = dtb_pa
```

The **static/dynamic switch lives in `uiox_soc_main.c` Stage 8**, not in the
dispatcher. `uiox_arch_main` and its header are identical in both modes.

---

## The dispatcher — `uiox_arch_main`

```c
int uiox_arch_main(unsigned long dtb_pa)
{
    rc = arch_init();       /* Stage 1 — ISA: cache, GIC/APIC/PLIC, VBAR/stvec/IDT */
    if (rc) return rc;

    rc = uiox_soc_init();   /* Stage 2 — SoC pipeline, Stages 0a–8 */
    if (rc) return rc;

    uiox_arch_dtb_pa = dtb_pa;   /* stored for higher layers */
    return 0;
}
```

It also provides weak stubs — `uiox_kernel_main()`, `syscall_dispatch()` — so
the standalone dynamic ELF links before the kernel's strong symbols are present.

---

## The layer boundary (enforced by `arch_init.c`)

`arch_init.c` states its scope explicitly, and this is the boundary the whole
layer is built around:

**In `arch_init` — ISA-defined operations only:**

1. CPU identification (`MIDR_EL1` / `MPIDR_EL1`)
2. Enable I/D caches (`SCTLR_EL1`)
3. Invalidate TLB and I-cache
4. Configure GIC-400 distributor + CPU interface
5. Install `VBAR_EL1`
6. Configure generic timer (`CNTFRQ_EL0` / `CNTP_CTL_EL0`)
7. Register IRQ handlers via `20_DriverInterfaces`
8. Enable global interrupts (DAIF clear)

**Not in `arch_init` — belongs in `03_SoC`:**

| Concern | Owner |
|---------|-------|
| PL011 baud rate | `03_SoC/uiox_soc_<arch>.c` |
| VirtIO device init | `03_SoC/uiox_soc_<arch>.c` |
| Clock PLL / CCM setup | `03_SoC/uiox_soc_clk.c` |
| Power domain control | `03_SoC/uiox_soc_pm.c` |
| SoC MMIO map | `03_SoC/include/uiox_soc_map.h` |

> **Rule:** if an operation is defined by the ISA, it lives in `10_Arch`; if it
> is defined by the SoC's memory map, it lives in `03_SoC`. Nothing straddles
> the line.

---

## Component state

### `uiox_soc_clk.c` — clock / PLL

**Implemented:** the clock model — a table with per-arch defaults, enable
flags, and three API faces:

- stateless `uiox_soc_clock_*` (module-internal table)
- stateful `uiox_soc_clk_*` (explicit `uiox_clk_ctx_t *`)
- legacy forwarders `uiox_fw_clock_*`

```c
#if defined(__aarch64__)
static void clock_table_init(void)
{
    clock_set(UIOX_SOC_CLK_CPU,   "cpu",   CLK_ARM64_CPU_HZ,     true);
    clock_set(UIOX_SOC_CLK_BUS,   "bus",   CLK_ARM64_BUS_HZ,     true);
    clock_set(UIOX_SOC_CLK_UART0, "uart0", CLK_ARM64_UART_HZ,    true);
    clock_set(UIOX_SOC_CLK_EMMC,  "emmc",  CLK_ARM64_STORAGE_HZ, false);
    /* … */
}
#endif
```

**Not implemented:** the actual PLL MMIO writes. The header says so:

> *On QEMU targets `set_hz()` records the requested value only. **Real SoC
> ports: add CMU / CCF MMIO writes where marked.***

So the gap is precise: **clock model present, register writes pending.**

### `uiox_soc_power.c` — power

**Implemented and real:**

- ARM64/ARM: PSCI `CPU_ON` via `hvc #0` (registers x0–x3, checks `x0 == 0`)
- ARM/ARM64: `wfi` idle
- x86: ACPI S5 power-off (`outw` to PM1a control block)
- per-CPU state tracking (`ON`/`OFF`), `num_cpus = 4`

This is not a stub.

### `uiox_kernel_loader.c` — dynamic-path kernel load

**Implemented:** CRC32 (IEEE 802.3), a UIF magic-header check, entry/DTB
override from the header, and `loader_platform_defaults()` filling QEMU-virt
addresses per arch.

**Not implemented:** the storage read. The function reads from a
`src_flash_base` **address**, and the actual driver call is a placeholder:

```c
/*
 * On real hardware: call the storage driver to DMA/copy
 * …
 */
```

`desc->verify_sig = UIOX_FALSE;  /* set true when secboot active */` — the
signature-verification hook exists as a flag but is not active.

---

## The two load models (integration conflict)

This is the one genuine conflict in the boot chain. **Two implementations read
the kernel, and they assume different things:**

| | `01_uBoot` (primary) | `10_BSP` (secondary, dynamic) |
|---|---|---|
| Read path | media layer → UNFS | `src_flash_base` MMIO → copy |
| Sees | a filesystem | a memory-mapped image |
| Path known by | UNFS directory walk (`/boot/uiox_kernel.elf`) | a fixed flash base address |
| Verified by | SHA-256 vs image header | CRC32 (+ `verify_sig` flag, off) |
| Call | `unfs_boot_load()` | `uiox_kernel_load()` |

### Why this matters

In a **dynamic** build the chain is `01_uBoot` → `uiox_bsp_entry` → `arch_init`
→ `uiox_soc_init` → Stage 8 → `uiox_kernel_load()`. If `01_uBoot` also loaded a
kernel (its Stage 4), the image is either loaded twice, or `10_BSP` re-reads a
kernel the primary already placed in DRAM.

### Reconciliation — pick one, per mode

**Option A — `01_uBoot` loads; `10_BSP` does not.**
Static or dynamic, the primary reads the kernel from UNFS and passes its entry
in the boot-args. `10_BSP`'s Stage 8 skips `uiox_kernel_load()` and jumps to the
entry it was handed. `uiox_kernel_loader.c` is compiled out.

- Pros: one read path (the richer one — UNFS, directory lookup, extents, SHA-256)
- Cons: `10_BSP` dynamic mode can no longer be used standalone

**Option B — `10_BSP` loads; `01_uBoot` does not.**
The primary hands over after board-agnostic setup; `10_BSP`'s
`uiox_kernel_load()` reads from the storage device. `01_uBoot`'s Stage 4 is
skipped on a dynamic build.

- Pros: `10_BSP` is self-contained as a second stage
- Cons: the flash-base read model is weaker than UNFS — no directory walk, no
  extents; and it duplicates the storage driver work into `10_BSP`

**Recommended: Option A.** Reasons:

1. **UNFS is the designed filesystem** for this project (`31_BufferCache` →
   `UnfsFs`), and `01_uBoot` already has a complete, tested reader. The
   `src_flash_base` model treats the kernel as a raw blob at a fixed address —
   which is what `unfs_read_file()` gives you *after* a proper lookup.
2. **One read path means one storage driver.** Option B would need the SDHCI /
   VirtIO driver reachable from `10_BSP` as well as `01_uBoot` — two
   integration points to the same controller.
3. **`10_BSP`'s strength is arch/SoC bring-up**, which is exactly what Stage 1
   and Stage 2 do. Loading is not its competence; `01_uBoot`'s media layer is.

**What this means concretely:**

- `01_uBoot` Stage 4 stays authoritative: load + verify + entry.
- The boot-args it builds already carry `kernel_entry` — `10_BSP` reads it from there.
- `10_BSP` Stage 8, on the dynamic path, jumps to `boot_args->kernel_entry`
  instead of calling `uiox_kernel_load()`.
- `uiox_kernel_loader.c` remains for the standalone-BSP case (a second-stage
  brought up without `01_uBoot`), but is not the normal path.

### The one thing to fix first

Whichever option is chosen, **the boot-args must be the single source of the
entry point.** Today `01_uBoot` passes `(dtb_pa, args_pa)` in registers and puts
`kernel_entry` in the args struct; `10_BSP` computes its own `entry_addr` from
`loader_platform_defaults()`. Those two must not disagree — the args struct wins.

---

## Gap summary for real silicon

| Item | State | Owner |
|------|-------|-------|
| ISA init (GIC, VBAR, timer, caches) | ✅ implemented | `10_Arch/<arch>/arch_init.c` |
| Clock **model** + per-arch table | ✅ implemented | `03_SoC/uiox_soc_clk.c` |
| Clock **PLL MMIO writes** | ❌ marked TODO | `03_SoC/uiox_soc_clk.c` |
| Power (PSCI / ACPI) | ✅ implemented | `03_SoC/uiox_soc_power.c` |
| Arch/SoC source splitting | ✅ fixed in v1.8.0 | `Makefile` |
| Kernel storage read | ❌ placeholder | `03_SoC/uiox_kernel_loader.c` |
| Signature verification | ❌ flag off | `desc->verify_sig` |
| Single load path | ❌ two models | `01_uBoot` vs `10_BSP` |

The gap is **two concrete holes plus one integration decision** — PLL writes,
the storage read, and the load-model reconciliation — not a missing layer.

---

## Notes

**v1.8.0 fixed a real bug** worth remembering: the wildcard
`03_SoC/src/*.c` compiled every arch backend for every arch, producing
`SOC_TIMER0_BASE undeclared` when `uiox_soc_arm32.c` was built with
`-D__aarch64__`. Fix: list shared vs arch-specific sources explicitly instead
of globbing. Any Makefile that globs per-arch sources will hit this.

**Two cosmetic items worth cleaning:**

- `uiox_bsp_main.c` carries a paste artifact — `// After the existing #include … line, add:` — an instruction to a human that shipped in source.
- `arch_init.c` (arm64) has a commented-out `//#include "uiox_syscall.h"` and an `arch_syscall_entry` remnant — the same syscall-relocation fossil seen in `01_uBoot/src/arch_init.c`.

---

*Companion to the `01_uBoot` call-flow and portability documents. Update all
three together when the boot chain changes.*
