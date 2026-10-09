/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_read.c
 *
 * SCFS — Algorithm read.  Bach, The Design of the UNIX Operating System.
 *
 * ── Bach's algorithm, verbatim ─────────────────────────────────────────
 * {
 *   get file table entry from user file descriptor;
 *   check file accessibility;
 *   set parameters in u area for user address, byte count, I/O to user;
 *   get inode from file table;
 *   lock inode;
 *   set byte offset in u area from file table offset;
 *   while (count not satisfied)
 *   {
 *       convert file offset to disk block (algorithm bmap);
 *       calculate offset into block, number of bytes to read;
 *       if (number of bytes to read is 0)     // try to read end of file
 *           break;                            // out of loop
 *       read block (algorithm breada if with read ahead, bread otherwise);
 *       copy data from system buffer to user address;
 *       update u area fields for file byte offset, read count, address;
 *       release buffer;                       // locked in bread
 *   }
 *   unlock inode;
 *   update file table offset for next read;
 *   return (total number of bytes read);
 * }
 *
 * ── layering ───────────────────────────────────────────────────────────
 * The loop body is one call to 01_fsa's readi(), which is itself this
 * walk: bmap() for the block, bread() to cache it, memcpy, brelse().  The
 * u-area fields Bach names are the locals below, since 01_fsa takes them
 * as arguments rather than through a global u area.
 *
 * ── pread / readv ──────────────────────────────────────────────────────
 * Both are in this file: pread is the same walk at a private offset, and
 * readv repeats it per iovec segment.
 *
 * ── v1.1: the device branch ────────────────────────────────────────────
 * Bach's read is a FILE read — blocks, bmap, bread.  A character special
 * file bound to a device has no blocks to read: the device PRODUCES
 * records, and read() is how it hands them up.  That is the second half of
 * the pair whose first half is ioctl; ioctl carries commands down, read
 * carries records up.
 *
 * The branch sits after the inode is resolved and before the file walk.  A
 * char node bound to a driver never reaches readi().
 *
 * Where the event functions live — this is the part that is easy to get
 * wrong.  uiox_devclass.h §6 says a class's ops struct STARTS with the
 * head, so the layout is:
 *
 *     +---------------------------+
 *     | uiox_dev_ops_t    head    |  probe / init / deinit / ioctl
 *     +---------------------------+  <- offset 0; ioctl reads here
 *     | uiox_event_ops_t  ev      |  event_pending / event_read / isr
 *     +---------------------------+  <- offset sizeof(uiox_dev_ops_t)
 *
 * event_read is NOT a member of uiox_dev_ops_t.  It is the second member,
 * and only EVENT and STREAM families have one — a REG class (als, chg, …)
 * has no second member at all, which is why the family predicate guards
 * the offset before it is computed.
 *
 * @version 1.1.0  @date 2026-10-09
 */
#include "uiox_kix_scfs_internal.h"

/* The device vocabulary: uiox_dev_dev_t, uiox_dev_ops_t, uiox_event_ops_t,
 * uiox_devclass_t, UIOX_DEVCLASS_FAMILY(), UIOX_DEVFAM_EVENT — and the
 * declaration of uiox_dev_lookup(). */
#include "uiox_devclass.h"

/*
 * read() — Algorithm read.
 *
 * The offset SCFS keeps is uint32 because 01_fsa's InCoreInode.size is;
 * a file that large cannot be addressed by bmap() anyway (the block map
 * tops out at the triple-indirect range).
 */
int uiox_kix_scfs_read(int fd, char *buf, uint32_t count)
{
    /* ── 1. get the FILE TABLE entry from the fd ────────────────────── */
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    /* ── 2. check file accessibility ────────────────────────────────── */
    if (!(f->f_flag & FREAD)) return SCFS_EACCES;

    /* ── 3. Bach sets u-area params here (user address, byte count,
     *       I/O-to-user).  SCFS needs no u area: the buffer and the
     *       count are arguments, and the copy never crosses a privilege
     *       boundary inside this layer — the syscall stub has already
     *       validated the user pointer. ─────────────────────────────── */
    if (count == 0u) return 0;
    if (!buf)        return SCFS_EFAULT;

    /* ── 4. the inode ───────────────────────────────────────────────── */
    InCoreInode *ip = f->f_inode;
    if (!ip) return SCFS_EBADF;

    /* ══ 4a. THE DEVICE BRANCH ═══════════════════════════════════════
     * A character special file with a bound device is not a file: there
     * are no blocks, so no bmap and no offset to walk.  The device
     * produced records and read() is how they leave the kernel.
     *
     * Keyed on the same (cls, unit) pair ioctl uses, through the same
     * registry — so a node that can be commanded can also be read.  A
     * node bound to nothing falls through to the file walk, which finds
     * size 0 and returns 0 (EOF) — the same answer a zero-length regular
     * file gives, and the honest one for a node with no driver.
     * ═══════════════════════════════════════════════════════════════ */
    uint16_t dtype = (uint16_t)(ip->mode & SCFS_S_IFMT);

    if (dtype == SCFS_S_IFCHR) {
        uiox_dev_dev_t *dev = uiox_dev_lookup((uiox_devclass_t)ip->idev_class,
                                              ip->idev_unit);
        if (dev) {
            const uiox_dev_ops_t *head = (const uiox_dev_ops_t *)dev->ops;

            /* The event pair is the SECOND member of the class's ops
             * struct — the head comes first (§6), so it sits at
             * sizeof(uiox_dev_ops_t).  Guard the family BEFORE computing
             * the offset: for a REG class the memory after the head
             * belongs to something else, and reading it as a vtable would
             * call through a pointer that was never meant to be one. */
            if (!head ||
                UIOX_DEVCLASS_FAMILY((uiox_devclass_t)ip->idev_class)
                    != UIOX_DEVFAM_EVENT)
                return SCFS_ENOTTY;

            const uiox_event_ops_t *ev =
                (const uiox_event_ops_t *)((const char *)dev->ops
                                           + sizeof(uiox_dev_ops_t));

            /* A class in the EVENT family with no reader is a
             * producer-only device that was bound without one — the
             * descriptor is the wrong kind for this call, which is what
             * ENOTTY says. */
            if (!ev->event_read) return SCFS_ENOTTY;

            /* event_pending() is advisory: a non-blocking caller skips it
             * and still gets EAGAIN from the read below, rather than a
             * zero that would look like EOF. */
            if (!ev->event_pending || !ev->event_pending(dev))
                return SCFS_EAGAIN;

            /* One record per call.  The adapter returns sizeof(record);
             * batching would make a short return ambiguous with EOF,
             * which the ops contract forbids (see uiox_event_ops_t). */
            return ev->event_read(dev, buf, (uint16_t)count);
        }
        /* Not bound: fall through.  See the note above. */
    }

    /* ── 5. lock the inode ──────────────────────────────────────────── */
    ip->locked = true;                       /* 01_fsa's lock flag */

    /* ── 6. byte offset comes from the FILE TABLE entry ─────────────── */
    uint32_t offset = f->f_offset;

    /*
     * ── 7. the read loop ────────────────────────────────────────────
     * Bach's loop body is bmap → bread → copy → brelse, repeated until
     * the count is satisfied or the file ends.  01_fsa's readi() is that
     * loop, and it clamps to ip->size so it can never run past the end
     * into a neighbouring file's blocks.
     */
    int32_t n = readi(ip, buf, count, &offset);

    /* ── 8. unlock the inode ────────────────────────────────────────── */
    ip->locked = false;

    if (n < 0) return (int)n;

    /* ── 9. update the FILE TABLE offset for the next read ──────────── */
    f->f_offset = offset;

    return (int)n;
}

/*
 * pread() — the same walk at a private offset.
 *
 * Bach expresses pread as lseek + read + lseek-back.  Two descriptors
 * from dup() share ONE file table entry, so a positioned read must not
 * move f_offset — hence readi_at(), which takes the offset by value.
 *
 * A device has no offset, so pread on a char node is refused rather than
 * silently served from a position that means nothing.
 */
int uiox_kix_scfs_pread(int fd, char *buf, uint32_t count, uint32_t off)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;
    if (!(f->f_flag & FREAD)) return SCFS_EACCES;

    if (count == 0u) return 0;
    if (!buf)        return SCFS_EFAULT;

    InCoreInode *ip = f->f_inode;
    if (!ip) return SCFS_EBADF;

    /* A device record has no file offset to position at. */
    if ((uint16_t)(ip->mode & SCFS_S_IFMT) == SCFS_S_IFCHR)
        return SCFS_ESPIPE;

    uint32_t moved = 0u;
    int32_t  n = readi_at(ip, buf, count, off, &moved);

    /* f->f_offset deliberately untouched. */
    return (int)n;
}

/*
 * readv() — scatter read.
 *
 * Each iovec segment is an independent read at the running offset.  A
 * short segment means EOF or a hole, which ends the whole call — that is
 * readv's contract, and why the loop breaks rather than erroring.
 */
/*
 * ── the iovec layout, kept LOCAL ──────────────────────────────────────
 * The public prototype is opaque on purpose:
 *
 *     int32_t uiox_kix_scfs_readv(int fd, const void *iov, int iovcnt);
 *
 * so this file must not name a struct type in its signature — an earlier
 * revision wrote `const scfs_iovec_t *`, and that type is declared
 * nowhere in the tree:
 *
 *     read.c:132: unknown type name 'scfs_iovec_t'
 *     read.c:132: conflicting types for 'uiox_kix_scfs_readv';
 *                 have 'int(int, const int *, int)'
 *     uiox_kix_scfs.h:353: previous declaration ... 'int32_t(int,
 *                 const void *, int)'
 *
 * The five member-access errors (iov_base / iov_len) were a consequence:
 * an opaque pointer cannot be indexed.  Casting once, here, is what lets
 * the walk address the segments while the public type stays opaque — the
 * syscall stub owns the ABI, this function owns the walk.
 *
 * The layout matches the kernel's: a pointer then a length, in that
 * order, both pointer-sized / 32-bit respectively.
 * ───────────────────────────────────────────────────────────────────── */
typedef struct {
    void    *iov_base;
    uint32_t iov_len;
} scfs_iovec_local_t;

/* int32_t, not int — the header declares int32_t, and a differing return
 * type is its own compile error. */
int32_t uiox_kix_scfs_readv(int fd, const void *iov, int iovcnt)
{
    const scfs_iovec_local_t *vec = (const scfs_iovec_local_t *)iov;

    if (!vec || iovcnt <= 0 || iovcnt > 1024) return SCFS_EINVAL;

    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;
    if (!(f->f_flag & FREAD)) return SCFS_EACCES;

    InCoreInode *ip = f->f_inode;
    if (!ip) return SCFS_EBADF;

    /* A device has no offset to walk, and readv on a node whose records
     * are not positional would scatter meaningless data.  Same answer as
     * pread: the descriptor is right for read(), wrong for positioning. */
    if ((uint16_t)(ip->mode & SCFS_S_IFMT) == SCFS_S_IFCHR)
        return SCFS_ESPIPE;

    int32_t total = 0;

    for (int i = 0; i < iovcnt; i++) {
        if (!vec[i].iov_base || vec[i].iov_len == 0u) continue;

        uint32_t offset = f->f_offset;
        int32_t  n = readi(ip, (char *)vec[i].iov_base,
                           vec[i].iov_len, &offset);
        if (n < 0) return (total > 0) ? total : n;

        f->f_offset = offset;
        total += n;

        if ((uint32_t)n < vec[i].iov_len) break;   /* short read ends it */
    }
    return total;
}
