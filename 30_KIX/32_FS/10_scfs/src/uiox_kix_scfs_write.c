/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_write.c
 *
 * SCFS - Algorithm write.  Bach, The Design of the UNIX Operating System.
 *
 * -- Bach's description, verbatim -------------------------------------
 * "if the file does not contain a block that corresponds to the byte
 *  offset to be written, the kernel allocates a new block using algorithm
 *  alloc and assigns the block number to the correct position in the
 *  inode's table of contents."
 *
 * The algorithm is otherwise identical to read: get the file table entry,
 * check accessibility, lock the inode, walk the byte range with bmap, copy
 * the other direction, release the buffer, update the file table offset.
 * The one difference is that a missing block is ALLOCATED rather than
 * treated as end-of-file.
 *
 * -- layering ---------------------------------------------------------
 * 01_fsa's writei() is this walk: bmap_alloc() where readi() calls bmap(),
 * bwrite() instead of a bare brelse(), and it grows ip->size when the
 * write runs past the old end.
 *
 * -- pwrite / writev --------------------------------------------------
 * Both are in this file: pwrite is the walk at a private offset, writev
 * repeats it per iovec segment.
 *
 * -- CHANGED: writev's signature --------------------------------------
 * The public prototype is opaque on purpose:
 *
 *     int32_t uiox_kix_scfs_writev(int fd, const void *iov, int iovcnt);
 *
 * An earlier revision wrote `const scfs_iovec_t *` and returned int.  The
 * type is declared nowhere in the tree, and the return type disagreed with
 * the header:
 *
 *     write.c:112: unknown type name 'scfs_iovec_t'
 *     write.c:112: conflicting types for 'uiox_kix_scfs_writev';
 *                  have 'int(int, const int *, int)'
 *     uiox_kix_scfs.h:354: previous declaration ... 'int32_t(int,
 *                  const void *, int)'
 *
 * The five member-access errors (iov_base / iov_len) were a consequence:
 * an opaque pointer cannot be indexed.  Casting once, below, is what lets
 * the walk address the segments while the public type stays opaque - the
 * syscall stub owns the ABI, this function owns the walk.
 *
 * @version 1.1.0  @date 2026-09-26
 */
#include "uiox_kix_scfs_internal.h"

/* write() - Algorithm write. */
int uiox_kix_scfs_write(int fd, const char *buf, uint32_t count)
{
    /* -- 1. get the FILE TABLE entry from the fd ---------------------- */
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    /* -- 2. check file accessibility ---------------------------------- */
    if (!(f->f_flag & FWRITE)) return SCFS_EACCES;

    if (count == 0u) return 0;
    if (!buf)        return SCFS_EFAULT;

    /* -- 3. the inode, then lock it ----------------------------------- */
    InCoreInode *ip = f->f_inode;
    if (!ip) return SCFS_EBADF;
    ip->locked = true;                       /* 01_fsa's lock flag */

    /*
     * O_APPEND: every write lands at the current end of file.  Bach
     * reads the end from the inode each time rather than seeking once,
     * so a concurrent append cannot land in the middle.
     */
    if (f->f_flag & FAPPEND) f->f_offset = ip->size;

    uint32_t offset = f->f_offset;

    /*
     * -- 4. the write loop ------------------------------------------
     * Bach's loop, with alloc() where read's loop had nothing: a missing
     * block is created, the block number assigned into the inode's map,
     * the block zeroed and filled, and the buffer written back.
     * 01_fsa's writei() does exactly this through bmap_alloc(),
     * which is still a stub - see the note in truncate.c about ENOSYS.
     */
    int32_t n = writei(ip, buf, count, &offset);

    /* -- 5. unlock the inode ------------------------------------------ */
    ip->locked = false;

    if (n < 0) return (int)n;

    /* -- 6. update the FILE TABLE offset, and mark the inode dirty ---- */
    f->f_offset = offset;
    ip->flags  |= IFLAG_MODIFIED;            /* size or data moved */

    return (int)n;
}

/*
 * pwrite() - the same walk at a private offset.
 *
 * f_offset is not touched, so a file table entry shared by dup() keeps
 * its position.  writei_at() takes the offset by value for that reason.
 */
int uiox_kix_scfs_pwrite(int fd, const char *buf, uint32_t count, uint32_t off)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;
    if (!(f->f_flag & FWRITE)) return SCFS_EACCES;

    if (count == 0u) return 0;
    if (!buf)        return SCFS_EFAULT;

    InCoreInode *ip = f->f_inode;
    if (!ip) return SCFS_EBADF;

    uint32_t moved = 0u;
    int32_t  n = writei_at(ip, buf, count, off, &moved);

    if (n >= 0) ip->flags |= IFLAG_MODIFIED;
    return (int)n;
}

/*
 * -- the iovec layout, kept LOCAL --------------------------------
 * The public prototype is opaque:
 *
 *     int32_t uiox_kix_scfs_writev(int fd, const void *iov, int iovcnt);
 *
 * so this file must not name a struct type in its signature.  The layout
 * matches the kernel's: a pointer then a length, in that order.
 * ---------------------------------------------------------------- */
typedef struct {
    void    *iov_base;
    uint32_t iov_len;
} scfs_iovec_local_t;

/*
 * writev() - gather write.
 *
 * Unlike readv, a short segment does NOT end the loop: a write has no
 * end-of-file condition, so every segment is attempted and the total is
 * returned.  A negative return from any segment stops the call, with the
 * bytes already written reported rather than discarded.
 *
 * int32_t, not int - the header declares int32_t and a differing return
 * type is its own compile error.
 */
int32_t uiox_kix_scfs_writev(int fd, const void *iov, int iovcnt)
{
    const scfs_iovec_local_t *vec = (const scfs_iovec_local_t *)iov;

    if (!vec || iovcnt <= 0 || iovcnt > 1024) return SCFS_EINVAL;

    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;
    if (!(f->f_flag & FWRITE)) return SCFS_EACCES;

    InCoreInode *ip = f->f_inode;
    if (!ip) return SCFS_EBADF;

    int32_t total = 0;

    for (int i = 0; i < iovcnt; i++) {
        if (!vec[i].iov_base || vec[i].iov_len == 0u) continue;

        uint32_t offset = f->f_offset;
        int32_t  n = writei(ip, (const char *)vec[i].iov_base,
                            vec[i].iov_len, &offset);
        if (n < 0) return (total > 0) ? total : n;

        f->f_offset = offset;
        total += n;
    }

    if (total > 0) ip->flags |= IFLAG_MODIFIED;
    return total;
}
