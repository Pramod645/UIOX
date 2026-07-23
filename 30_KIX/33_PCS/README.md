# System Header Removal — 33_PCS

## Single replacement header

`33_PCS/include/uiox_klibc.h` replaces every system header used
across the PCS subsystem:

| Was | Section in uiox_klibc.h |
|-----|-------------------------|
| `<stdint.h>`  | §1 fixed-width integer typedefs |
| `<stdbool.h>` | §2 bool / true / false |
| `<stddef.h>`  | §3 NULL, size_t, offsetof |
| `<time.h>`    | §4 clock_t, time_t |
| `<string.h>`  | §5-6 memset/cpy/cmp/strlen/strcmp (inline) |
| `<stdlib.h>`  | §3 NULL (no heap; static pools only) |
| `<stdio.h>`   | §7 uiox_printf() → BSP SoC stdio |
| `<math.h>`    | §8 integer-only min/max/abs (no FPU) |

## Compatibility aliases

§9 of `uiox_klibc.h` maps `memset → uiox_memset`,
`printf → uiox_printf`, etc. so **all existing `.c` files
compile without any text changes** — only the system
`#include` lines need to be removed.

## How sched_types.h is the root

Every `.c` in `01_schedular/` and `02_MemMngnt/` includes
`sched_types.h` (directly or via `scheduler.h`).
Since `sched_types.h` now includes `uiox_klibc.h`, the direct
system includes in those `.c` files are redundant — removing them
is the only edit required in each source file.

## LoadAvg: double removed

`LoadAvg.load_1/5/15` was `double`. Changed to `uint32_t` scaled
×1000 to eliminate the FPU dependency (e.g. 1.234 → 1234).

## Files delivered

| Path | Action |
|------|--------|
| `33_PCS/include/uiox_klibc.h` | NEW — freestanding header |
| `33_PCS/01_schedular/include/sched_types.h` | UPDATED |
| `33_PCS/40_procStruct/include/process.h` | UPDATED |
| `33_PCS/02_MemMngnt/include/mm.h` | UPDATED |
| `33_PCS/patches/*.patch.txt` | Line-level instructions per .c |
| `33_PCS/patches/Makefile.patch.txt` | Add -nostdinc |