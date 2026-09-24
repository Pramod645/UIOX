/*
 *  30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_sync.c
 *
 *  SCFS — sync, fsync, fdatasync.  CORRECTED.
 *
 *  ── what Bach says ───────────────────────────────────────────────────
 *  Ch.3 §4 on the buffer cache and Ch.4 §1.2 on the inode cache.  Bach's
 *  kernel is write-behind: a write() puts data in a buffer marked DELAYED
 *  WRITE and returns; the buffer reaches disk later.  These calls force
 *  that delay to end.
 *
 *    sync()       schedule EVERY delayed buffer and every changed inode,
 *                 asynchronously, system-wide.  Normally issued by
 *                 update(8) on a timer, not by a program.
 *    fsync()      write this file's data AND metadata, and WAIT.
 *    fdatasync()  the same, but only the metadata needed to READ THE DATA
 *                 BACK — the size and the block map.  A pure mtime change
 *                 is skipped, which is why the call exists.
 *
 *  ── the correction ───────────────────────────────────────────────────
 *  The first cut called inode_cache_sync() to sweep the inode cache for
 *  changed inodes.  NO SUCH FUNCTION EXISTS in 01_fsa.  inode.c exposes
 *  iget / iput / iupdate / inode_disk_read and nothing that walks the
 *  cache.
 *
 *  sync() therefore does what this pair of layers can actually do:
 *
 *    · buf_sync()  — 01_fsa's flush of every dirty buffer.  THIS IS THE
 *                    REAL WORK in this filesystem, because stores are
 *                    simulated: a "dirty buffer" IS a block that has not
 *                    reached the disk array, and buf_sync() is exactly
 *                    the write-behind drain Bach describes.
 *    · iupdate()   — applied on the inodes this layer holds, which are
 *                    the open files.  An inode changed and then closed
 *                    was already written by iput(), so nothing is lost.
 *
 *  What is NOT done, stated plainly: an inode that is changed, still
 *  cached, and NOT open is not swept — because no function can reach it.
 *  That is a gap in 01_fsa, not a choice here.  Adding an inode cache
 *  walk (inode_cache_sync) to inode.c closes it, and the call site below
 *  is marked for it.
 *
 *  ── fdatasync's distinction is real ──────────────────────────────────
 *  fs_types.h gives the flag word the split this call needs:
 *
 *      IFLAG_ACCESSED  metadata: a read happened
 *      IFLAG_CHANGED   metadata: the inode moved
 *      IFLAG_MODIFIED  DATA:     the file's contents moved
 *
 *  fdatasync tests IFLAG_MODIFIED.  A pure timestamp update sets only
 *  CHANGED, so it is skipped — which is the whole point of the call.
 *
 *  v1.1: inode_cache_sync removed; the cache-walk gap named.
 */
#include "uiox_kix_scfs_internal.h"

/* ═════════════════════════════════════════════════════════════════════
 * sync() — system-wide, asynchronous
 * ═════════════════════════════════════════════════════════════════════ */
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

    /* The super block's own modified flag is readable through sb_access.c;
     * module-free this filesystem keeps it in memory only. */
    if (sb_is_modified()) sb_clear_modified();
}

/* ═════════════════════════════════════════════════════════════════════
 * fsync() — this file's data and metadata, and wait
 * ═════════════════════════════════════════════════════════════════════ */
int uiox_kix_scfs_fsync(int fd)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    /* Metadata first: the inode carries size, block map and timestamps,
     * so writing it records WHERE the data is before the data lands
     * — which is the order that makes a crash recoverable rather than
     * leaving blocks allocated to nothing. */
    iupdate(f->f_inode);                    /* 01_fsa */

    /* Then the data.  buf_sync() drains the whole pool because the cache
     * has no per-file handle — a narrower flush would need the list of
     * blocks this inode owns, which is what fsync's guarantee is about
     * but the cache cannot yet express. */
    buf_sync();                             /* 01_fsa */

    /* The call returns only after the flush ran — the difference from
     * sync(). */
    return SCFS_OK;
}

/* ═════════════════════════════════════════════════════════════════════
 * fdatasync() — only what is needed to read the data back
 * ═════════════════════════════════════════════════════════════════════ */
int uiox_kix_scfs_fdatasync(int fd)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    /* The split fs_types.h already makes:
     *
     *   IFLAG_MODIFIED  the DATA changed      -> must reach the disk
     *   IFLAG_CHANGED   metadata only changed -> may be skipped
     *   IFLAG_ACCESSED  a read happened       -> may be skipped
     *
     * A pure timestamp update (utime on a file nobody wrote) sets only
     * CHANGED, so skipping the inode write here is correct and is exactly
     * what fdatasync is for.  A size or block-map change sets MODIFIED
     * alongside it, so it is not skipped. */
    if (f->f_inode->flags & IFLAG_MODIFIED) {
        iupdate(f->f_inode);                /* 01_fsa */
        buf_sync();                         /* 01_fsa */
    }

    return SCFS_OK;
}

/* ═════════════════════════════════════════════════════════════════════
 * syncfs() — one filesystem, not the whole system
 * ═════════════════════════════════════════════════════════════════════ */
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

/* ═════════════════════════════════════════════════════════════════════
 * sync_file_range() — flush a byte range
 * ═════════════════════════════════════════════════════════════════════ */
int uiox_kix_scfs_sync_file_range(int fd, uint32_t off, uint32_t len, int flags)
{
    scfs_file_t *f;

    (void)off; (void)len; (void)flags;

    f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    /* The buffer cache holds whole blocks with no notion of "the part of
     * this block that belongs to this range", so a range flush cannot be
     * expressed.  Flushing everything is a superset of what was asked —
     * safe, and honest about being coarser. */
    iupdate(f->f_inode);                    /* 01_fsa */
    buf_sync();                             /* 01_fsa */
    return SCFS_OK;
}

/* sys_* aliases — the consolidated syscalls.c owns these; keep one. */
void sys_sync(void)    { uiox_kix_scfs_sync(); }
int  sys_fsync(int fd) { return uiox_kix_scfs_fsync(fd); }
