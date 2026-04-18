#include "../include/fs.h"
#include "../include/inode.h"
#include "../include/mount.h"
#include "../include/buf.h"
#include <string.h>

mount_t mount_table[NMOUNT];

/*
 * Algorithm mount
 * input : block special file name, mount point directory name, options
 * output: none (0 on success)
 */
int fs_mount(const char *special, const char *dir, int flags)
{
    if (u.u_uid != 0)
        return FS_EPERM;

    /* Get inode for block special file (algorithm namei) */
    inode_t *spec_ip = namei(special);
    if (!spec_ip) return FS_ENOENT;

    /* Legality checks */
    if ((spec_ip->i_mode & IFMT) != IFBLK) {
        iput(spec_ip);
        return FS_EINVAL;
    }

    uint16_t dev = (uint16_t)((spec_ip->i_major << 8) |
                               spec_ip->i_minor);

    /* Get inode for mount point directory (algorithm namei) */
    inode_t *dir_ip = namei(dir);
    if (!dir_ip) { iput(spec_ip); return FS_ENOENT; }

    if ((dir_ip->i_mode & IFMT) != IFDIR) {
        iput(dir_ip);
        iput(spec_ip);
        return FS_ENOTDIR;
    }
    /* Reference count > 1 means directory is busy */
    if (dir_ip->i_count > 1) {
        iput(dir_ip);
        iput(spec_ip);
        return FS_EBUSY;
    }

    /* Find empty slot in mount table */
    mount_t *mp = mount_alloc();
    if (!mp) { iput(dir_ip); iput(spec_ip); return FS_ENFILE; }

    /* Invoke block device driver open routine (simulated) */

    /* Get free buffer from buffer cache; read super block */
    buf_t *bp = getblk(dev, 1);   /* super block at block 1 */
    if (!bp) { mount_free(mp); iput(dir_ip); iput(spec_ip); return FS_EINVAL; }

    /* Read super block into free buffer */
    buf_t *sb_buf = bread(dev, 1);

    /* Initialize super block fields */
    memcpy(&mp->m_sb, sb_buf ? sb_buf->b_data : bp->b_data,
           sizeof(super_block_t));
    if (sb_buf) brelse(sb_buf);
    brelse(bp);

    if (flags & MNT_RDONLY) {
        mp->m_sb.s_ronly = 1;
        mp->m_flags      = MNT_RDONLY;
    }

    mp->m_dev   = dev;
    mp->m_bufp  = NULL;
    mp->m_inodp = dir_ip;

    /* Get root inode of mounted device (algorithm iget) */
    inode_t *root_ip = iget(dev, 1);    /* root inode = 1 */
    if (!root_ip) { mount_free(mp); iput(dir_ip); iput(spec_ip); return FS_EINVAL; }

    mp->m_mount_root = root_ip;

    /* Mark inode of mounted-on directory as mount point */
    dir_ip->i_flag |= IMOUNT;
    iunlock(dir_ip);

    /* Release special file inode */
    iput(spec_ip);

    iunlock(root_ip);
    return FS_OK;
}

/*
 * Algorithm umount
 * input : special file name of file system to unmount
 * output: none (0 on success)
 */
int fs_umount(const char *special)
{
    if (u.u_uid != 0)
        return FS_EPERM;

    /* Get inode of special file (algorithm namei) */
    inode_t *spec_ip = namei(special);
    if (!spec_ip) return FS_ENOENT;

    /* Extract major/minor device numbers */
    uint16_t dev = (uint16_t)((spec_ip->i_major << 8) |
                               spec_ip->i_minor);

    /* Get mount table entry based on device number */
    mount_t *mp = getmount(dev);

    /* Release inode of special file (algorithm iput) */
    iput(spec_ip);

    if (!mp) return FS_EINVAL;

    /* Check that no files from this file system are still in use */
    for (int i = 0; i < NINODE; i++) {
        if (inode_table[i].i_dev == dev &&
            inode_table[i].i_count > 0) {
            return FS_EBUSY;
        }
    }

    /* Update super block, flush all dirty buffers */
    /* (real kernel: write super block, sync inodes) */

    /* Get root inode of mounted file system from mount table */
    inode_t *root_ip = mp->m_mount_root;
    ilock(root_ip);
    iput(root_ip);              /* release root inode */

    /* Invoke close routine for special device (simulated) */

    /* Invalidate buffers in pool from unmounted file system */
    for (int i = 0; i < NBUF; i++) {
        extern buf_t buf_pool[];
        if (buf_pool[i].b_dev == dev) {
            buf_pool[i].b_flags &= ~B_VALID;
        }
    }

    /* Get inode of mount point from mount table */
    inode_t *mnt_ip = mp->m_inodp;
    ilock(mnt_ip);

    /* Clear mount point flag */
    mnt_ip->i_flag &= ~IMOUNT;
    iput(mnt_ip);

    /* Free mount table slot */
    mount_free(mp);

    return FS_OK;
}

/* ── Mount table helpers ─────────────────────────────────────── */
mount_t *mount_alloc(void)
{
    for (int i = 0; i < NMOUNT; i++) {
        if (mount_table[i].m_dev == 0)
            return &mount_table[i];
    }
    return NULL;
}

void mount_free(mount_t *mp)
{
    if (mp) memset(mp, 0, sizeof(mount_t));
}

mount_t *getmount(uint16_t dev)
{
    for (int i = 0; i < NMOUNT; i++) {
        if (mount_table[i].m_dev == dev)
            return &mount_table[i];
    }
    return NULL;
}
