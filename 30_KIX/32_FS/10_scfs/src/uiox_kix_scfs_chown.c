/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_chown.c
 *
 * SCFS — Algorithm chown.  Bach, The Design of the UNIX Operating System.
 *
 * ── what Bach says ─────────────────────────────────────────────────────
 * "To change the owner of a file, the kernel converts the file name to an
 *  inode using algorithm namei, ... and releases the inode via algorithm
 *  iput."
 *
 * The whole call is that — resolve, write two fields, write the inode
 * back, release:
 *
 *   input:  file name, new owner, new group
 *   output: none
 *   {
 *       get inode for the file name (algorithm namei);
 *       if (the inode cannot be resolved) return (error);
 *       set the owner and group fields in the in-core inode;
 *       write the inode to disk;
 *       release the inode (algorithm iput);
 *   }
 *
 * ── the privilege question ─────────────────────────────────────────────
 * Bach's kernel checks that the caller is the owner or the super user.
 * That check needs a credential, which 33_PCS owns — this layer has none.
 * The check is therefore not performed here, and the function is
 * documented as privileged rather than pretending to have tested it.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

int uiox_kix_scfs_chown(const char *path, uint16_t uid, uint16_t gid)
{
    if (!path) return SCFS_EFAULT;

    /* ── convert the file name to an inode (algorithm namei) ────────── */
    InCoreInode *ip = namei(path, scfs_cwd_get(), 0u, 0u);   /* 01_fsa */
    if (!ip) return SCFS_ENOENT;

    /* ── set the owner and group in the in-core inode ───────────────── */
    /* 01_fsa stores both as uint16_t, which is what this call takes. */
    ip->uid = uid;
    ip->gid = gid;

    /* Bach's iput() writes the inode back when it is marked changed,
     * but a direct iupdate() is clearer and does the same work now. */
    ip->flags |= IFLAG_CHANGED;
    iupdate(ip);                            /* 01_fsa */

    /* ── release the inode (algorithm iput) ─────────────────────────── */
    iput(ip);                               /* 01_fsa */

    return SCFS_OK;
}

/*
 * fchown() — the fd-addressed form.
 *
 * The descriptor already names the inode, so no path walk is needed —
 * this is the one form Bach predates.  The inode reference belongs to
 * the file table entry, so there is no iput here.
 */
int uiox_kix_scfs_fchown(int fd, uint16_t uid, uint16_t gid)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    f->f_inode->uid    = uid;
    f->f_inode->gid    = gid;
    f->f_inode->flags |= IFLAG_CHANGED;

    iupdate(f->f_inode);                    /* 01_fsa */

    return SCFS_OK;
}
