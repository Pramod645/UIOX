# `10_BSP` — Architecture

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
