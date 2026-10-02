/*
 *  31_BufferCache/00_FileBuff/buffers/src/bread.c
 *
 *  Algorithm bread — Bach, Ch.3 §2.  Read a disk block.
 *
 *  ── Bach's algorithm, verbatim ────────────────────────────────────────
 *  input:  file system block number
 *  output: buffer containing data
 *  {
 *      get buffer for a block (algorithm getblk);
 *      if (buffer data valid)
 *          return buffer;
 *      initiate disk read;
 *      sleep(event disk read complete);
 *      return buffer;
 *  }
 *
 *  ── the format the read is in ─────────────────────────────────────────
 *  Bach's bread does not know the block's format; the caller does.  This
 *  implementation likewise returns the buffer and lets the filesystem
 *  interpret it — 01_fsa's inode.c reads a DiskInode out of it, namei.c
 *  reads DirEntry records, bmap.c reads uint32 block pointers.
 *
 *  ── getblk cannot fail silently here ──────────────────────────────────
 *  getblk returns NULL only when the pool is starved, which is a
 *  different condition from a device error.  Both are reported as NULL so
 *  the caller has one thing to test, but the counters distinguish them:
 *  bcache_stats.free_waits / busy_waits mean starvation, and an
 *  unreachable device means the platform hook failed.
 *
 *  ── the read errors are counted, not swallowed ────────────────────────
 *  A miss increments bcache_stats.reads and sets BUF_ERROR if the
 *  platform read reported failure.  The bit travels with the buffer, so
 *  a caller that checks can tell a real zero-filled block from a failed
 *  read that happens to look like one.
 *
 *  @version 2.0.0  @date 2026-09-24
 */
#include "bcache.h"
#include "bcache_internal.h"

BufHdr *bread(uint8_t dev, uint32_t blkno)
{
    BufHdr *b;
    uint32_t expected;

    /* ── get buffer for the block (algorithm getblk) ───────────────── */
    b = getblk(dev, blkno);
    if (!b) return (BufHdr *)0;         /* pool starved — already logged */

    /* ── the block is already resident: nothing to read ────────────── */
    if (b->status & BUF_VALID) {
        bcache_stats.reads++;
        return b;                       /* locked, as Bach returns it */
    }

    /* ── a read past the end of the device ───────────────────────────
     * The platform hook clamps, but a block number beyond the device
     * would otherwise come back zero-filled and look like valid data.
     * The check is here so a corrupt block pointer in an inode shows up
     * as an error rather than as a silently empty directory. */
    expected = bcache_plat_num_blocks(dev);
    if (expected != 0u && blkno >= expected) {
        printf("[bread] ERROR: dev=%u blk=%u past device end (%u blocks)\n",
               (unsigned)dev, (unsigned)blkno, (unsigned)expected);
        b->status |= BUF_ERROR;
        return b;                       /* still locked — caller releases */
    }

    /* ── initiate the disk read ─────────────────────────────────────── */
    b->status |= BUF_IOBUSY;
    bcache_plat_read_block(dev, blkno, b->data);

    b->status &= ~BUF_IOBUSY;
    b->status |=  BUF_VALID;
    b->status &= ~BUF_ERROR;

    bcache_stats.reads++;
    return b;                           /* locked; caller must brelse */
}
