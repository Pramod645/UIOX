/*
 *  30_KIX/32_FS/10_scfs/src/mount.c  — freestanding fix v1.1
 *    FIXED: ../../33_PCS path, fprintf(stderr,...), for (int i=...)
 */
#include "../include/fs.h"
#include "../include/inode.h"
#include "../include/mount.h"
#include "uiox_klibc.h"

mount_t mount_table[NMOUNT];

/*
 * Algorithm mount
 */
int fs_mount(const char *special, const char *dir, int flags)
{
    inode_t *spec_ip, *dir_ip, *root_ip;
    mount_t *mp;
    uint16_t dev;
    (void)flags;

    if (!special || !dir) return FS_ENOENT;
    if (u.u_uid != 0) return FS_EPERM;

    spec_ip = namei(special);
    if (!spec_ip) return FS_ENOENT;
    dev = (uint16_t)((spec_ip->i_major << 8) | spec_ip->i_minor);
    iput(spec_ip);

    dir_ip = namei(dir);
    if (!dir_ip) return FS_ENOENT;
    if ((dir_ip->i_mode & IFMT) != IFDIR) { iput(dir_ip); return FS_ENOTDIR; }

    mp = mount_alloc();
    if (!mp) { iput(dir_ip); return FS_EBUSY; }

    mp->m_dev   = dev;
    mp->m_bufp  = NULL;
    mp->m_inodp = dir_ip;
    dir_ip->i_flag |= IMOUNT;

    root_ip = iget(dev, 1);
    if (!root_ip) { mount_free(mp); iput(dir_ip); return FS_ENOENT; }
    mp->m_mount_root = root_ip;
    iput(root_ip);

    printf("[mount] '%s' on '%s' dev=%u\n", special, dir, dev);
    return FS_OK;
}

int fs_umount(const char *special)
{
    inode_t *spec_ip, *mnt_ip;
    uint16_t dev;
    mount_t *mp;

    if (!special) return FS_ENOENT;
    if (u.u_uid != 0) return FS_EPERM;

    spec_ip = namei(special);
    if (!spec_ip) return FS_ENOENT;
    dev = (uint16_t)((spec_ip->i_major << 8) | spec_ip->i_minor);
    iput(spec_ip);

    mp = getmount(dev);
    if (!mp) return FS_ENOENT;

    mnt_ip = mp->m_inodp;
    if (mnt_ip) {
        mnt_ip->i_flag &= (uint16_t)~IMOUNT;
        iput(mnt_ip);
    }
    mount_free(mp);
    printf("[umount] '%s' unmounted\n", special);
    return FS_OK;
}

mount_t *mount_alloc(void)
{
    int i;
    for (i = 0; i < NMOUNT; i++)
        if (mount_table[i].m_dev == 0) return &mount_table[i];
    return NULL;
}
void mount_free(mount_t *mp) { if (mp) memset(mp, 0, sizeof(mount_t)); }
mount_t *getmount(uint16_t dev)
{
    int i;
    for (i = 0; i < NMOUNT; i++)
        if (mount_table[i].m_dev == dev) return &mount_table[i];
    return NULL;
}
