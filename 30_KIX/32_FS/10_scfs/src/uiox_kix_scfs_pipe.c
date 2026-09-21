/*
 *  30_KIX/32_FS/10_scfs/src/pipe.c  — freestanding fix v1.1
 *    FIXED: ../../33_PCS path, fprintf(stderr,...)
 */
#include "../include/fs.h"
#include "../include/inode.h"
#include "../include/file.h"
#include "uiox_klibc.h"

#define PIPE_DEV 0

/*
 * Algorithm pipe (unnamed)
 * input : none
 * output: read fd, write fd (via fd[2])
 */
int fs_pipe(int fd[2])
{
    inode_t *ip;
    file_t  *rfp, *wfp;
    int      rfd, wfd;

    ip = ialloc(PIPE_DEV);
    if (!ip) return FS_ENFILE;

    ip->i_mode = (uint16_t)(IFIFO | 0600);
    ip->i_size = 0;
    ip->i_pipe_readers = 1;
    ip->i_pipe_writers = 1;
    ip->i_flag |= IUPD;

    rfp = falloc(); if (!rfp) { iput(ip); return FS_ENFILE; }
    wfp = falloc(); if (!wfp) { f_close(rfp); iput(ip); return FS_ENFILE; }

    rfd = ufalloc(); if (rfd < 0) { f_close(rfp); f_close(wfp); iput(ip); return FS_ENFILE; }
    wfd = ufalloc(); if (wfd < 0) { u.u_ofile.ufd_file[rfd]=NULL; f_close(rfp); f_close(wfp); iput(ip); return FS_ENFILE; }

    rfp->f_inode = wfp->f_inode = ip;
    rfp->f_flag  = O_RDONLY;
    wfp->f_flag  = O_WRONLY;
    rfp->f_count = wfp->f_count = 1;

    u.u_ofile.ufd_file[rfd] = rfp;
    u.u_ofile.ufd_file[wfd] = wfp;

    fd[0] = rfd;
    fd[1] = wfd;
    printf("[pipe] rfd=%d wfd=%d (ino=%u)\n", rfd, wfd, ip->i_number);
    return FS_OK;
}
