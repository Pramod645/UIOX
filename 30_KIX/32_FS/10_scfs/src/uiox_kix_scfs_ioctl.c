#include "uiox_kix_scfs_internal.h"

/*
 * Bach, Ch.10 §3.1: "The ioctl system call is used to set or get the
 * values of device-dependent characteristics.  Rather than have the
 * kernel know the characteristics of every device, the kernel routes the
 * call through the device's entry in the character or block device switch
 * table, and the DEVICE DRIVER performs the operation."
 *
 * ── the correction this file carries ─────────────────────────────────
 * The first cut read ip->i_major to index the switch table.  There is no
 * i_major field, so step 3 cannot be performed — there is no key to look
 * the driver up with.  Steps 1 and 2 ARE performed and are correct.
 *
 *   plain file or directory   -> ENOTTY (Bach's answer: not a device)
 *   CHAR or BLOCK special     -> ENOSYS (no major number to dispatch on)
 */
int uiox_kix_scfs_ioctl(int fd, uint32_t cmd, void *arg)
{
    scfs_file_t *f;
    uint16_t     type;

    (void)cmd; (void)arg;

    f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    /* ── step 2: is it a special file at all? ──────────────────────── */
    type = (uint16_t)(f->f_inode->mode & SCFS_S_IFMT);

    if (type != SCFS_S_IFCHR && type != SCFS_S_IFBLK) {
        /* This is a different answer from "this kernel has no such
         * call" — the call is understood, the descriptor is the wrong
         * kind. */
        return SCFS_ENOTTY;
    }

    /* ── step 3: the switch-table entry, keyed by major number ───────
     * There is no major number in the inode, so there is no key. */
    return SCFS_ENOSYS;
}

/*
 * flock — Bach has no flock.  His only lock is the inode lock, held for
 * one system call.  flock(2) is a lock held by an open file ACROSS calls,
 * which needs a lock owner on the file table entry and a global lock
 * table.  Neither exists.
 */
int uiox_kix_scfs_flock(int fd, int operation)
{
    scfs_file_t *f;
    int          op;

    f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    op = operation & ~SCFS_LOCK_NB;     /* LOCK_NB is a modifier */

    if (op == SCFS_LOCK_UN) {
        /* Releasing is always safe to honour: a caller that unlocks a
         * lock it never held loses nothing. */
        f->f_locked = 0u;
        return SCFS_OK;
    }

    if (op == SCFS_LOCK_SH || op == SCFS_LOCK_EX) {
        /* Taking one cannot be honoured without the table.  A caller
         * that believed it held an exclusive lock would skip the
         * synchronisation it wrote the flock for. */
        return SCFS_ENOSYS;
    }

    return SCFS_EINVAL;
}

int uiox_kix_scfs_close_range(unsigned int first, unsigned int last, int flags)
{
    unsigned int fd;

    if (first > last) return SCFS_EINVAL;
    if (first >= (unsigned int)NOFILE) return SCFS_EINVAL;
    if (last  >= (unsigned int)NOFILE) last = (unsigned int)NOFILE - 1u;

    /* The only flag is CLOSE_RANGE_UNSHARE, which matters when the fd
     * table is shared after fork.  There is no fork yet. */
    (void)flags;

    for (fd = first; fd <= last; fd++)
        if (scfs_u->ufd_file[fd]) uiox_kix_scfs_close((int)fd);

    return SCFS_OK;
}
