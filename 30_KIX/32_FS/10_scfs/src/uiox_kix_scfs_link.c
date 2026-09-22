/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_link.c
 *
 * link / unlink / symlink / readlink / rename
 *
 * Bach Ch.4 — a hard link is a second name for one i-node (i_nlink++);
 * unlink drops a name and iput() frees the i-node only when i_nlink and
 * i_count both reach zero.  Ch.5 — a symbolic link is an i-node holding a
 * path string, followed by namei.  Ch.9 — rename is confined to one
 * filesystem (crossing a mount point is EXDEV, here EPERM).
 *
 * dirname() is NOT defined here; mkdir.c owns the single global.
 *
 * Merged unit: the whole name-binding surface lives here.
 *
 * @version 1.0.0  @date 2026-09-21
 */
#include "uiox_kix_scfs_internal.h"

/* ── hard links ─────────────────────────────────────────────────────── */

/* link() — increment i_nlink and add the new name to a directory.  No new
 * i-node is allocated; both names share one. */
long uiox_kix_scfs_link(uiox_reg_t oldp, uiox_reg_t newp,
                        uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (oldp == 0u || newp == 0u) return -SCFS_EFAULT;

    uiox_inode_t *inode = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)oldp, &inode, 0u);
    if (rc < 0) return rc;
    if (!inode) return -SCFS_ENOENT;
    if (inode->i_mode & UIOX_S_IFDIR) return -SCFS_EACCES;  /* no dir links */

    uiox_inode_t *parent = (uiox_inode_t *)0;
    rc = vfs_path_lookup((const char *)newp, &parent, UIOX_VFS_PARENT);
    if (rc < 0) return rc;
    if (!parent || !parent->i_op || !parent->i_op->link) return -SCFS_ENOSYS;

    rc = parent->i_op->link(parent, inode, (const char *)newp);
    if (rc < 0) return rc;

    inode->i_nlink++;
    return SCFS_OK;
}

/* unlink() — Bach Ch.4 iput(): drop the name; free blocks and i-node only
 * when i_nlink == 0 AND no open file still references it. */
long uiox_kix_scfs_unlink(uiox_reg_t uptr,
                          uiox_reg_t a1, uiox_reg_t a2, uiox_reg_t a3,
                          uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    if (uptr == 0u) return -SCFS_EFAULT;

    uiox_inode_t *inode = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)uptr, &inode, 0u);
    if (rc < 0) return rc;
    if (!inode) return -SCFS_ENOENT;

    rc = (inode->i_op && inode->i_op->unlink)
       ? inode->i_op->unlink(inode)
       : vfs_unlink((const char *)uptr);
    if (rc < 0) return rc;

    if (inode->i_nlink > 0u) inode->i_nlink--;

    /* Deferred free — Bach Ch.7: the file table holds a reference. */
    if (inode->i_nlink == 0u && inode->i_count == 0u &&
        inode->i_op && inode->i_op->free_inode)
        return inode->i_op->free_inode(inode);

    return SCFS_OK;
}

/* ── symbolic links ─────────────────────────────────────────────────── */

/* symlink() — create a symbolic link whose i-node data is the target path. */
long uiox_kix_scfs_symlink(uiox_reg_t target, uiox_reg_t linkpath,
                           uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (target == 0u || linkpath == 0u) return -SCFS_EFAULT;
    return vfs_symlink((const char *)target, (const char *)linkpath);
}

/* readlink() — copy the stored target path out; not NUL-terminated. */
long uiox_kix_scfs_readlink(uiox_reg_t uptr, uiox_reg_t ubuf, uiox_reg_t bufsiz,
                            uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a3; (void)a4; (void)a5;
    if (uptr == 0u || ubuf == 0u || bufsiz == 0u) return -SCFS_EINVAL;

    uiox_inode_t *inode = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)uptr, &inode, UIOX_VFS_NOFOLLOW);
    if (rc < 0) return rc;
    if (!inode || !(inode->i_mode & UIOX_S_IFLNK)) return -SCFS_EINVAL;

    return vfs_readlink((const char *)uptr, (char *)ubuf, (size_t)bufsiz);
}

/* ── rename ─────────────────────────────────────────────────────────── */

/* rename() — move a directory entry.  Bach Ch.9: a rename cannot cross a
 * mount point, so a device mismatch is refused. */
long uiox_kix_scfs_rename(uiox_reg_t oldp, uiox_reg_t newp,
                          uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (oldp == 0u || newp == 0u) return -SCFS_EFAULT;

    uiox_inode_t *old_in = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)oldp, &old_in, 0u);
    if (rc < 0) return rc;
    if (!old_in) return -SCFS_ENOENT;

    uiox_inode_t *new_parent = (uiox_inode_t *)0;
    rc = vfs_path_lookup((const char *)newp, &new_parent, UIOX_VFS_PARENT);
    if (rc < 0) return rc;

    if (new_parent && old_in->i_dev != new_parent->i_dev)
        return -SCFS_EPERM;                 /* EXDEV — cross-filesystem */

    return vfs_rename((const char *)oldp, (const char *)newp);
}
