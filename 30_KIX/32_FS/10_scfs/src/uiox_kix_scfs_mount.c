/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_mount.c
 *
 * mount / umount / umount2 / statfs / fstatfs
 *
 * Bach Ch.9 — the mount table holds one entry per mounted filesystem:
 * device number, mount-point i-node, and the mounted-on superblock.  A path
 * walk crossing a mount point is redirected to the mounted root.  umount
 * refuses while the filesystem is busy and flushes dirty i-nodes first.
 * Ch.4 — statfs reports the superblock's free-block / free-inode counts.
 *
 * Merged unit: the mount table and filesystem-statistics surface.
 *
 * @version 1.0.0  @date 2026-09-21
 */
#include "uiox_kix_scfs_internal.h"

/* ── mount / unmount ────────────────────────────────────────────────── */

/* mount() — attach a filesystem at a directory.
 *   a0 = source   a1 = target mount point   a2 = fs type   a3 = flags   a4 = data */
long uiox_kix_scfs_mount(uiox_reg_t src, uiox_reg_t tgt, uiox_reg_t fstype,
                         uiox_reg_t flags, uiox_reg_t data, uiox_reg_t a5)
{
    (void)src; (void)flags; (void)data; (void)a5;
    if (tgt == 0u || fstype == 0u) return -SCFS_EINVAL;

    uiox_inode_t *mntpt = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)tgt, &mntpt, 0u);
    if (rc < 0) return rc;
    if (!mntpt) return -SCFS_ENOENT;
    if (!(mntpt->i_mode & UIOX_S_IFDIR)) return -SCFS_ENOTDIR;

    return vfs_mount((const char *)tgt, (const char *)fstype);
}

/* umount() — detach the filesystem mounted at a path.  Busy check, flush,
 * then remove the mount-table entry (Bach Ch.9). */
long uiox_kix_scfs_umount(uiox_reg_t tgt, uiox_reg_t flags,
                          uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)flags; (void)a2; (void)a3; (void)a4; (void)a5;
    if (tgt == 0u) return -SCFS_EFAULT;

    if (vfs_mount_busy((const char *)tgt)) return -SCFS_EBUSY;

    long rc = vfs_sync_all();               /* flush before detach */
    if (rc < 0) return rc;

    return vfs_unmount((const char *)tgt);
}

/* umount2() — unmount with flags; MNT_FORCE skips the busy check. */
long uiox_kix_scfs_umount2(uiox_reg_t tgt, uiox_reg_t flags,
                           uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (tgt == 0u) return -SCFS_EFAULT;

    if (!((uint32_t)flags & UIOX_MNT_FORCE) && vfs_mount_busy((const char *)tgt))
        return -SCFS_EBUSY;

    long rc = vfs_sync_all();
    if (rc < 0) return rc;

    return vfs_unmount((const char *)tgt);
}

/* ── filesystem statistics ──────────────────────────────────────────── */

/* statfs() — filesystem statistics by path (Bach Ch.4 superblock counts). */
long uiox_kix_scfs_statfs(uiox_reg_t uptr, uiox_reg_t ubuf,
                          uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (uptr == 0u || ubuf == 0u) return -SCFS_EFAULT;
    return vfs_statfs((const char *)uptr, (void *)ubuf);
}

/* fstatfs() — same, addressed by the descriptor's superblock. */
long uiox_kix_scfs_fstatfs(uiox_reg_t fd, uiox_reg_t ubuf,
                           uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (ubuf == 0u) return -SCFS_EFAULT;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_inode) return -SCFS_EBADF;
    return vfs_statfs_sb(f->f_inode->i_dev, (void *)ubuf);
}
