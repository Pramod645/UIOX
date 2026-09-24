#include "uiox_kix_scfs_internal.h"

/*
 * Bach: "use a similar approach as chroot to changing the flag modes in
 * the inode instead of the owner numbers."
 *
 * ── the encoding this must respect ───────────────────────────────────
 * ialloc() writes  mode = (ftype << 12) | perm  and inode_type() reads
 * (mode >> 12) & 0xF.  So the type lives in bits 12..15 and the
 * permissions in bits 0..8.  Assigning the whole word would silently turn
 * every file into FT_FREE.
 */
int uiox_kix_scfs_chmod(const char *path, uint16_t perm)
{
    InCoreInode *ip;

    if (!path) return SCFS_EFAULT;

    ip = namei(path, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!ip) return SCFS_ENOENT;

    ip->mode = (uint16_t)((ip->mode & SCFS_S_IFMT) | (perm & 0777u));

    ip->flags |= IFLAG_CHANGED;
    iupdate(ip);

    iput(ip);
    return SCFS_OK;
}

/* fchmod — the fd-addressed form.  No iput: the entry holds the ref. */
int uiox_kix_scfs_fchmod(int fd, uint16_t perm)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    f->f_inode->mode   = (uint16_t)((f->f_inode->mode & SCFS_S_IFMT)
                                    | (perm & 0777u));
    f->f_inode->flags |= IFLAG_CHANGED;

    iupdate(f->f_inode);
    return SCFS_OK;
}
