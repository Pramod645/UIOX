#include "bmap.h"
#include "superblock.h"
#include <stdio.h>
#include <string.h>

/* ─────────────────────────────────────────────────────────────
 * Internal: read one indirect block and extract a block pointer.
 * 'level' is used only for trace output.
 * ───────────────────────────────────────────────────────────── */
static uint32_t indirect_lookup(uint32_t indirect_blkno,
                                 uint32_t index,
                                 BufEntry **prev_buf)
{
    if (*prev_buf) {
        brelse(*prev_buf);
        *prev_buf = NULL;
    }

    BufEntry *buf = bread(indirect_blkno);
    if (!buf) return 0;

    uint32_t *ptrs   = (uint32_t *)buf->data;
    uint32_t  result = (index < PTRS_PER_BLOCK) ? ptrs[index] : 0;
    *prev_buf        = buf;  /* caller must brelse */
    return result;
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm bmap  (§3)
 * ───────────────────────────────────────────────────────────── */
BmapResult bmap(InCoreInode *ip, uint32_t byte_offset)
{
    BmapResult r = {0};

    if (!ip) return r;

    /* ── Output 2: start byte within block ────────────────── */
    r.blk_offset = byte_offset % BLOCK_SIZE;

    /* ── Output 3: bytes available for I/O in this block ──── */
    r.io_bytes   = BLOCK_SIZE - r.blk_offset;

    /* ── Logical block number within file ─────────────────── */
    uint32_t logical_blk = byte_offset / BLOCK_SIZE;

    /* ── Read-ahead: next logical block (output 4) ─────────── */
    uint32_t readahead_logical = logical_blk + 1;

    BufEntry *prev_buf = NULL;
    uint32_t  blkno    = 0;

    /* ── Direct blocks ────────────────────────────────────── */
    if (logical_blk < NDIRECT) {
        blkno = ip->addr[logical_blk];
        /* Read-ahead from direct array */
        if (readahead_logical < NDIRECT)
            r.readahead_blk = ip->addr[readahead_logical];
        goto done;
    }
    logical_blk -= NDIRECT;

    /* ── Single indirect ─────────────────────────────────── */
    uint32_t ptrs_per = PTRS_PER_BLOCK;

    if (logical_blk < ptrs_per) {
        uint32_t si_blk = ip->addr[NDIRECT];
        if (!si_blk) goto done;
        blkno = indirect_lookup(si_blk, logical_blk, &prev_buf);
        goto done;
    }
    logical_blk -= ptrs_per;

    /* ── Double indirect ─────────────────────────────────── */
    if (logical_blk < ptrs_per * ptrs_per) {
        uint32_t di_blk = ip->addr[NDIRECT + NINDIRECT];
        if (!di_blk) goto done;

        uint32_t di_idx = logical_blk / ptrs_per;
        uint32_t si_idx = logical_blk % ptrs_per;

        uint32_t si_blk = indirect_lookup(di_blk, di_idx, &prev_buf);
        if (!si_blk) goto done;
        blkno = indirect_lookup(si_blk, si_idx, &prev_buf);
        goto done;
    }
    logical_blk -= ptrs_per * ptrs_per;

    /* ── Triple indirect ─────────────────────────────────── */
    {
        uint32_t ti_blk = ip->addr[NDIRECT + NINDIRECT + NDINDIRECT];
        if (!ti_blk) goto done;

        uint32_t ti_idx = logical_blk / (ptrs_per * ptrs_per);
        uint32_t di_idx = (logical_blk / ptrs_per) % ptrs_per;
        uint32_t si_idx = logical_blk % ptrs_per;

        uint32_t di_blk = indirect_lookup(ti_blk, ti_idx, &prev_buf);
        if (!di_blk) goto done;
        uint32_t si_blk = indirect_lookup(di_blk, di_idx, &prev_buf);
        if (!si_blk) goto done;
        blkno = indirect_lookup(si_blk, si_idx, &prev_buf);
    }

done:
    if (prev_buf) brelse(prev_buf);
    r.blkno = blkno;
    r.valid = (blkno != 0);
    printf("[bmap] byte_off=%u → blk=%u  blk_off=%u  io_bytes=%u\n",
           byte_offset, r.blkno, r.blk_offset, r.io_bytes);
    return r;
}

/* ─────────────────────────────────────────────────────────────
 * bmap_alloc — like bmap but creates missing blocks on the fly
 * ───────────────────────────────────────────────────────────── */
BmapResult bmap_alloc(InCoreInode *ip, uint32_t byte_offset)
{
    uint32_t   logical_blk = byte_offset / BLOCK_SIZE;
    BmapResult r           = {0};

    r.blk_offset = byte_offset % BLOCK_SIZE;
    r.io_bytes   = BLOCK_SIZE - r.blk_offset;

    /* ── Direct ──────────────────────────────────────────── */
    if (logical_blk < NDIRECT) {
        if (!ip->addr[logical_blk]) {
            BufEntry *nb = fs_alloc();
            if (!nb) return r;
            ip->addr[logical_blk] = nb->blkno;
            ip->flags |= IFLAG_CHANGED;
            brelse(nb);
        }
        r.blkno = ip->addr[logical_blk];
        r.valid = true;
        printf("[bmap_alloc] direct blk=%u  off=%u\n",
               r.blkno, byte_offset);
        return r;
    }
    logical_blk -= NDIRECT;

    /* ── Single indirect ─────────────────────────────────── */
    if (logical_blk < PTRS_PER_BLOCK) {
        if (!ip->addr[NDIRECT]) {
            BufEntry *nb = fs_alloc();
            if (!nb) return r;
            ip->addr[NDIRECT] = nb->blkno;
            ip->flags |= IFLAG_CHANGED;
            brelse(nb);
        }
        BufEntry *si_buf = bread(ip->addr[NDIRECT]);
        if (!si_buf) return r;
        uint32_t *ptrs = (uint32_t *)si_buf->data;
        if (!ptrs[logical_blk]) {
            BufEntry *nb = fs_alloc();
            if (!nb) { brelse(si_buf); return r; }
            ptrs[logical_blk] = nb->blkno;
            si_buf->dirty = true;
            bwrite(si_buf);
            brelse(nb);
        }
        r.blkno = ptrs[logical_blk];
        brelse(si_buf);
        r.valid = true;
        printf("[bmap_alloc] indirect blk=%u  off=%u\n",
               r.blkno, byte_offset);
        return r;
    }

    fprintf(stderr, "[bmap_alloc] double/triple indirect not implemented\n");
    return r;
}
