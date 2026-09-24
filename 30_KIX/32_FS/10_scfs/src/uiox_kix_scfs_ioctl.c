/*
 *  30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_ioctl.c
 *
 *  SCFS — ioctl, flock, close_range.  CORRECTED.
 *
 *  ── Bach, Ch.10 §3.1, on ioctl ───────────────────────────────────────
 *  "The ioctl system call is used to set or get the values of
 *   device-dependent characteristics.  Rather than have the kernel know
 *   the characteristics of every device, the kernel routes the call
 *   through the device's entry in the character or block device switch
 *   table, and the DEVICE DRIVER performs the operation."
 *
 *  Bach's Algorithm ioctl:
 *    1. get the file table entry from the descriptor
 *    2. determine whether the inode is a char or block special file
 *    3. find the driver's entry in the switch table (major number)
 *    4. call the driver's ioctl routine
 *    5. return the driver's result
 *
 *  ── the correction ───────────────────────────────────────────────────
 *  The first cut read ip->i_major to index the switch table.  There is no
 *  i_major field.  InCoreInode carries no device numbers and DiskInode
 *  does not either, so step 3 cannot be performed — there is no key to
 *  look the driver up with.
 *
 *  Steps 1 and 2 ARE performed and are correct.  Step 3 onwards cannot
 *  be: a special file on this filesystem cannot name its driver.
 *
 *  What the call reports:
 *    · a plain file or directory        → ENOTTY (Bach's answer: the
 *                                          descriptor is not a device)
 *    · a CHAR or BLOCK special file     → ENOSYS (no major number to
 *                                          dispatch on)
 *
 *  ── flock ─────────────────────────────────────────────────────────────
 *  Bach has no flock.  His only lock is the inode lock, held for one
 *  system call.  flock(2) is a lock held by an open file ACROSS calls,
 *  which needs a lock owner on the file table entry and a global lock
 *  table.  Neither exists.  LOCK_UN is honoured because releasing a lock
 *  never held costs a caller nothing; taking one is ENOSYS, because a
 *  caller that believed it held a lock would skip its own
 *  synchronisation.
 *
 *  v1.1: i_major removed; ENODEV path replaced by ENOSYS with a reason.
 */
#include "uiox_kix_scfs_internal.h"

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
        /* A plain file or a directory has no device behind it.  Bach's
         * kernel returns ENOTTY here and so does this one — the call is
         * understood, the descriptor is the wrong kind.  This is a
         * different answer from "this kernel has no such call". */
        return SCFS_ENOTTY;
    }

    /* ── step 3: the switch-table entry, keyed by major number ───────
     * There is no major number in the inode, so there is no key.  See
     * the header note. */
    return SCFS_ENOSYS;
}

/* ── flock ──────────────────────────────────────────────────────────── */
int uiox_kix_scfs_flock(int fd, int operation)
{
    scfs_file_t *f;
    int          op;

    f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    /* LOCK_NB is a modifier, not an operation. */
    op = operation & ~SCFS_LOCK_NB;

    if (op == SCFS_LOCK_UN) {
        /* Releasing is always safe to honour — see the header note. */
        f->f_locked = 0u;
        return SCFS_OK;
    }

    if (op == SCFS_LOCK_SH || op == SCFS_LOCK_EX) {
        /* Taking one cannot be honoured without a lock table. */
        return SCFS_ENOSYS;
    }

    return SCFS_EINVAL;
}

/* ── close_range ────────────────────────────────────────────────────── */
int uiox_kix_scfs_close_range(unsigned int first, unsigned int last, int flags)
{
    unsigned int fd;

    if (first > last) return SCFS_EINVAL;
    if (first >= (unsigned int)NOFILE) return SCFS_EINVAL;
    if (last  >= (unsigned int)NOFILE) last = (unsigned int)NOFILE - 1u;

    /* The only flag is CLOSE_RANGE_UNSHARE, which matters when the fd
     * table is shared after fork.  There is no fork yet, so the flag has
     * nothing to do and is accepted rather than refused. */
    (void)flags;

    for (fd = first; fd <= last; fd++)
        if (scfs_u->ufd_file[fd]) uiox_kix_scfs_close((int)fd);

    return SCFS_OK;
}

/* sys_* aliases — the consolidated syscalls.c owns these; keep one. */
int sys_ioctl(int fd, uint32_t cmd, void *arg)
{ return uiox_kix_scfs_ioctl(fd, cmd, arg); }
int sys_flock(int fd, int op)
{ return uiox_kix_scfs_flock(fd, op); }
