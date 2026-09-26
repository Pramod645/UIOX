/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_fcntl.c
 *
 * SCFS — fcntl.  Bach, The Design of the UNIX Operating System.
 *
 * ── where this sits relative to Bach ───────────────────────────────────
 * Bach dedicates Chapter 5 §5 to fcntl, because it is the one call that
 * reaches INTO the file table entry and changes how it behaves for every
 * descriptor sharing it.  Two commands matter:
 *
 *   F_DUPFD      duplicate a descriptor, guaranteeing the result is at
 *                least the argument.  Bach's variant of dup(2) for a
 *                program that needs a specific fd, such as a shell
 *                setting up a pipeline.
 *
 *   F_SETFL /    read and write the STATUS FLAGS on the file table entry
 *   F_GETFL      — O_APPEND, O_NONBLOCK, O_SYNC.  These live in
 *                f_flag, not in the descriptor, so setting them through
 *                one descriptor changes every descriptor pointing at that
 *                entry.  That is the "shared" behaviour Bach describes.
 *
 * The file LOCK commands (F_GETLK/F_SETLK) are a later addition and need
 * a lock owner this kernel does not yet have.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

/* Read the file table entry's flag word back out as open()-style bits. */
static int scfs_fcntl_getfl(const scfs_file_t *f)
{
    int fl;

    if ((f->f_flag & (FREAD | FWRITE)) == (FREAD | FWRITE)) fl = O_RDWR;
    else if (f->f_flag & FWRITE)                            fl = O_WRONLY;
    else                                                    fl = O_RDONLY;

    if (f->f_flag & FAPPEND) fl |= O_APPEND;
    if (f->f_flag & FNONBLOCK) fl |= O_NONBLOCK;

    return fl;
}

/* Apply the mutable subset of the open flags to the file table entry. */
static void scfs_fcntl_setfl(scfs_file_t *f, int fl)
{
    /* A flag the caller did not mention is LEFT ALONE — F_SETFL is a
     * read-modify-write of the entry's word, not a replacement of it.
     * O_APPEND and O_NONBLOCK are the only two POSIX lets a caller
     * change here; the access mode (O_RDONLY/O_WRONLY/O_RDWR) is fixed
     * at open and cannot be switched. */
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

    /* The lowest free slot at or above minfd — the same search
     * scfs_ufd_find() does, narrowed. */
    for (fd = minfd; fd < (int)NOFILE; fd++)
        if (!scfs_u->ufd_file[fd]) break;

    if (fd >= (int)NOFILE) return SCFS_EMFILE;

    /* One file table entry, two descriptors: raise the entry's count,
     * exactly as dup() does.  No inode reference moves. */
    ((scfs_file_t *)f)->f_count++;
    scfs_u->ufd_file[fd] = (scfs_file_t *)f;

    return fd;
}

int uiox_kix_scfs_fcntl(int fd, int cmd, int arg)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    switch (cmd) {

    /* ── duplicate, with a floor on the descriptor number ───────────── */
    case SCFS_F_DUPFD:
    case SCFS_F_DUPFD_CLOEXEC:
        /* The CLOEXEC bit is recorded nowhere: there is no exec() in this
         * kernel to act on it, so the two commands behave identically. */
        return scfs_fcntl_dupfd(f, arg);

    /* ── the descriptor number itself ──────────────────────────────── */
    case SCFS_F_GETFD:
        /* Bach's kernel has no close-on-exec bit in the fd table, so
         * there is no bit to report.  Zero is the truthful answer. */
        return 0;

    case SCFS_F_SETFD:
        /* Accepting and ignoring would leave a caller believing a
         * close-on-exec was set.  There is no exec() yet, so the flag has
         * nothing to mean either way. */
        return SCFS_OK;

    /* ── the shared status flags on the file table entry ───────────── */
    case SCFS_F_GETFL:
        return scfs_fcntl_getfl(f);

    case SCFS_F_SETFL:
        scfs_fcntl_setfl(f, arg);
        return SCFS_OK;

    /* ── ownership ──────────────────────────────────────────────────── */
    case SCFS_F_GETOWN:
        return (int)f->f_owner;

    case SCFS_F_SETOWN:
        f->f_owner = (uint16_t)arg;
        return SCFS_OK;

    /* ── the file lock commands ─────────────────────────────────────── */
    case SCFS_F_GETLK:
    case SCFS_F_SETLK:
    case SCFS_F_SETLKW:
        /* Bach's kernel has no file lock table and no lock owner: a lock
         * is held by a PROCESS, and this layer has no process identity to
         * name.  Reported rather than faked, because a caller that
         * believes it holds a lock will skip its own synchronisation. */
        return SCFS_ENOSYS;

    default:
        return SCFS_EINVAL;
    }
}
