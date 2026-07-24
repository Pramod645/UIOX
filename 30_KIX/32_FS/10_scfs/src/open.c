/*
 *  30_KIX/32_FS/10_scfs/src/open.c  — freestanding fix v1.1
 *    FIXED: ../../33_PCS path, fprintf(stderr,...)
 */
#include "../include/fs.h"
#include "../include/inode.h"
#include "../include/file.h"
#include "uiox_klibc.h"

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
    int      rwmode = (flags & O_RDWR) ? (O_RDONLY | O_WRONLY) : (flags & 3);

    if (flags & O_CREAT)
        return fs_creat(path, mode);

    ip = namei(path);
    if (!ip) return FS_ENOENT;

    if ((rwmode & O_RDONLY) && !iaccess(ip, 4)) goto eacces;
    if ((rwmode & O_WRONLY) && !iaccess(ip, 2)) goto eacces;

    /* No write on directory */
    if ((rwmode & O_WRONLY) && (ip->i_mode & IFMT) == IFDIR)
        goto eisdir;

    fp = falloc();
    if (!fp) { iput(ip); return FS_ENFILE; }

    fd = ufalloc();
    if (fd < 0) { f_close(fp); iput(ip); return FS_ENFILE; }

    fp->f_inode  = ip;
    fp->f_offset = (flags & O_APPEND) ? ip->i_size : 0;
    fp->f_flag   = (uint16_t)rwmode;
    fp->f_count  = 1;

    if (flags & O_TRUNC) itrunc(ip);

    u.u_ofile.ufd_file[fd] = fp;
    printf("[open] '%s' fd=%d flags=0x%x\n", path, fd, flags);
    return fd;

eacces: iput(ip); return FS_EACCES;
eisdir: iput(ip); return FS_EISDIR;
}
