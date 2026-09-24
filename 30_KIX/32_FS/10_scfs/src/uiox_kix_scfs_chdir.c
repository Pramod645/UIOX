/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_chdir.c
 *
 * SCFS — Algorithm chdir.  Bach, The Design of the UNIX Operating System.
 *
 * ── Bach's algorithm, verbatim ─────────────────────────────────────────
 * input: new directory name
 * output: none
 * {
 *   get inode for new directory name (algorithm namei);
 *   if (inode not that of directory or process not permitted access)
 *   {
 *       release inode (algorithm iput);
 *       return (error);
 *   }
 *   unlock (inode);
 *   release "old" current directory inode (algorithm iput);
 *   place new inode into current directory slot in u area;
 * }
 *
 * ── Bach's note on the cwd ─────────────────────────────────────────────
 * "When the system is first booted, process 0 makes the file system root
 *  its current directory during initialization.  It executes algorithm
 *  iget on the root inode, saves it in the u area as its current
 *  directory, and releases the inode lock.  When a new process is created
 *  via fork, the new process inherits the current directory of the old
 *  process in its u area, and the kernel increments the inode reference
 *  count accordingly."
 *
 * That is why this unit keeps its own reference on the new cwd and drops
 * the old one: the u-area slot holds a reference, exactly as a file table
 * entry does.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

int uiox_kix_scfs_chdir(const char *path)
{
    if (!path) return SCFS_EFAULT;

    /* ── get the inode for the new directory name (namei) ───────────── */
    InCoreInode *ip = namei(path, scfs_cwd_get(), 0u, 0u);   /* 01_fsa */
    if (!ip) return SCFS_ENOENT;

    /* ── must be a directory, and executable (searchable) ───────────── */
    /* Mode tests use 01_fsa's encoding: (mode >> 12) & 0xF == FT_DIR. */
    if (!SCFS_S_ISDIR(ip->mode)) {
        iput(ip);
        return SCFS_ENOTDIR;
    }
    /* Bach checks "permitted access"; for a directory the access that
     * matters is execute — the right to walk through it. */
    if (!inode_access_ok(ip, 0u, 0u, 0, 0, 1)) {    /* 01_fsa */
        iput(ip);
        return SCFS_EACCES;
    }

    /* ── unlock ─────────────────────────────────────────────────────── */
    ip->locked = false;

    /* ── release the OLD current directory's reference ──────────────── */
    InCoreInode *old = scfs_cwd_get();
    if (old && old != ip) iput(old);                /* 01_fsa */

    /* ── place the new inode in the current directory slot ─────────── */
    /* namei() already took a reference for us; that reference is what the
     * u-area slot now holds, so no further iput is due. */
    scfs_cwd_set(ip);

    return SCFS_OK;
}
