#include "uiox_kix_scfs_internal.h"

/*
 * Bach has no mmap.  Ch.6 describes the process's address space in units
 * of REGIONS and Ch.7 §1 maps those onto physical pages, but nothing let
 * a program ASK for a mapping.
 *
 * The kernel work splits cleanly:
 *   SCFS's part   the FILE side — resolve the descriptor, check it is
 *                 mappable, capture the inode and the block map
 *   the VM part   the ADDRESS side — choose a virtual range, build the
 *                 page table entries, handle the page fault
 *
 * 33_PCS owns the address space.  There is no MMU-backed paging in this
 * build at all, so there is no address side to hand the file side to.
 * These calls return ENOSYS.  The file half below is written out because
 * it is correct and complete, but a return of success would hand the
 * caller a virtual address whose first read faults into nothing.
 */

/* Validate a mapping request against its file.  This is the file-side
 * half; it returns the inode the mapping would be backed by, or NULL
 * with *err set to the reason. */
static InCoreInode *scfs_mmap_validate(int fd, uint32_t length,
                                       int prot, int flags, int *err)
{
    scfs_file_t *f;

    f = scfs_getf(fd);
    if (!f) { *err = SCFS_EBADF; return (InCoreInode *)0; }

    if (length == 0u) { *err = SCFS_EINVAL; return (InCoreInode *)0; }

    /* A directory has no byte stream to map. */
    if (SCFS_IS_DIR(f->f_inode->mode)) {
        *err = SCFS_ENODEV;
        return (InCoreInode *)0;
    }

    /* A shared writable mapping needs the descriptor to be writable —
     * otherwise two processes could write through the same pages and only
     * one of them would have asked permission. */
    if ((flags & SCFS_MAP_SHARED) && (prot & SCFS_PROT_WRITE) &&
        !(f->f_flag & FWRITE)) {
        *err = SCFS_EACCES;
        return (InCoreInode *)0;
    }

    /* MAP_SHARED and MAP_PRIVATE are mutually exclusive and exactly one
     * must be given. */
    {
        int shared = (flags & SCFS_MAP_SHARED)  ? 1 : 0;
        int priv   = (flags & SCFS_MAP_PRIVATE) ? 1 : 0;
        if (shared == priv) { *err = SCFS_EINVAL; return (InCoreInode *)0; }
    }

    *err = SCFS_OK;
    return f->f_inode;      /* borrowed — the file table entry owns it */
}

void *uiox_kix_scfs_mmap(void *addr, uint32_t length, int prot,
                         int flags, int fd, uint32_t offset)
{
    int          err = SCFS_OK;
    InCoreInode *ip  = scfs_mmap_validate(fd, length, prot, flags, &err);

    if (!ip) return SCFS_MAP_FAILED;

    /* The file must be long enough to cover the request.  A mapping past
     * the end is not an error at map time in POSIX — the fault handler
     * delivers SIGBUS — but with no fault handler to deliver anything,
     * the honest test is here. */
    if (offset > ip->size || length > ip->size - offset)
        return SCFS_MAP_FAILED;

    /* The offset must be block-aligned: a mapping is built out of whole
     * pages, and the file's blocks are whole blocks. */
    if (offset % (uint32_t)BLOCK_SIZE)
        return SCFS_MAP_FAILED;

    /* ── the address side, which does not exist ──────────────────────
     * Bach's region code would now allocate a virtual range, record the
     * backing inode and offset in the region table, and arrange for a
     * page fault to call bmap() for the block that holds the faulting
     * address.  33_PCS owns the address space and this build has no MMU
     * paging, so there is nothing to hand the validated request to. */
    (void)addr; (void)prot;
    return SCFS_MAP_FAILED;
}

int uiox_kix_scfs_munmap(void *addr, uint32_t length)
{
    (void)addr; (void)length;
    /* Nothing was ever mapped, so there is nothing to tear down.  A
     * refusal is more truthful than a silent success that would leave a
     * caller believing a mapping had been released. */
    return SCFS_ENOSYS;
}

int uiox_kix_scfs_msync(void *addr, uint32_t length, int flags)
{
    (void)addr; (void)length; (void)flags;
    return SCFS_ENOSYS;
}

int uiox_kix_scfs_mprotect(void *addr, uint32_t length, int prot)
{
    (void)addr; (void)length; (void)prot;
    return SCFS_ENOSYS;
}

int uiox_kix_scfs_madvise(void *addr, uint32_t length, int advice)
{
    (void)addr; (void)length; (void)advice;
    return SCFS_ENOSYS;
}

/* Needs a resident-page bitmap for a mapping, so it is downstream of the
 * same missing address side. */
int uiox_kix_scfs_mincore(void *addr, uint32_t length, uint8_t *vec)
{
    (void)addr; (void)length; (void)vec;
    return SCFS_ENOSYS;
}
