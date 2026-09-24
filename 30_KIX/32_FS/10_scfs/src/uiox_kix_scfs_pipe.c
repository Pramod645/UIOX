#include "uiox_kix_scfs_internal.h"

/*
 * Bach's Algorithm pipe, verbatim.
 * {
 *   assign new inode from pipe device (algorithm ialloc);
 *   allocate file table entry for reading, another for writing;
 *   initialize file table entries to point to new inode;
 *   allocate user file descriptor for reading, another for writing;
 *   set inode reference count to 2;
 *   initialize count of inode readers, writers to 1;
 * }
 *
 * ── the correction this file carries ─────────────────────────────────
 * The first cut set ip->i_pipe_readers and ip->i_pipe_writers.  Neither
 * field exists, and there is no buffer field either — so a FIFO on this
 * filesystem cannot carry data.
 *
 * What IS correct and stays: the inode is allocated FT_FIFO, TWO file
 * table entries point at it with FREAD / FWRITE, and TWO user fd slots
 * address them.  close() and dup() behave, and the reference count is 2.
 * The structural half is Bach's; the data half is the missing part.
 */
int uiox_kix_scfs_pipe(int *fds)
{
    InCoreInode *ip;
    scfs_file_t *fr;
    scfs_file_t *fw;
    int          rfd;
    int          wfd;

    if (!fds) return SCFS_EFAULT;

    ip = ialloc(FT_FIFO, 0600u, 0u, 0u);
    if (!ip) return SCFS_ENOSPC;

    ip->nlink = 1;

    fr = scfs_falloc(ip, O_RDONLY, 0600u);
    if (!fr) { iput(ip); return SCFS_ENFILE; }

    fw = scfs_falloc(ip, O_WRONLY, 0600u);
    if (!fw) {
        scfs_fclose_entry(fr);
        iput(ip);
        return SCFS_ENFILE;
    }

    fr->f_flag = FREAD;
    fw->f_flag = FWRITE;

    rfd = scfs_ufd_alloc(fr);
    if (rfd < 0) {
        scfs_fclose_entry(fr);
        scfs_fclose_entry(fw);
        iput(ip);
        return SCFS_EMFILE;
    }

    wfd = scfs_ufd_alloc(fw);
    if (wfd < 0) {
        scfs_u->ufd_file[rfd] = (scfs_file_t *)0;
        scfs_fclose_entry(fr);
        scfs_fclose_entry(fw);
        iput(ip);
        return SCFS_EMFILE;
    }

    if (ip->refcount != 2) ip->refcount = 2;

    ip->flags |= IFLAG_CHANGED;
    iupdate(ip);
    ip->locked = false;

    fds[0] = rfd;
    fds[1] = wfd;

    return SCFS_ENOSYS;     /* no pipe buffer in the inode — see above */
}
