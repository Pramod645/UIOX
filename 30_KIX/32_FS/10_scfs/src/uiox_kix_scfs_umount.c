#include "uiox_kix_scfs_internal.h"

/*
 * Bach's Algorithm unmount, verbatim.
 * {
 *   if (not super user) return (error);
 *   get inode of special file (algorithm namei);
 *   extract major, minor number of device being unmounted;
 *   get mount table entry, based on major, minor number;
 *   release inode of special file (algorithm iput);
 *   remove shared text entries from region table;
 *   update super block, inode, flush buffers;
 *   if (file from file system still in use) return (error);
 *   get root inode of mounted file system from mount table;
 *   lock inode; release inode (algorithm iput);
 *   invoke close routine for special devices;
 *   invalidate buffers in pool from unmounted file system;
 *   get inode of mount point from mount table;
 *   lock inode; clear flag marking it as mount point;
 *   release inode (algorithm iput);
 *   free buffer used for super block;
 *   free mount table slot;
 * }
 *
 * ── the order that matters ───────────────────────────────────────────
 * The busy check comes BEFORE anything is torn down: once the mount point
 * flag is cleared, a path walk can no longer reach the mounted filesystem,
 * so a process still holding a file open under it would be stranded.
 */
static int scfs_umount_common(const char *dir, int force)
{
    InCoreInode *mp;
    scfs_mount_t *m = (scfs_mount_t *)0;
    uint32_t      i;

    if (!dir) return SCFS_EFAULT;
    if (!scfs_is_super()) return SCFS_EPERM;

    mp = namei(dir, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!mp) return SCFS_ENOENT;

    if (!(mp->flags & IMOUNT)) {
        iput(mp);
        return SCFS_EINVAL;
    }

    for (i = 0u; i < NMOUNT; i++) {
        if (scfs_mount_table[i].m_inuse &&
            scfs_mount_table[i].m_mountpt == mp) {
            m = &scfs_mount_table[i];
            break;
        }
    }

    if (!m) { iput(mp); return SCFS_EINVAL; }

    /* flush before the check — Bach's order */
    buf_sync();

    if (!force) {
        for (i = 0u; i < NFILE; i++) {
            if (!scfs_file_table[i].f_inuse) continue;
            if (scfs_file_table[i].f_inode == m->m_root) {
                iput(mp);
                return SCFS_EBUSY;
            }
        }
    }

    if (m->m_root) iput(m->m_root);

    mp->flags &= (uint8_t)~IMOUNT;
    mp->locked = false;

    /* frees the slot and drops m_mountpt, the reference namei gave us */
    scfs_mount_free(m);

    return SCFS_OK;
}

int uiox_kix_scfs_umount(const char *dir)
{ return scfs_umount_common(dir, 0); }

/* umount2 — MNT_FORCE skips the busy check, which is what a shutdown path
 * needs when a process is misbehaving. */
int uiox_kix_scfs_umount2(const char *dir, int flags)
{ return scfs_umount_common(dir, (flags & SCFS_MNT_FORCE) ? 1 : 0); }
