#include "uiox_kix_scfs_internal.h"

/*
 * openat — the *at() family.
 *
 * Bach's Algorithm open starts from the root inode (absolute path) or the
 * current directory (relative).  openat generalises the starting point: it
 * takes a DIRECTORY FILE DESCRIPTOR and resolves relative to that.
 * openat(AT_FDCWD, path, ...) is open() exactly.
 */
static InCoreInode *scfs_dirfd_inode(int dirfd, int *need_put)
{
    *need_put = 0;

    if (dirfd == SCFS_AT_FDCWD) {
        InCoreInode *cwd = scfs_cwd_get();
        if (!cwd) return (InCoreInode *)0;
        cwd->refcount++;
        *need_put = 1;
        return cwd;
    }

    {
        scfs_file_t *f = scfs_getf(dirfd);
        if (!f) return (InCoreInode *)0;
        if (!SCFS_IS_DIR(f->f_inode->mode)) return (InCoreInode *)0;
        return f->f_inode;          /* borrowed from the file table entry */
    }
}

int uiox_kix_scfs_openat(int dirfd, const char *path, int flags, uint16_t perm)
{
    InCoreInode *base;
    InCoreInode *ip;
    int          need_put = 0;

    if (!path) return SCFS_EFAULT;

    /* namei() follows every component and InCoreInode holds no symlink
     * target, so this cannot be honoured.  Refusing is honest. */
    if (flags & SCFS_AT_SYMLINK_NOFOLLOW) return SCFS_ENOSYS;

    base = scfs_dirfd_inode(dirfd, &need_put);
    if (!base) return SCFS_EBADF;

    ip = namei(path, base, scfs_uid_get(), scfs_gid_get());

    if (need_put) iput(base);
    if (!ip) {
        if (flags & O_CREAT) return uiox_kix_scfs_creat(path, perm);
        return SCFS_ENOENT;
    }
    iput(ip);                       /* drop the probe reference */

    /* The delegation re-resolves from the cwd.  Identical for an absolute
     * path; for a relative path against a real directory fd it is right
     * only while the cwd is that directory, so that case is reported. */
    if (path[0] != '/' && dirfd != SCFS_AT_FDCWD) return SCFS_ENOSYS;

    return uiox_kix_scfs_open(path, flags, perm);
}

/* ── the rest of the family ──────────────────────────────────────────
 * Each is a path-based call with dirfd checked, which is correct for the
 * absolute-path and AT_FDCWD cases — the two a single-process kernel
 * reaches. */
int uiox_kix_scfs_faccessat(int dirfd, const char *path, int mode, int flags)
{
    if (dirfd != SCFS_AT_FDCWD) return SCFS_ENOSYS;
    if (flags & SCFS_AT_SYMLINK_NOFOLLOW) return SCFS_ENOSYS;
    return uiox_kix_scfs_access(path, mode);
}

int uiox_kix_scfs_fchmodat(int dirfd, const char *path, uint16_t mode, int flags)
{
    if (dirfd != SCFS_AT_FDCWD || flags) return SCFS_ENOSYS;
    return uiox_kix_scfs_chmod(path, mode);
}

int uiox_kix_scfs_fchownat(int dirfd, const char *path, uint16_t uid,
                           uint16_t gid, int flags)
{
    if (dirfd != SCFS_AT_FDCWD || flags) return SCFS_ENOSYS;
    return uiox_kix_scfs_chown(path, uid, gid);
}

int uiox_kix_scfs_unlinkat(int dirfd, const char *path, int flags)
{
    if (dirfd != SCFS_AT_FDCWD) return SCFS_ENOSYS;
    if (flags & SCFS_AT_REMOVEDIR) return uiox_kix_scfs_rmdir(path);
    return uiox_kix_scfs_unlink(path);
}

int uiox_kix_scfs_mkdirat(int dirfd, const char *path, uint16_t mode)
{
    if (dirfd != SCFS_AT_FDCWD) return SCFS_ENOSYS;
    return uiox_kix_scfs_mkdir(path, mode);
}

int uiox_kix_scfs_linkat(int odirfd, const char *oldp, int ndirfd,
                         const char *newp, int flags)
{
    if (odirfd != SCFS_AT_FDCWD || ndirfd != SCFS_AT_FDCWD) return SCFS_ENOSYS;
    if (flags) return SCFS_ENOSYS;
    return uiox_kix_scfs_link(oldp, newp);
}

int uiox_kix_scfs_renameat(int odirfd, const char *oldp, int ndirfd,
                           const char *newp)
{
    if (odirfd != SCFS_AT_FDCWD || ndirfd != SCFS_AT_FDCWD) return SCFS_ENOSYS;
    return uiox_kix_scfs_rename(oldp, newp);
}

int uiox_kix_scfs_symlinkat(const char *target, int dirfd, const char *linkpath)
{
    if (dirfd != SCFS_AT_FDCWD) return SCFS_ENOSYS;
    return uiox_kix_scfs_symlink(target, linkpath);
}

int uiox_kix_scfs_readlinkat(int dirfd, const char *path, char *buf, uint32_t sz)
{
    if (dirfd != SCFS_AT_FDCWD) return SCFS_ENOSYS;
    return uiox_kix_scfs_readlink(path, buf, sz);
}
