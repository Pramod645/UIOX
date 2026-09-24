/*
 *  30_KIX/32_FS/01_fsa/src/readwrite.c
 *
 *  Bach's read and write loops, lifted out of the system call.
 *  WRITTEN AGAINST THE BUFFER LAYER'S REAL CONTRACT.
 *
 *  ── the seam this file closes ────────────────────────────────────────
 *  The buffer layer (31_BufferCache/00_FileBuff) is multi-device:
 *
 *      BufHdr *getblk (uint8_t dev, uint32_t blkno);
 *      BufHdr *bread  (uint8_t dev, uint32_t blkno);
 *      BufHdr *breada (uint8_t dev, uint32_t blkno, uint32_t ra_blkno);
 *      void    bwrite (BufHdr *buf, bool sync, bool delayed);
 *      void    brelse (BufHdr *buf);
 *
 *  Every one of those takes a DEVICE.  The first cut of this file called
 *  bread(blkno) — one argument — and stored through BufEntry's
 *  valid/dirty fields, neither of which exists on BufHdr.  Both are now
 *  corrected:
 *
 *    · device   comes from BmapResult.dev, which bmap() now fills
 *    · validity comes from the BUF_VALID status bit
 *    · dirtiness is expressed by passing the bwrite flags, not by a field
 *
 *  ── Bach's loop, and the one call that differs read vs write ─────────
 *    read   bmap        a hole ends the read, clamped to ip->size
 *    write  bmap_alloc  a hole is allocated, and ip->size grows
 *
 *  Bach puts this loop INSIDE Algorithm read and Algorithm write.  This
 *  is the loop; the syscall wrapper around it is 10_scfs's.
 *
 *  ── the read-ahead wire, now connected ───────────────────────────────
 *  bmap() computes readahead_blk on every direct-block hit and until now
 *  nothing consumed it.  Bach's breada() takes exactly that second
 *  argument, so the READ path calls breada(dev, blkno, readahead_blk)
 *  when bmap offered one, and plain bread() when it did not.  The write
 *  path never reads ahead: it is about to overwrite the block.
 *
 *  ── the two forms ────────────────────────────────────────────────────
 *    readi / writei        advance the caller's offset
 *    readi_at / writei_at  positioned — the offset is passed by value, so
 *                          a file table entry shared through dup() keeps
 *                          its f_offset and pread/pwrite do not move it
 *
 *  @version 2.0.0  @date 2026-09-24
 */
#include "bmap.h"
#include "superblock.h"
#include "buffer.h"
#include "uiox_klibc.h"

/* ─────────────────────────────────────────────────────────────
 * Internal: one iteration's clamp.
 *
 * How many bytes move in THIS pass.  Bach's three limits, in the order
 * he applies them: what the caller asked for, what is left in the block
 * the offset lands in, and — for a read — what is left in the file.
 * ───────────────────────────────────────────────────────────── */
static uint32_t rw_chunk(const InCoreInode *ip, uint32_t off,
                         uint32_t count, const BmapResult *bm, int is_read)
{
    uint32_t n = count;

    if (bm->io_bytes < n) n = bm->io_bytes;

    if (is_read) {
        /* Bach: a read stops at the end of the file.  A write does not —
         * it grows it. */
        if (off >= ip->size) return 0u;
        if (ip->size - off < n) n = ip->size - off;
    }

    return n;
}

/*
 * Internal: fetch a block for reading, using read-ahead when bmap
 * offered a next block.
 *
 * Bach has both bread and breada; this picks between them from the
 * readahead field bmap() already fills.  A zero readahead_blk means
 * "no next block known" — the end of the direct range, or an indirect
 * level where the next block was not computed — so plain bread is used.
 */
static BufHdr *rw_bread(const BmapResult *bm)
{
    if (bm->readahead_blk != 0u && bm->readahead_blk != bm->blkno)
        return breada(bm->dev, bm->blkno, bm->readahead_blk);

    return bread(bm->dev, bm->blkno);
}

/* ═════════════════════════════════════════════════════════════
 * Algorithm read
 * ═════════════════════════════════════════════════════════════ */

/*
 * readi_at — read into kbuf from the inode's file at 'off'.
 *
 * Returns the number of bytes read, or a negative SCFS_E* on error.
 * A short return with a non-zero byte count means end of file.
 */
int32_t readi_at(InCoreInode *ip, char *kbuf, uint32_t count,
                 uint32_t off, uint32_t *moved)
{
    uint32_t done = 0u;

    if (moved) *moved = 0u;
    if (!ip || !kbuf) return -1;
    if (count == 0u) return 0;

    while (done < count) {
        BmapResult bm;
        BufHdr    *buf;
        uint32_t   n;
        uint32_t   i;

        /* ── map the file byte offset to a disk block (algorithm bmap) ─ */
        bm = bmap(ip, off);

        /* ── a hole, or past the end: stop.  There is no sparse file on
         *    this filesystem, so an unmapped block means end of data. ── */
        if (!bm.valid) break;

        n = rw_chunk(ip, off, count - done, &bm, 1);
        if (n == 0u) break;

        /* ── read the block into the cache, with read-ahead ────────── */
        buf = rw_bread(&bm);
        if (!buf) break;

        /* ── copy the bytes out of the buffer's data area ──────────── */
        for (i = 0u; i < n; i++)
            kbuf[done + i] = (char)buf->data[bm.blk_offset + i];

        brelse(buf);                            /* buffer layer */

        /* ── adjust the offset, the count and the buffer ───────────── */
        done += n;
        off  += n;
    }

    /* Bach's read does not change i_size.  It does mark the inode as
     * accessed, which is a metadata change iupdate will persist. */
    if (done > 0u) ip->flags |= IFLAG_ACCESSED;

    if (moved) *moved = done;
    return (int32_t)done;
}

/* readi — the advancing form.  The caller's offset is updated in place. */
int32_t readi(InCoreInode *ip, char *kbuf, uint32_t count, uint32_t *off)
{
    uint32_t moved = 0u;
    uint32_t start;
    int32_t  rc;

    if (!off) return -1;
    start = *off;

    rc = readi_at(ip, kbuf, count, start, &moved);
    if (moved > 0u) *off = start + moved;

    return rc;
}

/* ═════════════════════════════════════════════════════════════
 * Algorithm write
 * ═════════════════════════════════════════════════════════════ */

/*
 * writei_at — write kbuf into the inode's file at 'off'.
 *
 * Returns the number of bytes written, or a negative SCFS_E* on error.
 *
 * The difference from readi_at is one call: bmap_alloc in place of bmap.
 * That is what extends the file — bmap_alloc allocates a block when the
 * offset maps to nothing.
 *
 * ── bwrite's flags, and why sync=true here ───────────────────────────
 * The buffer layer's bwrite(buf, sync, delayed) replaced the old
 * one-argument form.  bwrite(buf, true, false) means "write it now and
 * release the buffer", which is what the first cut assumed and what a
 * synchronous filesystem wants.  The delayed path is reached only
 * through bdwrite(), which the buffer layer owns — and which nothing in
 * this layer calls, deliberately: a delayed write that is never flushed
 * would leave the file's bytes in DRAM while its inode said otherwise.
 */
int32_t writei_at(InCoreInode *ip, const char *kbuf, uint32_t count,
                  uint32_t off, uint32_t *moved)
{
    uint32_t done = 0u;

    if (moved) *moved = 0u;
    if (!ip || !kbuf) return -1;
    if (count == 0u) return 0;

    while (done < count) {
        BmapResult bm;
        BufHdr    *buf;
        uint32_t   n;
        uint32_t   i;

        /* ── map, ALLOCATING a block if this offset has none ────────── */
        bm = bmap_alloc(ip, off);
        if (!bm.valid) break;       /* out of space — stop, return short */

        n = rw_chunk(ip, off, count - done, &bm, 0);
        if (n == 0u) break;

        /* The write path never reads ahead: this block is about to be
         * overwritten, so breada's second argument would be wasted. */
        buf = bread(bm.dev, bm.blkno);
        if (!buf) break;

        /* ── copy the bytes in ─────────────────────────────────────── */
        for (i = 0u; i < n; i++)
            buf->data[bm.blk_offset + i] = (uint8_t)kbuf[done + i];

        /* ── synchronous write: the buffer is released by bwrite ───── */
        bwrite(buf, true, false);

        done += n;
        off  += n;
    }

    /* ── update the inode size ───────────────────────────────────────
     * Bach: "if (write) { update inode size; unlock inode; }".  The size
     * only grows — a write that overwrites the middle of a file leaves
     * it where it was. */
    if (off > ip->size) ip->size = off;

    if (done > 0u) {
        ip->flags |= IFLAG_MODIFIED | IFLAG_CHANGED;
        iupdate(ip);
    }

    if (moved) *moved = done;
    return (int32_t)done;
}

/* writei — the advancing form. */
int32_t writei(InCoreInode *ip, const char *kbuf, uint32_t count, uint32_t *off)
{
    uint32_t moved = 0u;
    uint32_t start;
    int32_t  rc;

    if (!off) return -1;
    start = *off;

    rc = writei_at(ip, kbuf, count, start, &moved);
    if (moved > 0u) *off = start + moved;

    return rc;
}
