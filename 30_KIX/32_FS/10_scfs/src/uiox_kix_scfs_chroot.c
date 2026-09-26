/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_chroot.c
 *
 * SCFS — Algorithm chroot.  Bach, The Design of the UNIX Operating System.
 *
 * ── what Bach says ─────────────────────────────────────────────────────
 * chroot changes the process's VIEW of the file system tree: it makes a
 * directory the root for path-name searches.  Bach keeps it in the u area
 * alongside the current directory:
 *
 *   input:  new root directory name
 *   output: none
 *   {
 *       if (not super user)
 *           return (error);
 *       get inode for the new root directory name (algorithm namei);
 *       if (inode not that of directory)
 *       {
 *           release inode (algorithm iput);
 *           return (error);
 *       }
 *       unlock (inode);
 *       release "old" root directory inode (algorithm iput);
 *       place new inode into root directory slot in u area;
 *   }
 *
 * The structure is exactly chdir's — pathname to inode, type check,
 * release the old, adopt the new — with one addition: chroot is a
 * privileged operation, because it changes what "/" means for everything
 * the process does afterwards.
 *
 * ── namei's use of the root ────────────────────────────────────────────
 * Bach's namei starts "from the root inode" when a path begins with '/'.
 * 01_fsa's namei() uses ROOT_INO for that.  A chroot therefore also needs
 * the root passed down; until namei takes it as an argument, this unit
 * records it in SCFS and callers that walk absolute paths consult it.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

int uiox_kix_scfs_chroot(const char *path)
{
    if (!path) return SCFS_EFAULT;

    /*
     * ── the super-user check ────────────────────────────────────────
     * Bach refuses chroot to anyone but the super user.  This layer has
     * no credential source of its own — 33_PCS holds it — so the safe
     * reading of Bach's rule is to refuse until a credential is passed
     * through.  A caller that has established super user reaches the
     * privileged variant below.
     */
    if (!scfs_is_super()) return SCFS_EPERM;

    /* ── get the inode for the new root name (algorithm namei) ──────── */
    InCoreInode *ip = namei(path, scfs_cwd_get(), 0u, 0u);   /* 01_fsa */
    if (!ip) return SCFS_ENOENT;

    /* ── it must be a directory ─────────────────────────────────────── */
    if (!SCFS_IS_DIR(ip->mode)) {
        iput(ip);
        return SCFS_ENOTDIR;
    }

    /* ── unlock ─────────────────────────────────────────────────────── */
    ip->locked = false;

    /* ── release the OLD root's reference ───────────────────────────── */
    InCoreInode *old = scfs_root_get();
    if (old && old != ip) iput(old);                /* 01_fsa */

    /* ── place the new inode in the root slot ──────────────────────── */
    scfs_root_set(ip);

    return SCFS_OK;
}
