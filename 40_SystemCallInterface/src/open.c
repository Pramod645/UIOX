#include "../include/fs.h"
#include "../include/inode.h"
#include "../include/file.h"
#include <string.h>

/*
 * Algorithm open
 * input : file name, type of open, file permission
 * output: file descriptor
 */
int fs_open(const char *path, int flags, uint16_t mode)
{
    inode_t *ip;
    file_t  *fp;
    int      fd;
    int      rwmode = flags & O_RDWR ? (O_RDONLY | O_WRONLY)
                                     : flags & 3;

    if (flags & O_CREAT) {
        fd = fs_creat(path, mode);
        return fd;
    }

    /* Convert file name to inode (algorithm namei) */
    ip = namei(path);
    if (!ip)
        return FS_ENOENT;

    /* Permission check */
    if (rwmode == O_RDONLY && !iaccess(ip, 4)) goto eacces;
    if (rwmode == O_WRONLY && !iaccess(ip, 2)) goto eacces;
    if (rwmode == O_RDWR  && !iaccess(ip, 6)) goto eacces;

    /* Allocate file table entry */
    fp = falloc();
    if (!fp) { iput(ip); return FS_ENFILE; }

    /* Allocate user file descriptor */
    fd = ufalloc();
    if (fd < 0) { f_close(fp); iput(ip); return FS_ENFILE; }

    fp->f_inode  = ip;
    fp->f_offset = 0;
    fp->f_flag   = (uint16_t)rwmode;

    /* Truncate if O_TRUNC */
    if (flags & O_TRUNC) {
        itrunc(ip);
    }

    u.u_ofile.ufd_file[fd] = fp;
    iunlock(ip);              /* namei locked inode */
    return fd;

eacces:
    iput(ip);
    return FS_EACCES;
}
