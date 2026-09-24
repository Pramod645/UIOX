/*
 *  31_BufferCache/00_FileBuff/buffers/src/breada.c
 *
 *  Algorithm breada — Bach, Ch.3 §2.  Read a block and read ahead.
 *
 *  ── Bach's algorithm, verbatim ────────────────────────────────────────
 *  input:  1. file system block number for immediate read
 *          2. file system block number for asynchronous read
 *  output: buffer containing data for the immediate read
 *  {
 *      if (first block not in cache)
 *      {
 *          get buffer for first block (algorithm getblk);
 *          if (buffer data not valid)
 *              initiate disk read;
 *      }
 *      if (second block not in cache)
 *      {
 *          get buffer for second block (algorithm getblk);
 *          if (buffer data valid)
 *              release buffer (algorithm brelse);
 *          else
 *              initiate disk read;
 *      }
 *      if (first block was originally in cache)
 *      {
 *          read first block (algorithm bread);
 *          return buffer;
 *      }
 *      sleep(event first buffer contains valid data);
 *      return buffer;
 *  }
 *
 *  ── the shape of the algorithm, which is easy to get wrong ────────────
 *  Bach tests the cache BEFORE calling getblk, for each block separately,
 *  and the third step re-runs bread when the first block was already
 *  resident.  That is not redundant: getblk on a cached block returns it
 *  LOCKED, and the third step is what guarantees the caller gets a lock
 *  on the block it asked for even when the read path below was skipped.
 *
 *  ── what this implementation keeps, and what it changes ───────────────
 *  KEPT:
 *    · the per-block cache test before getblk
 *    · the read-ahead buffer is released when its I/O completes — here
 *      the "I/O" is a synchronous platform copy, so release is immediate
 *    · a cached read-ahead block is NOT re-read and NOT displaced
 *    · the returned buffer is the immediate block, locked
 *
 *  CHANGED:
 *    · the sleep in the third step has nothing to sleep against, so the
 *      immediate read is completed inline and the buffer returned ready
 *    · the read-ahead is not asynchronous.  Bach's "asynchronous read"
 *      needs an interrupt to complete it; with a synchronous platform
 *      hook the copy happens now.  The BENEFIT Bach is after — the next
 *      block is resident before it is asked for — is preserved; only the
 *      concurrency is not.
 *
 *  ── where the second argument comes from ──────────────────────────────
 *  01_fsa's bmap() fills BmapResult.readahead_blk on every direct-block
 *  hit, and readwrite.c's rw_bread() passes it here.  A value of 0 means
 *  "no next block known" — the end of the direct range, or an indirect
 *  level where bmap did not compute one — and is served as plain bread.
 *
 *  @version 2.0.0  @date 2026-09-24
 */
#include "bcache.h"
#include "bcache_internal.h"

BufHdr *breada(uint8_t dev, uint32_t blkno, uint32_t ra_blkno)
{
    BufHdr  *b;
    int      first_was_cached;

    /* ── an absent read-ahead is just a read ───────────────────────── */
    if (ra_blkno == 0u || ra_blkno == blkno)
        return bread(dev, blkno);

    /* ── step 1: is the FIRST block already in the cache? ───────────
     * Bach tests before getblk, because the answer decides whether the
     * third step has to re-read. */
    first_was_cached = (bcache_hash_lookup(dev, blkno) != (BufHdr *)0);

    /* Bach: "if (first block not in cache) { get buffer for first block;
     * if (buffer data not valid) initiate disk read; }" */
    if (!first_was_cached) {
        b = getblk(dev, blkno);
        if (!b) return (BufHdr *)0;     /* pool starved — logged */

        if (!(b->status & BUF_VALID)) {
            b->status |= BUF_IOBUSY;
            bcache_plat_read_block(dev, blkno, b->data);
            b->status &= ~BUF_IOBUSY;
            b->status |=  BUF_VALID;
            bcache_stats.reads++;
        }
    }

    /* ── step 2: the read-ahead block ───────────────────────────────
     * Bach checks the cache FIRST.  If it is already there and valid,
     * the read-ahead is pointless — the block is left alone rather than
     * re-read or moved in the replacement order. */
    {
        BufHdr *ra = bcache_hash_lookup(dev, ra_blkno);

        if (ra && (ra->status & BUF_VALID)) {
            /* already resident — nothing to do, and it must NOT be
             * displaced or re-read */
        } else {
            BufHdr *ra_b = getblk(dev, ra_blkno);

            if (ra_b) {
                if (ra_b->status & BUF_VALID) {
                    /* Rach: "if (buffer data valid) release buffer" —
                     * it came in valid between the two checks. */
                    brelse(ra_b);
                } else {
                    ra_b->status |= BUF_IOBUSY;
                    bcache_plat_read_block(dev, ra_blkno, ra_b->data);
                    ra_b->status &= ~BUF_IOBUSY;
                    ra_b->status |=  BUF_VALID;
                    bcache_stats.readaheads++;
                }

                /* The read-ahead buffer is released when its I/O is
                 * done — Bach's "the read-ahead buffer is released
                 * automatically when I/O done".  With a synchronous
                 * platform hook, that is now. */
                brelse(ra_b);
            }
            /* A starved pool on the read-ahead is NOT fatal: the block
             * the caller asked for is the one that matters, and it is
             * already in hand below. */
        }
    }

    /* ── step 3: the immediate block, locked, for the caller ────────
     * Bach: "if (first block was originally in cache) { read first
     * block (bread); return buffer; }"  bread() returns it locked. */
    if (first_was_cached) {
        b = bread(dev, blkno);
        return b;
    }

    /* Bach's sleep(event first buffer contains valid data) has nothing
     * to wait on: the read in step 1 was synchronous, so b is valid and
     * locked already.  It is returned as-is. */
    return b;
}
