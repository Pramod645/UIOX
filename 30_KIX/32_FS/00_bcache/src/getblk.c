/*
 *  31_BufferCache/00_FileBuff/buffers/src/getblk.c
 *
 *  Algorithm getblk — Bach, The Design of the UNIX Operating System,
 *  Ch.3 §2.  Allocates a buffer for a disk block.
 *
 *  ── Bach's algorithm, verbatim ────────────────────────────────────────
 *  input:  file system number, block number
 *  output: locked buffer that can now be used for block
 *  {
 *      while (buffer not found)
 *      {
 *          if (block in hash queue)
 *          {
 *              if (buffer busy)                        // scenario 5
 *              {
 *                  sleep(event buffer becomes free);
 *                  continue;                            // back to while
 *              }
 *              mark buffer busy;                        // scenario 1
 *              remove buffer from free list;
 *              return buffer;
 *          }
 *          else  // block not on hash queue 
 *          {
 *              if (there are no buffers on free list)   // scenario 4
 *              {
 *                  sleep(event any buffer becomes free);
 *                  continue;                            // back to while
 *              }
 *              remove buffer from free list;
 *              if (buffer marked for delayed write)     // scenario 3
 *              {
 *                  asynchronous write buffer to disk;
 *                  continue;                            // back to while
 *              }
 *                                                      // scenario 2
 *              remove buffer from old hash queue;
 *              put buffer onto new hash queue;
 *              return buffer;
 *          }
 *      }
 *  }
 *
 *  ── the two loops that do not return ──────────────────────────────────
 *  Scenarios 4 and 5 call sleep().  This kernel has no scheduler to sleep
 *  against, so both spin WITH A BOUND: if the pool does not free up
 *  within BCACHE_SPIN_LIMIT passes, getblk returns NULL and the caller
 *  sees a real error instead of a hang.
 *
 *  That bound matters.  Without it a buffer left locked forever — which
 *  is what the previous bwrite's async arm did — drains the pool and the
 *  kernel stops with no diagnostic at all.  BCACHE_SPIN_LIMIT is the
 *  difference between "the filesystem reports ENOSPC" and "nothing
 *  happens".
 *
 *  ── scenario 3, and the two write forms ───────────────────────────────
 *  Bach says "asynchronous write buffer to disk".  This layer has no
 *  interrupt-driven completion to hook, so the flush here is the
 *  synchronous one — the buffer is written and released before the loop
 *  retries.  The effect Bach wants (a delayed-write buffer must reach
 *  disk before its slot is reused) is preserved; only the concurrency
 *  differs.
 *
 *  ── the hash function ─────────────────────────────────────────────────
 *  Bach: "Kernel uses hashing function that distributes the buffer
 *  uniformly across the set of hash queues and hash function is simple
 *  to improve performance."  Ours folds dev in with a small multiplier
 *  and masks — cheap, and uniform enough for 256 buffers over 64 queues.
 *
 *  @version 2.0.0  @date 2026-09-24
 */
#include "bcache.h"
#include "bcache_internal.h"

/* How many times a scenario-4/5 loop may go round before giving up. */
#define BCACHE_SPIN_LIMIT  100000u

/* ═════════════════════════════════════════════════════════════
 * Hash queue operations
 * ═════════════════════════════════════════════════════════════ */

uint32_t bcache_hash_slot(uint8_t dev, uint32_t blkno)
{
    return (((uint32_t)dev * 31u) ^ blkno) % NUM_HASH_QUEUES;
}

void bcache_hash_insert(BufHdr *b)
{
    uint32_t slot = bcache_hash_slot(b->dev, b->blkno);

    b->hash_next       = hash_heads[slot].hash_next;
    b->hash_prev       = &hash_heads[slot];
    if (b->hash_next) b->hash_next->hash_prev = b;
    hash_heads[slot].hash_next = b;
}

void bcache_hash_remove(BufHdr *b)
{
    if (b->hash_prev) b->hash_prev->hash_next = b->hash_next;
    if (b->hash_next) b->hash_next->hash_prev = b->hash_prev;

    b->hash_next = (BufHdr *)0;
    b->hash_prev = (BufHdr *)0;
}

BufHdr *bcache_hash_lookup(uint8_t dev, uint32_t blkno)
{
    uint32_t slot = bcache_hash_slot(dev, blkno);
    BufHdr  *b    = hash_heads[slot].hash_next;

    while (b) {
        if (b->dev == dev && b->blkno == blkno) return b;
        b = b->hash_next;
    }
    return (BufHdr *)0;
}

/* ═════════════════════════════════════════════════════════════
 * Free list operations
 *
 * Bach, on the policy: "used this buffer pool according to least
 * recently used algorithm.  After it allocates a buffer to a disk, it
 * cannot use the buffer for another block until all other buffers have
 * been used more recently."
 *
 * So: the HEAD of the list is the least-recently-used buffer and is
 * taken first; the TAIL is the most-recently-used.
 * ═════════════════════════════════════════════════════════════ */

void bcache_fl_remove(BufHdr *b)
{
    if (b->free_prev) b->free_prev->free_next = b->free_next;
    if (b->free_next) b->free_next->free_prev = b->free_prev;

    b->free_next = (BufHdr *)0;
    b->free_prev = (BufHdr *)0;
}

/* Insert before the sentinel = the TAIL = most-recently-used */
void bcache_fl_insert_tail(BufHdr *b)
{
    BufHdr *prev = free_head.free_prev;

    b->free_prev          = prev;
    b->free_next          = &free_head;
    prev->free_next       = b;
    free_head.free_prev   = b;
}

/* Insert after the sentinel = the HEAD = evicted next */
void bcache_fl_insert_head(BufHdr *b)
{
    BufHdr *next = free_head.free_next;

    b->free_next        = next;
    b->free_prev        = &free_head;
    next->free_prev     = b;
    free_head.free_next = b;
}

/* Take the least-recently-used buffer, or NULL when the list is empty. */
BufHdr *bcache_fl_pop_head(void)
{
    BufHdr *b;

    if (free_head.free_next == &free_head) return (BufHdr *)0;

    b = free_head.free_next;
    bcache_fl_remove(b);
    return b;
}

/* Is the free list empty?  Scenario 4's test. */
int bcache_fl_empty(void)
{
    return (free_head.free_next == &free_head) ? 1 : 0;
}

/* ═════════════════════════════════════════════════════════════
 * Algorithm getblk  —  Bach Ch.3 §2
 * ═════════════════════════════════════════════════════════════ */
BufHdr *getblk(uint8_t dev, uint32_t blkno)
{
    uint32_t spins = 0u;

    while (spins++ < BCACHE_SPIN_LIMIT) {
        BufHdr *b = bcache_hash_lookup(dev, blkno);

        /* ── the block IS on its hash queue ────────────────────────── */
        if (b) {
            /* scenario 5 — found, but a process holds it */
            if (b->status & BUF_LOCKED) {
                bcache_stats.busy_waits++;
                continue;                   /* Bach: sleep, retry */
            }

            /* scenario 1 — found and free */
            bcache_fl_remove(b);
            b->status |= BUF_LOCKED;
            b->status &= ~BUF_WANTED;
            bcache_stats.hits++;
            return b;
        }

        /* ── the block is NOT in the cache ─────────────────────────── */

        /* scenario 4 — no buffer available at all */
        if (bcache_fl_empty()) {
            bcache_stats.free_waits++;
            continue;                       /* Bach: sleep, retry */
        }

        b = bcache_fl_pop_head();

        /* scenario 3 — the buffer we took is marked for delayed write.
         * Write it before its slot is reused, then retry. */
        if (b->status & BUF_DELWRITE) {
            bcache_stats.delayed_writes++;

            b->status |= BUF_LOCKED;
            bcache_plat_write_block(b->dev, b->blkno, b->data);
            b->status &= ~(BUF_DELWRITE | BUF_IOBUSY);
            b->status |= BUF_VALID;
            brelse(b);                      /* back on the free list */

            continue;                       /* Bach: continue */
        }

        /* scenario 2 — reassign this free buffer to the new block */
        bcache_hash_remove(b);

        b->dev    = dev;
        b->blkno  = blkno;
        b->status = BUF_LOCKED;   /* valid cleared — caller must read */

        bcache_hash_insert(b);

        bcache_stats.misses++;
        return b;
    }

    /* The bound was reached.  The pool is starved — every buffer is
     * locked and nothing released one.  Reported rather than spun on. */
    printf("[getblk] ERROR: pool starved after %u spins "
           "(dev=%u blk=%u) — a buffer is locked and never released\n",
           (unsigned)BCACHE_SPIN_LIMIT, (unsigned)dev, (unsigned)blkno);
    return (BufHdr *)0;
}
