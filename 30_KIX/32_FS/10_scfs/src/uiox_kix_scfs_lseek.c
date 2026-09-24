/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_lseek.c
 *
 * SCFS — Algorithm lseek.  Bach, The Design of the UNIX Operating System.
 *
 * ── what Bach says ─────────────────────────────────────────────────────
 * lseek repositions the file offset without doing I/O.  The offset lives
 * in the FILE TABLE entry, not in the inode — which is why two
 * descriptors from dup() share a position, and why a fresh open() starts
 * at zero.
 *
 *   input:  user file descriptor, offset, whence
 *   output: the new byte offset
 *   {
 *       get file table entry from user file descriptor;
 *       calculate the new offset from whence:
 *           SEEK_SET — from the start of the file
 *           SEEK_CUR — from the current offset
 *           SEEK_END — from the file's size (read from the inode)
 *       if the result is negative, return error;
 *       store the new offset in the file table entry;
 *       return (new offset);
 *   }
 *
 * No lock is taken: nothing on disk is touched.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

int uiox_kix_scfs_lseek(int fd, int32_t offset, int whence)
{
    /* ── get the file table entry from the fd ───────────────────────── */
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    /* ── the base each whence measures from ─────────────────────────── */
    int32_t base;
    switch (whence) {
    case SEEK_SET:
        base = 0;
        break;
    case SEEK_CUR:
        base = (int32_t)f->f_offset;
        break;
    case SEEK_END:
        /* The size is the inode's; 01_fsa's InCoreInode.size is uint32. */
        base = (int32_t)f->f_inode->size;
        break;
    default:
        return SCFS_EINVAL;
    }

    /* ── compute, refusing to go negative ───────────────────────────── */
    int32_t np = base + offset;
    if (np < 0) return SCFS_EINVAL;

    /* ── store in the FILE TABLE entry ──────────────────────────────── */
    f->f_offset = (uint32_t)np;

    return np;
}
