/*
 *  30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_truncate.c
 *
 *  SCFS — truncate, ftruncate, fallocate.
 *  Bach, The Design of the UNIX Operating System.
 *
 *  ── where this sits relative to Bach ─────────────────────────────────
 *  Bach has no separate truncate algorithm: it is the tail of Algorithm
 *  creat — "if the file did exist at time of create, free all file
 *  blocks."  That is one 01_fsa call, fs_free_inode_blocks().
 *
 *  truncate(2) is that same call WITHOUT the directory work and WITHOUT
 *  the open.  ftruncate() is the same for an open descriptor.
 *
 *  ── FIXED: the stale block pointers ──────────────────────────────────
 *  The first version freed the blocks past the cut but never cleared the
 *  matching entries in ip->addr[].  That left the inode pointing at
 *  blocks which had been returned to the free list — so a later
 *  bmap_alloc() on the same logical offset could hand the SAME block to a
 *  second file.  Two files, one block: silent data loss, and it is
 *  reachable by an ordinary truncate followed by a write.
 *
 *  There are two places the map has to be cleared, and the first version
 *  had neither:
 *
 *    1. DIRECT entries   ip->addr[i] past the cut must be zeroed.
 *    2. INDIRECT blocks  the pointers INSIDE the indirect block must be
 *                        zeroed too — clearing only the inode's top-level
 *                        pointer would free the indirect block itself
 *                        while leaving it full of dangling numbers.
 *
 *  scfs_clear_map_past() below does both, walking the same three levels
 *  bmap() reads.  It is deliberately conservative: it clears entries and
 *  frees the indirect blocks themselves only when they become empty.
 *
 *  ── growing ──────────────────────────────────────────────────────────
 *  POSIX lets ftruncate EXTEND a file, producing a sparse hole that reads
 *  back as zeros.  01_fsa's bmap() has no hole concept, so extension is
 *  implemented by writing zero blocks up to the new size.  Honest and
 *  portable; it costs disk where a sparse file would not.
 *
 *  ── the double/triple indirect limit ─────────────────────────────────
 *  01_fsa's bmap_alloc() implements direct and single-indirect only and
 *  prints an ERROR otherwise.  A file is therefore capped at
 *  NDIRECT + PTRS_PER_BLOCK blocks — about 10 KB with BLOCK_SIZE 512.
 *  The grow path reports that as ENOSPC rather than looping.
 *
 *  v1.3: stale block pointers cleared (direct + indirect); grow path
 *        bounded by the allocator's real reach.
 */
#include "uiox_kix_scfs_internal.h"

/* How many map slots an inode has, in total. */
#define SCFS_MAP_SLOTS (NDIRECT + NINDIRECT + NDINDIRECT + NTINDIRECT)

/* What bmap_alloc can actually reach: direct blocks, then one level of
 * single-indirect.  Anything past this returns an invalid mapping. */
#define SCFS_ALLOC_MAX_BLOCKS (NDIRECT + PTRS_PER_BLOCK)

/* ─────────────────────────────────────────────────────────────────────
 * scfs_clear_map_past — drop every map entry at or beyond block index
 * 'from_blk', and free the indirect blocks that become empty.
 *
 * Called only from the shrink path, after the data blocks themselves
 * have been freed.  This function frees NO data blocks — if it did, the
 * two would double-free.
 * ───────────────────────────────────────────────────────────────────── */
static void scfs_clear_map_past(InCoreInode *ip, uint32_t from_blk)
{
    uint32_t i;

    /* ── level 0: the direct entries ──────────────────────────────── */
    for (i = from_blk; i < NDIRECT && i < SCFS_MAP_SLOTS; i++)
        ip->addr[i] = 0u;

    /* Nothing past the direct level: done. */
    if (from_blk < NDIRECT) {
        /* The indirect slots are untouched when the cut is inside the
         * direct range, so return without walking them. */
        return;
    }

    /* ── level 1: single indirect ─────────────────────────────────────
     * from_blk is now expressed relative to the first indirect block. */
    {
        uint32_t rel = from_blk - NDIRECT;

        if (ip->addr[NDIRECT]) {
            BufEntry *b = bread(ip->addr[NDIRECT]);         /* 01_fsa */
            if (b) {
                uint32_t *ptrs = (uint32_t *)b->data;
                uint32_t  j;
                int       any_left = 0;

                for (j = 0u; j < PTRS_PER_BLOCK; j++) {
                    if (j >= rel) {
                        if (ptrs[j]) {
                            /* The data block was freed by the caller's
                             * loop; only the POINTER is cleared here. */
                            ptrs[j] = 0u;
                            b->dirty = true;
                        }
                    } else if (ptrs[j]) {
                        any_left = 1;
                    }
                }

                bwrite(b, true, false);                 /* persist + release */

                /* If every pointer is gone the indirect block itself has
                 * no reason to exist — free it and clear the slot. */
                if (!any_left) {
                    fs_free(ip->addr[NDIRECT]);             /* 01_fsa */
                    ip->addr[NDIRECT] = 0u;
                }
            }
        }
    }

    /* ── level 2 and 3 ────────────────────────────────────────────────
     * bmap_alloc() cannot create these, so a shrink that reaches them
     * means the inode already held them.  Reading them is still correct:
     * leaving dangling pointers two levels down would be the same bug.
     *
     * The two slots are cleared outright when the cut is past their whole
     * range, and the walk is bounded so a corrupt pointer cannot loop. */
    {
        uint32_t double_start = NDIRECT + NINDIRECT;
        uint32_t triple_start = NDIRECT + NINDIRECT + NDINDIRECT;

        if (from_blk <= double_start && ip->addr[double_start] &&
            ip->addr[NINDIRECT + NDIRECT]) {
            /* The cut is at or before the double-indirect range: the
             * whole subtree goes.  Freeing the data blocks was the
             * caller's job; here the container blocks are freed. */
            BufEntry *di = bread(ip->addr[double_start]);    /* 01_fsa */
            if (di) {
                uint32_t *p1 = (uint32_t *)di->data;
                uint32_t  a;

                for (a = 0u; a < PTRS_PER_BLOCK; a++) {
                    if (!p1[a]) continue;

                    {
                        BufEntry *si = bread(p1[a]);         /* 01_fsa */
                        if (si) {
                            uint32_t *p2 = (uint32_t *)si->data;
                            uint32_t  c;

                            for (c = 0u; c < PTRS_PER_BLOCK; c++) p2[c] = 0u;
                            si->dirty = true;
                            bwrite(si);                      /* 01_fsa */
                            brelse(si);                      /* 01_fsa */
                        }
                    }
                    fs_free(p1[a]);                          /* 01_fsa */
                    p1[a] = 0u;
                }

                di->dirty = true;
                bwrite(di);                                  /* 01_fsa */
                brelse(di);                                  /* 01_fsa */
            }
            fs_free(ip->addr[double_start]);                 /* 01_fsa */
            ip->addr[double_start] = 0u;
        }

        if (from_blk <= triple_start && ip->addr[triple_start]) {
            /* Same treatment one level deeper.  Two nested reads, then
             * the container blocks go. */
            BufEntry *ti = bread(ip->addr[triple_start]);    /* 01_fsa */
            if (ti) {
                uint32_t *t1 = (uint32_t *)ti->data;
                uint32_t  a;

                for (a = 0u; a < PTRS_PER_BLOCK; a++) {
                    if (!t1[a]) continue;

                    {
                        BufEntry *di = bread(t1[a]);         /* 01_fsa */
                        if (di) {
                            uint32_t *p1 = (uint32_t *)di->data;
                            uint32_t  c;

                            for (c = 0u; c < PTRS_PER_BLOCK; c++) {
                                if (p1[c]) {
                                    fs_free(p1[c]);          /* 01_fsa */
                                    p1[c] = 0u;
                                }
                            }
                            di->dirty = true;
                            bwrite(di);                      /* 01_fsa */
                            brelse(di);                      /* 01_fsa */
                        }
                    }
                    fs_free(t1[a]);                          /* 01_fsa */
                    t1[a] = 0u;
                }

                ti->dirty = true;
                bwrite(ti);                                  /* 01_fsa */
                brelse(ti);                                  /* 01_fsa */
            }
            fs_free(ip->addr[triple_start]);                 /* 01_fsa */
            ip->addr[triple_start] = 0u;
        }
    }
}

static int scfs_do_truncate(InCoreInode *ip, uint32_t newsize)
{
    if (SCFS_IS_DIR(ip->mode)) return SCFS_EISDIR;

    /* ── the shrink-to-zero case: free everything, clear everything ── */
    if (newsize == 0u) {
        fs_free_inode_blocks(ip);       /* 01_fsa: frees data + indirect */
        ip->size = 0u;
        ip->flags |= (IFLAG_CHANGED | IFLAG_MODIFIED);
        iupdate(ip);
        return SCFS_OK;
    }

    if (newsize == ip->size) return SCFS_OK;

    /* ── the partial-shrink case ───────────────────────────────────── */
    if (newsize < ip->size) {
        uint32_t last_blk_off = newsize & (uint32_t)~(BLOCK_SIZE - 1u);
        uint32_t tail         = newsize - last_blk_off;
        uint32_t offset;

        /* Zero the tail of the block that straddles newsize, so a later
         * read of that block does not see the old bytes. */
        if (tail != 0u) {
            BmapResult r = bmap(ip, last_blk_off);      /* 01_fsa */
            if (r.valid && r.blkno) {
                BufEntry *b = bread(r.blkno);           /* 01_fsa */
                if (b) {
                    uint32_t i;
                    for (i = tail; i < (uint32_t)BLOCK_SIZE; i++)
                        b->data[i] = 0u;
                    b->dirty = true;
                    bwrite(b);                          /* 01_fsa */
                    brelse(b);                          /* 01_fsa */
                }
            }
        }

        offset = (tail != 0u) ? last_blk_off + BLOCK_SIZE : last_blk_off;

        /* Free the DATA blocks past the cut.  This loop frees blocks
         * only — the pointers follow, in the walk below. */
        while (offset < ip->size) {
            BmapResult r = bmap(ip, offset);            /* 01_fsa */
            if (r.valid && r.blkno != 0u) fs_free(r.blkno);
            offset += BLOCK_SIZE;
        }

        /* ── THE FIX ────────────────────────────────────────────────
         * Drop every map entry at or beyond the first block past the
         * cut.  Without this the inode still names blocks that are on
         * the free list, and the next bmap_alloc() can hand one of them
         * to another file. */
        {
            uint32_t first_clear = (newsize + BLOCK_SIZE - 1u) / BLOCK_SIZE;
            scfs_clear_map_past(ip, first_clear);
        }

        ip->size = newsize;
        ip->flags |= (IFLAG_CHANGED | IFLAG_MODIFIED);
        iupdate(ip);
        return SCFS_OK;
    }

    /* ── the grow case: write zero blocks up to the new size ───────── */
    {
        static const char zeros[BLOCK_SIZE];
        uint32_t off = ip->size;

        /* bmap_alloc() cannot reach past the single-indirect level.  A
         * request beyond that is reported rather than looping against an
         * allocator that will keep saying no. */
        if ((newsize + BLOCK_SIZE - 1u) / BLOCK_SIZE > SCFS_ALLOC_MAX_BLOCKS)
            return SCFS_ENOSPC;

        while (off < newsize) {
            uint32_t chunk = newsize - off;
            uint32_t moved = 0u;
            int32_t  w;

            if (chunk > (uint32_t)BLOCK_SIZE) chunk = (uint32_t)BLOCK_SIZE;

            w = writei_at(ip, zeros, chunk, off, &moved);
            if (w < 0) return (int)w;

            /* Zero moved means the allocator refused — out of blocks, or
             * past its reach.  Stop rather than spin. */
            if (moved == 0u) return SCFS_ENOSPC;

            off += moved;
        }

        if (ip->size > newsize) ip->size = newsize;
        ip->flags |= (IFLAG_CHANGED | IFLAG_MODIFIED);
        iupdate(ip);
    }

    return SCFS_OK;
}

/* ── truncate() — by path name ──────────────────────────────────────── */
int uiox_kix_scfs_truncate(const char *path, uint32_t len)
{
    InCoreInode *ip;
    int          rc;

    if (!path) return SCFS_EFAULT;

    ip = namei(path, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!ip) return SCFS_ENOENT;

    if (!inode_access_ok(ip, scfs_uid_get(), scfs_gid_get(), 0, 1, 0)) {
        iput(ip);
        return SCFS_EACCES;
    }

    rc = scfs_do_truncate(ip, len);

    ip->locked = false;
    iput(ip);
    return rc;
}

/* ── ftruncate() — by descriptor ────────────────────────────────────── */
int uiox_kix_scfs_ftruncate(int fd, uint32_t len)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;
    if (!(f->f_flag & FWRITE)) return SCFS_EINVAL;

    /* The entry owns the inode reference — no iput.  The offset stays
     * where it was even if it now points past the end. */
    return scfs_do_truncate(f->f_inode, len);
}

/*
 * fallocate() — reserve space without writing it.
 *
 * Needs an allocator that can mark blocks allocated while leaving them
 * unwritten, which 01_fsa's bitmap does not distinguish.  Reported rather
 * than implemented as "write zeros", which is a different syscall's
 * semantics — a caller preallocating a large file would get a full disk
 * instead of a reservation.
 */
int uiox_kix_scfs_fallocate(int fd, int mode, uint32_t off, uint32_t len)
{
    (void)fd; (void)mode; (void)off; (void)len;
    return SCFS_ENOSYS;
}
