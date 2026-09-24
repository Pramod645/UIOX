/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_chmod.c
 *
 * SCFS — Algorithm chmod.  Bach, The Design of the UNIX Operating System.
 *
 * ── what Bach says ─────────────────────────────────────────────────────
 * "use a similar approach as chroot to changing the flag modes in the
 *  inode instead of the owner numbers."
 *
 * So: resolve the name with namei, change a field in the in-core inode,
 * write it back, release.  Identical in shape to chown — the only
 * difference is WHICH field moves, and that only the low nine bits of the
 * mode word are permission bits.  The high nibble carries the file type,
 * set when the inode was allocated, and chmod must not disturb it.
 *
 *   input:  file name, new permission mode
 *   output: none
 *   {
 *       get inode for the file name (algorithm namei);
 *       if (the inode cannot be resolved) return (error);
 *       replace the permission bits, preserving the file type;
 *       write the inode to disk;
 *       release the inode (algorithm iput);
 *   }
 *
 * ── the encoding this must respect ─────────────────────────────────────
 * 01_fsa's ialloc() writes  mode = (ftype << 12) | perm  and inode_type()
 * reads (mode >> 12) & 0xF.  So the type lives in bits 12..15 and the
 * permissions in bits 0..8.  A chmod that assigned the whole word would
 * silently turn every file into FT_FREE.
 *
 * As with chown, the owner-or-super-user check belongs to 33_PCS and is
 * not performed at this layer.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

int uiox_kix_scfs_chmod(const char *path, uint16_t perm)
{
    if (!path) return SCFS_EFAULT;

    /* ── convert the file name to an inode (algorithm namei) ────────── */
    InCoreInode *ip = namei(path, scfs_cwd_get(), 0u, 0u);   /* 01_fsa */
    if (!ip) return SCFS_ENOENT;

    /* ── replace the permission bits, preserving the file type ──────── */
    ip->mode = (uint16_t)((ip->mode & SCFS_S_IFMT) | (perm & 0777u));

    /* ── write the inode to disk ────────────────────────────────────── */
    ip->flags |= IFLAG_CHANGED;
    iupdate(ip);                            /* 01_fsa */

    /* ── release the inode (algorithm iput) ─────────────────────────── */
    iput(ip);                               /* 01_fsa */

    return SCFS_OK;
}

/*
 * fchmod() — the fd-addressed form.
 *
 * The descriptor already names the inode, so no path walk.  The inode
 * reference belongs to the file table entry, so no iput either.
 */
int uiox_kix_scfs_fchmod(int fd, uint16_t perm)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    f->f_inode->mode   = (uint16_t)((f->f_inode->mode & SCFS_S_IFMT)
                                    | (perm & 0777u));
    f->f_inode->flags |= IFLAG_CHANGED;

    iupdate(f->f_inode);                    /* 01_fsa */

    return SCFS_OK;
}
