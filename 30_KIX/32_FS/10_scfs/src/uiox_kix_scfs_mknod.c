#include "uiox_kix_scfs_internal.h"

/*
 * Bach's Algorithm mknod.
 *
 * ── the correction this file carries ─────────────────────────────────
 * The first cut wrote ip->i_major and ip->i_minor.  Neither exists:
 * InCoreInode has no device-number fields and DiskInode does not either,
 * so Bach's step 7 has nowhere to write.  A CHAR or BLOCK node is
 * therefore REFUSED — the numbers that make it useful cannot be stored.
 * A FIFO needs no device numbers and is created normally.
 */
int uiox_kix_scfs_mknod(const char *path, uint16_t mode,
                        uint8_t major, uint8_t minor)
{
    InCoreInode *dir;
    InCoreInode *ip;
    uint16_t     type;
    char         parent[SCFS_PATH_MAX];
    const char  *name = (const char *)0;
    int          rc;

    (void)major; (void)minor;   /* no inode field to hold them */

    if (!path) return SCFS_EFAULT;

    type = (uint16_t)(mode & SCFS_S_IFMT);

    if (type == SCFS_S_IFCHR || type == SCFS_S_IFBLK) {
        return SCFS_ENOSYS;     /* cannot name a driver — see above */
    }
    if (type != SCFS_S_IFIFO) {
        return SCFS_EINVAL;     /* a file is open(O_CREAT)'s job */
    }

    rc = scfs_path_split(path, parent, sizeof(parent), &name);
    if (rc != SCFS_OK) return rc;

    dir = namei(parent, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!dir) return SCFS_ENOENT;
    if (!SCFS_IS_DIR(dir->mode)) { iput(dir); return SCFS_ENOTDIR; }
    if (!inode_access_ok(dir, scfs_uid_get(), scfs_gid_get(), 0, 1, 0)) {
        iput(dir);
        return SCFS_EACCES;
    }

    if (dir_lookup(dir, name) != 0u) { iput(dir); return SCFS_EEXIST; }

    ip = ialloc(FT_FIFO, scfs_apply_umask((uint16_t)(mode & 0777u)), 0u, 0u);
    if (!ip) { iput(dir); return SCFS_ENOSPC; }

    ip->nlink = 1;

    if (dir_add(dir, name, ip->ino) != 0) {
        iput(ip);
        iput(dir);
        return SCFS_EIO;
    }

    iput(dir);

    ip->size = 0;
    ip->flags |= IFLAG_CHANGED;
    iupdate(ip);
    iput(ip);
    return SCFS_OK;
}
