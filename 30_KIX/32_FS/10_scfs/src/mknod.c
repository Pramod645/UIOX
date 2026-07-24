#include "../include/fs.h"
#include "../include/inode.h"
#include "../../33_PCS/include/uiox_klibc.h"  
/*
 * Algorithm mknod
 * input : node name, file type, permissions, major, minor
 * output: none (0 on success)
 *
 * Creates special files: named pipes, device files, directories.
 */
int fs_mknod(const char *path, uint16_t mode,
             uint8_t major, uint8_t minor_num)
{
    /* Only super user can create non-pipe nodes */
    int is_pipe = (mode & IFMT) == IFIFO;
    if (!is_pipe && u.u_uid != 0)
        return FS_EPERM;

    /* Get inode of parent directory (algorithm namei) */
    char path_copy[256];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    inode_t *parent_ip = namei(dirname(path_copy));
    if (!parent_ip) return FS_ENOENT;

    /* Check if new node already exists */
    inode_t *exist = namei(path);
    if (exist) {
        iput(exist);
        iput(parent_ip);       /* release parent inode (algorithm iput) */
        return FS_EEXIST;
    }

    /* Assign free inode from file system (algorithm ialloc) */
    inode_t *ip = ialloc(parent_ip->i_dev);
    if (!ip) {
        iput(parent_ip);
        return FS_ENFILE;
    }

    ip->i_mode  = mode & ~u.u_umask;
    ip->i_nlink = 1;
    ip->i_uid   = u.u_uid;
    ip->i_gid   = u.u_gid;
    ip->i_flag |= IUPD;

    /* Create new directory entry in parent directory */
    /* (real kernel: write dirent into parent dir blocks) */

    /* Release parent directory inode (algorithm iput) */
    iput(parent_ip);

    /* Write major/minor numbers into inode for device files */
    if ((mode & IFMT) == IFBLK || (mode & IFMT) == IFCHR) {
        ip->i_major = major;
        ip->i_minor = minor_num;
        ip->i_flag |= IUPD;
    }

    /* Release new node inode (algorithm iput) */
    iput(ip);
    return FS_OK;
}
