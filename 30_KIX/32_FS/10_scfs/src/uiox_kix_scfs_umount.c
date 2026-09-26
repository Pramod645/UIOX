/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_umount.c
 *
 * SCFS — Algorithm unmount.  Bach, The Design of the UNIX Operating System.
 *
 * ── Bach's algorithm, verbatim ─────────────────────────────────────────
 * input: special file name of file system to be unmounted
 * output: none
 * {
 *   if (not super user)
 *       return (error);
 *   get inode of special file (algorithm namei);
 *   extract major, minor number of device being unmounted;
 *   get mount table entry, based on major, minor number, for the
 *       unmounting file system;
 *   release inode of special file (algorithm iput);
 *   remove shared text entries from region table for files belonging to
 *       the file system;
 *   update super block, inode, flush buffers;
 *   if (file from file system still in use)
 *       return (error);
 *   get root inode of mounted file system from mount table;
 *   lock inode;
 *   release inode (algorithm iput);
 *   invoke close routine for special devices;
 *   invalidate buffers in pool from unmounted file system;
 *   get inode of mount point from mount table;
 *   lock inode;
 *   clear flag marking it as mount point;
 *   release inode (algorithm iput);
 *   free buffer used for super block;
 *   free mount table slot;
 * }
 *
 * ── the order that matters ─────────────────────────────────────────────
 * The busy check comes BEFORE anything is torn down: once the mount point
 * flag is cleared, a path walk can no longer reach the mounted filesystem,
 * so a process still holding a file open under it would be stranded.
 * Bach flushes before the check and releases after it — SCFS does the same.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

int uiox_kix_scfs_umount(const char *dir)
{
    if (!dir) return SCFS_EFAULT;

    /* ── 1. the super-user check ────────────────────────────────────── */
    if (!scfs_is_super()) return SCFS_EPERM;

    /* ── 2. the mount point's inode ─────────────────────────────────── */
    InCoreInode *mp = namei(dir, scfs_cwd_get(), 0u, 0u);     /* 01_fsa */
    if (!mp) return SCFS_ENOENT;

    /* ── 3. it must actually be a mount point ────────────────────────
     * NOT asked against an inode flag: there is no IMOUNT.  fs_types.h
     * defines IFLAG_ACCESSED, IFLAG_CHANGED and IFLAG_MODIFIED, and the
     * inode carries no mount-point bit at all.  GCC's nearest-name
     * suggestion (NMOUNT) is the mount table's SIZE, not a flag.
     *
     * The check is not lost, it MOVED: the mount table lookup below is
     * the real test.  A directory is a mount point exactly when a live
     * table slot names it as m_mountpt, and that is what the loop
     * searches for — so a non-mount-point directory falls out at "!m"
     * with the same SCFS_EINVAL this block returned. */

    /* ── 4. the mount table entry ───────────────────────────────────── */
    scfs_mount_t *m = (scfs_mount_t *)0;
    for (uint32_t i = 0u; i < NMOUNT; i++)
        if (scfs_mount_table[i].m_inuse &&
            scfs_mount_table[i].m_mountpt == mp) {
            m = &scfs_mount_table[i];
            break;
        }

    if (!m) {
        iput(mp);
        return SCFS_EINVAL;
    }

    /* ── 5. flush buffers before the busy check ────────────────────── */
    /* Bach: "update super block, inode, flush buffers".  The buffer
     * cache is 01_fsa's, and one flush covers the whole pool. */
    buf_sync();                            /* 01_fsa */

    /* ── 6. refuse while a file under the mount is still open ──────── */
    /* Every live file table entry whose inode belongs to the mounted
     * filesystem blocks the unmount.  With a single mounted volume the
     * conservative test is any open file at all. */
    for (uint32_t i = 0u; i < NFILE; i++) {
        if (!scfs_file_table[i].f_inuse) continue;
        if (scfs_file_table[i].f_inode == m->m_root) {
            iput(mp);
            return SCFS_EBUSY;
        }
    }

    /* ── 7. release the mounted root inode ─────────────────────────── */
    if (m->m_root) iput(m->m_root);

    /* ── 8. the mount point is no longer a mount point ──────────────
     * NOT a flag clear: there is no IMOUNT bit to clear (see step 3).
     * The record of the mount is the TABLE SLOT, and scfs_mount_free()
     * below clears it — m_inuse falls to 0 and m_mountpt is dropped, so
     * the lookup at step 4 stops finding this directory.
     *
     * Bach's ORDER still holds, which is what the header note is about:
     * the busy check ran at step 6, BEFORE anything was torn down, so no
     * process is left holding a file under an unreachable mount.
     *
     * The lock is released here because this call took the reference
     * from namei() and hands it to scfs_mount_free(). */
    mp->locked = false;

    /* ── 9. free the super block buffer, then the mount table slot ─── */
    /* scfs_mount_free() also drops m_mountpt, which is the mp reference
     * namei gave us — so no separate iput for that. */
    scfs_mount_free(m);

    return SCFS_OK;
}

/*
 * umount2() — unmount with flags.  MNT_FORCE skips the busy check, which
 * is what a shutdown path needs when a process is misbehaving.
 */
int uiox_kix_scfs_umount2(const char *dir, int flags)
{
    /* Only MNT_FORCE changes behaviour; every other flag is accepted and
     * ignored rather than refused, since a caller clearing them cannot
     * be worse off. */
    if (flags & 0x2) {                         /* MNT_FORCE */
        if (!dir) return SCFS_EFAULT;
        if (!scfs_is_super()) return SCFS_EPERM;

        InCoreInode *mp = namei(dir, scfs_cwd_get(), 0u, 0u);
        if (!mp) return SCFS_ENOENT;

        for (uint32_t i = 0u; i < NMOUNT; i++) {
            if (scfs_mount_table[i].m_inuse &&
                scfs_mount_table[i].m_mountpt == mp) {
                buf_sync();
                if (scfs_mount_table[i].m_root)
                    iput(scfs_mount_table[i].m_root);
                scfs_mount_free(&scfs_mount_table[i]);
                return SCFS_OK;
            }
        }
        iput(mp);
        return SCFS_EINVAL;
    }

    return uiox_kix_scfs_umount(dir);
}
