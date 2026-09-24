#include "uiox_kix_scfs_internal.h"

/*
 * ── the correction this file carries ─────────────────────────────────
 * The first cut called inode_cache_sync() to sweep the inode cache for
 * changed inodes.  NO SUCH FUNCTION EXISTS in 01_fsa — inode.c exposes
 * iget / iput / iupdate / inode_disk_read and nothing that walks the
 * cache.  sync() therefore does what the two layers can actually do.
 */

void uiox_kix_scfs_sync(void)
{
    uint32_t i;

    /* ── the buffer cache drain ─────────────────────────────────────
     * Bach: "the sync system call ... schedules all the delayed write
     * operations for the buffer cache and the inode cache."  In this
     * filesystem buf_sync() is that drain — it walks all 64 cache slots
     * and writes every dirty one to the disk array. */
    buf_sync();                             /* 01_fsa */

    /* ── the inodes this layer can reach ────────────────────────────
     * Every open file's inode is in the file table, so sweeping that
     * table reaches every inode that is both cached and in use. */
    for (i = 0u; i < NFILE; i++) {
        scfs_file_t *f = &scfs_file_table[i];

        if (!f->f_inuse || !f->f_inode) continue;
        if (f->f_inode->flags & (IFLAG_ACCESSED | IFLAG_CHANGED | IFLAG_MODIFIED))
            iupdate(f->f_inode);            /* 01_fsa */
    }

    /* ── the gap ─────────────────────────────────────────────────────
     * An inode that is changed, still in the cache, and NOT open is not
     * reached by the loop above.  Reaching it needs a cache walk that
     * 01_fsa does not export:
     *
     *     inode_cache_sync();      // would go here — does not exist
     *
     * Add it to 01_fsa/src/inode.c (walk icache[0..MAX_INCACHE-1],
     * iupdate any slot with refcount > 0 and dirty flags set) and the
     * uncomment below closes the gap with no change to this unit's
     * callers. */
    /* inode_cache_sync(); */

    if (sb_is_modified()) sb_clear_modified();
}

int uiox_kix_scfs_fsync(int fd)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    /* Metadata first: the inode carries size, block map and timestamps,
     * so writing it records WHERE the data is before the data lands. */
    iupdate(f->f_inode);                    /* 01_fsa */

    /* Then the data.  buf_sync() drains the whole pool because the cache
     * has no per-file handle. */
    buf_sync();                             /* 01_fsa */

    return SCFS_OK;
}

int uiox_kix_scfs_fdatasync(int fd)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    /* The split fs_types.h already makes:
     *   IFLAG_MODIFIED  the DATA changed      -> must reach the disk
     *   IFLAG_CHANGED   metadata only changed -> may be skipped
     *   IFLAG_ACCESSED  a read happened       -> may be skipped
     *
     * A pure timestamp update sets only CHANGED, so skipping the inode
     * write here is exactly what fdatasync is for. */
    if (f->f_inode->flags & IFLAG_MODIFIED) {
        iupdate(f->f_inode);                /* 01_fsa */
        buf_sync();                         /* 01_fsa */
    }

    return SCFS_OK;
}

int uiox_kix_scfs_syncfs(int fd)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    /* 01_fsa owns one mounted filesystem, so syncing it and syncing the
     * system are the same work today.  Kept separate because it will not
     * be once mount() can read a second device. */
    uiox_kix_scfs_sync();
    return SCFS_OK;
}

int uiox_kix_scfs_sync_file_range(int fd, uint32_t off, uint32_t len, int flags)
{
    scfs_file_t *f;

    (void)off; (void)len; (void)flags;

    f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    /* The buffer cache holds whole blocks with no notion of "the part of
     * this block that belongs to this range", so a range flush cannot be
     * expressed.  Flushing everything is a superset of what was asked. */
    iupdate(f->f_inode);                    /* 01_fsa */
    buf_sync();                             /* 01_fsa */
    return SCFS_OK;
}
