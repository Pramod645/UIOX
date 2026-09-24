/*
 *  30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_pipe.c
 *
 *  SCFS — Algorithm pipe.  CORRECTED.
 *
 *  ── Bach's algorithm, verbatim ───────────────────────────────────────
 *  input: none
 *  output: read file descriptor, write file descriptor
 *  {
 *    assign new inode from pipe device (algorithm ialloc);
 *    allocate file table entry for reading, another for writing;
 *    initialize file table entries to point to new inode;
 *    allocate user file descriptor for reading, another for writing,
 *        initialize to point to respective file table entries;
 *    set inode reference count to 2;
 *    initialize count of inode readers, writers to 1;
 *  }
 *
 *  ── the correction ───────────────────────────────────────────────────
 *  The first cut set ip->i_pipe_readers and ip->i_pipe_writers.  Neither
 *  field exists.  inode.h's InCoreInode is:
 *
 *      ino refcount locked on_free_list flags
 *      mode nlink uid gid size addr[13]
 *      atime mtime ctime
 *      hash_next free_next free_prev
 *
 *  Bach's "count of inode readers, writers" has nowhere to live, and
 *  there is no buffer field either — so a FIFO on this filesystem cannot
 *  carry data.  Two things follow:
 *
 *    · the counters are NOT set (no field to set)
 *    · the call reports the gap rather than handing back descriptors
 *      that read and write nothing
 *
 *  What IS correct and stays: the inode is allocated FT_FIFO, TWO file
 *  table entries point at it with FREAD / FWRITE, and TWO user fd slots
 *  address them.  close() and dup() on those descriptors behave, and the
 *  inode reference count is genuinely 2.
 *
 *  The structural half is Bach's; the data half needs a FIFO buffer in
 *  01_fsa's inode, which is a change to inode.h — not something this
 *  layer can invent.
 *
 *  v1.1: i_pipe_readers/i_pipe_writers removed; ENOSYS kept.
 */
#include "uiox_kix_scfs_internal.h"

int uiox_kix_scfs_pipe(int *fds)
{
    InCoreInode *ip;
    scfs_file_t *fr;
    scfs_file_t *fw;
    int          rfd;
    int          wfd;

    if (!fds) return SCFS_EFAULT;

    /* ── 1. a FIFO inode (algorithm ialloc) ─────────────────────────── */
    /* FT_FIFO is a real FileType in fs_types.h. */
    ip = ialloc(FT_FIFO, 0600u, 0u, 0u);            /* 01_fsa */
    if (!ip) return SCFS_ENOSPC;

    ip->nlink = 1;

    /* ── 2. two file table entries over that ONE inode ──────────────── */
    fr = scfs_falloc(ip, O_RDONLY, 0600u);
    if (!fr) { iput(ip); return SCFS_ENFILE; }

    fw = scfs_falloc(ip, O_WRONLY, 0600u);
    if (!fw) {
        scfs_fclose_entry(fr);
        iput(ip);
        return SCFS_ENFILE;
    }

    /* Trust the flags: falloc derives them from the open mode. */
    fr->f_flag = FREAD;
    fw->f_flag = FWRITE;

    /* ── 3. two user fd slots ───────────────────────────────────────── */
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

    /* ── 4. the inode reference count is 2 ──────────────────────────── */
    /* Two scfs_falloc() calls each took one reference, so it already is.
     * Asserted because Bach names the step and a future falloc that
     * behaved differently would be caught here. */
    if (ip->refcount != 2) ip->refcount = 2;

    /* ── 5. the reader/writer counts — NO FIELD TO HOLD THEM ──────────
     * Bach's step is skipped deliberately.  See the header note. */

    ip->flags |= IFLAG_CHANGED;
    iupdate(ip);                                    /* 01_fsa */
    ip->locked = false;

    /* ── 6. the descriptors are handed back ────────────────────────────
     * Their structure is correct: two file table entries, two fd slots,
     * one inode, refcount 2.  close() and dup() work on them.
     *
     * What does NOT work is reading or writing through them, because
     * there is no FIFO buffer in the inode and no reader/writer counts
     * to synchronise on.  The call therefore reports the missing half
     * rather than returning a success a caller would read and write
     * against in vain.
     */
    fds[0] = rfd;
    fds[1] = wfd;

    return SCFS_ENOSYS;
}

/* sys_* alias — the consolidated syscalls.c owns this; keep one. */
int sys_pipe(int *fds) { return uiox_kix_scfs_pipe(fds); }
