#include "uiox_kix_scfs_internal.h"

/*
 * Bach's Algorithm mkdir: identical to mknod except that the inode gets
 * the directory type and the caller creates the "." and ".." entries.
 *
 * ── the two link counts, which is the part that bites ────────────────
 * A directory starts at link count 2, not 1: its own "." and the entry
 * its PARENT holds.  The parent's link count also rises by one, because
 * the new directory's ".." points back.  Getting this wrong makes find
 * count a directory twice or never.
 *
 * ── order ────────────────────────────────────────────────────────────
 * The parent entry is written FIRST, then "." and "..".  Either order
 * leaves a crash window; this one leaves the child INVISIBLE rather than
 * BROKEN — an unreferenced inode is recoverable by fsck, a directory with
 * no "." is not.
 */
int uiox_kix_scfs_mkdir(const char *path, uint16_t perm)
{
    InCoreInode *dir;
    InCoreInode *ip;
    char         parent[SCFS_PATH_MAX];
    const char  *name = (const char *)0;
    int          rc;

    if (!path) return SCFS_EFAULT;

    rc = scfs_path_split(path, parent, sizeof(parent), &name);
    if (rc != SCFS_OK) return rc;

    dir = namei(parent, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!dir) return SCFS_ENOENT;
    if (!SCFS_IS_DIR(dir->mode)) { iput(dir); return SCFS_ENOTDIR; }
    if (!inode_access_ok(dir, scfs_uid_get(), scfs_gid_get(), 0, 1, 1)) {
        iput(dir);
        return SCFS_EACCES;
    }

    if (dir_lookup(dir, name) != 0u) { iput(dir); return SCFS_EEXIST; }

    ip = ialloc(FT_DIR, scfs_apply_umask(perm), 0u, 0u);
    if (!ip) { iput(dir); return SCFS_ENOSPC; }

    if (dir_add(dir, name, ip->ino) != 0) {
        iput(ip);
        iput(dir);
        return SCFS_EIO;
    }

    if (dir_add(ip, ".", ip->ino) != 0 ||
        dir_add(ip, "..", dir->ino) != 0) {
        /* roll the parent entry back — the child never became visible */
        dir_remove(dir, name);
        iput(ip);
        iput(dir);
        return SCFS_EIO;
    }

    /* ── the two link counts ───────────────────────────────────────── */
    ip->nlink = 2;          /* "." and the parent's entry */

    dir->nlink++;           /* the child's ".." points back at us */
    dir->flags |= (IFLAG_CHANGED | IFLAG_MODIFIED);
    iupdate(dir);

    ip->flags |= (IFLAG_CHANGED | IFLAG_MODIFIED);
    iupdate(ip);

    iput(dir);
    iput(ip);
    return SCFS_OK;
}

/*
 * rmdir — the reverse, and stricter than unlink.  Bach: a directory may be
 * removed only when it is EMPTY, and only by the super user or its owner.
 */
int uiox_kix_scfs_rmdir(const char *path)
{
    InCoreInode *ip;
    InCoreInode *dir;
    char         parent[SCFS_PATH_MAX];
    const char  *name = (const char *)0;
    int          rc;

    if (!path) return SCFS_EFAULT;

    rc = scfs_path_split(path, parent, sizeof(parent), &name);
    if (rc != SCFS_OK) return rc;

    if (name[0] == '.' && (name[1] == '\0' ||
        (name[1] == '.' && name[2] == '\0'))) {
        return SCFS_EINVAL;         /* never remove "." or ".." */
    }

    ip = namei(path, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!ip) return SCFS_ENOENT;

    if (!SCFS_IS_DIR(ip->mode)) { iput(ip); return SCFS_ENOTDIR; }
    if (!inode_access_ok(ip, scfs_uid_get(), scfs_gid_get(), 0, 1, 1)) {
        iput(ip);
        return SCFS_EACCES;
    }

    /* The emptiness test: nlink above 2 means it holds a subdirectory. */
    if (ip->nlink > 2u) { iput(ip); return SCFS_ENOTEMPTY; }

    dir = namei(parent, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!dir) { iput(ip); return SCFS_ENOENT; }

    if (dir_remove(dir, name) != 0) {
        iput(dir);
        iput(ip);
        return SCFS_EIO;
    }

    /* ── the link counts, mirrored ─────────────────────────────────── */
    if (dir->nlink > 0u) dir->nlink--;
    dir->flags |= (IFLAG_CHANGED | IFLAG_MODIFIED);
    iupdate(dir);

    fs_free_inode_blocks(ip);
    ip->nlink = 0u;
    ip->flags |= (IFLAG_CHANGED | IFLAG_MODIFIED);
    iupdate(ip);

    ip->locked = false;
    iput(dir);
    iput(ip);
    return SCFS_OK;
}
