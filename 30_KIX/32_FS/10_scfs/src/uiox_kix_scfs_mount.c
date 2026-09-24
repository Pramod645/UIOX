#include "uiox_kix_scfs_internal.h"

/*
 * Bach's Algorithm mount, verbatim.
 * {
 *   if (not super user) return (error);
 *   get inode for block special file (algorithm namei);
 *   make legality checks;
 *   get inode for "mount on" directory name (algorithm namei);
 *   if (not directory, or reference count > 1)
 *   { release inode (algorithm iput); return (error); }
 *   find empty slot in mount table;
 *   invoke block device driver open routine;
 *   get free buffer from buffer cache;
 *   read super block into free buffer;
 *   initialize super block fields;
 *   get root inode of mount device (algorithm iget), save in mount table;
 *   mark inode of "mounted on" directory as mount point;
 *   release special file inode;
 *   unlock inode of mount point directory;
 * }
 *
 * The mount TABLE is SCFS's own.  Two steps belong below: reading the
 * super block through the buffer cache, and iget() on the mounted root.
 * 01_fsa's buffer cache is addressed by block number with no device
 * field, so a second device cannot be read through it.  The slot is
 * filled and the structure is correct; the device half reports the gap.
 */
int uiox_kix_scfs_mount(const char *dev, const char *dir, int flags)
{
    InCoreInode *spec;
    InCoreInode *mp;
    scfs_mount_t *m;

    if (!dev || !dir) return SCFS_EFAULT;

    if (!scfs_is_super()) return SCFS_EPERM;

    spec = namei(dev, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!spec) return SCFS_ENOENT;

    if (!SCFS_IS_BLK(spec->mode)) {
        iput(spec);
        return SCFS_ENOTBLK;
    }
    if (spec->flags & IMOUNT) {
        iput(spec);
        return SCFS_EBUSY;
    }

    mp = namei(dir, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!mp) { iput(spec); return SCFS_ENOENT; }

    if (!SCFS_IS_DIR(mp->mode)) {
        iput(mp); iput(spec);
        return SCFS_ENOTDIR;
    }
    if (mp->refcount > 1) {
        iput(mp); iput(spec);
        return SCFS_EBUSY;
    }

    m = scfs_mount_alloc();
    if (!m) { iput(mp); iput(spec); return SCFS_ENOSPC; }

    m->m_dev     = 0u;          /* no device field anywhere — see header */
    m->m_mountpt = mp;
    m->m_root    = (InCoreInode *)0;
    m->m_sb_buf  = (BufEntry *)0;
    m->m_rdonly  = (flags & 1) ? 1u : 0u;

    mp->flags |= IMOUNT;
    mp->locked = false;

    iput(spec);

    m->m_fstype[0] = '?';
    m->m_fstype[1] = '\0';

    return SCFS_ENOSYS;     /* no device path in 01_fsa yet */
}
