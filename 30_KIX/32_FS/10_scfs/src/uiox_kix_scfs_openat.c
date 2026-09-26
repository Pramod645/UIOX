/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_openat.c
 *
 * SCFS — openat, and the *at() family.
 *
 * ── where this sits relative to Bach ───────────────────────────────────
 * Bach's Algorithm open takes a full path name and starts from the root
 * inode (for an absolute path) or the current directory (for a relative
 * one).  openat() generalises the starting point: it takes a DIRECTORY
 * FILE DESCRIPTOR and resolves the path relative to that directory
 * instead.
 *
 * That is the whole difference.  openat(AT_FDCWD, path, ...) is open()
 * exactly.  Any other dirfd names a directory inode the walk starts from,
 * which is what makes a process's view of the tree immune to another
 * process renaming a parent directory underneath it.
 *
 * ── the shape ──────────────────────────────────────────────────────────
 *   1. resolve the starting inode from dirfd
 *   2. run the same body open() runs, from that inode
 *
 * Steps 2 is open()'s; this unit only does 1 and delegates.  Absent a
 * NOFOLLOW mode in 01_fsa's namei(), AT_SYMLINK_NOFOLLOW cannot be
 * honoured and is refused rather than silently ignored.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

/*
 * Resolve the directory a dirfd names.  AT_FDCWD means the cwd; anything
 * else must be an open descriptor on a directory.
 *
 * The returned inode is a BORROWED reference — it comes out of the file
 * table entry, which owns it.  Whether the caller must iput() is reported
 * through need_put, because the AT_FDCWD case takes a reference of its own
 * to keep the two paths symmetric.
 */
static InCoreInode *scfs_dirfd_inode(int dirfd, int *need_put)
{
    *need_put = 0;

    if (dirfd == SCFS_AT_FDCWD) {
        InCoreInode *cwd = scfs_cwd_get();
        if (!cwd) return (InCoreInode *)0;
        cwd->refcount++;            /* borrow: caller will iput it */
        *need_put = 1;
        return cwd;
    }

    scfs_file_t *f = scfs_getf(dirfd);
    if (!f) return (InCoreInode *)0;
    if (!SCFS_IS_DIR(f->f_inode->mode)) return (InCoreInode *)0;

    return f->f_inode;              /* borrowed from the file table entry */
}

int uiox_kix_scfs_openat(int dirfd, const char *path, int flags,
                         uint16_t perm)
{
    if (!path) return SCFS_EFAULT;

    /* AT_SYMLINK_NOFOLLOW has no implementation below — namei() follows
     * every component and InCoreInode holds no symlink target.  Refusing
     * is honest; ignoring it would hand back a followed inode the caller
     * believed it had not followed. */
    if (flags & SCFS_AT_SYMLINK_NOFOLLOW) return SCFS_ENOSYS;

    int need_put = 0;
    InCoreInode *base = scfs_dirfd_inode(dirfd, &need_put);
    if (!base) return SCFS_EBADF;

    /*
     * A path relative to a directory fd is a path namei() walks from that
     * inode.  01_fsa's namei() takes a starting inode, so a relative path
     * is passed through unchanged and namei starts where told.
     *
     * An ABSOLUTE path ignores dirfd entirely — POSIX says so, and Bach's
     * namei does the same by switching to the root inode on a leading '/'.
     */
    InCoreInode *ip = namei(path, base, 0u, 0u);     /* 01_fsa */

    if (need_put) iput(base);
    if (!ip) {
        if (flags & O_CREAT) return uiox_kix_scfs_creat(path, perm);
        return SCFS_ENOENT;
    }

    /* Everything from here is open()'s body.  Delegating keeps one
     * implementation of the access check, the truncation and the two
     * allocation steps, rather than a second copy that can drift. */
    iput(ip);                       /* drop the probe reference */

    /*
     * The delegation is a full re-resolve from the cwd.  For an absolute
     * path that is identical; for a path relative to a non-AT_FDCWD
     * directory fd it is right only while the cwd is that directory, so
     * the relative case is reported rather than guessed at.
     */
    if (!(path[0] == '/') && dirfd != SCFS_AT_FDCWD) return SCFS_ENOSYS;

    return uiox_kix_scfs_open(path, flags, perm);
}

/* ── the rest of the *at() family ─────────────────────────────────────
 * Each is a path-based call with dirfd ignored, which is correct for the
 * absolute-path case and the AT_FDCWD case — the two a single-process
 * kernel reaches.  A relative path against a real directory fd needs the
 * starting inode threaded through namei() at every call site, which is
 * the same work openat() above still owes.
 */

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
    /* AT_REMOVEDIR turns unlinkat into rmdir. */
    if (flags & 0x200) return uiox_kix_scfs_rmdir(path);
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
