/*
 *  31_BufferCache/00_FileBuff/buffers/src/bwrite.c
 *
 *  Algorithm bwrite — Bach, Ch.3 §2.  Write a block.
 *  Plus bdwrite (delayed write) and bflush (drain the pool).
 *
 *  ── Bach's algorithm, verbatim ────────────────────────────────────────
 *  input:  buffer
 *  output: none
 *  {
 *      initiate disk write;
 *      if (I/O synchronous)
 *      {
 *          sleep(event I/O complete);
 *          release buffer (algorithm brelse);
 *      }
 *      else if (buffer marked for delayed write)
 *          mark buffer to put at head of free list;
 *  }
 *
 *  ── what changed from the earlier version, and why it mattered ────────
 *  The earlier bwrite had three arms: sync, delayed, and a third that
 *  started the write and returned with the buffer still BUF_LOCKED, on
 *  the promise that "the caller releases on completion".  There was no
 *  completion callback anywhere in the layer, and the platform write is
 *  a synchronous memcpy — so the buffer stayed locked forever.
 *
 *  getblk spins on a locked buffer (scenario 5), so after 256 such
 *  writes the pool was entirely locked and every subsequent getblk spun
 *  until its bound.  That is a whole-filesystem hang from an ordinary
 *  write path.
 *
 *  The third arm is CLOSED here.  `delayed` now selects between exactly
 *  two outcomes, and there is no path that leaves a buffer locked with
 *  nobody to release it:
 *
 *      delayed = true    mark BUF_DELWRITE, release.  bflush() or
 *                        getblk's scenario-3 path writes it later.
 *      delayed = false   write NOW, release.  `sync` is accepted and
 *                        documented: with a synchronous platform hook,
 *                        every write is already the synchronous one.
 *
 *  Bach's async arm needs an interrupt handler to complete it.  When the
 *  BSP provides one, the third arm comes back here — with a real
 *  completion path, not a promise.
 *
 *  ── the flag discipline ───────────────────────────────────────────────
 *  A written buffer is VALID (its contents match disk), not dirty and
 *  not delayed.  Clearing BUF_DELWRITE matters: leaving it set would
 *  make getblk's scenario 3 write the same block again on the next pass.
 *
 *  @version 2.0.0  @date 2026-09-24
 */
#include "bcache.h"
#include "bcache_internal.h"

/* ═════════════════════════════════════════════════════════════
 * Algorithm bwrite  —  Bach Ch.3 §2
 * ═════════════════════════════════════════════════════════════ */
void bwrite(BufHdr *buf, bool sync, bool delayed)
{
    (void)sync;     /* with a synchronous platform hook, every write
                     * below is the synchronous one; the flag is kept so
                     * the signature matches Bach's two cases */

    if (!buf) return;

    /* ── delayed: mark and release, do NOT write now ────────────────
     * Bach's bdwrite.  The buffer keeps its BUF_LOCKED clear and sits on
     * the free list marked; whoever takes it next (getblk scenario 3) or
     * whoever calls bflush() writes it first. */
    if (delayed) {
        buf->status |= BUF_DELWRITE;
        brelse(buf);                    /* releases, status preserved */
        bcache_stats.writes++;
        return;
    }

    /* ── not delayed: write now, then release ───────────────────────
     * This is the arm the filesystem uses.  The buffer is written and
     * returned to the pool in one call, so no caller can leak a lock. */
    buf->status |= BUF_IOBUSY;
    bcache_plat_write_block(buf->dev, buf->blkno, buf->data);
    buf->status &= ~BUF_IOBUSY;

    buf->status |=  BUF_VALID;                       /* matches the disk */
    buf->status &= ~(BUF_DELWRITE | BUF_ERROR);

    bcache_stats.writes++;

    brelse(buf);                        /* release — always */
}

/* ═════════════════════════════════════════════════════════════
 * bdwrite — delayed write
 * ═════════════════════════════════════════════════════════════ */
void bdwrite(BufHdr *buf)
{
    bwrite(buf, false, true);
}

/* ═════════════════════════════════════════════════════════════
 * bflush — write every delayed buffer for one device
 *
 * Called from above by sync/fsync.  Bach: "update super block, inode,
 * flush buffers."
 *
 * A buffer that is currently LOCKED is skipped: the holder is mid-work
 * on it, and writing its data area now would persist a half-built block.
 * It will be written by whoever releases it, or by the next bflush.
 *
 * ── why the scan is over the whole pool ───────────────────────────────
 * The pool has no per-device index, and a delayed buffer can be anywhere
 * in it.  Scanning 256 headers is cheap next to a device write; adding
 * an index would be the optimisation, not the requirement.
 * ═════════════════════════════════════════════════════════════════ */
void bflush(uint8_t dev)
{
    uint32_t i;
    uint32_t flushed = 0u;

    for (i = 0u; i < NUM_BUFFERS; i++) {
        BufHdr *b = &bcache_pool[i];

        if (b->dev != dev)                       continue;
        if (!(b->status & BUF_DELWRITE))         continue;
        if (b->status & BUF_LOCKED)              continue;  /* in use */

        b->status |= BUF_IOBUSY;
        bcache_plat_write_block(b->dev, b->blkno, b->data);
        b->status &= ~BUF_IOBUSY;

        b->status |=  BUF_VALID;
        b->status &= ~BUF_DELWRITE;

        bcache_stats.writes++;
        flushed++;
    }

    printf("[bflush] dev=%u: flushed %u delayed buffers\n",
           (unsigned)dev, (unsigned)flushed);
}

/* bflush_all — every delayed buffer, every device.  sync(2)'s helper. */
void bflush_all(void)
{
    uint8_t dev;

    for (dev = 0u; dev < MAX_DEVICES; dev++)
        bflush(dev);
}
