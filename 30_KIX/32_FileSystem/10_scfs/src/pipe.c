#include "../include/fs.h"
#include "../include/inode.h"
#include "../include/file.h"

#define PIPE_DEV    0   /* pipe device number */

/*
 * Algorithm pipe (unnamed)
 * input : none
 * output: read fd, write fd (via fd[2])
 */
int fs_pipe(int fd[2])
{
    /* Assign new inode from pipe device (algorithm ialloc) */
    inode_t *ip = ialloc(PIPE_DEV);
    if (!ip) return FS_ENFILE;

    ip->i_mode         = IFIFO;
    ip->i_flag        |= IUPD;
    ip->i_pipe_readers = 1;
    ip->i_pipe_writers = 1;

    /* Allocate file table entry for reading */
    file_t *fp_read = falloc();
    if (!fp_read) { iput(ip); return FS_ENFILE; }

    /* Allocate file table entry for writing */
    file_t *fp_write = falloc();
    if (!fp_write) { f_close(fp_read); iput(ip); return FS_ENFILE; }

    /* Initialize file table entries to point to new inode */
    fp_read->f_inode  = ip;
    fp_read->f_flag   = O_RDONLY;
    fp_read->f_offset = 0;

    fp_write->f_inode  = ip;
    fp_write->f_flag   = O_WRONLY;
    fp_write->f_offset = 0;

    /* Set inode reference count to 2 */
    ip->i_count = 2;

    /* Allocate user file descriptors */
    fd[0] = ufalloc();   /* read end */
    if (fd[0] < 0) goto err;

    fd[1] = ufalloc();   /* write end */
    if (fd[1] < 0) goto err;

    u.u_ofile.ufd_file[fd[0]] = fp_read;
    u.u_ofile.ufd_file[fd[1]] = fp_write;

    iunlock(ip);
    return FS_OK;

err:
    f_close(fp_read);
    f_close(fp_write);
    iput(ip);
    return FS_ENFILE;
}

/*
 * Algorithm dup
 * input : file descriptor
 * output: new file descriptor pointing to same file table entry
 */
int fs_dup(int fd)
{
    if (fd < 0 || fd >= NOFILE) return FS_EBADF;

    file_t *fp = u.u_ofile.ufd_file[fd];
    if (!fp) return FS_EBADF;

    int new_fd = ufalloc();
    if (new_fd < 0) return FS_ENFILE;

    /* Both descriptors point to same file table entry */
    fp->f_count++;
    u.u_ofile.ufd_file[new_fd] = fp;

    return new_fd;
}
