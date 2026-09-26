/*
 *  31_BufferCache/00_FileBuff/src/breada.c
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
 *  ── the shape of the algorithm, and where this diverges ───────────────
 *  Bach tests the cache BEFORE getblk, for each block separately, and the
 *  third step re-runs bread when the first block was already resident.
 *  He needs that because getblk on a cached block returns it LOCKED, and
 *  the third step is what guarantees the caller holds a lock on the block
 *  it asked for even when the read path above was skipped.
 *
 *  THIS IMPLEMENTATION HAS ONE CALL SITE FOR THE IMMEDIATE BLOCK, not two.
 *  bread() already performs the same cache test and returns immediately on
 *  a hit, so Bach's two paths collapse into one:
 *
 *      Bach                                    here
 *      ──────────────────────────────────      ──────────────────────────
 *      if (not in cache) { getblk; read; }     b = bread(dev, blkno);
 *      if (was in cache)  { bread; return; }
 *
 *  The earlier revision kept both, with a first_was_cached flag choosing
 *  between them.  That produced a `b` the compiler could not prove was
 *  initialised on every path (-Werror=maybe-uninitialized), and — more
 *  importantly — it RETURNED EARLY when the first block was cached, so the
 *  read-ahead never ran in exactly the case it is most useful: a
 *  sequential scan that keeps finding the current block already resident.
 *
 *  ── what is kept from Bach ────────────────────────────────────────────
 *    · the read-ahead block is cache-tested before getblk
 *    · a resident read-ahead block is NOT re-read and NOT displaced
 *    · the read-ahead buffer is released when its I/O completes — with a
 *      synchronous platform hook, immediately
 *    · the returned buffer is the immediate block, locked
 *
 *  ── what is not asynchronous ──────────────────────────────────────────
 *  Bach's second read is started in parallel and completed by an interrupt.
 *  This layer has no interrupt to hook, so the copy happens inline.  The
 *  BENEFIT — the next block is resident before it is asked for — is
 *  preserved; only the concurrency is not.  The same applies to Bach's
 *  sleep(event first buffer contains valid data): there is nothing to
 *  sleep against, so the buffer is returned ready.
 *
 *  ── where the second argument comes from ──────────────────────────────
 *  01_fsa's bmap() fills BmapResult.readahead_blk on every direct-block
 *  hit, and readwrite.c's rw_bread() passes it here.  A value of 0 means
 *  "no next block known" — the end of the direct range, or an indirect
 *  level where bmap did not compute one — and is served as plain bread.
 *
 *  @version 2.2.0  @date 2026-09-25
 */
#include "bcache.h"
#include "bcache_internal.h"
#include "uiox_klibc.h"        /* uint*_t, bool, memset, memcpy, printf */
#include "uiox_soc_stdio.h"    /* early_puts — NOT printf */

BufHdr *breada(uint8_t dev, uint32_t blkno, uint32_t ra_blkno)
{
    BufHdr *b;
    BufHdr *ra;

    /* ── an absent read-ahead is just a read ───────────────────────── */
    if (ra_blkno == 0u || ra_blkno == blkno)
        return bread(dev, blkno);

    /* ══ step 1: the immediate block, locked, for the caller ═════════
     * Bach: "if (first block not in cache) { get buffer for first block
     * (algorithm getblk); if (buffer data not valid) initiate disk read; }"
     * followed by "if (first block was originally in cache) { read first
     * block (algorithm bread); return buffer; }"
     *
     * Both branches do the same thing — fetch the immediate block and
     * hand it back locked — so they collapse into one call.  bread()
     * tests the cache itself: a hit returns the buffer immediately, a
     * miss reads the device.  Either way b is assigned, so there is no
     * path on which the return below reads an uninitialised pointer. */
    b = bread(dev, blkno);
    if (!b) return (BufHdr *)0;         /* pool starved */

    /* ── step 2: the read-ahead block ───────────────────────────────
     * Bach checks the cache FIRST.  If it is already there and valid,
     * the read-ahead is pointless — the block is left alone rather than
     * re-read or moved in the replacement order.
     *
     * Bach's step: "if (second block not in cache) { get buffer for
     * second block (algorithm getblk); if (buffer data valid) release
     * buffer (algorithm brelse); else initiate disk read; }" */
    ra = bcache_hash_lookup(dev, ra_blkno);

    if (!ra || !(ra->status & BUF_VALID)) {
        BufHdr *ra_b = getblk(dev, ra_blkno);

        if (ra_b) {
            if (ra_b->status & BUF_VALID) {
                /* It came in valid between the two checks — Bach:
                 * "if (buffer data valid) release buffer". */
                brelse(ra_b);
            } else {
                ra_b->status |= BUF_IOBUSY;
                bcache_plat_read_block(dev, ra_blkno, ra_b->data);
                ra_b->status &= ~BUF_IOBUSY;
                ra_b->status |=  BUF_VALID;
                bcache_stats.readaheads++;

                /* The read-ahead buffer is released when its I/O is
                 * done — Bach: "the read-ahead buffer is released
                 * automatically when I/O done".  With a synchronous
                 * platform hook, that is now. */
                brelse(ra_b);
            }
        }
        /* A starved pool on the read-ahead is NOT fatal: the block the
         * caller asked for is in hand, and it is the one that matters. */
    }

    /* ── step 3: return the immediate block ═════════════════════════
     * Bach's sleep(event first buffer contains valid data) has nothing
     * to wait on: the read in step 1 was synchronous, so b is valid and
     * locked already.  It is returned as-is.
     *
     * Bach returns here too — "return buffer" once the first block is
     * in hand — and this is that return, for both of his paths. */
    return b;
}
