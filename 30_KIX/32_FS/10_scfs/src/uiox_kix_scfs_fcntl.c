#include "uiox_kix_scfs_internal.h"

/* Read the file table entry's flag word back out as open()-style bits. */
static int scfs_fcntl_getfl(const scfs_file_t *f)
{
    int fl;

    if ((f->f_flag & (FREAD | FWRITE)) == (FREAD | FWRITE)) fl = O_RDWR;
    else if (f->f_flag & FWRITE)                            fl = O_WRONLY;
    else                                                    fl = O_RDONLY;

    if (f->f_flag & FAPPEND)   fl |= O_APPEND;
    if (f->f_flag & FNONBLOCK) fl |= O_NONBLOCK;

    return fl;
}

/* Apply the mutable subset.  A flag the caller did not mention is LEFT
 * ALONE — F_SETFL is a read-modify-write, not a replacement.  The access
 * mode is fixed at open and cannot be switched. */
static void scfs_fcntl_setfl(scfs_file_t *f, int fl)
{
    if (fl & O_APPEND)   f->f_flag |= FAPPEND;
    else                 f->f_flag &= (uint16_t)~FAPPEND;

    if (fl & O_NONBLOCK) f->f_flag |= FNONBLOCK;
    else                 f->f_flag &= (uint16_t)~FNONBLOCK;
}

/* F_DUPFD / F_DUPFD_CLOEXEC — dup, but not below the argument. */
static int scfs_fcntl_dupfd(const scfs_file_t *f, int minfd)
{
    int fd;

    if (minfd < 0 || minfd >= (int)NOFILE) return SCFS_EINVAL;

    for (fd = minfd; fd < (int)NOFILE; fd++)
        if (!scfs_u->ufd_file[fd]) break;

    if (fd >= (int)NOFILE) return SCFS_EMFILE;

    ((scfs_file_t *)f)->f_count++;
    scfs_u->ufd_file[fd] = (scfs_file_t *)f;

    return fd;
}

int uiox_kix_scfs_fcntl(int fd, int cmd, int arg)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    switch (cmd) {

    case SCFS_F_DUPFD:
    case SCFS_F_DUPFD_CLOEXEC:
        /* The CLOEXEC bit is recorded nowhere: no exec() to act on it. */
        return scfs_fcntl_dupfd(f, arg);

    case SCFS_F_GETFD:
        /* No close-on-exec bit in the fd table, so none to report. */
        return 0;

    case SCFS_F_SETFD:
        return SCFS_OK;

    case SCFS_F_GETFL:
        return scfs_fcntl_getfl(f);

    case SCFS_F_SETFL:
        scfs_fcntl_setfl(f, arg);
        return SCFS_OK;

    case SCFS_F_GETOWN:
        return (int)f->f_owner;

    case SCFS_F_SETOWN:
        f->f_owner = (uint16_t)arg;
        return SCFS_OK;

    case SCFS_F_GETLK:
    case SCFS_F_SETLK:
    case SCFS_F_SETLKW:
        /* No lock table and no lock owner: a lock is held by a PROCESS,
         * and this layer has no process identity to name. */
        return SCFS_ENOSYS;

    default:
        return SCFS_EINVAL;
    }
}
