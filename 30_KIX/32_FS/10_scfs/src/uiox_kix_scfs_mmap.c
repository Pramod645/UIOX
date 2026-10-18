/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_mmap.c
 *
 * SCFS — mmap, munmap, msync, mprotect.
 * Bach, The Design of the UNIX Operating System.
 *
 * ── where this sits relative to Bach ───────────────────────────────────
 * Bach has no mmap.  Chapter 6 describes the process's address space in
 * units of REGIONS — "a contiguous area of a process's address space,
 * such as text, data, or stack" — and Chapter 7 §1 maps those regions
 * onto physical pages.  A file could be mapped as the backing store of a
 * text region, but nothing let a program ASK for a mapping.
 *
 * mmap(2) is that ask.  The kernel work it needs splits cleanly:
 *
 *   SCFS's part   the FILE side — resolve the descriptor, check it is
 *                 mappable, capture the inode and the block map so the
 *                 fault handler can find the bytes
 *   the VM part   the ADDRESS side — choose a virtual range, build the
 *                 page table entries, handle the page fault
 *
 * 33_PCS owns the address space.  There is no MMU-backed paging in this
 * build at all, so there is no address side to hand the file side to —
 * and mmap without a page fault handler cannot be faked, because the
 * whole point is that the mapping is populated lazily by the hardware.
 *
 * ── two kinds of backing, and only one is blocks ───────────────────────
 * A REGULAR FILE maps its blocks: the validator checks the request
 * against ip->size and UNFS_BLOCK_SIZE below.
 *
 * A CHAR NODE BOUND TO A CMD CLASS maps its device's MMIO window —
 * descriptor.base, which the probe filled from hw.base_addr
 * (gpu_hw.h:118, hdmi_hw.h:151, mon_hw.h:135).  It has no blocks, no
 * byte stream and usually size 0, so the block tests must not run for it.
 * scfs_mmap_is_cmd_node() below is that second case.
 *
 * ── the refusal, stated precisely ──────────────────────────────────────
 * These calls return ENOSYS.  The file half below is written out because
 * it is correct and complete — a program's mapping request is validated
 * against the inode exactly as Bach's region code would need it to be —
 * but a return of success would hand the caller a virtual address whose
 * first read faults into nothing.
 *
 * @version 1.1.0  @date 2026-10-18
 */
#include "uiox_kix_scfs_internal.h"
#include "uiox_devclass.h"   /* uiox_dev_dev_t, uiox_dev_lookup */

/* Validate a mapping request against its file.  This is the file-side
 * half; it returns the inode the mapping would be backed by, or NULL
 * with *err set to the reason. */
static InCoreInode *scfs_mmap_validate(int fd, uint32_t length,
                                       int prot, int flags, int *err)
{
    scfs_file_t *f = scfs_getf(fd);
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
    if ((flags & 0x1) && (prot & 0x2) && !(f->f_flag & FWRITE)) {
        *err = SCFS_EACCES;
        return (InCoreInode *)0;
    }

    /* MAP_SHARED and MAP_PRIVATE are mutually exclusive and exactly one
     * must be given. */
    {
        int shared  = (flags & 0x1) ? 1 : 0;
        int priv    = (flags & 0x2) ? 1 : 0;
        if (shared == priv) { *err = SCFS_EINVAL; return (InCoreInode *)0; }
    }

    *err = SCFS_OK;
    return f->f_inode;      /* borrowed — the file table entry owns it */
}

/*
 * ── the device case ───────────────────────────────────────────────────
 * A char node bound to a CMD class has no blocks to map: the mapping
 * would be backed by the device's MMIO window — descriptor.base, which
 * the probe filled from hw.base_addr (gpu_hw.h:118, hdmi_hw.h:151,
 * mon_hw.h:135).  The inode's size and block-alignment tests in
 * uiox_kix_scfs_mmap() are about a regular file's blocks and do not
 * apply to such a node.
 *
 * Only CMD reaches here: BLOCK maps through 00_bcache, STREAM and EVENT
 * have read() and a pool, and REG is reached by ioctl and read().
 *
 * Note there is no window SIZE on the descriptor or on these HALs (gpu
 * exposes vram_size, which is memory behind the part, not the register
 * window), so the extent of a CMD mapping cannot be validated here.
 * That belongs to the half that owns addresses.
 */
static int scfs_mmap_is_cmd_node(int fd, uintptr_t *base_out)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return 0;

    if ((uint16_t)(f->f_inode->mode & SCFS_S_IFMT) != SCFS_S_IFCHR)
        return 0;

    uiox_dev_dev_t *dev = uiox_dev_lookup(
        (uiox_devclass_t)f->f_inode->idev_class, f->f_inode->idev_unit);
    if (!dev) return 0;                       /* named, nothing bound */

    if (UIOX_DEVCLASS_FAMILY((uiox_devclass_t)f->f_inode->idev_class)
            != UIOX_DEVFAM_CMD)
        return 0;                             /* not a mappable family */

    if (!dev->base) return 0;                 /* no window to map */

    if (base_out) *base_out = dev->base;
    return 1;
}

void *uiox_kix_scfs_mmap(void *addr, uint32_t length, int prot,
                         int flags, int fd, uint32_t offset)
{
    int          err = SCFS_OK;
    uintptr_t    win = 0u;
    InCoreInode *ip  = scfs_mmap_validate(fd, length, prot, flags, &err);
    if (!ip) return SCFS_MAP_FAILED;

    /* ══ the device branch ═══════════════════════════════════════════
     * Tested BEFORE the inode's size/alignment tests, because those are
     * about a regular file's blocks and a CMD node has none — its size is
     * 0, so ``offset > ip->size`` would reject every mapping request for
     * a reason that does not apply.
     *
     * The backing is dev->base, not bmap()'s block. */
    if (scfs_mmap_is_cmd_node(fd, &win)) {
        /*
         * ── the address side, still 33_PCS's ────────────────────────
         * Bach's region code would now allocate a virtual range and
         * record ``win + offset`` as its backing, so the fault handler
         * installs the controller's page rather than a file block.
         *
         * The range cannot be bounded here: the descriptor carries an
         * address and no length, and the three CMD HALs expose no window
         * size.  That check belongs to whoever owns the address space.
         */
        (void)addr; (void)prot; (void)win;
        return SCFS_MAP_FAILED;
    }

    /* ── regular file: the block-backed path ───────────────────────── */
    if (offset > ip->size || length > ip->size - offset)
        return SCFS_MAP_FAILED;

    /* The offset must be block-aligned: a mapping is built out of whole
     * pages, and the file's blocks are whole blocks. */
    if (offset % UNFS_BLOCK_SIZE)
        return SCFS_MAP_FAILED;

    /*
     * ── the address side, which does not exist ──────────────────────
     * Bach's region code would now allocate a virtual range, record the
     * backing inode and offset in the region table, and arrange for a
     * page fault to call bmap() for the block that holds the faulting
     * address.  33_PCS owns the address space and this build has no MMU
     * paging, so there is nothing to hand the validated request to.
     */
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

/* ── madvise — telling the VM about access patterns ─────────────────── */
int uiox_kix_scfs_madvise(void *addr, uint32_t length, int advice)
{
    (void)addr; (void)length; (void)advice;
    return SCFS_ENOSYS;
}

/*
 * ── mincore — "which pages of this mapping are resident?" ────────────
 * Needs a resident-page bitmap for a mapping, so it is downstream of the
 * same missing address side.
 */
int uiox_kix_scfs_mincore(void *addr, uint32_t length, uint8_t *vec)
{
    (void)addr; (void)length; (void)vec;
    return SCFS_ENOSYS;
}

/* sys_* aliases — same bodies. */
void *sys_mmap(void *a, uint32_t l, int p, int f, int fd, uint32_t o)
{ return uiox_kix_scfs_mmap(a, l, p, f, fd, o); }
int sys_munmap(void *a, uint32_t l)  { return uiox_kix_scfs_munmap(a, l); }
