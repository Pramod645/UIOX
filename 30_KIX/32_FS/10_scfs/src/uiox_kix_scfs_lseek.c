#include "uiox_kix_scfs_internal.h"

/*
 * Bach's Algorithm lseek.
 * The offset lives in the FILE TABLE entry, not the inode — which is why
 * dup() shares a position and a fresh open() starts at zero.
 * {
 *     get file table entry from user file descriptor;
 *     calculate the new offset from whence;
 *     if the result is negative, return error;
 *     store the new offset in the file table entry;
 *     return (new offset);
 * }
 */
int uiox_kix_scfs_lseek(int fd, int32_t offset, int whence)
{
    scfs_file_t *f = scfs_getf(fd);
    int32_t base;
    int32_t np;

    if (!f) return SCFS_EBADF;

    switch (whence) {
    case SEEK_SET: base = 0;                              break;
    case SEEK_CUR: base = (int32_t)f->f_offset;           break;
    case SEEK_END: base = (int32_t)f->f_inode->size;      break;
    default:       return SCFS_EINVAL;
    }

    np = base + offset;
    if (np < 0) return SCFS_EINVAL;

    f->f_offset = (uint32_t)np;
    return np;
}
