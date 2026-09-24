#include "uiox_kix_scfs_internal.h"

/*
 * Bach's Algorithm dup.
 * The new descriptor points at the SAME file table entry; its reference
 * count rises.  No new inode reference — both share the offset.
 */
int uiox_kix_scfs_dup(int fd)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;
    return scfs_ufd_link(f);
}

int uiox_kix_scfs_dup2(int oldfd, int newfd)
{
    scfs_file_t *f;

    if (oldfd < 0 || oldfd >= (int)NOFILE) return SCFS_EBADF;
    if (newfd < 0 || newfd >= (int)NOFILE) return SCFS_EBADF;
    if (oldfd == newfd) return scfs_getf(oldfd) ? newfd : SCFS_EBADF;

    f = scfs_getf(oldfd);
    if (!f) return SCFS_EBADF;

    if (scfs_u->ufd_file[newfd]) uiox_kix_scfs_close(newfd);

    f->f_count++;
    scfs_u->ufd_file[newfd] = f;
    return newfd;
}

int uiox_kix_scfs_dup3(int oldfd, int newfd, int flags)
{
    if (oldfd == newfd) return SCFS_EINVAL;
    (void)flags;                 /* O_CLOEXEC — no exec() to act on */
    return uiox_kix_scfs_dup2(oldfd, newfd);
}
