/*
 *  31_BufferCache/00_FileBuff/include/bcache_types.h
 *
 *  Geometry, limits, status flags and statistics for the block buffer
 *  cache.  Included by bcache.h, and transitively by every file in the
 *  three layers that touches a buffer.
 *
 *  ── why the two sizes are separate ────────────────────────────────────
 *  A physical sector is 512 bytes and that is fixed by the device.  A
 *  logical page is 4096 — eight sectors — and that is a policy choice,
 *  because the page cache above works in pages while this layer works in
 *  sectors.
 *
 *      BCACHE_SECTOR_SIZE   512    the unit this layer moves
 *      BCACHE_PAGE_SIZE    4096    the unit the page cache above uses
 *
 *  BLOCK_SIZE is kept as an alias for the SECTOR size, because the
 *  filesystem layer's own headers (fs_types.h, bmap.h) were written
 *  against a 512-byte block and that must not change underneath them.
 *  A file's block map, its directory entries and its super block geometry
 *  are all counted in 512-byte units.
 *
 *  ── CHANGED in this revision ───────────────────────────────────────────
 *  bcache_plat_num_blocks() is DECLARED here, not defined.
 *
 *  The earlier revision had it as
 *
 *      __attribute__((weak))
 *      static inline uint32_t bcache_plat_num_blocks(uint8_t dev) { ... }
 *
 *  A static inline has internal linkage: it is a private copy in every
 *  translation unit that includes this header, so a BSP cannot replace
 *  it at link time.  The header's copy always wins inside each unit — so
 *  "override from 10_BSP for real device size" could not work, and
 *  bread.c's past-device-end check always read the default.
 *
 *  It is now an extern declaration with ONE definition, which lives in
 *  bcache_init.c.  10_BSP overrides it by providing its own definition
 *  and not linking ours.  The matching declaration is also in
 *  bcache_internal.h; this one is here so the type is complete for any
 *  caller that includes only bcache_types.h.
 *
 *  @version 2.1.0  @date 2026-09-24
 */
#ifndef UIOX_BCACHE_TYPES_H
#define UIOX_BCACHE_TYPES_H

//#include "uiox_base_types.h"
#include "uiox_klibc.h"

/* ═════════════════════════════════════════════════════════════════════
 * Geometry
 * ═════════════════════════════════════════════════════════════════════ */
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

/* ═════════════════════════════════════════════════════════════════════
 * Platform geometry
 *
 * How many blocks a device holds, in SECTOR units.  Returns 0 for
 * "unknown", which makes callers skip the bound check rather than treat
 * every block as past the end.
 *
 * DECLARED, not defined — see the header note.  The one definition is in
 * bcache_init.c; 10_BSP provides its own for a real device.
 * ═════════════════════════════════════════════════════════════════════ */
#define NUM_DISK_BLOCKS_DEFAULT  4096u

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
 * The first four are Bach's three getblk scenarios plus I/O counts.
 * The last four are back-pressure: free_waits and busy_waits are
 * scenarios 4 and 5, and either climbing means the pool is at or past
 * its working set.
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
} BufStats;

#endif /* UIOX_BCACHE_TYPES_H */
