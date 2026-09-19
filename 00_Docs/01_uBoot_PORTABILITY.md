# 01_uBoot Portability Matrix

**Document:** `00_Docs/01_uBoot_PORTABILITY.md`
**Scope:** Every source, header, linker script, and top-level file in `01_uBoot/`
**Last reviewed:** 2026-09-18
**Status:** Baseline — update on every structural change to the bootloader

---

## Purpose

This document classifies every file in `01_uBoot/` by **where it can run**.
It exists because the phrase "not for real silicon" was used imprecisely and
caused confusion: hardware-agnostic boot infrastructure was conflated with
board-specific driver code. This table is the single source of truth.

**Read this before changing any file's class.** A file that moves from
🟢 to 🔴 is an architectural change, not a refactor.

---

## The three classes

| Class | Meaning | Rule of thumb |
|-------|---------|---------------|
| 🟢 **Portable core** | Runs on real silicon **and** QEMU, unchanged | Makes no assumption about *where* hardware lives |
| 🟡 **Board-specific** | Correct structure; needs SoC register work to run on a board | Hardcodes a base address, a load address, or an init sequence |
| 🔴 **QEMU-only** | Works only on a virtual machine | Depends on a device only a hypervisor provides, or on a QEMU load convention |

The real axis is **portable core vs. board-specific** — *not* "bootloader vs.
silicon." A file is portable if it is agnostic to hardware *location*.

---

## `include/`

| File | Class | Rationale |
|------|:-----:|-----------|
| `uiox_boot.h` | 🟢 | Master include; no hardware assumption |
| `uiox_boot_types.h` | 🟢 | Integer typedefs, error codes, magic numbers |
| `uiox_boot_console.h` | 🟢 | `BOOT_*` macros; pure text formatting |
| `uiox_boot_mem.h` | 🟢 | Memory-map types, bump allocator, runtime SoC map |
| `uiox_boot_verify.h` | 🟢 | SHA-256 + image-header declarations |
| `uiox_boot_unfs.h` | 🟢 | UNFS on-disk structures + reader API |
| `uiox_boot_handoff.h` | 🟢 | ELF64 structs, boot-args, static/dynamic modes |
| `uiox_boot_media.h` | 🟢 | Media abstraction interface |
| `uiox_boot_hw.h` | 🟡 | Declares the HAL vtable — the *interface* is portable; every *implementation* is board-specific |

---

## `src/` — common sources

| File | Class | Rationale |
|------|:-----:|-----------|
| `uiox_boot_console.c` | 🟢 | printf/puts engine; formatting only |
| `uiox_boot_mem.c` | 🟢 | memcpy/memcmp, FDT parser, bump allocator |
| `uiox_boot_verify.c` | 🟢 | SHA-256, header validation |
| `uiox_boot_unfs.c` | 🟢 | UNFS reader; storage-agnostic (calls a read callback) |
| `uiox_boot_bridge_unfs.c` | 🟢 | UNFS ↔ pipeline glue; media-agnostic |
| `uiox_boot_handoff.c` | 🟢 | Boot-args build + static/dynamic dispatch + ELF64 load |
| `uiox_boot_dt.c` | 🟢 | `/chosen` + `/soc` extraction — runs on any DT platform |
| `uiox_boot_main.c` | 🟡 | The pipeline is portable; the Stage-4 "simulation path" and hardcoded `UIOX_CMDLINE` are QEMU-era |
| `uiox_boot_fs.c` | 🔴 | Legacy FAT32 path (superseded by UNFS — delete) |

## `src/boot_media/`

| File | Class | Rationale |
|------|:-----:|-----------|
| `uiox_boot_media.c` | 🟢 | Dispatcher: register / select / read |
| `boot_media_virtio.c` | 🔴 | VirtIO exists **only under a hypervisor** — no such device on bare metal |
| `boot_media_bsp.c` | 🟡 | Routes reads to `10_BSP`; structure is portable, the backend must be real |
| `boot_media_none.c` | 🟢 | Template / fallback driver |

## `src/boot_dt/`

| File | Class | Rationale |
|------|:-----:|-----------|
| `uiox_boot_dt.c` | 🟢 | Device-tree runtime extraction; architecture-neutral |

---

## `src/arch/<arch>/` — the board-tied layer

| File | Class | Rationale |
|------|:-----:|-----------|
| `arm64/uiox_boot_entry_arm64.S` | 🟡 | Entry stub is real; assumes a firmware handoff (`x0`=DTB, stack set) |
| `arm64/uiox_boot_hw_arm64.c` | 🔴 | PL011/GIC with QEMU virt bases; `read_block()` reads address `0`; no clk/power/pinctrl |
| `arm32/uiox_boot_entry_arm32.S` | 🟡 | Assumes U-Boot / versatilepb handoff |
| `arm32/uiox_boot_hw_arm32.c` | 🔴 | QEMU versatilepb bases; address-`0` read |
| `riscv64/uiox_boot_entry_riscv64.S` | 🟡 | Assumes OpenSBI M-mode handoff (correct on virt and many real RV boards) |
| `riscv64/uiox_boot_hw_riscv64.c` | 🔴 | NS16550/CLINT/PLIC with QEMU virt bases; address-`0` read |
| `x86_64/uiox_boot_entry_x86.S` | 🔴 | QEMU q35 (`-kernel`) load convention — not how real PC firmware boots |
| `x86_64/uiox_boot_hw_x86.c` | 🔴 | COM1/PIT with fixed bases; no ACPI/E820 discovery; address-`0` read |

---

## `linker/` and top level

| File | Class | Rationale |
|------|:-----:|-----------|
| `linker/uiox_boot_arm64.ld` | 🟡 | QEMU virt load address (`0x40080000`); retarget per board |
| `linker/uiox_boot_arm32.ld` | 🟡 | versatilepb addresses |
| `linker/uiox_boot_riscv64.ld` | 🟡 | virt RAM at `0x80000000` |
| `linker/uiox_boot_x86_64.ld` | 🔴 | q35 layout |
| `Makefile` | 🟢 | Build system; portable |
| `uBoot.md`, `*.png` | 🟢 | Documentation / diagrams |
| `.DS_Store` | — | Stray editor artifact — should not be tracked |

---

## Summary

### The core is portable

Everything in `include/` except `uiox_boot_hw.h`, and every `src/*.c` except
`uiox_boot_fs.c`, runs on silicon unchanged:

`console` · `mem` · `verify` · `unfs` · `bridge` · `handoff` · `dt` ·
`boot_media` (abstraction) — the **bulk of the bootloader**.

> **Correction of record:** an earlier statement claimed these files "aren't
> for real silicon." That was **wrong** — they are the portable core. What is
> QEMU-bound is the *arch driver layer*, not these files.

### The QEMU-only set is small and concentrated

1. The four `arch/*/uiox_boot_hw_*.c` drivers (QEMU bases + address-`0` read)
2. `boot_media_virtio.c` (a VM has a disk; bare metal does not)
3. The x86 entry + linker script (QEMU `-kernel` convention)
4. `uiox_boot_fs.c` (dead FAT32)
5. The Stage-4 simulation path inside `main.c`

### The board-specific set is unimplemented support, not a portability defect

Clock / power / pinctrl init, the linker load address, and the storage
controller. These are **absent board support**, not files that were written
for the wrong target.

---

## Where the real-silicon work lives

| Gap | Layer | File(s) |
|-----|-------|---------|
| Clock / power / pinctrl init | `10_BSP` / `03_SoC` | `uiox_bsp_main.c`, `uiox_soc_clk.c`, `uiox_soc_power.c` |
| Real storage controller | `02_FwHal` | `uiox_fw_sd.c`, `uiox_fw_nvme.c`, … (currently stubs) |
| Board register values | per-SoC | `TODO(board)` slots in the BSP skeleton |
| Syscall implementation | `40_SCIX` | *(no `src/` — largest gap in the stack)* |

### The one structural difference between the two targets

`boot_media_virtio.c` is not "needs new addresses" — on real hardware there is
**no VirtIO device at all**. It must be *replaced* by a physical controller
driver, not retargeted. That is the sharpest architectural difference between
QEMU and silicon.

---

## Call chain (for reference)

### Static build

```
uiox_kernel_main()                    [30_KIX]
  └─► uiox_bsp_init()                 [10_BSP/src/uiox_bsp_main.c]
        ├─► arch_init()               [10_BSP/10_Arch/<arch>/src/arch_init.c]
        └─► uiox_soc_init()           [10_BSP/03_SoC/src/uiox_soc_main.c]
```

### Dynamic build

```
uiox_boot_arch_jump()                 [01_uBoot/src/uiox_boot_handoff.c]
  └─► uiox_bsp_entry()                [10_BSP/src/bsp_entry.S → uiox_bsp_main.c]
        ├─► arch_init()
        ├─► uiox_soc_init()
        ├─► load_kernel_elf()         [dynamic only]
        └─► uiox_bsp_jump_to_kernel()
              └─► uiox_kernel_main()  [30_KIX]
```

---

## Maintenance rules

1. **Every new file gets a class** before it is merged.
2. **Re-classing a file is an architectural change** — it requires review, not
   a quiet edit.
3. **A 🟢 file must never gain a hardcoded address.** If it needs one, it
   belongs in the 🟡 layer.
4. **Keep the QEMU-only set small and named.** It is the blast radius of a
   board port.

---

*This document is the correction of record for the "portable core vs. silicon"
confusion. When in doubt, consult this table, not recollection.*
