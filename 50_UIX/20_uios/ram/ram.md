Application / Device Access API        (uiox_ram_device)
  → RAM Subsystem: allocator, pool, MPU, ECC monitor     (uiox_ram_subsys)
    → Memory Manager: heap, slab, buddy allocator         (uiox_ram_mgr)
    → ECC / Error handling: scrub, correct, log           (uiox_ram_ecc)
    → Interface driver: DRAM controller, timing, mode     (uiox_ram_if)
      → Hardware Abstraction: MMIO, PHY, DFI, IRQ         (uiox_ram_hw)
    ↔ Buffer Manager: memory region descriptors           (uiox_ram_buf)
========================================================================
uiox-ram/
├── include/
│   ├── uiox_ram_hw.h          # Layer 1  — HAL: MMIO, PHY, DFI, IRQ
│   ├── uiox_ram_buf.h         # Memory region descriptor pool
│   ├── uiox_ram_if.h          # Layer 2  — Interface: DRAM ctrl, timing
│   ├── uiox_ram_ecc.h         # ECC: scrub, correct, error log
│   ├── uiox_ram_mgr.h         # Layer 3  — Memory manager: heap/slab/buddy
│   ├── uiox_ram_subsys.h      # Layer 4  — Subsystem: pools, MPU, stats
│   └── uiox_ram_device.h      # Layer 5  — Application-facing API
└── src/
    ├── uiox_ram_hw.c
    ├── uiox_ram_buf.c
    ├── uiox_ram_if.c
    ├── uiox_ram_ecc.c
    ├── uiox_ram_mgr.c
    ├── uiox_ram_subsys.c
    ├── uiox_ram_device.c
    └── uiox_ram_demo.c
==========================================================================
Key Design Decisions:
Decision	Rationale
Three allocator types (heap / buddy / slab)	Heap for general variable-size allocations; buddy for page-aligned DMA/kernel objects; slab for fixed-size structs — each optimal for its use case
First-fit heap with boundary tags	Simple and cache-friendly; coalesces adjacent free blocks immediately on free — prevents long-term fragmentation
Buddy power-of-2 split/merge	O(log n) allocation and free; guaranteed natural alignment — critical for DMA buffers that must be cache-line or page-aligned
Slab with backing-memory fallback to heap	Object caches eliminate fragmentation for frequently-allocated fixed-size objects; falls back to heap when slab is full
Static region descriptor pool (256 entries)	Descriptors live in SRAM/flash — zero heap dependency for memory management metadata; prevents chicken-and-egg allocation problem
ECC background scrub in tick()	64 KB chunks per tick avoids memory bandwidth monopolisation; scrub position persists across ticks for full-range coverage
CE/UE event fire on delta	Fires event only when error count changes — not every tick — prevents event storm during burst errors
ZQ calibration every 1 second	JEDEC LPDDR5 requires ZQ long calibration every 1 second minimum; periodic tick tracks elapsed time without OS timer dependency
Self-refresh in if_stop()	DRAM enters self-refresh on interface stop rather than power-down — preserves data for warm restart / suspend-to-RAM
Low-memory threshold at 10%	Alert when free heap drops below 10% — gives application time to free or compact before OOM
Portable Makefile (no GNU ld flags)	LDFLAGS:= empty — works with GNU ld, Apple ld (macOS Clang), and LLVM lld without modification
Vtable ops pattern	LPDDR5 (Samsung/SK Hynix), DDR5 ECC DIMM, SRAM controllers all plug into the same HAL vtable