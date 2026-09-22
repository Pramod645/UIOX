/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_read.c
 *
 * read / pread / pwrite / readv / writev
 *
 * Bach Ch.7 — pread/pwrite are private-offset read/write (lseek/read/lseek
 * back, so a shared file-table entry keeps its f_pos); readv/writev walk the
 * iovec and hand each segment to the file's read/write operation.
 *
 * @version 1.0.0  @date 2026-09-21
 */
#include "uiox_kix_scfs_internal.h"

/* read() — sequential read into the user buffer, advancing f_pos. */
long uiox_kix_scfs_read(uiox_reg_t fd, uiox_reg_t ubuf, uiox_reg_t count,
                        uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a3; (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_op || !f->f_op->read) return -SCFS_ENOSYS;
    return (long)f->f_op->read(f, (void *)ubuf, (size_t)count, &f->f_pos);
}

/* pread() — positioned read; f_pos is saved and restored. */
long uiox_kix_scfs_pread(uiox_reg_t fd, uiox_reg_t ubuf, uiox_reg_t count,
                         uiox_reg_t off, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_op || !f->f_op->read) return -SCFS_ENOSYS;

    uint64_t saved = f->f_pos;
    f->f_pos = (uint64_t)off;
    ssize_t n = f->f_op->read(f, (void *)ubuf, (size_t)count, &f->f_pos);
    f->f_pos = saved;
    return (long)n;
}

/* pwrite() — positioned write; f_pos saved and restored. */
long uiox_kix_scfs_pwrite(uiox_reg_t fd, uiox_reg_t ubuf, uiox_reg_t count,
                          uiox_reg_t off, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_op || !f->f_op->write) return -SCFS_ENOSYS;

    uint64_t saved = f->f_pos;
    f->f_pos = (uint64_t)off;
    ssize_t n = f->f_op->write(f, (const void *)ubuf, (size_t)count, &f->f_pos);
    f->f_pos = saved;
    return (long)n;
}

/* readv() — scatter read across the iovec until it is full or the file ends. */
long uiox_kix_scfs_readv(uiox_reg_t fd, uiox_reg_t iov_uptr, uiox_reg_t iovcnt,
                         uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a3; (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_op || !f->f_op->read) return -SCFS_ENOSYS;

    const uiox_iovec_t *iov = (const uiox_iovec_t *)iov_uptr;
    long total = 0;
    for (uint32_t i = 0u; i < (uint32_t)iovcnt; i++) {
        ssize_t n = f->f_op->read(f, iov[i].iov_base, iov[i].iov_len, &f->f_pos);
        if (n < 0) return (total > 0) ? total : (long)n;
        total += n;
        if ((size_t)n < iov[i].iov_len) break;
    }
    return total;
}

/* writev() — gather write across the iovec. */
long uiox_kix_scfs_writev(uiox_reg_t fd, uiox_reg_t iov_uptr, uiox_reg_t iovcnt,
                          uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a3; (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_op || !f->f_op->write) return -SCFS_ENOSYS;

    const uiox_iovec_t *iov = (const uiox_iovec_t *)iov_uptr;
    long total = 0;
    for (uint32_t i = 0u; i < (uint32_t)iovcnt; i++) {
        ssize_t n = f->f_op->write(f, iov[i].iov_base, iov[i].iov_len, &f->f_pos);
        if (n < 0) return (total > 0) ? total : (long)n;
        total += n;
    }
    return total;
}
