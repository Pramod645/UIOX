/*
 *  31_BufferCache/00_FileBuff/include/bcache.h
 *
 *  The block buffer cache — Bach Ch.3.
 *
 *  ── Bach's five algorithms, one per file ──────────────────────────────
 *    buffers/src/getblk.c    getblk   allocate a buffer for a disk block
 *    buffers/src/brelse.c    brelse   release a buffer
 *    buffers/src/bread.c     bread    read a block
 *    buffers/src/breada.c    breada   read a block + read ahead
 *    buffers/src/bwrite.c    bwrite   write a block
 *
 *  plus bdwrite (delayed-write convenience) and bflush (drain the pool).
 *
 *  ── Bach's buffer header ──────────────────────────────────────────────
 *  The struct below is his BufferHeaders, with one deliberate difference:
 *
 *      Bach                       here
 *      ─────────────────────      ──────────────────────────────────────
 *      device_num  (uint8)        dev      (uint8)
 *      block_num   (uint8*)       blkno    (uint32)  ← a real number,
 *                                                 not a pointer
 *      status      (uint8)        status   (uint32)  ← four states Bach
 *                                                 packs in one byte no
 *                                                 longer fit in it
 *      *ptrtodataArea             data[]   ← inline array, not a pointer
 *      *ptr next/prev hash        hash_next, hash_prev
 *      *ptr next/prev free        free_next, free_prev
 *
 *  ── the five states Bach's `status` records ──────────────────────────
 *  He lists them in prose, so the bits are named after his sentences:
 *
 *    "The buffer is currently locked (locked is used busy sometimes)."
 *        → BUF_LOCKED
 *    "The buffer contains valid data."
 *        → BUF_VALID
 *    "The kernel must write the buffer contents to disk before
 *     reassigning the buffer.  This is known as delayed-write."
 *        → BUF_DELWRITE
 *    "The kernel is currently reading or writing the contents of the
 *     buffer to disk."
 *        → BUF_IOBUSY
 *    "A process is currently waiting for the buffer to become free."
 *        → BUF_WANTED
 *
 *  ── the interface 01_fsa consumes ────────────────────────────────────
 *  Every call takes (dev, blkno).  That is the contract 01_fsa's bmap.c,
 *  inode.c, namei.c and superblock.c were written against, and it is the
 *  same (dev, blkno) shape uiox_pc_map_fn_t declares one layer up.
 *
 *  @version 2.0.0  @date 2026-09-24
 */
#ifndef UIOX_BCACHE_H
#define UIOX_BCACHE_H

#include "bcache_types.h"

/* ═════════════════════════════════════════════════════════════════════
 * Buffer header — Bach's BufferHeaders
 *
 * Every buffer lives simultaneously on:
 *   • exactly one hash queue  (always)
 *   • the LRU free list       (only when not busy)
 *
 * Hash queue  — doubly-linked, one per hash slot.
 * Free list   — doubly-linked circular, with a dummy head.
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct BufHdr {
    /* ── Identity ─────────────────────────────────────────────────── */
    uint8_t   dev;            /* filesystem the block belongs to        */
    uint32_t  blkno;          /* block number within that filesystem    */

    /* ── State — Bach's five conditions ───────────────────────────── */
    uint32_t  status;         /* OR of the BUF_* bits                   */

    /* ── Data area ────────────────────────────────────────────────── */
    uint8_t   data[BLOCK_SIZE];

    /* ── Hash-queue links ─────────────────────────────────────────── */
    struct BufHdr *hash_next;
    struct BufHdr *hash_prev;

    /* ── Free-list links ──────────────────────────────────────────── */
    struct BufHdr *free_next;
    struct BufHdr *free_prev;
} BufHdr;

/* ── The pool, and the two sentinels ───────────────────────────────── */
extern BufHdr   hash_heads[NUM_HASH_QUEUES];
extern BufHdr   free_head;          /* dummy head of the LRU free list */
extern BufStats bcache_stats;

/* ── Initialisation ────────────────────────────────────────────────── */
void     bcache_init(void);

/* ── Algorithm 1 — getblk ───────────────────────────────────────────
 * Allocate a locked buffer for (dev, blkno).  Bach's five scenarios:
 *
 *   1  block on its hash queue, buffer free       → lock and return
 *   2  block not in cache, a free buffer exists   → reassign and return
 *   3  block not in cache, free buffer is
 *        marked delayed-write                     → write it, then retry
 *   4  block not in cache, free list EMPTY        → sleep / bounded spin
 *   5  block on its hash queue but buffer busy    → sleep / bounded spin
 *
 * Scenarios 4 and 5 are the ones that do NOT return — they loop.  A
 * kernel with no scheduler cannot sleep, so this implementation spins
 * with a bound and reports exhaustion rather than hanging forever.
 *
 * Returns a locked BufHdr, or NULL if the bound was reached.
 * ─────────────────────────────────────────────────────────────────── */
BufHdr  *getblk(uint8_t dev, uint32_t blkno);

/* ── Algorithm 2 — brelse ───────────────────────────────────────────
 * Release a locked buffer back to the pool.
 *
 *   • wake processes waiting on this buffer, and on any buffer
 *   • contents valid and not old → enqueue at TAIL (LRU: keep longer)
 *   • contents invalid or old    → enqueue at HEAD (evict soon)
 *   • clear BUF_LOCKED
 *
 * The TAIL/HEAD placement IS the replacement policy — a stale buffer is
 * reused first.  Appending unconditionally degrades LRU toward FIFO.
 * ─────────────────────────────────────────────────────────────────── */
void     brelse(BufHdr *buf);

/* ── Algorithm 3 — bread ────────────────────────────────────────────
 * Read a block.  Cache hit returns immediately; a miss reads the device.
 * Returns a locked buffer.
 * ─────────────────────────────────────────────────────────────────── */
BufHdr  *bread(uint8_t dev, uint32_t blkno);

/* ── Algorithm 4 — breada ───────────────────────────────────────────
 * Read 'blkno' immediately and start an asynchronous read of 'ra_blkno'.
 * Returns a locked buffer for 'blkno' only; the read-ahead buffer is
 * released when its I/O completes.
 *
 * Bach checks the cache for EACH block separately and falls back to
 * bread() when the first was already cached — this implementation does
 * the same, so a read-ahead never displaces or double-locks a block that
 * was already resident.
 *
 * A ra_blkno of 0 means "no next block known" and is treated as bread.
 * ─────────────────────────────────────────────────────────────────── */
BufHdr  *breada(uint8_t dev, uint32_t blkno, uint32_t ra_blkno);

/* ── Algorithm 5 — bwrite ───────────────────────────────────────────
 * Write a buffer to disk.
 *
 *   sync    true   initiate the write and WAIT; release the buffer
 *   delayed true   do NOT write now; mark BUF_DELWRITE and release
 *   neither        start the write and return; the buffer stays locked
 *                  until the I/O completes
 *
 * ── the async case, and why it is now safe ─────────────────────────
 * The earlier version left the third case to the caller, with no
 * completion callback anywhere in the layer — so the buffer stayed
 * BUF_LOCKED forever and getblk spun on it.  A filesystem that allocates
 * 256 blocks would drain the pool and hang.
 *
 * This implementation has no interrupt-driven completion to hook, so the
 * third case is CLOSED: 'delayed' chooses between a synchronous write
 * and a marked-for-later one, and there is no path that leaves a buffer
 * locked with no release.  Bach's third case needs a real interrupt
 * handler; when the BSP provides one, the async arm comes back here.
 * ─────────────────────────────────────────────────────────────────── */
void     bwrite(BufHdr *buf, bool sync, bool delayed);

/* ── bdwrite — delayed write ────────────────────────────────────────
 * Bach's bdwrite: mark for delayed write and release.  The buffer is
 * flushed by bflush(), by getblk's scenario-3 path, or by the caller
 * running out of buffers.
 * ─────────────────────────────────────────────────────────────────── */
void     bdwrite(BufHdr *buf);

/* ── bflush — write every delayed buffer for one device ─────────────
 * Called by sync/fsync from above.  Bach's "update super block, inode,
 * flush buffers".
 * ─────────────────────────────────────────────────────────────────── */
void     bflush(uint8_t dev);

/* ── bflush_all — every delayed buffer, all devices ────────────────  */
void     bflush_all(void);

/* ── Debug ─────────────────────────────────────────────────────────── */
void     bcache_print(void);
void     bcache_stats_print(void);

#endif /* UIOX_BCACHE_H */
