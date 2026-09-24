/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_mount.c
 *
 * SCFS — Algorithm mount.  Bach, The Design of the UNIX Operating System.
 *
 * ── Bach's algorithm, verbatim ─────────────────────────────────────────
 * inputs: file name of block special file, directory name of mount point,
 *         options (read only)
 * output: none
 * {
 *   if (not super user)
 *       return (error);
 *   get inode for block special file (algorithm namei);
 *   make legality checks;
 *   get inode for "mount on" directory name (algorithm namei);
 *   if (not directory, or reference count > 1)
 *   {
 *       release inode (algorithm iput);
 *       return (error);
 *   }
 *   find empty slot in mount table;
 *   invoke block device driver open routine;
 *   get free buffer from buffer cache;
 *   read super block into free buffer;
 *   initialize super block fields;
 *   get root inode of mount device (algorithm iget), save in mount table;
 *   mark inode of "mounted on" directory as mount point;
 *   release special file inode (algorithm);
 *   unlock inode of mount point directory;
 * }
 *
 * ── what this layer can and cannot do ──────────────────────────────────
 * The mount TABLE is SCFS's own (Bach names it as one of the syscall
 * layer's three structures), and the legality checks are here.  Two steps
 * belong below: reading the super block through the buffer cache, and
 * iget() on the mounted root.  01_fsa's buffer cache and inode cache are
 * addressed by block number, not by device, so a second device cannot be
 * read through them.  The slot is filled and the structure is correct;
 * the device half reports what is missing.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

int uiox_kix_scfs_mount(const char *dev, const char *dir, int flags)
{
    if (!dev || !dir) return SCFS_EFAULT;

    /* ── 1. the super-user check ────────────────────────────────────── */
    if (!scfs_is_super()) return SCFS_EPERM;

    /* ── 2. inode for the block special file ───────────────────────── */
    InCoreInode *spec = namei(dev, scfs_cwd_get(), 0u, 0u);   /* 01_fsa */
    if (!spec) return SCFS_ENOENT;

    /* ── 3. legality checks ────────────────────────────────────────── */
    /* Only a block special file may be mounted, and only one that is not
     * itself already a mount point. */
    if ((spec->mode & SCFS_S_IFMT) != SCFS_S_IFBLK) {
        iput(spec);
        return SCFS_ENOTDIR;
    }
    if (spec->flags & IMOUNT) {          /* already a mount point */
        iput(spec);
        return SCFS_EBUSY;
    }

    /* ── 4. inode for the "mount on" directory ─────────────────────── */
    InCoreInode *mp = namei(dir, scfs_cwd_get(), 0u, 0u);     /* 01_fsa */
    if (!mp) {
        iput(spec);
        return SCFS_ENOENT;
    }

    /* ── 5. not a directory, or in use ─────────────────────────────── */
    if (!SCFS_S_ISDIR(mp->mode)) {
        iput(mp);
        iput(spec);
        return SCFS_ENOTDIR;
    }
    /* Bach refuses a mount point with a reference count above 1: someone
     * has it open, so the mount would change what their path means. */
    if (mp->refcount > 1u) {
        iput(mp);
        iput(spec);
        return SCFS_EBUSY;
    }

    /* ── 6. find an empty slot in the mount table ──────────────────── */
    scfs_mount_t *m = scfs_mount_alloc();
    if (!m) {
        iput(mp);
        iput(spec);
        return SCFS_ENOSPC;
    }

    m->m_dev       = (uint16_t)((spec->i_major << 8) | spec->i_minor);
    m->m_mountpt   = mp;         /* holds the reference namei gave us */
    m->m_root      = (InCoreInode *)0;
    m->m_sb_buf    = (BufEntry *)0;
    m->m_rdonly    = (flags & 1) ? 1u : 0u;

    /* ── 7. mark the "mounted on" inode as a mount point ───────────── */
    mp->flags |= IMOUNT;
    mp->locked = false;          /* unlock the mount point directory */

    /* ── 8. release the special file's inode ───────────────────────── */
    iput(spec);

    /*
     * ── 9. the device half ──────────────────────────────────────────
     * Bach now: open the block device, bread() its super block,
     * initialize the fields, iget() the mounted root.
     *
     * 01_fsa's buffer cache and inode cache are keyed by block number
     * with no device field, so a second device cannot be read through
     * them.  The mount-table entry exists and the mount point is marked;
     * the super block and mounted root are what the layer below still
     * owes.
     */
    m->m_fstype[0] = '?';
    m->m_fstype[1] = '\0';

    return SCFS_ENOSYS;          /* no device path in 01_fsa yet */
}
