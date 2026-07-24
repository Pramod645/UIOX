/*
 *  30_KIX/32_FS/10_scfs/src/close.c  — freestanding fix v1.1
 *    FIXED: ../../33_PCS path, fprintf(stderr,...)
 */
#include "../include/fs.h"
#include "../include/file.h"
#include "uiox_klibc.h"

/*
 * Algorithm close
 * input : user file descriptor
 * output: 0 on success
 */
int fs_close(int fd)
{
    file_t *fp;
    if (fd < 0 || fd >= NOFILE) return FS_EBADF;
    fp = u.u_ofile.ufd_file[fd];
    if (!fp) return FS_EBADF;
    u.u_ofile.ufd_file[fd] = NULL;
    f_close(fp);
    printf("[close] fd=%d\n", fd);
    return FS_OK;
}
