#include "uiox_kix_scfs_internal.h"

/*
 * Bach: "To change the owner of a file, the kernel converts the file name
 * to an inode using algorithm namei, ... and releases the inode via
 * algorithm iput."
 *
 * The owner-or-super-user check needs a credential 33_PCS owns; this layer
 * has none, so it is not performed here and the function is documented as
 * privileged rather than pretending to have tested it.
 */
int uiox_kix_scfs_chown(const char *path, uint16_t uid, uint16_t gid)
{
    InCoreInode *ip;

    if (!path) return SCFS_EFAULT;

    ip = namei(path, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!ip) return SCFS_ENOENT;

    ip->uid = uid;
    ip->gid = gid;
    ip->flags |= IFLAG_CHANGED;
    iupdate(ip);

    iput(ip);
    return SCFS_OK;
}

/* fchown — the fd-addressed form.  No path walk; the inode reference
 * belongs to the file table entry, so there is no iput here. */
int uiox_kix_scfs_fchown(int fd, uint16_t uid, uint16_t gid)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    f->f_inode->uid    = uid;
    f->f_inode->gid    = gid;
    f->f_inode->flags |= IFLAG_CHANGED;

    iupdate(f->f_inode);
    return SCFS_OK;
}
