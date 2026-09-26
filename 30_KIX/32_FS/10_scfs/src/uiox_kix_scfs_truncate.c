/*
 *  30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_truncate.c
 *
 *  SCFS - truncate, ftruncate, fallocate.
 *  Bach, The Design of the UNIX Operating System.
 *
 *  -- where this sits relative to Bach -------------------------------
 *  Bach has no separate truncate algorithm: it is the tail of Algorithm
 *  creat - "if the file did exist at time of create, free all file
 *  blocks."  That is one 01_fsa call, fs_free_inode_blocks().
 *
 *  truncate(2) is that same call WITHOUT the directory work and WITHOUT
 *  the open.  ftruncate() is the same for an open descriptor.
 *
 *  -- REWRITTEN: the map is an EXTENT ARRAY, not an indirect tree ------
 *  The previous body walked Bach's addr[] tree: ten direct block
 *  pointers, then single-, double- and triple-indirect blocks reached
 *  through bread().  UNFS stores none of that.
 *
 *      old                        UNFS
 *      -----------------------    ------------------------------------
 *      uint32_t addr[13]          unfs_extent_t i_extents[4]
 *      NDIRECT / NINDIRECT / ...  no counterpart - the split is gone
 *      PTRS_PER_BLOCK             128 pointers per 512-byte block;
 *                                 irrelevant, there is no pointer array
 *      an indirect CONTAINER      i_extent_tree, ONE overflow block
 *      free the container when    nothing to free - an extent has no
 *        its pointers empty         container
 *
 *  So this is not a rename.  'clear the map past block N' means TRIM
 *  AN EXTENT, not zero a pointer: an extent names a run of consecutive
 *  physical blocks, so the cut either falls before it, inside it, or
 *  after it - and the middle case SHRINKS e_len rather than splitting
 *  into two entries.
 *
 *  -- the stale-pointer bug this file was written to fix ------------
 *  The first version freed the blocks past the cut but never touched
 *  the map, so the inode still named blocks that were on the free list
 *  - and the next allocation could hand the same block to a second
 *  file.  Two files, one block, silent data loss.
 *
 *  That bug is not fixed by the rewrite; it is fixed by KEEPING the
 *  clear pass, which is why scfs_clear_map_past() is still here.  The
 *  invariant it maintains is unchanged: after a shrink, no extent may
 *  name a physical block at or beyond the cut.
 *
 *  -- the units -------------------------------------------------------
 *  bmap(), the extent fields and UNFS_BLOCK_SIZE are all in 4096-byte
 *  UNFS blocks.  BLOCK_SIZE is 512 - the buffer cache's SECTOR size.
 *  The previous body mixed the two: it stepped the free loop by
 *  BLOCK_SIZE and computed the first block to clear with
 *  (newsize + BLOCK_SIZE - 1) / BLOCK_SIZE, so it walked eight times
 *  as many iterations as there were blocks and cleared at the wrong
 *  boundary.  Everything below is in UNFS_BLOCK_SIZE.
 *
 *  -- growing ----------------------------------------------------------
 *  POSIX lets ftruncate EXTEND a file, producing a sparse hole.  UNFS
 *  HAS hole extents (UNFS_EXT_HOLE), so a proper implementation would
 *  add one and write nothing.
 *
 *      bmap_alloc() is still a stub that reports valid == false, and
 *      no code in this tree creates a hole extent.
 *
 *  Until allocation exists, the grow path cannot put a single block on
 *  disk, so it reports ENOSYS - the same answer mount(), mmap() and
 *  mmap's siblings give for their own missing halves.  Returning ENOSPC
 *  would claim the disk is full, which is a different and false fact.
 *
 *  @version 2.0.0  @date 2026-09-26
 */
#include "uiox_kix_scfs_internal.h"

/*
 * -- scfs_clear_map_past ----------------------------------------------
 * Trim every extent in 'ip' so that none names a logical block at or
 * beyond 'from_blk'.
 *
 * Frees NO data blocks.  The caller frees them, then calls this.  If
 * this freed too the two would double-free - and that split is why the
 * function exists separately at all.
 *
 * Three cases per extent, and the middle one is the whole point:
 *
 *     from_blk >= end      the extent is wholly below the cut: keep it
 *     from_blk <= start    wholly past the cut: empty the slot
 *     otherwise            the cut falls INSIDE: shrink e_len to end
 *                          at from_blk, and free the physical run that
 *                          the shrink just orphaned
 *
 * The middle case is where data would leak if it were skipped: the
 * blocks past the cut are still on disk and no longer named by anything.
 */
static void scfs_clear_map_past(InCoreInode *ip, uint32_t from_blk)
{
    uint32_t i;

    if (!ip) return;

    for (i = 0u; i < UNFS_INLINE_EXTENTS; i++) {
        unfs_extent_t *e = &ip->i_extents[i];
        uint32_t       e_start;
        uint32_t       e_end;

        if (e->e_len == 0u) continue;              /* empty slot      */

        e_start = (uint32_t)e->e_logical;          /* inclusive        */
        e_end   = e_start + (uint32_t)e->e_len;    /* exclusive        */

        /* -- wholly below the cut: nothing to do ------------------- */
        if (from_blk >= e_end) continue;

        /* -- wholly past the cut: the slot goes -------------------- */
        if (from_blk <= e_start) {
            e->e_logical  = 0u;
            e->e_physical = 0u;
            e->e_len      = 0u;
            e->e_flags    = 0u;
            continue;
        }

        /* -- the cut falls INSIDE: shrink, and free the orphaned run -
         *
         * Keep blocks [e_start, from_blk) and release
         * [from_blk, e_end).  A hole extent owns no blocks, so there is
         * nothing to free in that case - the shrink alone is correct. */
        {
            uint32_t keep  = from_blk - e_start;
            uint32_t drop  = e_end - from_blk;
            uint32_t first = (uint32_t)e->e_physical + keep;
            uint32_t n;

            if (!(e->e_flags & UNFS_EXT_HOLE)) {
                for (n = 0u; n < drop; n++)
                    fs_free(ip->dev, first + n);   /* 01_fsa */
            }

            if (keep == 0u) {
                e->e_logical  = 0u;
                e->e_physical = 0u;
                e->e_len      = 0u;
                e->e_flags    = 0u;
            } else {
                e->e_len = (uint16_t)keep;
            }
        }
    }

    /* -- the overflow extent tree ------------------------------------
     * ONE 4 KB block holding an array of unfs_extent_t, terminated by an
     * entry with e_len == 0 - a single level, as the bootloader's
     * extent_lookup() assumes.
     *
     * bmap_alloc() is a stub and nothing in this build sets
     * i_extent_tree, so this branch is defence against a foreign or
     * future image rather than a live path.  It is written out rather
     * than omitted because leaving dangling entries in a tree block
     * that this function is supposed to be clearing is exactly the bug
     * the file exists to prevent. */
    if (ip->i_extent_tree != 0u) {
        uint32_t per = (uint32_t)(UNFS_BLOCK_SIZE / sizeof(unfs_extent_t));
        uint8_t  tree[UNFS_BLOCK_SIZE];
        uint32_t s;
        bool     ok = true;
        bool     dirty = false;

        for (s = 0u; s < (uint32_t)UNFS_SECTORS_PER_BLOCK; s++) {
            BufHdr *buf = bread(ip->dev,
                                ip->i_extent_tree * (uint32_t)UNFS_SECTORS_PER_BLOCK + s);
            if (!buf) { ok = false; break; }

            memcpy(tree + (s * (uint32_t)BCACHE_SECTOR_SIZE),
                   buf->data, (size_t)BCACHE_SECTOR_SIZE);
            brelse(buf);
        }

        if (ok) {
            unfs_extent_t *et = (unfs_extent_t *)tree;

            for (i = 0u; i < per; i++) {
                uint32_t e_start, e_end;

                if (et[i].e_len == 0u) break;      /* end of entries  */

                e_start = (uint32_t)et[i].e_logical;
                e_end   = e_start + (uint32_t)et[i].e_len;

                if (from_blk >= e_end) continue;

                if (from_blk <= e_start) {
                    et[i].e_logical  = 0u;
                    et[i].e_physical = 0u;
                    et[i].e_len      = 0u;
                    et[i].e_flags    = 0u;
                    dirty = true;
                    continue;
                }

                {
                    uint32_t keep  = from_blk - e_start;
                    uint32_t drop  = e_end - from_blk;
                    uint32_t first = (uint32_t)et[i].e_physical + keep;
                    uint32_t n;

                    if (!(et[i].e_flags & UNFS_EXT_HOLE)) {
                        for (n = 0u; n < drop; n++)
                            fs_free(ip->dev, first + n);   /* 01_fsa */
                    }

                    et[i].e_len = (uint16_t)keep;
                    dirty = true;
                }
            }

            /* Write the block back only if something in it changed -
             * the buffer layer expresses dirtiness through bwrite's
             * flags, not a field on the header. */
            if (dirty) {
                for (s = 0u; s < (uint32_t)UNFS_SECTORS_PER_BLOCK; s++) {
                    BufHdr *buf = bread(ip->dev,
                                        ip->i_extent_tree * (uint32_t)UNFS_SECTORS_PER_BLOCK + s);
                    if (!buf) break;

                    memcpy(buf->data,
                           tree + (s * (uint32_t)BCACHE_SECTOR_SIZE),
                           (size_t)BCACHE_SECTOR_SIZE);
                    bwrite(buf, true, false);       /* 01_fsa */
                }
            }
        }
    }
}

/*
 * -- scfs_zero_tail --------------------------------------------------
 * Zero the bytes from 'tail' to the end of the UNFS block at
 * 'blk_off'.  Called when a shrink cuts part-way through a block, so a
 * later read of that block does not see the bytes of the longer file.
 *
 * The write is read-modify-write in UNFS block units - the buffer cache
 * moves 512-byte sectors, so the whole 4 KB block is assembled, edited
 * and written back.
 */
static void scfs_zero_tail(InCoreInode *ip, uint32_t blk_off, uint32_t tail)
{
    BmapResult r = bmap(ip, blk_off);           /* 01_fsa */
    uint8_t    blk[UNFS_BLOCK_SIZE];
    uint32_t   s;
    bool       ok = true;

    if (!r.valid || r.blkno == 0u) return;      /* a hole - already zero */

    for (s = 0u; s < (uint32_t)UNFS_SECTORS_PER_BLOCK; s++) {
        BufHdr *buf = bread(r.dev,
                            r.blkno * (uint32_t)UNFS_SECTORS_PER_BLOCK + s);
        if (!buf) { ok = false; break; }

        memcpy(blk + (s * (uint32_t)BCACHE_SECTOR_SIZE),
               buf->data, (size_t)BCACHE_SECTOR_SIZE);
        brelse(buf);
    }
    if (!ok) return;

    for (s = tail; s < (uint32_t)UNFS_BLOCK_SIZE; s++) blk[s] = 0u;

    for (s = 0u; s < (uint32_t)UNFS_SECTORS_PER_BLOCK; s++) {
        BufHdr *buf = bread(r.dev,
                            r.blkno * (uint32_t)UNFS_SECTORS_PER_BLOCK + s);
        if (!buf) break;

        memcpy(buf->data,
               blk + (s * (uint32_t)BCACHE_SECTOR_SIZE),
               (size_t)BCACHE_SECTOR_SIZE);
        bwrite(buf, true, false);               /* 01_fsa */
    }
}

static int scfs_do_truncate(InCoreInode *ip, uint32_t newsize)
{
    if (!ip) return SCFS_EINVAL;
    if (inode_is_dir(ip)) return SCFS_EISDIR;

    /* -- shrink to zero: 01_fsa frees the data AND the map ----------
     * fs_free_inode_blocks() walks the inline extents and the overflow
     * tree itself, so calling it here and then trimming the map would
     * free every block twice.  It zeroes the map as it goes - see
     * superblock.c - so nothing else is needed. */
    if (newsize == 0u) {
        fs_free_inode_blocks(ip);               /* 01_fsa */
        ip->size   = 0u;
        ip->flags |= (IFLAG_CHANGED | IFLAG_MODIFIED);
        iupdate(ip);                            /* 01_fsa */
        return SCFS_OK;
    }

    if (newsize == ip->size) return SCFS_OK;

    /* -- partial shrink ---------------------------------------------- */
    if (newsize < ip->size) {
        /* All arithmetic in UNFS 4096-byte blocks.  BLOCK_SIZE is 512
         * - the sector size - and using it here was the unit bug. */
        uint32_t last_blk = newsize / (uint32_t)UNFS_BLOCK_SIZE;
        uint32_t tail     = newsize % (uint32_t)UNFS_BLOCK_SIZE;
        uint32_t offset;
        uint32_t first_clear;

        /* The block that straddles newsize: zero its tail so a later
         * read does not see bytes from the longer file.  None when the
         * new size lands exactly on a block boundary. */
        if (tail != 0u) {
            scfs_zero_tail(ip, last_blk * (uint32_t)UNFS_BLOCK_SIZE, tail);
        }

        /* The first logical block that no longer exists.  Rounding UP
         * is what 'tail != 0' decides: a partial block is still live. */
        first_clear = (tail != 0u) ? (last_blk + 1u) : last_blk;
        offset      = first_clear * (uint32_t)UNFS_BLOCK_SIZE;

        /* Free the DATA blocks past the cut.  Extents name runs of
         * CONSECUTIVE blocks, so this is a walk in 4 KB units, not a
         * pointer chase. */
        while (offset < ip->size) {
            BmapResult r = bmap(ip, offset);    /* 01_fsa */

            if (r.valid && r.blkno != 0u)
                fs_free(r.dev, r.blkno);        /* 01_fsa */

            offset += UNFS_BLOCK_SIZE;
        }

        /* THE INVARIANT: after this call no extent names a block at or
         * beyond 'first_clear'.  Without it the inode still points at
         * freed blocks and the next allocation can hand one to another
         * file. */
        scfs_clear_map_past(ip, first_clear);

        ip->size   = newsize;
        ip->flags |= (IFLAG_CHANGED | IFLAG_MODIFIED);
        iupdate(ip);                            /* 01_fsa */
        return SCFS_OK;
    }

    /* -- grow ---------------------------------------------------------
     *
     * NOT IMPLEMENTED, and it cannot be faked.  Extending a file needs
     * an extent that names blocks the file does not yet own, and
     * bmap_alloc() is a stub: it reports valid == false and writes
     * nothing, so the first block would never reach the disk.
     *
     * ENOSYS, not ENOSPC.  The disk is not full - the allocator does
     * not exist.  A caller that treats ENOSPC as retryable would loop
     * forever against a stub. */
    return SCFS_ENOSYS;
}

/* -- truncate() - by path name --------------------------------------- */
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
    iput(ip);                               /* 01_fsa */
    return rc;
}

/* -- ftruncate() - by descriptor ------------------------------------- */
int uiox_kix_scfs_ftruncate(int fd, uint32_t len)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;
    if (!(f->f_flag & FWRITE)) return SCFS_EINVAL;

    /* The file table entry owns the inode reference - no iput.  The
     * entry's offset stays where it was even if it now points past the
     * end; POSIX leaves it alone and a later read returns 0. */
    return scfs_do_truncate(f->f_inode, len);
}

/*
 * fallocate() - reserve space without writing it.
 *
 * Needs an allocator that can mark blocks allocated while leaving them
 * unwritten, which 01_fsa's bitmap does not distinguish.  Reported rather
 * than implemented as "write zeros", which is a different syscall's
 * semantics - a caller preallocating a large file would get a full disk
 * instead of a reservation.
 */
int uiox_kix_scfs_fallocate(int fd, int mode, uint32_t off, uint32_t len)
{
    (void)fd; (void)mode; (void)off; (void)len;
    return SCFS_ENOSYS;
}
