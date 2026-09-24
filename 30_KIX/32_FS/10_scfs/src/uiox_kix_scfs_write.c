#include "uiox_kix_scfs_internal.h"

static int32_t scfs_do_write(int fd, const char *buf, uint32_t count,
                             int positioned, uint32_t pos)
{
    scfs_file_t *f = scfs_getf(fd);
    uint32_t     off;
    uint32_t     moved = 0u;
    int32_t      rc;

    if (!f) return SCFS_EBADF;
    if (!buf && count) return SCFS_EFAULT;
    if (!(f->f_flag & FWRITE)) return SCFS_EBADF;

    off = positioned ? pos : f->f_offset;

    /* append mode: every write goes to the end, whatever the offset is */
    if ((f->f_flag & FAPPEND) && !positioned) off = f->f_inode->size;

    rc = writei_at(f->f_inode, buf, count, off, &moved);
    if (rc < 0) return rc;

    if (!positioned) f->f_offset = off + moved;
    return (int32_t)moved;
}

int32_t uiox_kix_scfs_write(int fd, const char *buf, uint32_t count)
{ return scfs_do_write(fd, buf, count, 0, 0u); }

int32_t uiox_kix_scfs_pwrite(int fd, const char *buf, uint32_t count, uint32_t off)
{ return scfs_do_write(fd, buf, count, 1, off); }

int32_t uiox_kix_scfs_writev(int fd, const void *iov, int iovcnt)
{
    const scfs_iovec_t *v = (const scfs_iovec_t *)iov;
    int32_t total = 0;
    int     i;

    if (!iov || iovcnt < 0) return SCFS_EINVAL;

    for (i = 0; i < iovcnt; i++) {
        int32_t n = scfs_do_write(fd, (const char *)v[i].base,
                                  (uint32_t)v[i].len, 0, 0u);
        if (n < 0) return (total > 0) ? total : n;
        total += n;
        if (n < (int32_t)v[i].len) break;
    }
    return total;
}
