# `01_uBoot` — Call Flow

**Document:** `00_Docs/01_uBoot_CALLFLOW.md`
**Companion:** `00_Docs/01_uBoot_PORTABILITY.md` (file-by-file target classification)
**Last reviewed:** 2026-09-20
**Status:** Baseline — update on any structural change to the boot pipeline

---

## Purpose

This is the function-level call flow of the `01_uBoot` primary bootloader, from
the firmware handoff to the kernel entry. It answers "what calls what, in what
order" — the chart the portability matrix does not carry.

All paths are relative to `01_uBoot/`.

---

## The flow, in full

```
FIRMWARE / PRIOR STAGE
  └─ places a DTB pointer in the boot register, transfers control to _start
       │
       ▼
src/arch/<arch>/uiox_boot_entry_<arch>.S
  │   The entry stub does ONLY CPU/pipeline setup. It registers no HAL and
  │   prints nothing — the UART is dead until Stage 1 runs. Everything below
  │   the branch/call into main is C.
  │
  ├─ ARM64: read CurrentEL; drop EL2 → EL1 if running at EL2; disable
  │          MMU / D-cache / I-cache via SCTLR_EL1; invalidate TLBs
  ├─ ARM32: (EL setup per machine), disable MMU/caches, invalidate TLBs
  ├─ RISC-V: park secondary harts (only hart 0 proceeds);
  │          csrci mstatus, MIE=0; csrwi mie, 0
  ├─ x86_64: Multiboot2 header; build identity page tables; enable PAE +
  │          EFER.LME; enable paging; long-jump to 64-bit entry; load GDT
  ├─ set up the early stack from _boot_stack_top
  ├─ zero .bss  (__bss_start .. __bss_end)
  └─ call/br uiox_boot_main  with the DTB PA in arg0
       │   ARM64  x0 = dtb_pa        ARM32  r2 = dtb_pa
       │   RISC-V a0 = dtb_pa        x86_64 rdi = Multiboot2 info ptr
       │
       ▼
src/uiox_boot_main.c                          ← everything below is C
  │
  ├─ STAGE 1  HW init
  │    ├─ UIOX_HW_REGISTER()                  (per-arch macro)
  │    │     └─ uiox_boot_hw_<arch>_register()  [src/arch/<arch>/uiox_boot_hw_<arch>.c]
  │    │           └─ uiox_boot_hw_register(&ops)   [src/uiox_boot_hw.c]
  │    │                 └─ ops->init()  ==  <arch>_hw_init()
  │    │                       ├─ uiox_board_bringup()  [board/uiox_board_<variant>_<arch>.c]
  │    │                       │     (clock/PLL · power · pin-mux — before the UART)
  │    │                       └─ uart_init()  ·  gic|plic|vic_init()
  │    ├─ uiox_boot_console_init()
  │    ├─ BOOT_BANNER(UIOX_ARCH_STR)
  │    └─ BOOT_LOG(1, "HW init"); BOOT_OK()
  │
  ├─ STAGE 2  Memory probe
  │    └─ uiox_boot_mem_probe(dtb_pa, &mem_map)   [src/uiox_boot_mem.c]
  │          ├─ fdt_parse_memory()   → walks /memory "reg"
  │          └─ probe_fallback()     → hardcoded map if no DTB
  │    └─ uiox_boot_mem_alloc_init(&alloc, &mem_map, args_pa, size)
  │
  ├─ STAGE 2.5  Device-tree runtime extraction
  │    └─ uiox_boot_dt_apply(dtb_pa, bootargs, len, &soc)   [src/uiox_boot_dt.c]
  │          ├─ uiox_boot_dt_chosen()  → /chosen "bootargs"
  │          └─ uiox_boot_dt_soc()     → /soc peripheral bases
  │    (bootargs overrides UIOX_CMDLINE for the handoff)
  │
  ├─ STAGE 3  Storage
  │    ├─ uiox_boot_media_register(virtio)     [src/uiox_boot_media*.c]
  │    ├─ uiox_boot_media_register(sdmmc)
  │    └─ uiox_boot_media_select()
  │          ├─ probe each driver: present()?
  │          └─ first present + init()==OK  →  s_active
  │
  ├─ STAGE 4  Load kernel
  │    ├─ uiox_boot_mem_alloc()  (kernel staging buffer)
  │    └─ unfs_boot_probe() / unfs_boot_load()   [src/uiox_boot_bridge_unfs.c]
  │          └─ unfs_mount()                      [src/uiox_boot_unfs.c]
  │                ├─ read superblock, verify CRC32C
  │                ├─ read group descriptors
  │                ├─ unfs_lookup("/boot/uiox_kernel.elf")
  │                │     ├─ read_inode()
  │                │     ├─ dir_lookup()
  │                │     │     └─ extent_lookup()  → physical block
  │                │     └─ read_block()
  │                │           └─ uiox_boot_media_read_block()
  │                │                 └─ media driver (virtio | sdmmc)
  │                └─ unfs_read_file()  → copy to kernel load PA
  │
  ├─ STAGE 5  Verify
  │    └─ uiox_boot_verify_image(hdr, payload, len, arch)  [src/uiox_boot_verify.c]
  │          └─ SHA-256 of payload  vs  image-header hash
  │    (then detect ELF by magic + class)
  │
  ├─ STAGE 6  ELF load / flat binary
  │    ├─ uiox_boot_elf64_load()         [src/uiox_boot_handoff.c]
  │    │     ├─ walk PT_LOAD segments → copy to p_paddr
  │    │     ├─ zero BSS tail (p_memsz > p_filesz)
  │    │     └─ dcache_flush() / icache_inv()
  │    └─ (or) uiox_boot_flat_load(payload, len, hdr->load_addr)
  │
  └─ STAGE 7  Handoff — never returns
       └─ uiox_boot_handoff(entry, dtb, args_pa, &mem_map, cmdline)
            ├─ build_args()  → fills uiox_boot_args_t at args_pa
            │    (magic, version, kernel_entry, dtb_pa, args_pa,
            │     mem_map, cmdline, arch)
            ├─ uiox_boot_hw_barrier()   → drain console
            └─ uiox_boot_arch_jump(entry, dtb, args)
                 │
                 ├─ STATIC build → kernel entry
                 │     └─ uiox_kernel_main()            [30_KIX]
                 │           └─ uiox_bsp_init()         [10_BSP]
                 │                 ├─ arch_init()       [10_BSP/10_Arch]
                 │                 └─ uiox_soc_init()   [10_BSP/03_SoC]
                 │
                 └─ DYNAMIC build → BSP entry
                       └─ uiox_bsp_entry()              [10_BSP]
                             ├─ uiox_bsp_init()
                             ├─ load_kernel_elf()       (BSP reads storage)
                             └─ uiox_bsp_jump_to_kernel()
                                   └─ uiox_kernel_main()
```

---

## The entry stub, per architecture

The `.S` file's job is CPU/pipeline setup and one branch into `main`. It does
**not** call `uiox_boot_hw_register()` — that happens in Stage 1 of
`uiox_boot_main()`. Step counts differ per architecture:

| Arch | Steps the stub performs | DTB/arg register |
|------|-------------------------|------------------|
| **ARM64** | CurrentEL check → drop EL2→EL1 (`HCR_EL2`, `SPSR_EL2`, `ELR_EL2`, `eret`) → disable MMU/D/I cache (`SCTLR_EL1`) → `tlbi vmalle1` → set stack → zero `.bss` → `bl` | `x0` |
| **ARM32** | disable MMU/caches, invalidate TLBs → set stack → zero `.bss` → branch | `r2` |
| **RISC-V 64** | park secondary harts (`mhartid != 0` → spin) → `csrci mstatus,(1<<3)` → `csrwi mie,0` → set stack → zero `.bss` → `call` | `a0` |
| **x86_64** | Multiboot2 header → build identity page tables → `CR3`/PAE/EFER.LME → enable paging → `ljmp` to 64-bit → load GDT → set stack → zero `.bss` → `call` | `rdi` (Multiboot2 info) |

Two consequences worth stating:

1. **The entry stub cannot print anything.** The UART isn't initialised until
   Stage 1 runs inside `main`. A fault in the stub — bad stack symbol, wrong
   `.bss` range, bad page table — is silent with no console to debug it. The
   stub must be correct by inspection.
2. **The x86 stub is the heaviest.** Unlike the others it must build page
   tables and enter long mode before any C can run.

---

## The arch-jump ABI

`uiox_boot_arch_jump(entry, dtb_pa, args_pa)` places values in each
architecture's boot convention registers before branching:

| Arch | Register convention |
|------|---------------------|
| ARM64 | `x0 = dtb_pa`, `x1 = args_pa`, `x2/x3 = 0`, branch to `entry` |
| ARM32 | `r0/r1 = 0`, `r2 = dtb_pa`, `r3 = args_pa`, branch to `entry` |
| RISC-V 64 | `a0 = dtb_pa`, `a1 = args_pa`, jump to `entry` |
| x86_64 | `rdi = args_pa`, `rsi = dtb_pa`, `rdx = 0`, jump to `entry` |

This is the one place the four architectures differ in the boot contract.
Everything before the jump is architecture-neutral C.

---

## Two compile-time couplings (not calls)

These are not function calls — they are how addresses reach the code:

```
10_BSP/03_SoC/include/uiox_soc_map.h
    ── SOC_* macros ──▶  board/uiox_board_<variant>_<arch>.c
                              ── descriptor ──▶  src/arch/<arch>/uiox_boot_hw_<arch>.c

01_uBoot/linker/uiox_boot_<arch>.ld
    ── _boot_load_base / _kern_load_base / _args_base / _boot_stack_top ──▶
        src/uiox_boot_handoff.c  (and the board descriptor)
```

The SoC map supplies **hardware** addresses; the linker script supplies the
**image layout**. Neither is a runtime call.

---

## Ownership summary

| Concern | Owner |
|---------|-------|
| Pipeline / orchestration | `src/uiox_boot_main.c` |
| Entry / CPU setup | `src/arch/<arch>/uiox_boot_entry_<arch>.S` |
| Hardware (UART, GIC/PLIC/VIC, cache, timer) | `src/arch/<arch>/uiox_boot_hw_<arch>.c` |
| Shared register offsets (PL011, …) | `include/uiox_boot_hw.h` |
| Board addresses + bring-up | `board/uiox_board_<variant>_<arch>.c` |
| Storage device | `src/uiox_boot_media*.c` |
| Filesystem | `src/uiox_boot_unfs.c` |
| Integrity | `src/uiox_boot_verify.c` |
| Kernel entry / handoff | `src/uiox_boot_handoff.c` |
| Memory map + DT parse | `src/uiox_boot_mem.c` |
| DT runtime extraction | `src/uiox_boot_dt.c` |

> **Rule:** register offsets shared across architectures live in
> `include/uiox_boot_hw.h` (e.g. `PL011_*`). A per-arch driver must never
> re-declare them — that is what produces the `redefined` warnings and lets
> the four copies drift apart.

---

## The stages, one line each

1. **HW init** — register the HAL (whose `init()` runs board bring-up, then
   UART and interrupt controller), then print the banner.
2. **Memory probe** — parse `/memory` from the DTB into a region table; init
   the bump allocator.
3. **DT apply** — pull `bootargs` from `/chosen` and peripheral bases from `/soc`.
4. **Storage** — register media drivers, select the first present one.
5. **Load** — mount UNFS, resolve the kernel path, copy the image to DRAM.
6. **Verify** — SHA-256 against the image header; detect ELF vs flat.
7. **Handoff** — build `uiox_boot_args_t`, jump to the kernel or the BSP entry.

---

## Dual-target note

The same flow serves **QEMU and real silicon**. The only differences are
compile-time selections:

| Layer | QEMU | Real silicon |
|-------|------|--------------|
| Board descriptor | `uiox_board_qemu_*` | `uiox_board_generic_*` |
| Media driver | `boot_media_virtio.c` | `boot_media_sdmmc.c` |
| HAL bases | virt-machine | SoC datasheet values |
| Board bring-up | no-op | PLL / power / pin-mux |

Selected at build time by `make BOARD=qemu` vs `make BOARD=generic`.
The pipeline itself is identical — see `01_uBoot_PORTABILITY.md` for the
per-file classification.

---

*This document is the call-flow companion to the portability matrix. When the
pipeline changes, update both.*
