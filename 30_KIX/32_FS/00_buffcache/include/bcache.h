#ifndef UIOX_BCACHE_H
#define UIOX_BCACHE_H

#include "bcache_types.h"

/* ─────────────────────────────────────────────────────────────
 * Buffer Header  
 *
 * Every buffer lives simultaneously on:
 *   • exactly one hash queue  (always)
 *   • the LRU free list       (only when not busy)
 *
 * Hash queue  — circular doubly-linked list, one per hash slot.
 * Free list   — circular doubly-linked list with a dummy head.
 * ───────────────────────────────────────────────────────────── */
typedef struct BufHdr {
    /* ── Identity fields ──────────────────────────────────── */
    uint8_t   dev;           /* logical filesystem / device number   */
    uint32_t  blkno;         /* disk block number                    */

    /* ── State ────────────────────────────────────────────── */
    uint32_t  status;        /* OR of BUF_* flags                    */

    /* ── Data area ────────────────────────────────────────── */
    uint8_t   data[BLOCK_SIZE];

    /* ── Hash-queue doubly-linked list ────────────────────── */
    struct BufHdr *hash_next; /* forward  ptr on hash queue          */
    struct BufHdr *hash_prev; /* backward ptr on hash queue          */

    /* ── Free-list doubly-linked list ─────────────────────── */
    struct BufHdr *free_next; /* forward  ptr on free list           */
    struct BufHdr *free_prev; /* backward ptr on free list           */
} BufHdr;

/* ─────────────────────────────────────────────────────────────
 * Dummy-head nodes
 *
 * hash_heads[i]  — sentinel for hash queue i
 * free_head      — sentinel for the LRU free list
 *
 * Using sentinels removes all NULL-pointer edge cases and matches
 * the "doubly linked circular list with a dummy header" 
 * 
 * ───────────────────────────────────────────────────────────── */
extern BufHdr  hash_heads[NUM_HASH_QUEUES];
extern BufHdr  free_head;           /* dummy head of LRU free list  */
extern BufStats bcache_stats;

/* ─────────────────────────────────────────────────────────────
 * Initialisation
 * ───────────────────────────────────────────────────────────── */
void     bcache_init(void);

/* ─────────────────────────────────────────────────────────────
 * Algorithm 1 — getblk
 *
 * Allocate a locked buffer for (dev, blkno).
 * Implements all five scenarios 
 *
 *   Scenario 1 — block found in cache, buffer free
 *   Scenario 2 — block not in cache; clean free-list buffer reused
 *   Scenario 3 — block not in cache; delayed-write buf must be flushed
 *   Scenario 4 — block not in cache; free list empty → sleep
 *   Scenario 5 — block found in cache but buffer busy → sleep
 *
 * Returns a locked BufHdr.  Caller must eventually call brelse().
 * ───────────────────────────────────────────────────────────── */
BufHdr  *getblk(uint8_t dev, uint32_t blkno);

/* ─────────────────────────────────────────────────────────────
 * Algorithm 2 — brelse
 *
 * Release a locked buffer back to the pool.
 *   • Wakes processes sleeping on this buffer or any free buffer.
 *   • If valid and not old  → enqueue at TAIL  (LRU)
 *   • If invalid or old     → enqueue at HEAD  (evict soon)
 *   • Clears BUF_LOCKED.
 * ───────────────────────────────────────────────────────────── */
void     brelse(BufHdr *buf);

/* ─────────────────────────────────────────────────────────────
 * Algorithm 3 — bread
 *
 * Read a disk block into a buffer.
 * If the block is already valid in cache, return it immediately.
 * Otherwise initiate disk read, sleep until done, return buffer.
 * ───────────────────────────────────────────────────────────── */
BufHdr  *bread(uint8_t dev, uint32_t blkno);

/* ─────────────────────────────────────────────────────────────
 * Algorithm 4 — breada  (block read + read-ahead)
 *
 * Read 'blkno' immediately; initiate asynchronous read of
 * 'ra_blkno' (read-ahead block) in parallel.
 * Returns a locked buffer for 'blkno' only.
 * The read-ahead buffer is released automatically when I/O done.
 * ───────────────────────────────────────────────────────────── */
BufHdr  *breada(uint8_t dev, uint32_t blkno, uint32_t ra_blkno);

/* ─────────────────────────────────────────────────────────────
 * Algorithm 5 — bwrite
 *
 * Write a buffer to disk.
 *   synchronous  — sleep until I/O done; release buffer.
 *   asynchronous — start I/O; do NOT sleep; release on completion.
 *   delayed      — do NOT start I/O; mark BUF_DELWRITE; return.
 *
 * 'sync'    true  → synchronous write
 * 'delayed' true  → delayed write (implies !sync)
 * ───────────────────────────────────────────────────────────── */
void     bwrite(BufHdr *buf, bool sync, bool delayed);

/* ─────────────────────────────────────────────────────────────
 * Convenience helpers
 * ───────────────────────────────────────────────────────────── */

/* Mark a buffer for delayed write and release it */
void     bdwrite(BufHdr *buf);

/* Flush all delayed-write buffers to disk (like sync(2)) */
void     bflush(uint8_t dev);

/* Print pool state and statistics */
void     bcache_print(void);
void     bcache_stats_print(void);

#endif /* UIOX_BCACHE_H */
