#include "../include/fs.h"
#include "../include/inode.h"
#include "../include/file.h"
#include <string.h>
#include <libgen.h>

/*
 * Algorithm creat
 * input : file name, permission settings
 * output: file descriptor
 */
int fs_creat(const char *path, uint16_t mode)
{
    inode_t *ip;
    file_t  *fp;
    int      fd;
    int      existed = 0;

    /* Get inode for file name (algorithm namei) */
    ip = namei(path);

    if (ip) {
        /* File already exists */
        existed = 1;
        if (!iaccess(ip, 2)) {
            iput(ip);           /* release inode (algorithm iput) */
            return FS_EACCES;
        }
    } else {
        /* File does not exist — create it */
        /* Assign free inode from file system (algorithm ialloc) */

        /* Get parent directory inode */
        char path_copy[256];
        strncpy(path_copy, path, sizeof(path_copy) - 1);
        path_copy[sizeof(path_copy) - 1] = '\0';
        char *parent_path = dirname(path_copy);

        inode_t *parent_ip = namei(parent_path);
        if (!parent_ip) return FS_ENOENT;
        if (!iaccess(parent_ip, 2)) {
            iput(parent_ip);
            return FS_EACCES;
        }

        ip = ialloc(parent_ip->i_dev);
        if (!ip) { iput(parent_ip); return FS_ENFILE; }

        ip->i_mode  = IFREG | (mode & ~u.u_umask);
        ip->i_nlink = 1;
        ip->i_uid   = u.u_uid;
        ip->i_gid   = u.u_gid;
        ip->i_flag |= IUPD;

        /* Create new directory entry in parent directory */
        /* (real kernel: bread parent directory block, write dirent) */

        iput(parent_ip);
    }

    /* Allocate file table entry */
    fp = falloc();
    if (!fp) { iput(ip); return FS_ENFILE; }

    fd = ufalloc();
    if (fd < 0) { f_close(fp); iput(ip); return FS_ENFILE; }

    fp->f_inode  = ip;
    fp->f_offset = 0;
    fp->f_flag   = O_WRONLY;

    /* If file existed, free all its blocks (algorithm free) */
    if (existed)
        itrunc(ip);

    u.u_ofile.ufd_file[fd] = fp;
    iunlock(ip);
    return fd;
}
