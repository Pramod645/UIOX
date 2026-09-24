/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_close.c
 *
 * SCFS — Algorithm close.  Bach, The Design of the UNIX Operating System.
 *
 * ── what Bach says ─────────────────────────────────────────────────────
 * close() releases ONE user file descriptor.  It does not necessarily
 * close the file: if another descriptor still points at the same file
 * table entry (via dup or fork), the entry and its inode stay open.  The
 * file table entry is freed only when its reference count reaches zero,
 * and the inode is iput() only then.
 *
 *   input:  user file descriptor
 *   output: none
 *   {
 *       if (the user file descriptor is not active)
 *           return (error);
 *       set the corresponding entry in the user file descriptor table
 *           to NULL;
 *       if (the file table entry reference count is greater than 1)
 *           decrement the reference count;
 *       else
 *       {
 *           free the file table entry;
 *           release the inode (algorithm iput);
 *       }
 *   }
 *
 * The deferred release is what makes a deleted-but-open file keep
 * working: unlink() drops the name and the link count, but iput() does
 * not free the blocks while a descriptor still holds the entry.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

int uiox_kix_scfs_close(int fd)
{
    /* ── the descriptor must be active ──────────────────────────────── */
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    /* ── clear the user fd table slot ───────────────────────────────── */
    /* Bach sets the entry to NULL; the slot index is then free for the
     * next open(), which is why scfs_ufd_find() can allocate lowest-free. */
    scfs_u->ufd_file[fd] = (scfs_file_t *)0;

    /* ── decrement, or free the entry and iput the inode ────────────── */
    if (f->f_count > 1u) {
        f->f_count--;
    } else {
        /* Last reference: release the inode through 01_fsa.  iput()
         * checks the link count itself — if unlink() already dropped it
         * to zero, this is where the blocks are freed and the inode
         * returned to the free list (algorithm free, then ifree). */
        scfs_fclose_entry(f);
    }

    return SCFS_OK;
}
