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
 * The ADDRESS side is 33_PCS's, and it exists: 40_psa owns the region
 * table (allocreg / attachreg / detachreg / freereg) and 02_MemMngnt
 * owns the pages (uiox_kix_page_fault.c, uiox_phys_alloc.c).  What this
 * file lacked was the connector between them and the validated request
 * below — scfs_mmap_attach() is it.
 *
 * ── two kinds of backing, and only one is blocks ───────────────────────
 * A REGULAR FILE maps its blocks: the validator checks the request
 * against ip->size and UNFS_BLOCK_SIZE below, and the region then loads
 * — to LOADING, not VALID, because no readi() is wired from 01_fsa yet
 * (uiox_kix_psa_region.h:150 says so in loadreg's own banner).
 *
 * A CHAR NODE BOUND TO A CMD CLASS maps its device's MMIO window —
 * descriptor.base, which the probe filled from hw.base_addr
 * (gpu_hw.h:118, hdmi_hw.h:151, mon_hw.h:135).  Its pages are registers,
 * always present, so it needs no load step at all.
 *
 * ── what a successful return means here ────────────────────────────────
 * A region is allocated, attached to the current process, and its base
 * address returned.  For a CMD node that is a working mapping in the
 * software model this build has.  For a file it is an ATTACHED region
 * whose pages are not yet valid — a fault on it reports "no backing
 * wired" rather than returning stale bytes, which is the honest state
 * until the readi() half lands.  No MMU is enabled anywhere in this
 * build, so "mapped" means PCS's region table, not hardware entries.
 *
 * @version 1.2.0  @date 2026-10-18
 */
#include "uiox_kix_scfs_internal.h"
#include "uiox_devclass.h"          /* uiox_dev_dev_t, uiox_dev_lookup */
#include "uiox_kix_psa_region.h"    /* allocreg/attachreg/loadreg/freereg */
#include "uiox_kix_psa_process.h"   /* uiox_kix_psa_current_proc          */

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
 * A char node bound to a CMD class has no blocks to map: the mapping is
 * backed by the device's MMIO window — descriptor.base, which the probe
 * filled from hw.base_addr (gpu_hw.h:118, hdmi_hw.h:151, mon_hw.h:135).
 * The inode's size and block-alignment tests in uiox_kix_scfs_mmap() are
 * about a regular file's blocks and do not apply to such a node.
 *
 * Only CMD reaches here: BLOCK maps through 00_bcache, STREAM and EVENT
 * have read() and a pool, and REG is reached by ioctl and read().
 *
 * Note there is no window SIZE on the descriptor or on these HALs (gpu
 * exposes vram_size, which is memory behind the part, not the register
 * window), so the extent of a CMD mapping cannot be validated here.
 * That belongs to the half that owns addresses. */
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

/* ══ the connector — hand a validated request to 33_PCS ══════════════
 * PSA's two-step, and the pair is not optional: allocreg takes a free
 * region and sets r_refcnt to ZERO ("a fresh region belongs to nobody
 * until attachreg is called"), while attachreg records THIS process's
 * view of it at vaddr and increments the count.  One without the other
 * leaks a region or leaves a page table unattached, which is why every
 * failure below frees before returning.
 *
 * The inode cast is safe: PCS stores r_inode and never inspects it —
 * uiox_kix_psa_region.c:174 stores NULL, :216 stores the pointer, :444
 * hands it back to allocreg.  Three operations, none touching a field.
 * The tag difference between 01_fsa's InCoreInode and PCS's forward-
 * declared `struct inode` is spelling, not type.
 * ═══════════════════════════════════════════════════════════════════ */
static void *scfs_mmap_attach(struct inode *ip, uintptr_t vaddr,
                              uint32_t offset, uint32_t length,
                              uiox_kix_psa_region_type_t rt, bool load)
{
    uiox_kix_psa_region_t *rp = uiox_kix_psa_allocreg(ip, rt);
    if (!rp) return SCFS_MAP_FAILED;        /* no free region — ENOMEM */

    uiox_kix_psa_proc_t *p = uiox_kix_psa_current_proc;
    if (!p) { uiox_kix_psa_freereg(rp); return SCFS_MAP_FAILED; }

    uiox_kix_psa_pregion_t *prp =
        uiox_kix_psa_attachreg(rp, (struct uiox_kix_psa_proc *)p, vaddr, rt);
    if (!prp) { uiox_kix_psa_freereg(rp); return SCFS_MAP_FAILED; }

    /* Only a file-backed region loads, and even then it stops at
     * LOADING: uiox_kix_psa_region.h:150 records that no readi() is
     * wired from 01_fsa, so loadreg refuses to mark pages VALID or to
     * claim it read anything.  An MMIO window needs no load at all. */
    if (load)
        uiox_kix_psa_loadreg(prp, vaddr, ip, offset, length);

    return (void *)prp->pr_vaddr;
}

void *uiox_kix_scfs_mmap(void *addr, uint32_t length, int prot,
                         int flags, int fd, uint32_t offset)
{
    int          err = SCFS_OK;
    uintptr_t    win = 0u;
    InCoreInode *ip  = scfs_mmap_validate(fd, length, prot, flags, &err);
    if (!ip) return SCFS_MAP_FAILED;

    /* ══ CMD — an MMIO window, always backed ═════════════════════════
     * Tested BEFORE the inode's size/alignment tests, because those are
     * about a regular file's blocks and a CMD node has none — its size
     * is 0, so ``offset > ip->size`` would reject every mapping request
     * for a reason that does not apply.
     *
     * The backing is dev->base, not bmap()'s block. */
    if (scfs_mmap_is_cmd_node(fd, &win)) {
        (void)addr; (void)prot;
        return scfs_mmap_attach((struct inode *)0, win, 0u, length,
                                UIOX_KIX_PSA_REG_SHMEM, false);
    }

    /* ── regular file: blocks, so it loads (as far as LOADING) ────── */
    if (offset > ip->size || length > ip->size - offset)
        return SCFS_MAP_FAILED;

    /* The offset must be block-aligned: a mapping is built out of whole
     * pages, and the file's blocks are whole blocks. */
    if (offset % UNFS_BLOCK_SIZE)
        return SCFS_MAP_FAILED;

    (void)addr; (void)prot;
    return scfs_mmap_attach((struct inode *)ip, 0u, offset, length,
                            (flags & 0x1) ? UIOX_KIX_PSA_REG_SHMEM
                                          : UIOX_KIX_PSA_REG_DATA, true);
}

int uiox_kix_scfs_munmap(void *addr, uint32_t length)
{
    (void)addr; (void)length;
    /* detachreg/freereg are the teardown pair for what mmap built; the
     * caller's vaddr would have to be resolved back to its pregion to
     * reach them, which needs a lookup this file does not yet do. */
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
 * same lookup munmap would need.
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
