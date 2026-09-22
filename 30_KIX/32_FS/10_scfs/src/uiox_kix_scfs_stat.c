/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_stat.c
 *
 * stat / fstat / lstat / newfstatat / statx /
 * chmod / fchmod / chown / fchown / access / umask
 *
 * Bach Ch.4 — the i-node carries i_mode, i_uid, i_gid, i_size, i_nlink;
 * Ch.5 — namei() walks a path to an i-node (NOFOLLOW stops at a symlink).
 * access() checks the mode bits; umask is per-process state (u.u_umask).
 *
 * Merged unit: the whole metadata surface lives here.
 *
 * @version 1.0.0  @date 2026-09-21
 */
#include "uiox_kix_scfs_internal.h"

/* ── status ─────────────────────────────────────────────────────────── */

/* stat() — namei the path, copy i-node fields to user. */
long uiox_kix_scfs_stat(uiox_reg_t uptr, uiox_reg_t ubuf,
                        uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (uptr == 0u || ubuf == 0u) return -SCFS_EFAULT;
    uiox_inode_t *inode = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)uptr, &inode, 0u);
    if (rc < 0) return rc;
    return vfs_fill_stat(inode, (void *)ubuf);
}

/* fstat() — the fd already names the i-node; no namei. */
long uiox_kix_scfs_fstat(uiox_reg_t fd, uiox_reg_t ubuf,
                         uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (ubuf == 0u) return -SCFS_EFAULT;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    return vfs_fill_stat(f->f_inode, (void *)ubuf);
}

/* lstat() — stat that does not follow a final symlink. */
long uiox_kix_scfs_lstat(uiox_reg_t uptr, uiox_reg_t ubuf,
                         uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (uptr == 0u || ubuf == 0u) return -SCFS_EFAULT;
    uiox_inode_t *inode = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)uptr, &inode, UIOX_VFS_NOFOLLOW);
    if (rc < 0) return rc;
    return vfs_fill_stat(inode, (void *)ubuf);
}

/* newfstatat() — stat relative to a directory fd, honouring NOFOLLOW. */
long uiox_kix_scfs_newfstatat(uiox_reg_t dirfd, uiox_reg_t uptr, uiox_reg_t ubuf,
                              uiox_reg_t flags, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a4; (void)a5;
    if (uptr == 0u || ubuf == 0u) return -SCFS_EFAULT;

    uiox_inode_t *inode = (uiox_inode_t *)0;
    uint32_t lflags = ((uint32_t)flags & UIOX_AT_SYMLINK_NOFOLLOW)
                    ? UIOX_VFS_NOFOLLOW : 0u;
    long rc;
    if ((int)dirfd == UIOX_AT_FDCWD) {
        rc = vfs_path_lookup((const char *)uptr, &inode, lflags);
    } else {
        uiox_file_t *d;
        rc = scfs_fd_file(dirfd, &d);
        if (rc < 0) return rc;
        rc = vfs_path_lookup_at(d->f_inode, (const char *)uptr, &inode, lflags);
    }
    if (rc < 0) return rc;
    return vfs_fill_stat(inode, (void *)ubuf);
}

/* statx() — extended metadata with a requested-field mask. */
long uiox_kix_scfs_statx(uiox_reg_t dirfd, uiox_reg_t uptr, uiox_reg_t flags,
                         uiox_reg_t mask, uiox_reg_t ubuf, uiox_reg_t a5)
{
    (void)a5;
    if (uptr == 0u || ubuf == 0u) return -SCFS_EFAULT;

    uiox_inode_t *inode = (uiox_inode_t *)0;
    uint32_t lflags = ((uint32_t)flags & UIOX_AT_SYMLINK_NOFOLLOW)
                    ? UIOX_VFS_NOFOLLOW : 0u;
    long rc;
    if ((int)dirfd == UIOX_AT_FDCWD) {
        rc = vfs_path_lookup((const char *)uptr, &inode, lflags);
    } else {
        uiox_file_t *d;
        rc = scfs_fd_file(dirfd, &d);
        if (rc < 0) return rc;
        rc = vfs_path_lookup_at(d->f_inode, (const char *)uptr, &inode, lflags);
    }
    if (rc < 0) return rc;
    return vfs_fill_statx(inode, (void *)ubuf, (uint32_t)mask);
}

/* ── ownership / permissions ────────────────────────────────────────── */

/* chmod() — set i_mode permission bits by path. */
long uiox_kix_scfs_chmod(uiox_reg_t uptr, uiox_reg_t mode,
                         uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (uptr == 0u) return -SCFS_EFAULT;
    uiox_inode_t *inode = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)uptr, &inode, 0u);
    if (rc < 0) return rc;
    if (!inode) return -SCFS_ENOENT;

    inode->i_mode = (inode->i_mode & UIOX_S_IFMT) | ((uint32_t)mode & 0x0FFFu);
    if (inode->i_op && inode->i_op->write_inode)
        return inode->i_op->write_inode(inode);
    return SCFS_OK;
}

/* fchmod() — set i_mode permission bits by descriptor. */
long uiox_kix_scfs_fchmod(uiox_reg_t fd, uiox_reg_t mode,
                          uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_inode) return -SCFS_EBADF;

    f->f_inode->i_mode = (f->f_inode->i_mode & UIOX_S_IFMT)
                       | ((uint32_t)mode & 0x0FFFu);
    if (f->f_inode->i_op && f->f_inode->i_op->write_inode)
        return f->f_inode->i_op->write_inode(f->f_inode);
    return SCFS_OK;
}

/* chown() — resolve the path; the credential check is 33_PCS's job. */
long uiox_kix_scfs_chown(uiox_reg_t uptr, uiox_reg_t owner, uiox_reg_t group,
                         uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)owner; (void)group; (void)a3; (void)a4; (void)a5;
    if (uptr == 0u) return -SCFS_EFAULT;
    uiox_inode_t *inode = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)uptr, &inode, 0u);
    if (rc < 0) return rc;
    return SCFS_OK;
}

/* fchown() — owner/group update by descriptor. */
long uiox_kix_scfs_fchown(uiox_reg_t fd, uiox_reg_t owner, uiox_reg_t group,
                          uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)owner; (void)group; (void)a3; (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_inode) return -SCFS_EBADF;
    if (f->f_inode->i_op && f->f_inode->i_op->write_inode)
        return f->f_inode->i_op->write_inode(f->f_inode);
    return SCFS_OK;
}

/* access() — permission check against i_mode (Bach Ch.5). */
long uiox_kix_scfs_access(uiox_reg_t uptr, uiox_reg_t mode,
                          uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (uptr == 0u) return -SCFS_EFAULT;
    uiox_inode_t *inode = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)uptr, &inode, 0u);
    if (rc < 0) return rc;
    return vfs_permission_ok(inode, (uint32_t)mode) ? SCFS_OK : -SCFS_EACCES;
}

/* umask() — set the per-process creation mask; returns the previous value. */
long uiox_kix_scfs_umask(uiox_reg_t mask,
                         uiox_reg_t a1, uiox_reg_t a2, uiox_reg_t a3,
                         uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    return (long)pcs_umask_set((uint32_t)mask);
}
