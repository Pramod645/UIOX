/*
 * 30_KIX/33_PCS/40psa/src/uiox_kix_psa_region.c
 *
 * PSA region algorithms 3..9, plus the pool allocators they need.
 *
 *   3 allocreg   4 attachreg   5 growreg   6 loadreg
 *   7 freereg    8 detachreg   9 dupreg
 *
 * A region is SHARED and a pregion is one process's attachment to it.
 * Every algorithm below respects that split, because it is what makes
 * fork cheap: dupreg raises the reference count and the frames stay
 * shared until someone writes.
 *
 * ── the page-table pool ─────────────────────────────────────────────
 * calloc()/free() are unavailable freestanding, so page tables come from
 * a static pool: 128 regions x 1024 pages x 6 bytes is roughly 768 KB of
 * BSS.  The size is a deliberate trade of address space for having no
 * heap at all.
 *
 * ── the correction to growreg ───────────────────────────────────────
 * The original capped growth at MAX_REGION_SIZE but did not check the
 * LOWER bound.  Shrinking by more than r_size underflows a uint32, and
 * the derived r_npages then wraps to an enormous count — a page table
 * that would be walked far past its end.  The check is now present.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_psa_region.h"
#include "../include/uiox_kix_psa_process.h"
#include "../include/uiox_kix_psa_proc_algo.h"

/* ── Globals ───────────────────────────────────────────────────────── */
uiox_kix_psa_region_t  uiox_kix_psa_region_table[UIOX_KIX_PSA_NREGION];
uiox_kix_psa_region_t *uiox_kix_psa_free_region_list   = (uiox_kix_psa_region_t *)0;
uiox_kix_psa_region_t *uiox_kix_psa_active_region_list = (uiox_kix_psa_region_t *)0;

uint8_t  uiox_kix_psa_phys_mem_pool[UIOX_KIX_PSA_PHYS_MEM_SIZE];
uint32_t uiox_kix_psa_phys_mem_used = 0u;

/* ── Static page-table pool ────────────────────────────────────────── */
static uiox_kix_psa_pte_t s_pgtbl_pool[UIOX_KIX_PSA_NREGION][UIOX_KIX_PSA_MAX_PAGES];
static uint8_t            s_pgtbl_used[UIOX_KIX_PSA_NREGION];
static uint8_t            s_pgtbl_ready = 0u;

/* ── pgtbl_pool_init — one-time setup, idempotent ────────────────────
 * Called by both alloc and free so neither depends on region_init
 * having run first.  Marking every slot free is all it does. */
static void pgtbl_pool_init(void)
{
    uint32_t i;
    if (s_pgtbl_ready) return;
    for (i = 0u; i < (uint32_t)UIOX_KIX_PSA_NREGION; i++)
        s_pgtbl_used[i] = 0u;
    s_pgtbl_ready = 1u;
}

/* ── uiox_kix_psa_pgtbl_alloc ────────────────────────────────────────
 * Take a page table from the static pool.
 *
 * Two range checks first: a count of zero has nothing to map, and a count
 * above MAX_PAGES would overrun the pool row.  The chosen table is then
 * zeroed over npages entries so a recycled table cannot hand out stale
 * frame numbers — a stale pfn is a mapping into someone else's memory.
 *
 * Returns NULL when the pool is exhausted. */
uiox_kix_psa_pte_t *uiox_kix_psa_pgtbl_alloc(uint32_t npages)
{
    uint32_t i;

    if (npages == 0u || npages > (uint32_t)UIOX_KIX_PSA_MAX_PAGES)
        return (uiox_kix_psa_pte_t *)0;

    pgtbl_pool_init();

    for (i = 0u; i < (uint32_t)UIOX_KIX_PSA_NREGION; i++) {
        if (s_pgtbl_used[i] == 0u) {
            uiox_kix_psa_pte_t *t = s_pgtbl_pool[i];
            uint32_t j;
            for (j = 0u; j < npages; j++) {
                t[j].pte_pfn   = 0u;
                t[j].pte_flags = 0u;
            }
            s_pgtbl_used[i] = 1u;
            return t;
        }
    }
    return (uiox_kix_psa_pte_t *)0;
}

/* ── uiox_kix_psa_pgtbl_free ─────────────────────────────────────────
 * Return a page table to the pool.
 *
 * Identified by BASE ADDRESS, so a pointer into the middle of a table
 * frees nothing rather than the wrong row.  The npages argument is
 * unused: the pool row is a fixed size and freeing is row-granular. */
void uiox_kix_psa_pgtbl_free(uiox_kix_psa_pte_t *tbl, uint32_t npages)
{
    uint32_t i;
    (void)npages;
    if (!tbl) return;
    pgtbl_pool_init();
    for (i = 0u; i < (uint32_t)UIOX_KIX_PSA_NREGION; i++) {
        if (s_pgtbl_pool[i] == tbl) { s_pgtbl_used[i] = 0u; return; }
    }
}

/* ── uiox_kix_psa_pgtbl_copy ─────────────────────────────────────────
 * Copy npages entries from src to dst.
 *
 * dupreg uses this to give the child its own page table over the shared
 * frames.  Entry-by-entry rather than memcpy: the stride is the struct
 * size, and a byte copy of an array of structs would rely on there being
 * no padding — which is true here but not something to depend on. */
void uiox_kix_psa_pgtbl_copy(uiox_kix_psa_pte_t *dst,
                             uiox_kix_psa_pte_t *src, uint32_t npages)
{
    uint32_t i;
    if (!dst || !src) return;
    for (i = 0u; i < npages; i++) dst[i] = src[i];
}

/* ── uiox_kix_psa_pmalloc ────────────────────────────────────────────
 * Bump-allocate size bytes from the flat physical arena.
 *
 * The check is against the TOTAL arena, not against a per-block header,
 * because there are no headers — the bump pointer is the whole state.
 * Returns NULL when the remaining space is too small.
 *
 * Alignment is natural (1 byte).  Callers that need a page-aligned block
 * should round their request up and align their own accesses. */
void *uiox_kix_psa_pmalloc(uint32_t size)
{
    uint8_t *base;

    if (size == 0u) return (void *)0;
    if (uiox_kix_psa_phys_mem_used + size >
        (uint32_t)UIOX_KIX_PSA_PHYS_MEM_SIZE)
        return (void *)0;

    base = &uiox_kix_psa_phys_mem_pool[uiox_kix_psa_phys_mem_used];
    uiox_kix_psa_phys_mem_used += size;
    return (void *)base;
}

/* ── uiox_kix_psa_pmfree ─────────────────────────────────────────────
 * Reclaim an allocation.  A NO-OP, deliberately.
 *
 * A bump allocator cannot return an interior block: the space before it
 * is still live, so reclaiming this one would create a hole the next
 * allocation would happily hand out twice.  Pretending the free happened
 * would let a caller believe memory was returned when it was not, so the
 * function is honest and does nothing.  Reclamation happens by resetting
 * phys_mem_used in region_init, which is only safe once every region is
 * gone. */
void uiox_kix_psa_pmfree(void *addr, uint32_t size)
{
    (void)addr;
    (void)size;
}

/* ── uiox_kix_psa_region_init ────────────────────────────────────────
 * Thread every region onto the free list and reset the arena.
 *
 * Zeroes each descriptor, then links them with r_free_next so allocreg
 * can pop from the head.  The active list starts empty, because nothing
 * is attached yet.  Resetting phys_mem_used reclaims the arena — safe
 * only at initialisation, per pmfree's note. */
void uiox_kix_psa_region_init(void)
{
    uint32_t i;

    for (i = 0u; i < (uint32_t)UIOX_KIX_PSA_NREGION; i++) {
        uiox_kix_psa_region_table[i].r_type      = (uiox_kix_psa_region_type_t)0;
        uiox_kix_psa_region_table[i].r_inode     = (struct inode *)0;
        uiox_kix_psa_region_table[i].r_refcnt    = 0u;
        uiox_kix_psa_region_table[i].r_pgtbl     = (uiox_kix_psa_pte_t *)0;
        uiox_kix_psa_region_table[i].r_npages    = 0u;
        uiox_kix_psa_region_table[i].r_size      = 0u;
        uiox_kix_psa_region_table[i].r_status    = 0u;
        uiox_kix_psa_region_table[i].r_act_next  = (uiox_kix_psa_region_t *)0;
        uiox_kix_psa_region_table[i].r_free_next =
            (i + 1u < (uint32_t)UIOX_KIX_PSA_NREGION)
                ? &uiox_kix_psa_region_table[i + 1u]
                : (uiox_kix_psa_region_t *)0;
    }

    uiox_kix_psa_free_region_list   = &uiox_kix_psa_region_table[0];
    uiox_kix_psa_active_region_list = (uiox_kix_psa_region_t *)0;
    uiox_kix_psa_phys_mem_used      = 0u;

    pgtbl_pool_init();
}

/* ── PSA Algorithm 3 — uiox_kix_psa_allocreg ─────────────────────────
 * Take a region off the free list and mark it active.
 *
 * r_refcnt is left at ZERO deliberately: a freshly allocated region
 * belongs to nobody until attachreg is called, and starting the count at
 * one would make the first detachreg free a region another process is
 * about to attach.  It is also size-zero and page-table-less, so growreg
 * and loadreg have somewhere to build from.
 *
 * Pushed onto the active list so freereg has something to unlink from.
 * Returns NULL when no region is free — the caller should report ENOMEM. */
uiox_kix_psa_region_t *uiox_kix_psa_allocreg(struct inode *ip,
                                             uiox_kix_psa_region_type_t type)
{
    uiox_kix_psa_region_t *rp = uiox_kix_psa_free_region_list;

    if (!rp) return (uiox_kix_psa_region_t *)0;

    uiox_kix_psa_free_region_list = rp->r_free_next;
    rp->r_free_next = (uiox_kix_psa_region_t *)0;

    rp->r_type   = type;
    rp->r_inode  = ip;
    rp->r_refcnt = 0u;
    rp->r_pgtbl  = (uiox_kix_psa_pte_t *)0;
    rp->r_npages = 0u;
    rp->r_size   = 0u;
    rp->r_status = 0u;

    rp->r_act_next = uiox_kix_psa_active_region_list;
    uiox_kix_psa_active_region_list = rp;

    return rp;
}

/* ── PSA Algorithm 4 — uiox_kix_psa_attachreg ────────────────────────
 * Attach a region to a process at a chosen address.
 *
 * The pregion slot lives in the caller's U AREA, not on the process table
 * entry — the mapping is private to this process, and PSA keeps u-area
 * state out of the table.  The loop scans for the first invalid slot and
 * takes it.
 *
 * pr_vaddr records where THIS process sees the region, which is why two
 * processes sharing one region can have different pr_vaddr values.
 * pr_size caches r_size at attach time.
 *
 * The reference count rises, so the region survives either process
 * detaching.  Returns NULL when the process already holds
 * MAX_REG_PER_PROC regions — a real limit, not an error to retry. */
uiox_kix_psa_pregion_t *uiox_kix_psa_attachreg(uiox_kix_psa_region_t *rp,
                                               struct uiox_kix_psa_proc *p,
                                               uintptr_t vaddr,
                                               uiox_kix_psa_region_type_t type)
{
    int i;

    if (!rp || !p) return (uiox_kix_psa_pregion_t *)0;

    for (i = 0; i < UIOX_KIX_PSA_MAX_REG_PER_PROC; i++) {
        uiox_kix_psa_pregion_t *prp = &uiox_kix_psa_u.u_pregs[i];

        if (prp->pr_valid) continue;

        prp->pr_region = rp;
        prp->pr_proc   = p;
        prp->pr_type   = type;
        prp->pr_vaddr  = vaddr;
        prp->pr_size   = rp->r_size;
        prp->pr_valid  = 1;

        rp->r_refcnt++;
        return prp;
    }
    return (uiox_kix_psa_pregion_t *)0;
}

/* ── PSA Algorithm 5 — uiox_kix_psa_growreg ──────────────────────────
 * Change a region's size by delta, which may be negative.
 *
 * Two bounds, and BOTH matter:
 *
 *   upper — PSA caps a region at MAX_REGION_SIZE.  Exceeding it sets
 *           u_error to ENOMEM, which is what a caller expanding its data
 *           segment needs to hear.
 *
 *   lower — a shrink below zero sets u_error to EINVAL.  This is the
 *           check the original omitted, and without it newsize wraps
 *           through the uint32 conversion and r_npages becomes enormous.
 *
 * r_npages is recomputed by rounding UP, so a size that is not a whole
 * number of pages still gets the page its tail bytes live in.  A region
 * grown from nothing allocates its page table here.
 *
 * The size change is invisible until pr_size is refreshed, so both are
 * kept in step. */
void uiox_kix_psa_growreg(uiox_kix_psa_pregion_t *prp, int32_t delta)
{
    uiox_kix_psa_region_t *rp;
    int64_t                newsize;

    if (!prp || !prp->pr_region) return;
    rp = prp->pr_region;

    newsize = (int64_t)rp->r_size + (int64_t)delta;

    if (newsize > (int64_t)UIOX_KIX_PSA_MAX_REGION_SIZE) {
        uiox_kix_psa_u.u_error = ENOMEM;
        return;
    }
    if (newsize < 0) {
        uiox_kix_psa_u.u_error = EINVAL;
        return;
    }

    rp->r_size   = (uint32_t)newsize;
    rp->r_npages = (uint32_t)(((uint32_t)newsize +
                               UIOX_KIX_PSA_PAGE_SIZE - 1u) /
                              UIOX_KIX_PSA_PAGE_SIZE);
    prp->pr_size = rp->r_size;

    if (rp->r_npages && !rp->r_pgtbl)
        rp->r_pgtbl = uiox_kix_psa_pgtbl_alloc(rp->r_npages);
}

/* ── PSA Algorithm 6 — uiox_kix_psa_loadreg ──────────────────────────
 * Load a file's contents into a region.
 *
 * It marks the region LOADING and returns.  It does NOT mark pages VALID
 * and does NOT report how many bytes it read, because no
 * readi()-equivalent is wired from 01_fsa, so no byte actually moves.
 *
 * This is the honest behaviour rather than the convenient one: setting
 * REG_VALID here would announce data that was never read, and the
 * consumer would then map whatever happened to be in the frames.  That
 * is the same defect as fabricating a file header and then validating
 * the magic against what was written.
 *
 * TO FINISH: read through the inode path in 01_fsa, filling pages and
 * marking each VALID as its contents arrive. */
void uiox_kix_psa_loadreg(uiox_kix_psa_pregion_t *prp, uintptr_t vaddr,
                          struct inode *ip, uint32_t file_off,
                          uint32_t byte_count)
{
    (void)vaddr;
    (void)ip;
    (void)file_off;
    (void)byte_count;

    if (!prp || !prp->pr_region) return;

    prp->pr_region->r_status |= UIOX_KIX_PSA_REG_LOADING;
}

/* ── PSA Algorithm 7 — uiox_kix_psa_freereg ──────────────────────────
 * Return a region to the free list.
 *
 * Refuses while r_refcnt is non-zero: another process still maps these
 * frames, so freeing now would leave it with a page table pointing at
 * recycled memory.  The check is not defensive — it is the only thing
 * preventing that.
 *
 * Frees the page table first, then unlinks from the active list by
 * walking it, then pushes onto the free list.  The walk handles the
 * head case, which is where a region usually is after a fresh alloc. */
void uiox_kix_psa_freereg(uiox_kix_psa_region_t *rp)
{
    if (!rp) return;
    if (rp->r_refcnt > 0u) return;

    if (rp->r_pgtbl) {
        uiox_kix_psa_pgtbl_free(rp->r_pgtbl, rp->r_npages);
        rp->r_pgtbl = (uiox_kix_psa_pte_t *)0;
    }

    {
        uiox_kix_psa_region_t *cur  = uiox_kix_psa_active_region_list;
        uiox_kix_psa_region_t *prev = (uiox_kix_psa_region_t *)0;

        while (cur) {
            if (cur == rp) {
                if (prev) prev->r_act_next = cur->r_act_next;
                else      uiox_kix_psa_active_region_list = cur->r_act_next;
                break;
            }
            prev = cur;
            cur  = cur->r_act_next;
        }
    }

    rp->r_act_next  = (uiox_kix_psa_region_t *)0;
    rp->r_free_next = uiox_kix_psa_free_region_list;
    uiox_kix_psa_free_region_list = rp;
}

/* ── PSA Algorithm 8 — uiox_kix_psa_detachreg ────────────────────────
 * Remove one process's attachment to a region.
 *
 * Decrements the count and, when it reaches zero, frees the region too —
 * so a caller does not have to pair detachreg with freereg and can never
 * leak a region the last process let go of.
 *
 * Clearing pr_valid LAST is what makes this safe to call twice: the guard
 * at the top returns immediately on an already-detached pregion, so the
 * count is not decremented twice for one attachment.
 *
 * pr_proc and pr_vaddr are cleared so a stale pregion cannot be
 * mistaken for a live mapping. */
void uiox_kix_psa_detachreg(uiox_kix_psa_pregion_t *prp)
{
    if (!prp || !prp->pr_valid) return;

    if (prp->pr_region && prp->pr_region->r_refcnt > 0u)
        prp->pr_region->r_refcnt--;

    if (prp->pr_region && prp->pr_region->r_refcnt == 0u)
        uiox_kix_psa_freereg(prp->pr_region);

    prp->pr_region = (uiox_kix_psa_region_t *)0;
    prp->pr_proc   = (struct uiox_kix_psa_proc *)0;
    prp->pr_vaddr  = 0u;
    prp->pr_size   = 0u;
    prp->pr_valid  = 0;
}

/* ── PSA Algorithm 9 — uiox_kix_psa_dupreg ───────────────────────────
 * Duplicate a region for the child in fork.
 *
 * Takes a fresh region, gives it its own page table over the SAME frames,
 * and clears the write permission on every writable entry while setting
 * PTE_COW.
 *
 * That flag is the whole mechanism.  02_MemMngnt's fault handler tests it
 * and copies the frame on first write, which is what lets parent and
 * child share until one of them actually modifies a page.  Without it,
 * both processes would hold a writable mapping to one frame and silently
 * corrupt each other's data.
 *
 * Returns an UNATTACHED region.  The child is not mapped yet — the caller
 * attaches it with attachreg.  That ordering is PSA's own: fork runs
 * dupreg, then attachreg.
 *
 * On failure the partially built region is returned to the free list, so
 * a failed fork does not leak one. */
uiox_kix_psa_region_t *uiox_kix_psa_dupreg(uiox_kix_psa_region_t *rp)
{
    uiox_kix_psa_region_t *newrp;

    if (!rp) return (uiox_kix_psa_region_t *)0;

    newrp = uiox_kix_psa_allocreg(rp->r_inode, rp->r_type);
    if (!newrp) return (uiox_kix_psa_region_t *)0;

    newrp->r_size   = rp->r_size;
    newrp->r_npages = rp->r_npages;

    if (rp->r_pgtbl && rp->r_npages) {
        uint32_t i;

        newrp->r_pgtbl = uiox_kix_psa_pgtbl_alloc(rp->r_npages);
        if (!newrp->r_pgtbl) {
            uiox_kix_psa_freereg(newrp);
            return (uiox_kix_psa_region_t *)0;
        }

        uiox_kix_psa_pgtbl_copy(newrp->r_pgtbl, rp->r_pgtbl, rp->r_npages);

        for (i = 0u; i < rp->r_npages; i++) {
            if (newrp->r_pgtbl[i].pte_flags & UIOX_KIX_PSA_PTE_WRITE) {
                newrp->r_pgtbl[i].pte_flags &= ~UIOX_KIX_PSA_PTE_WRITE;
                newrp->r_pgtbl[i].pte_flags |=  UIOX_KIX_PSA_PTE_COW;
            }
        }
    }

    newrp->r_status = rp->r_status;
    return newrp;
}
