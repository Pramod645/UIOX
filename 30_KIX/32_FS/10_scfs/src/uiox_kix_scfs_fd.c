/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_fd.c
 *
 * dup / dup3 / fcntl / pipe / falloc / ufalloc
 *
 * Bach Ch.7 — the system-wide file table (NFILE) holds one entry per open
 * file; the per-process descriptor table points into it.
 *   falloc()   allocates a file-table entry AND a descriptor
 *   ufalloc()  finds a free descriptor slot only
 *   dup()      points a second descriptor at the SAME entry (f_count++)
 *   pipe()     one FIFO i-node + buffer shared by a read and a write entry
 *
 * Merged unit: the descriptor- and file-table surface lives here.
 *
 * @version 1.0.0  @date 2026-09-21
 */
#include "uiox_kix_scfs_internal.h"

/* ── file table (Bach Ch.7) ─────────────────────────────────────────── */

/* falloc() — allocate a (file-table entry, descriptor) pair; return the fd
 * or -1 when the descriptor table is full. */
int uiox_kix_scfs_falloc(uiox_inode_t *inode, uint32_t flags, uiox_file_t **out_file)
{
    if (!inode) return -SCFS_EINVAL;

    uiox_file_t *f = (uiox_file_t *)0;
    int fslot = vfs_file_alloc(&f);
    if (fslot < 0) return -1;

    f->f_inode = inode;
    f->f_flags = flags;
    f->f_count = 1;
    f->f_pos   = 0;
    f->f_op    = inode->i_fop;

    int fd = vfs_fd_alloc_slot(f);
    if (fd < 0) { vfs_file_free(fslot); return -1; }

    if (out_file) *out_file = f;
    return fd;
}

/* ufalloc() — the lowest unused descriptor index (no file-table entry). */
int uiox_kix_scfs_ufalloc(void)
{
    return vfs_fd_find_free();
}

/* ── descriptor duplication ─────────────────────────────────────────── */

/* dup() — both descriptors share one file-table entry (f_pos, f_flags). */
long uiox_kix_scfs_dup(uiox_reg_t fd,
                       uiox_reg_t a1, uiox_reg_t a2, uiox_reg_t a3,
                       uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    f->f_count++;
    return vfs_fd_dup((uint32_t)fd);
}

/* dup3() — dup to a chosen descriptor number. */
long uiox_kix_scfs_dup3(uiox_reg_t oldfd, uiox_reg_t newfd, uiox_reg_t flags,
                        uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a3; (void)a4; (void)a5;
    if ((uint32_t)oldfd == (uint32_t)newfd) return -SCFS_EINVAL;
    uiox_file_t *f;
    long rc = scfs_fd_file(oldfd, &f);
    if (rc < 0) return rc;
    f->f_count++;
    return vfs_fd_dup_to((uint32_t)oldfd, (uint32_t)newfd, (uint32_t)flags);
}

/* fcntl() — descriptor control.  Only file-table-level commands are handled
 * here; lock commands need the lock manager and fall through to the VFS. */
long uiox_kix_scfs_fcntl(uiox_reg_t fd, uiox_reg_t cmd, uiox_reg_t arg,
                         uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a3; (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;

    switch ((uint32_t)cmd) {
    case UIOX_F_DUPFD:  f->f_count++; return vfs_fd_dup_from((uint32_t)fd, (uint32_t)arg);
    case UIOX_F_GETFD:  return (long)f->f_flags;
    case UIOX_F_SETFD:  f->f_flags = (uint32_t)arg; return SCFS_OK;
    case UIOX_F_GETFL:  return (long)f->f_flags;
    case UIOX_F_SETFL:  f->f_flags = (uint32_t)arg; return SCFS_OK;
    default:            return vfs_vfs_fcntl(f, (uint32_t)cmd, (uint64_t)arg);
    }
}

/* ── pipe (Bach Ch.7 FIFO) ──────────────────────────────────────────── */

/* pipe() — allocate one FIFO i-node plus a buffer, then install a read and a
 * write file-table entry that share that i-node. */
long uiox_kix_scfs_pipe(uiox_reg_t fds_uptr,
                        uiox_reg_t a1, uiox_reg_t a2, uiox_reg_t a3,
                        uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    if (fds_uptr == 0u) return -SCFS_EFAULT;

    uiox_inode_t *fifo = (uiox_inode_t *)0;
    long rc = vfs_pipe_inode_alloc(&fifo);
    if (rc < 0) return rc;

    fifo->i_mode = UIOX_S_IFIFO | 0600u;
    fifo->i_pipe = vfs_pipe_buffer_alloc(UIOX_PIPE_SIZE);
    if (!fifo->i_pipe) { vfs_pipe_inode_free(fifo); return -SCFS_ENOMEM; }

    int rfd = uiox_kix_scfs_falloc(fifo, UIOX_O_RDONLY, (uiox_file_t **)0);
    if (rfd < 0) { vfs_pipe_inode_free(fifo); return -SCFS_EBADF; }

    int wfd = uiox_kix_scfs_falloc(fifo, UIOX_O_WRONLY, (uiox_file_t **)0);
    if (wfd < 0) {
        vfs_fd_free((uint32_t)rfd);
        vfs_pipe_inode_free(fifo);
        return -SCFS_EBADF;
    }

    int *udst = (int *)fds_uptr;
    udst[0] = rfd;
    udst[1] = wfd;
    return SCFS_OK;
}
