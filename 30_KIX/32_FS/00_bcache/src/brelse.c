/*
 *  31_BufferCache/00_FileBuff/buffers/src/brelse.c
 *
 *  Algorithm brelse — Bach, Ch.3 §2.  Release a locked buffer.
 *
 *  ── Bach's algorithm, verbatim ────────────────────────────────────────
 *  input:  locked buffer
 *  output: none
 *  {
 *      wakeup all procs: event, waiting for any buffer to become free;
 *      wakeup all procs: event, waiting for this buffer to become free;
 *      raise processor execution level to block interrupts;
 *      if (buffer contents valid and buffer not old)
 *          enqueue buffer at end of free list;
 *      else
 *          enqueue buffer at beginning of free list;
 *      lower processor execution level to allow interrupts;
 *      unlock(buffer);
 *  }
 *
 *  ── the placement IS the policy ──────────────────────────────────────
 *  Bach's two-branch enqueue is not decoration — it is the replacement
 *  policy, and it is why a stale buffer is reused before a fresh one:
 *
 *      valid and not old → TAIL  (most-recently-used, kept longer)
 *      invalid or old    → HEAD  (least-recently-used, evicted next)
 *
 *  A version that appends unconditionally degrades LRU toward FIFO: a
 *  buffer holding a block nobody has touched for hours would be reused
 *  no sooner than one read a moment ago.  The old flag exists so a
 *  caller can force its buffer to the head — used after a sequential
 *  scan, where the data is unlikely to be wanted again.
 *
 *  ── the wakeups ──────────────────────────────────────────────────────
 *  Bach raises the interrupt level before touching the free list because
 *  an interrupt handler also uses it.  This kernel has no preemption in
 *  the buffer path, so nothing is masked here; the two wakeups have no
 *  sleepers to wake, because getblk spins rather than sleeps.  Both are
 *  noted rather than silently omitted, so whoever adds a scheduler knows
 *  where the hooks belong.
 *
 *  ── tolerating NULL ──────────────────────────────────────────────────
 *  brelse(NULL) is a no-op.  A caller whose error path unwinds through
 *  several releases should not have to test each one.
 *
 *  @version 2.0.0  @date 2026-09-24
 */
#include "bcache.h"
#include "bcache_internal.h"

void brelse(BufHdr *buf)
{
    if (!buf) return;

    /* ── (1) and (2) the two wakeups ─────────────────────────────────
     * With no scheduler there are no sleeping processes.  When getblk
     * gains a real sleep, this is where it wakes them. */

    /* ── (3) the placement, which is the LRU policy ──────────────────
     * A buffer is "old" when its contents are unlikely to be wanted
     * again — a sequential-scan block, or one whose file was closed.
     * Those go to the HEAD so they are reused first. */
    if ((buf->status & BUF_VALID) && !(buf->status & BUF_OLD))
        bcache_fl_insert_tail(buf);         /* MRU — keep longer */
    else
        bcache_fl_insert_head(buf);         /* LRU — evict soon  */

    /* ── (4) unlock ──────────────────────────────────────────────────
     * BUF_WANTED is cleared too: anyone who set it while waiting is
     * about to be woken, and a stale WANTED bit would make the next
     * holder think a wait is still outstanding. */
    buf->status &= ~(BUF_LOCKED | BUF_WANTED | BUF_OLD);
}
