/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_dir.c
 *
 * chdir / fchdir / getcwd / chroot / getdents64
 *
 * Bach Ch.5 — the current directory (u.u_cdir) and the process root
 * (u.u_rootdir) are process state, owned by 33_PCS; SCFS resolves the path
 * and hands it over.  getdents64 walks a directory, which Bach models as a
 * file of fixed-size entries.
 *
 * @version 1.0.0  @date 2026-09-21
 */
#include "uiox_kix_scfs_internal.h"

/* chdir() — resolve the path, verify it is a directory, set the cwd. */
long uiox_kix_scfs_chdir(uiox_reg_t uptr,
                         uiox_reg_t a1, uiox_reg_t a2, uiox_reg_t a3,
                         uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    if (uptr == 0u) return -SCFS_EFAULT;

    uiox_inode_t *inode = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)uptr, &inode, 0u);
    if (rc < 0) return rc;
    if (inode && !(inode->i_mode & UIOX_S_IFDIR)) return -SCFS_ENOTDIR;

    return pcs_setcwd((const char *)uptr);
}

/* fchdir() — set the cwd from an open directory descriptor. */
long uiox_kix_scfs_fchdir(uiox_reg_t fd,
                          uiox_reg_t a1, uiox_reg_t a2, uiox_reg_t a3,
                          uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_inode || !(f->f_inode->i_mode & UIOX_S_IFDIR))
        return -SCFS_ENOTDIR;
    if (!f->f_inode->i_path) return -SCFS_ENOSYS;   /* no dentry path */
    return pcs_setcwd(f->f_inode->i_path);
}

/* getcwd() — read the process cwd into the user buffer. */
long uiox_kix_scfs_getcwd(uiox_reg_t ubuf, uiox_reg_t size,
                          uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (ubuf == 0u || size == 0u) return -SCFS_EINVAL;
    return pcs_getcwd((char *)ubuf, (uint32_t)size);
}

/* chroot() — set the process root prefix (Bach u.u_rootdir). */
long uiox_kix_scfs_chroot(uiox_reg_t uptr,
                          uiox_reg_t a1, uiox_reg_t a2, uiox_reg_t a3,
                          uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    if (uptr == 0u) return -SCFS_EFAULT;

    uiox_inode_t *inode = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)uptr, &inode, 0u);
    if (rc < 0) return rc;
    if (inode && !(inode->i_mode & UIOX_S_IFDIR)) return -SCFS_ENOTDIR;

    return pcs_setroot((const char *)uptr);
}

/* getdents64() — read directory entries into the user buffer, tracking
 * position in f_pos (Bach: a directory is a file of entries). */
long uiox_kix_scfs_getdents64(uiox_reg_t fd, uiox_reg_t ubuf, uiox_reg_t count,
                              uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a3; (void)a4; (void)a5;
    if (ubuf == 0u || count == 0u) return -SCFS_EINVAL;

    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_inode || !(f->f_inode->i_mode & UIOX_S_IFDIR))
        return -SCFS_ENOTDIR;
    if (!f->f_op || !f->f_op->readdir) return -SCFS_ENOSYS;

    return (long)f->f_op->readdir(f, (void *)ubuf, (size_t)count);
}
