/*
 *  30_KIX/32FileSystem/10_scfs/src/uiox_kix_scfs_ioctl.c
 *
 *  SCFS — ioctl, flock, close_range.
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
 *  ── the two corrections ──────────────────────────────────────────────
 *  v1.1 removed ip->i_major: the inode had no such field, so step 3 could
 *  not run and every special file answered ENOSYS.
 *
 *  v1.2 supplies the key that replaces it.  The inode now carries
 *
 *      uint16_t idev_class;   which subsystem owns the node
 *      uint16_t idev_unit;    which instance of that class
 *
 *  which are the same (cls, unit) pair uiox_dev_dev_t uses — see
 *  34_DSS/include/uiox_devclass.h, where a class value packs its family
 *  into the high nibble and the class into the low 12 bits.  So step 3 is
 *  a lookup in 34_DSS's binding registry rather than an index into a
 *  switch table, and no major number has to be allocated by anyone.
 *
 *  Steps 1, 2 and 5 are Bach's and are unchanged.  Step 4 now calls the
 *  ioctl slot on the driver's HEAD vtable (uiox_dev_ops_t), which every
 *  family's ops struct begins with — so this file reads it at one offset
 *  whatever the class is, and never learns which family it is talking to.
 *
 *  ── the four answers, kept distinct ──────────────────────────────────
 *    bad descriptor                  EBADF   no such descriptor
 *    plain file / directory          ENOTTY  understood; wrong kind of fd
 *    special, no device bound        ENODEV  the node names a subsystem
 *                                            that is not present
 *    bound, but no ioctl slot        ENOTTY  right kind of fd, no
 *                                            device-dependent call
 *  ENOSYS is no longer reachable from here: the call exists, so the only
 *  question is whether this descriptor can carry it.
 *
 *  ── flock ────────────────────────────────────────────────────────────
 *  Bach has no flock.  His only lock is the inode lock, held for one
 *  system call.  flock(2) is a lock held by an open file ACROSS calls,
 *  which needs a lock owner on the file table entry and a global lock
 *  table.  Neither exists.  LOCK_UN is honoured because releasing a lock
 *  never held costs a caller nothing; taking one is ENOSYS, because a
 *  caller that believed it held a lock would skip its own
 *  synchronisation.  The caller that ignores the return value is the
 *  hazard here, not the call.
 *
 *  v1.1: i_major removed; ENODEV path replaced by ENOSYS with a reason.
 *  v1.2: idev_class/idev_unit + the binding registry restore step 3.
 */
#include "uiox_kix_scfs_internal.h"

/* The device vocabulary: uiox_dev_dev_t, uiox_dev_ops_t, uiox_devclass_t. */
#include "uiox_devclass.h"

/* The binding registry: (cls, unit) -> descriptor.  Declared in
 * uiox_devclass.h section 9, defined in 34_DSS/src/uiox_devtable.c. */
/* uiox_dev_lookup() is declared by the include above — no extra header. */

int uiox_kix_scfs_ioctl(int fd, uint32_t cmd, void *arg)
{
    scfs_file_t          *f;
    uiox_dev_dev_t       *dev;
    const uiox_dev_ops_t *head;
    uint16_t              type;

    /* ── step 1: the file table entry ──────────────────────────────── */
    f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    /* ── step 2: is it a special file at all? ──────────────────────── */
    type = (uint16_t)(f->f_inode->mode & SCFS_S_IFMT);

    if (type != SCFS_S_IFCHR && type != SCFS_S_IFBLK) {
        /* A plain file or a directory has no device behind it.  The call
         * is understood, the descriptor is the wrong kind — which is a
         * different answer from "the node names nothing bound". */
        return SCFS_ENOTTY;
    }

    /* ── step 3: the subsystem, keyed by class + unit ───────────────
     * The filesystem does not interpret either field.  It hands both to
     * the registry and gets a descriptor or nothing, which is what keeps
     * this layer from needing to know how many device classes exist. */
    dev = uiox_dev_lookup((uiox_devclass_t)f->f_inode->idev_class,
                          f->f_inode->idev_unit);
    if (!dev)
        return SCFS_ENODEV;   /* named, but nothing is bound to that name */

    /* ── step 4: the bound driver's own routine ────────────────────── */
    head = (const uiox_dev_ops_t *)dev->ops;
    if (!head || !head->ioctl)
        return SCFS_ENOTTY;   /* bound, but no device-dependent call */

    /* cmd and arg are passed through uninterpreted: the command number
     * space belongs to the driver and nothing above it validates them. */
    return head->ioctl(dev, cmd, arg);   /* ── step 5 */
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
