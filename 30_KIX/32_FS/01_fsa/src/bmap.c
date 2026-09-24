/*
 *  30_KIX/32_FS/01_fsa/src/bmap.c
 *
 *  Algorithm bmap — block map of a logical file byte offset to a
 *  file system block.
 *  Bach, The Design of the UNIX Operating System, Ch.4 §3.
 *
 *  ── Bach's Algorithm bmap, verbatim ──────────────────────────────────
 *  input: inode, byte offset
 *  output: block number in the file system, byte offset into block,
 *          bytes of I/O in block, read ahead block number
 *  {
 *      calculate logical block number in file from byte offset;
 *      calculate start byte in block for I/O;          // output 2
 *      calculate number of bytes to copy to user;      // output 3
 *      check if read-ahead applicable, mark inode;     // output 4
 *      determine level of indirection;
 *      while (not at necessary level of indirection)
 *      {
 *          calculate index into inode or indirect block from logical
 *              block number in file;
 *          get disk block number from inode or indirect block;
 *          release buffer from previous disk read, if any (brelse);
 *          if (no more levels of indirection) return (block number);
 *          read indirect disk block (algorithm bread);
 *          adjust logical block number in file according to level of
 *              indirection;
 *      }
 *  }
 *
 *  ── COMPATIBILITY ─────────────────────────────────────────────────────
 *  BELOW (00_buffcache):
 *      indirect_lookup now calls  bread(dev, blkno)
 *      bmap_alloc now calls       bread(dev, blkno)
 *      bmap_alloc now calls       fs_alloc_begin() / fs_alloc_commit()
 *      every bwrite is             bwrite(buf, true, false)
 *
 *  ABOVE (10_scfs):
 *      BmapResult now carries .dev.  readwrite.c passes
 *      (bm.dev, bm.blkno) to bread/breada, and 01_fsa/readwrite.c's
 *      rw_bread() consumes .readahead_blk — the wire that was dangling.
 *
 *  ── the device ────────────────────────────────────────────────────────
 *  Every result fills r.dev = ip->dev.  An inode lives on exactly one
 *  filesystem, so the block it maps to lives on the same one.  This is
 *  the field that lets bmap's caller satisfy the buffer layer's
 *  (dev, blkno) contract.
 *
 *  @version 2.0.0  @date 2026-09-24
 */
#include "bmap.h"
#include "superblock.h"
#include "buffer.h"
#include "uiox_klibc.h"

/*
 * The two-phase allocator (in superblock.c) exists so this file never
 * returns a block that is attached to no inode and on no free list.
 * See the note in fs_alloc()/fs_alloc_commit().
 */
BufHdr *fs_alloc_begin(uint8_t dev, uint32_t *blkno_out);
void    fs_alloc_commit(uint32_t blkno);

/* ─────────────────────────────────────────────────────────────
 * Internal: read one indirect block and extract a block pointer.
 *
 * The dev argument is threaded through from the inode — an indirect
 * block lives on the same device as the inode that points at it.
 * ───────────────────────────────────────────────────────────── */
static uint32_t indirect_lookup(uint8_t dev, uint32_t indirect_blkno,
                                uint32_t index, BufHdr **prev_buf)
{
    BufHdr   *buf;
    uint32_t *ptrs;
    uint32_t  result;

    if (*prev_buf) {
        brelse(*prev_buf);
        *prev_buf = (BufHdr *)0;
    }

    buf = bread(dev, indirect_blkno);        /* ◀ (dev, blkno) */
    if (!buf) return 0u;

    ptrs   = (uint32_t *)buf->data;
    result = (index < (uint32_t)PTRS_PER_BLOCK) ? ptrs[index] : 0u;

    *prev_buf = buf;                         /* caller must brelse */
    return result;
}

/* ═════════════════════════════════════════════════════════════
 * Algorithm bmap  (§3)
 * ═════════════════════════════════════════════════════════════ */
BmapResult bmap(InCoreInode *ip, uint32_t byte_offset)
{
    uint32_t   logical_blk;
    BmapResult r;
    BufHdr    *prev_buf = (BufHdr *)0;
    uint32_t   ptrs_per;
    uint32_t   blkno    = 0u;

    memset(&r, 0, sizeof r);

    /* ── the device, from the inode ────────────────────────────────── */
    r.dev = ip->dev;

    logical_blk  = byte_offset / BLOCK_SIZE;
    r.blk_offset = byte_offset % BLOCK_SIZE;
    r.io_bytes   = BLOCK_SIZE - r.blk_offset;

    /* ── direct blocks ──────────────────────────────────────────────── */
    if (logical_blk < (uint32_t)NDIRECT) {
        uint32_t ra_logical;

        blkno = ip->addr[logical_blk];

        /* Bach's output 4: the next block, for breada's second argument.
         * Marked on the inode as well, as Bach's algorithm says. */
        ra_logical = logical_blk + 1u;
        if (ra_logical < (uint32_t)NDIRECT && ip->addr[ra_logical]) {
            r.readahead_blk = ip->addr[ra_logical];
            ip->flags |= IFLAG_ACCESSED;
        }
        goto done;
    }
    logical_blk -= (uint32_t)NDIRECT;

    ptrs_per = (uint32_t)PTRS_PER_BLOCK;

    /* ── single indirect ────────────────────────────────────────────── */
    if (logical_blk < ptrs_per) {
        uint32_t si_blk = ip->addr[NDIRECT];
        if (!si_blk) goto done;
        blkno = indirect_lookup(r.dev, si_blk, logical_blk, &prev_buf);
        goto done;
    }
    logical_blk -= ptrs_per;

    /* ── double indirect ────────────────────────────────────────────── */
    if (logical_blk < ptrs_per * ptrs_per) {
        uint32_t di_blk = ip->addr[NDIRECT + NINDIRECT];
        uint32_t di_idx;
        uint32_t si_idx;
        uint32_t si_blk;

        if (!di_blk) goto done;

        di_idx = logical_blk / ptrs_per;
        si_idx = logical_blk % ptrs_per;

        si_blk = indirect_lookup(r.dev, di_blk, di_idx, &prev_buf);
        if (!si_blk) goto done;
        blkno = indirect_lookup(r.dev, si_blk, si_idx, &prev_buf);
        goto done;
    }
    logical_blk -= ptrs_per * ptrs_per;

    /* ── triple indirect ────────────────────────────────────────────── */
    {
        uint32_t ti_blk = ip->addr[NDIRECT + NINDIRECT + NDINDIRECT];
        uint32_t ti_idx;
        uint32_t di_idx;
        uint32_t si_idx;
        uint32_t di_blk;
        uint32_t si_blk;

        if (!ti_blk) goto done;

        ti_idx = logical_blk / (ptrs_per * ptrs_per);
        di_idx = (logical_blk / ptrs_per) % ptrs_per;
        si_idx = logical_blk % ptrs_per;

        di_blk = indirect_lookup(r.dev, ti_blk, ti_idx, &prev_buf);
        if (!di_blk) goto done;
        si_blk = indirect_lookup(r.dev, di_blk, di_idx, &prev_buf);
        if (!si_blk) goto done;
        blkno = indirect_lookup(r.dev, si_blk, si_idx, &prev_buf);
    }

done:
    if (prev_buf) brelse(prev_buf);

    r.blkno = blkno;
    r.valid = (blkno != 0u);

    printf("[bmap] dev=%u byte_off=%u -> blk=%u blk_off=%u io=%u ra=%u\n",
           (unsigned)r.dev, (unsigned)byte_offset, (unsigned)r.blkno,
           (unsigned)r.blk_offset, (unsigned)r.io_bytes,
           (unsigned)r.readahead_blk);
    return r;
}

/* ═════════════════════════════════════════════════════════════
 * bmap_alloc — like bmap but creates missing blocks on the fly
 *
 * ── the two-phase allocator, and why ────────────────────────────────
 * The first cut called fs_alloc() directly, which did two things at
 * once: it detached the block from the free list AND zeroed + locked a
 * buffer for it.  If the inode's map entry was never written, the block
 * was then on no free list and attached to nothing — an unrecoverable
 * leak.
 *
 * fs_alloc() is now split so the caller can order those correctly:
 *
 *   1. fs_alloc_begin()   detach from the free list, lock the buffer
 *   2. attach the block number to the inode's map
 *   3. fs_alloc_commit()  zero + dirty + release the buffer
 *
 * A failure between 1 and 3 leaves the block locked and off the free
 * list — bad, but visible in bcache_stats — rather than silently lost.
 *
 * ── the double/triple indirect limit ────────────────────────────────
 * Bach's alloc handles every level.  This implementation allocates at
 * the direct and single-indirect levels only, and reports the limit
 * rather than returning a mapping that is not there.  Extending it is
 * the same pattern one level down: allocate the indirect block, write
 * its address into the parent, then allocate the data block.
 * ═════════════════════════════════════════════════════════════════ */
BmapResult bmap_alloc(InCoreInode *ip, uint32_t byte_offset)
{
    uint32_t   logical_blk;
    BmapResult r;

    memset(&r, 0, sizeof r);
    r.dev        = ip->dev;
    logical_blk  = byte_offset / BLOCK_SIZE;
    r.blk_offset = byte_offset % BLOCK_SIZE;
    r.io_bytes   = BLOCK_SIZE - r.blk_offset;

    /* ── direct ─────────────────────────────────────────────────────── */
    if (logical_blk < (uint32_t)NDIRECT) {
        if (!ip->addr[logical_blk]) {
            uint32_t new_blk = 0u;
            BufHdr  *nb      = fs_alloc_begin(r.dev, &new_blk);

            if (!nb) return r;              /* out of space */

            ip->addr[logical_blk] = new_blk;   /* attach FIRST   */
            ip->flags |= IFLAG_CHANGED;

            fs_alloc_commit(new_blk);          /* then dirty it  */
        }
        r.blkno = ip->addr[logical_blk];
        r.valid = true;
        printf("[bmap_alloc] direct dev=%u blk=%u off=%u\n",
               (unsigned)r.dev, (unsigned)r.blkno, (unsigned)byte_offset);
        return r;
    }
    logical_blk -= (uint32_t)NDIRECT;

    /* ── single indirect ────────────────────────────────────────────── */
    if (logical_blk < (uint32_t)PTRS_PER_BLOCK) {
        BufHdr   *si_buf;
        uint32_t *ptrs;

        /* the indirect block itself, if this is the first entry */
        if (!ip->addr[NDIRECT]) {
            uint32_t new_blk = 0u;
            BufHdr  *nb      = fs_alloc_begin(r.dev, &new_blk);

            if (!nb) return r;

            ip->addr[NDIRECT] = new_blk;       /* attach the container */
            ip->flags |= IFLAG_CHANGED;
            fs_alloc_commit(new_blk);
        }

        si_buf = bread(r.dev, ip->addr[NDIRECT]);    /* ◀ (dev, blkno) */
        if (!si_buf) return r;

        ptrs = (uint32_t *)si_buf->data;

        if (!ptrs[logical_blk]) {
            uint32_t new_blk = 0u;
            BufHdr  *nb      = fs_alloc_begin(r.dev, &new_blk);

            if (!nb) { brelse(si_buf); return r; }

            ptrs[logical_blk] = new_blk;           /* attach */
            bwrite(si_buf, true, false);           /* ◀ persist the map */
            fs_alloc_commit(new_blk);              /* then dirty data   */

            r.blkno = new_blk;
            r.valid = true;
            printf("[bmap_alloc] indirect dev=%u blk=%u off=%u\n",
                   (unsigned)r.dev, (unsigned)r.blkno, (unsigned)byte_offset);
            return r;
        }

        r.blkno = ptrs[logical_blk];
        brelse(si_buf);
        r.valid = true;
        printf("[bmap_alloc] indirect(cached) dev=%u blk=%u off=%u\n",
               (unsigned)r.dev, (unsigned)r.blkno, (unsigned)byte_offset);
        return r;
    }

    /* ── the limit, reported rather than faked ──────────────────────── */
    printf("[bmap_alloc] ERROR: this allocation is past the "
           "single-indirect level (dev=%u off=%u) — not implemented\n",
           (unsigned)r.dev, (unsigned)byte_offset);
    return r;      /* r.valid stays false — the caller writes short */
}
