#include "uiox_kix_scfs_internal.h"

/*
 * Bach's Algorithm close, verbatim.
 * input: user file descriptor
 * output: none
 * {
 *     if (the user file descriptor is not active) return (error);
 *     set the corresponding entry in the user file descriptor table to NULL;
 *     if (the file table entry reference count is greater than 1)
 *         decrement the reference count;
 *     else
 *     {
 *         free the file table entry;
 *         release the inode (algorithm iput);
 *     }
 * }
 */
int uiox_kix_scfs_close(int fd)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    scfs_u->ufd_file[fd] = (scfs_file_t *)0;

    if (f->f_count > 1u) {
        f->f_count--;
    } else {
        scfs_fclose_entry(f);
    }
    return SCFS_OK;
}
