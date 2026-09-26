/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_read.c
 *
 * SCFS — Algorithm read.  Bach, The Design of the UNIX Operating System.
 *
 * ── Bach's algorithm, verbatim ─────────────────────────────────────────
 * input: user file descriptor, address of buffer in user process,
 *        number of bytes to read
 * output: count of bytes copied into user space
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
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

/*
 * read() — Algorithm read.
 *
 * The offset SzCFS keeps is uint32 because 01_fsa's InCoreInode.size is;
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

    /* ── 4/5. the inode, then lock it ───────────────────────────────── */
    InCoreInode *ip = f->f_inode;
    if (!ip) return SCFS_EBADF;
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
 * revision wrote \`const scfs_iovec_t *\`, and that type is declared
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
