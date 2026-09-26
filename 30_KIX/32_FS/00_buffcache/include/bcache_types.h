/*
 *  31_BufferCache/00_FileBuff/include/bcache_types.h
 *
 *  Geometry, limits, status flags and statistics for the block buffer
 *  cache.  Included by bcache.h, and transitively by every file that
 *  touches a buffer.
 *
 *  ── THE TWO STDIO HEADERS, AND WHY THIS FILE USES NEITHER ─────────────
 *  This tree has two headers that both declare uiox_printf and both
 *  define the printf macro:
 *
 *      uiox_klibc.h:164       extern int  uiox_printf(const char *fmt, ...)
 *      uiox_klibc.h:221       #define printf  uiox_printf
 *
 *      uiox_soc_stdio.h:34    void        uiox_printf(const char *fmt, ...)
 *      uiox_soc_stdio.h:59    #define printf(...)  uiox_printf(__VA_ARGS__)
 *
 *  Same symbol, different return type; same macro name.  A translation
 *  unit that includes BOTH fails under -Werror with:
 *
 *      error: conflicting types for 'uiox_printf'
 *      error: 'printf' redefined
 *
 *  The two are ALTERNATIVES, not layers.  uiox_soc_stdio.h is the SoC
 *  backend's replacement for <stdio.h> and is self-contained — it
 *  includes uiox_base_types.h, not uiox_klibc.h.  uiox_klibc.h is the
 *  freestanding libc for kernel-side files.
 *
 *  An earlier revision of this header included uiox_klibc.h.  That was
 *  the wrong choice here for two reasons:
 *
 *    1. It dragged klibc's uiox_printf into every buffer source, so any
 *       buffer source that also needed early_puts() clashed.
 *    2. It was redundant — bcache.h's own consumers already have the
 *       integer types from whichever stdio header they chose.
 *
 *  What this file needs is uint8_t / uint32_t / uint64_t / uintptr_t /
 *  bool.  Those are the same in both headers, so this file does not name
 *  either: it takes them from whatever the including translation unit
 *  already has, and documents that requirement below.
 *
 *  ── WHAT AN INCLUDER MUST PROVIDE ─────────────────────────────────────
 *  Before including bcache.h (or bcache_types.h), a translation unit must
 *  have included ONE of:
 *
 *      uiox_klibc.h        — kernel-side files: printf returns int
 *      uiox_soc_stdio.h    — SoC/driver files:  printf returns void,
 *                            and early_puts() is available
 *
 *  ...or its own typedefs.  NEVER both — that is the clash above.
 *
 *  @version 3.0.0  @date 2026-09-25
 */
#ifndef UIOX_BCACHE_TYPES_H
#define UIOX_BCACHE_TYPES_H

/* ═════════════════════════════════════════════════════════════════════
 * The integer types
 *
 * Taken from uiox_base_types.h — the small header BOTH stdio variants
 * include, and the only one that is safe to include alongside either.
 * It carries the integer typedefs and nothing that clashes.
 * ═════════════════════════════════════════════════════════════════════ */
#include "uiox_base_types.h"

/* ═════════════════════════════════════════════════════════════════════
 * Geometry
 * ═════════════════════════════════════════════════════════════════════ */
#ifndef BCACHE_SECTOR_SIZE
#define BCACHE_SECTOR_SIZE       512u     /* physical sector — fixed      */
#define BCACHE_PAGE_SIZE         4096u    /* logical page  = 8 sectors    */
#define BCACHE_BLOCKS_PER_PAGE   (BCACHE_PAGE_SIZE / BCACHE_SECTOR_SIZE) /* 8 */

/* BLOCK_SIZE stays an alias for the SECTOR size.  fs_types.h, bmap.h and
 * the filesystem's on-disk geometry are all counted in 512-byte blocks,
 * so this must not be repointed at the page size. */
#define BLOCK_SIZE               BCACHE_SECTOR_SIZE

#define NUM_BUFFERS              256u     /* block buffer pool size       */
#define NUM_HASH_QUEUES          64u      /* hash table size (power of 2) */
#define MAX_DEVICES              8u       /* logical device count         */
#endif /* BCACHE_SECTOR_SIZE */

/* ═════════════════════════════════════════════════════════════════════
 * Platform geometry
 *
 * How many blocks a device holds, in SECTOR units.  Returns 0 for
 * "unknown", which makes callers skip the bound check rather than treat
 * every block as past the end.
 *
 * DECLARED, not defined — the one definition is in bcache_init.c;
 * 10_BSP provides its own for a real device by not linking ours.
 * ═════════════════════════════════════════════════════════════════════ */
/* ── the device size, in BCACHE_SECTOR_SIZE blocks ────────────────────
 * 524288 x 512 B = 256 MiB, matching MAX_BLOCKS in 01_fsa/fs_types.h.
 * Those two constants describe the same volume and must agree: with
 * MAX_BLOCKS higher than this, fs_alloc() hands out block numbers past
 * the device end and bread() reports them only through the read_oob
 * counter — a silent short read, not a clear failure.
 *
 * RAISING THIS RAISES THE DRAM FOOTPRINT.  The default backing in
 * bcache_init.c places device N at
 *
 *     BCACHE_DRAM_BASE + N * BCACHE_DEV_STRIDE_DEFAULT
 *
 * and the stride is this value rounded up to a page multiple:
 *
 *     stride = 524288 x 512 = 268435456 B = 256 MiB
 *     x MAX_DEVICES (8)    = 2147483648 B =   2 GiB
 *
 * So the default DRAM window is now 2 GiB, starting at 0x46000000.
 * A device that cannot map that much must override
 * bcache_plat_dram_base() AND bcache_plat_num_blocks() from 10_BSP —
 * which is what those hooks exist for. */
#define NUM_DISK_BLOCKS_DEFAULT  524288u

uint32_t bcache_plat_num_blocks(uint8_t dev);

/* ═════════════════════════════════════════════════════════════════════
 * Buffer status flags — Bach's five conditions, plus three of ours
 *
 * Bach describes five states in prose:
 *     locked, valid, delayed-write, I/O in progress, process waiting
 *
 * BUF_ASYNC, BUF_ERROR and BUF_DIRTY are additions: the first marks a
 * read-ahead in flight, the second records a failed device operation,
 * and the third is used by the page cache one layer up, which shares
 * this flag word rather than defining its own.
 * ═════════════════════════════════════════════════════════════════════ */
#define BUF_LOCKED   (1u << 0)   /* buffer is locked — in use            */
#define BUF_VALID    (1u << 1)   /* contains valid data                  */
#define BUF_DELWRITE (1u << 2)   /* delayed write — flush before reuse   */
#define BUF_IOBUSY   (1u << 3)   /* I/O in progress                      */
#define BUF_WANTED   (1u << 4)   /* a process is waiting for it          */
#define BUF_OLD      (1u << 5)   /* goes to head of free list            */
#define BUF_ASYNC    (1u << 6)   /* asynchronous I/O in flight           */
#define BUF_ERROR    (1u << 7)   /* I/O error occurred                   */
#define BUF_DIRTY    (1u << 8)   /* page cache dirty (page cache use)    */

/* ═════════════════════════════════════════════════════════════════════
 * Statistics
 *
 * The first eight are Bach's three getblk scenarios, the I/O counts, and
 * the two back-pressure counters — free_waits and busy_waits are
 * scenarios 4 and 5, and either climbing means the pool is at or past its
 * working set.
 *
 * The last four are errors, COUNTED rather than logged: each replaced a
 * printf() that sat on the data path, and a block read cannot afford a
 * console write.
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct {
    uint64_t hits;             /* scenario 1 — found in cache, free    */
    uint64_t misses;           /* scenario 2 — free buffer reassigned  */
    uint64_t delayed_writes;   /* scenario 3 — delayed buffer flushed  */
    uint64_t free_waits;       /* scenario 4 — free list empty         */
    uint64_t busy_waits;       /* scenario 5 — cached but locked       */
    uint64_t reads;            /* device reads issued                  */
    uint64_t writes;           /* device writes issued                 */
    uint64_t readaheads;       /* read-ahead blocks fetched            */

    /* ── errors, counted instead of logged ──────────────────────── */
    uint64_t starved;          /* getblk gave up — pool never freed    */
    uint64_t read_oob;         /* read past the device end             */
    uint64_t write_oob;        /* write past the device end — dropped  */
    uint64_t flushed;          /* delayed buffers written by bflush    */
} BufStats;

#endif /* UIOX_BCACHE_TYPES_H */
