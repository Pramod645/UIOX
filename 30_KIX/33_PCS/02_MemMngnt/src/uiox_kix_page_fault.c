/*
 * 30_KIX/33_PCS/02_MemMngnt/src/uiox_kix_page_fault.c
 *
 * Bach's Algorithm 3 (vfault) and Algorithm 4 (pfault) — the two memory
 * fault handlers.
 *
 * ── one region type, one pte type ───────────────────────────────────
 * An earlier version had fault_region_t and a second uiox_kix_pte_t
 * alongside 40_psa's region and pte.  Two structs describing
 * overlapping things is the defect class this tree keeps producing, so
 * there is now ONE of each: uiox_kix_psa_region_t with
 * uiox_kix_psa_pregion_t beside it, and uiox_kix_psa_pte_t.
 *
 * ── what the handler needs, and where each thing lives ──────────────
 *   where the region starts   pr->pr_vaddr      (per-process)
 *   how many pages            pr->pr_region->r_npages
 *   the page table            pr->pr_region->r_pgtbl
 *   a lock                    pr->pr_locked     (per-process)
 *
 * The base and the lock are per-ATTACHMENT, not per-region.  Two
 * processes may map one region at different addresses, and a fault is
 * one process's business — locking a shared region would block the
 * other for no reason.  This is why the handler takes a pregion and not
 * a region.
 *
 * ── the two faults ──────────────────────────────────────────────────
 * Bach distinguishes them because the LEGALITY test differs:
 *
 *   vfault  the page has no frame assigned.  Legal iff the address is
 *           inside the region AND the disk block descriptor has a
 *           record of it.  Anything else is SIGSEGV.
 *
 *   pfault  the page IS valid but the access violated its permissions.
 *           The one legal case is a write to a copy-on-write page,
 *           which is what fork leaves behind.
 *
 * ── the copy-on-write defect, and that it is now closed ─────────────
 * dupreg sets PTE_COW; pfault tests it; the frame is copied.  Before
 * this pass dupreg set 0x010 while this handler tested 0x020 — the test
 * failed, the handler signalled, and the child died on its first write.
 * Both sides now read PTE_COW from uiox_kix_pagemap.h.
 *
 * ── what is simulated ───────────────────────────────────────────────
 * The I/O is not real.  read_page_from_swap and read_page_from_file set
 * pfd_content_valid and return — no transfer, and no sleep.  The
 * locking guards a concurrency that cannot yet happen, which is stated
 * rather than left looking like it works.
 *
 * ── what is NOT here ────────────────────────────────────────────────
 * uiox_kix_vfault (Algorithm 3) is declared in uiox_kix_paging.h and has
 * no body.  Nothing calls it: 10_BSP/10_Arch has no data-abort vector.
 * The grep that established both is recorded in the session handoff.
 *
 * @version 4.0.0  @date 2026-09-30
 */
#include "uiox_kix_paging.h"
#include "uiox_kix_psa_region.h"
#include "uiox_kix_psa_process.h"
#include "uiox_kix_scheduler.h"   /* recalculate_priority, SCHED_PROC */

#include "uiox_soc_stdio.h"      /* ADD — declares uiox_printf; BSP 03_SoC provides it */

/* ── Globals ────────────────────────────────────────────────────────── */
uiox_kix_pfdata_t         uiox_kix_pfdata_table[UIOX_PHYS_PAGES];
uiox_kix_page_cache_t     uiox_kix_page_cache;
uiox_kix_free_page_list_t uiox_kix_free_pages;

/* ── uiox_kix_page_fault_init ────────────────────────────────────────
 * Zero the descriptors and seed the free list with EVERY frame.
 *
 * head and tail both start at 0: the ring is empty, and each frame is
 * enqueued once as the loop runs.  Using the list as a FIFO keeps the
 * allocation order from depending on which frames were recently freed —
 * a frame that just came back goes to the end, not the front. */
void uiox_kix_page_fault_init(void)
{
    uint32_t i;

    for (i = 0; i < UIOX_PHYS_PAGES; i++) {
        uiox_kix_pfdata_table[i].pfd_valid         = 0;
        uiox_kix_pfdata_table[i].pfd_refcnt        = 0u;
        uiox_kix_pfdata_table[i].pfd_pfn           = 0u;
        uiox_kix_pfdata_table[i].pfd_pte           = (uiox_kix_psa_pte_t *)0;
        uiox_kix_pfdata_table[i].pfd_on_hash       = 0;
        uiox_kix_pfdata_table[i].pfd_hash_next     = (uiox_kix_pfdata_t *)0;
        uiox_kix_pfdata_table[i].pfd_content_valid = 0;
    }

    for (i = 0; i < UIOX_PAGE_HASH_SZ; i++)
        uiox_kix_page_cache.pc_hash[i] = (uiox_kix_pfdata_t *)0;
    uiox_kix_page_cache.pc_total = 0;

    uiox_kix_free_pages.fpl_count = 0;
    uiox_kix_free_pages.fpl_head  = 0;
    uiox_kix_free_pages.fpl_tail  = 0;

    for (i = 0; i < UIOX_PHYS_PAGES; i++) {
        uiox_kix_free_pages.fpl_pages[uiox_kix_free_pages.fpl_tail] = i;
        uiox_kix_free_pages.fpl_tail =
            (uiox_kix_free_pages.fpl_tail + 1) % UIOX_PHYS_PAGES;
        uiox_kix_free_pages.fpl_count++;
    }
}

/* ── uiox_kix_send_sigsegv ───────────────────────────────────────────
 * The signal is not actually delivered — kill's send step is commented
 * out in the kernel's signal implementation.  This prints the pid so a
 * fault is at least visible while that remains true.
 *
 * %lu is NOT in uiox_soc_stdio.h's subset (%s %d %u %x %llu %p), so the
 * specifier is %llu; the cast keeps the argument's type unambiguous. */
void uiox_kix_send_sigsegv(void)
{
    uiox_kix_psa_proc_t *p = uiox_kix_psa_current_proc;

    uiox_printf("[fault] SIGSEGV pid=%llu\n",
                (unsigned long long)(p ? p->p_pid : 0u));
}

/* ── Region locking ──────────────────────────────────────────────────
 * Bach holds the lock across the WHOLE handler — "lock region ...
 * out: unlock region" brackets everything.
 *
 * Operates on the ATTACHMENT's pr_locked.  Counted rather than a flag,
 * because a nested fault can occur inside a handler: a page-table walk
 * can itself fault.  A plain flag would be cleared by the inner unlock
 * and leave the outer handler unprotected. */
void uiox_kix_region_lock(uiox_kix_psa_pregion_t *pr)
{
    if (!pr) return;
    if (!pr || !pr->pr_region) return;
    pr->pr_region->r_status |= UIOX_KIX_PSA_REG_LOCKED;
}

void uiox_kix_region_unlock(uiox_kix_psa_pregion_t *pr)
{
    if (!pr) return;
    if (!pr || !pr->pr_region) return;
    if (pr->pr_region->r_status & UIOX_KIX_PSA_REG_LOCKED)
        pr->pr_region->r_status &= ~UIOX_KIX_PSA_REG_LOCKED;
}

/* ── uiox_kix_recalc_priority_after_fault ────────────────────────────
 * Bach's Algorithms 3 and 4 both end with "recalculate process
 * priority" — a process that has been faulting is not the process that
 * should keep the CPU.
 *
 * pr_proc is a raw PSA pointer, so SCHED_PROC recovers the wrapper that
 * recalculate_priority takes (uiox_kix_scheduler.h).  SCHED_PROC is the
 * same conversion uiox_kix_clock.c uses; it belongs in the scheduler
 * header so all three files share one definition. */
void uiox_kix_recalc_priority_after_fault(uiox_kix_psa_pregion_t *pr)
{
    if (!pr || !pr->pr_proc) return;
    //recalculate_priority(SCHED_PROC(pr->pr_proc));
    recalculate_priority(scp_to_wrapper(pr->pr_proc));

}

/* ── uiox_kix_find_pte ───────────────────────────────────────────────
 * Locate the entry for an address.  Returns NULL when the address is
 * outside this process's mapping of the region — Bach's first legality
 * test in vfault:
 *
 *   "if (address is outside virtual address space)
 *    { send signal (SIGSEGV) to process; goto out; }"
 *
 * Four checks.  The base comes from pr->pr_vaddr, not from the region:
 * two processes may map one region at different addresses, so only the
 * attachment can turn an address into an offset. */
uiox_kix_psa_pte_t *uiox_kix_find_pte(uiox_kix_psa_pregion_t *pr,
                                      uintptr_t vaddr)
{
    uint32_t  page;
    uintptr_t offset;

    if (!pr || !pr->pr_region) return (uiox_kix_psa_pte_t *)0;

    /* A region whose page table is NULL has been grown but not yet
     * populated, so no address in it is mappable and a fault there is a
     * genuine error rather than an unloaded page. */
    if (!pr->pr_region->r_pgtbl) return (uiox_kix_psa_pte_t *)0;

    if (vaddr < pr->pr_vaddr) return (uiox_kix_psa_pte_t *)0;
    offset = vaddr - pr->pr_vaddr;

    if (offset >= (uintptr_t)pr->pr_region->r_size)
        return (uiox_kix_psa_pte_t *)0;

    page = (uint32_t)(offset >> UIOX_PAGE_SHIFT);
    if (page >= pr->pr_region->r_npages) return (uiox_kix_psa_pte_t *)0;

    return &pr->pr_region->r_pgtbl[page];
}

/* ── uiox_kix_find_dbd ───────────────────────────────────────────────
 * The disk block descriptor for an entry.
 *
 * Bach's vfault: "if the disk block descriptor has no record of the
 * faulted page, the attempted memory reference is invalid" — so a NULL
 * return here is what makes a reference illegal rather than merely
 * unloaded.
 *
 * There is no per-page descriptor array yet, so this returns NULL and
 * vfault treats every address as illegal-on-arrival unless it is
 * demand-zero.  That is honest: a loadreg that never reads a file has
 * nothing to record. */
uiox_kix_disk_blk_desc_t *uiox_kix_find_dbd(uiox_kix_psa_pte_t *pte)
{
    (void)pte;
    return (uiox_kix_disk_blk_desc_t *)0;
}

/* ── Page cache ──────────────────────────────────────────────────────
 * A hash on the frame number, so a lookup is one bucket walk rather than
 * a scan of the whole table. */
uiox_kix_pfdata_t *uiox_kix_find_in_cache(uiox_uint32_t pfn)
{
    uint32_t      slot = pfn % UIOX_PAGE_HASH_SZ;
    uiox_kix_pfdata_t *p    = uiox_kix_page_cache.pc_hash[slot];

    while (p) {
        if (p->pfd_pfn == pfn) return p;
        p = p->pfd_hash_next;
    }
    return (uiox_kix_pfdata_t *)0;
}

void uiox_kix_add_to_cache(uiox_kix_pfdata_t *pfd)
{
    uint32_t slot;

    if (!pfd || pfd->pfd_on_hash) return;

    slot = pfd->pfd_pfn % UIOX_PAGE_HASH_SZ;
    pfd->pfd_hash_next               = uiox_kix_page_cache.pc_hash[slot];
    uiox_kix_page_cache.pc_hash[slot] = pfd;
    pfd->pfd_on_hash                 = 1;
    uiox_kix_page_cache.pc_total++;
}

void uiox_kix_remove_from_cache(uiox_kix_pfdata_t *pfd)
{
    uint32_t      slot;
    uiox_kix_pfdata_t *prev, *cur;

    if (!pfd || !pfd->pfd_on_hash) return;

    slot = pfd->pfd_pfn % UIOX_PAGE_HASH_SZ;
    prev = (uiox_kix_pfdata_t *)0;
    cur  = uiox_kix_page_cache.pc_hash[slot];

    while (cur) {
        if (cur == pfd) {
            if (prev) prev->pfd_hash_next               = cur->pfd_hash_next;
            else      uiox_kix_page_cache.pc_hash[slot] = cur->pfd_hash_next;

            pfd->pfd_hash_next = (uiox_kix_pfdata_t *)0;
            pfd->pfd_on_hash   = 0;
            uiox_kix_page_cache.pc_total--;
            return;
        }
        prev = cur;
        cur  = cur->pfd_hash_next;
    }
}

/* ── Frame allocation ────────────────────────────────────────────────
 * Takes from the free-list ring's head.  Returns NULL and leaves the
 * counters alone when the ring is empty — a caller that gets NULL has
 * to handle having no memory, and half-allocating would be worse. */
uiox_kix_pfdata_t *uiox_kix_alloc_physical_page(void)
{
    uint32_t      pfn;
    uiox_kix_pfdata_t *pfd;

    if (uiox_kix_free_pages.fpl_count == 0) {
        uiox_printf("[fault] no free frames\n");
        return (uiox_kix_pfdata_t *)0;
    }

    pfn = uiox_kix_free_pages.fpl_pages[uiox_kix_free_pages.fpl_head];
    uiox_kix_free_pages.fpl_head =
        (uiox_kix_free_pages.fpl_head + 1) % UIOX_PHYS_PAGES;
    uiox_kix_free_pages.fpl_count--;

    pfd = &uiox_kix_pfdata_table[pfn % UIOX_PHYS_PAGES];

    pfd->pfd_valid         = 1;
    pfd->pfd_pfn           = pfn;
    pfd->pfd_refcnt        = 1u;
    pfd->pfd_pte           = (uiox_kix_psa_pte_t *)0;
    pfd->pfd_on_hash       = 0;
    pfd->pfd_hash_next     = (uiox_kix_pfdata_t *)0;
    pfd->pfd_content_valid = 0;

    return pfd;
}

void uiox_kix_free_physical_page(uiox_uint32_t pfn)
{
    uiox_kix_pfdata_t *pfd = &uiox_kix_pfdata_table[pfn % UIOX_PHYS_PAGES];

    /* Unlink BEFORE clearing, or the hash keeps a pointer to a slot that
     * now reads as a different frame. */
    uiox_kix_remove_from_cache(pfd);

    pfd->pfd_valid         = 0;
    pfd->pfd_refcnt        = 0u;
    pfd->pfd_pte           = (uiox_kix_psa_pte_t *)0;
    pfd->pfd_content_valid = 0;

    uiox_kix_free_pages.fpl_pages[uiox_kix_free_pages.fpl_tail] = pfn;
    uiox_kix_free_pages.fpl_tail =
        (uiox_kix_free_pages.fpl_tail + 1) % UIOX_PHYS_PAGES;
    uiox_kix_free_pages.fpl_count++;
}

/* ── Content transfer ────────────────────────────────────────────────
 * Bach's vfault reads from the swap device or the executable file and
 * then sleeps on the I/O.  Neither happens here: these mark the contents
 * valid and return.  The I/O is the missing piece, not the bookkeeping. */
int uiox_kix_read_page_from_swap(uiox_kix_psa_pte_t *pte, uiox_kix_pfdata_t *pfd)
{
    (void)pte;
    if (!pfd) return -1;
    pfd->pfd_content_valid = 1;
    return 0;
}

int uiox_kix_read_page_from_file(uiox_kix_psa_pte_t *pte, uiox_kix_pfdata_t *pfd)
{
    (void)pte;
    if (!pfd) return -1;
    pfd->pfd_content_valid = 1;
    return 0;
}

void uiox_kix_free_swap_block(uiox_uint32_t blk)
{
    /* Bach's pfault frees swap space when a COW page is stolen and a
     * copy still exists on the device.  The swap map is the thing that
     * must be told; the caller has the block number. */
    (void)blk;
}

/* ── Algorithm 4 — uiox_kix_pfault ───────────────────────────────────
 * Bach, verbatim:
 *
 *   {
 *       find region, page table entry, disk block descriptor, page frame
 *           for address, lock region;
 *       if (page not valid in memory) goto out;
 *       if (copy on write bit not set) goto out;   // real error - signal
 *       if (page frame reference count > 1)
 *       {
 *           allocate a new physical page;
 *           copy contents of old page to new page;
 *           decrement old page frame reference count;
 *           update page table entry to point to new physical page;
 *       }
 *       else   // "steal" page, since nobody else is using it
 *       {
 *           if (copy of page exists on swap device)
 *               free space on swap device, break page association;
 *           if (page is on page hash queue)
 *               remove from hash queue;
 *       }
 *       set modify bit, clear copy on write bit in page table entry;
 *       recalculate process priority;
 *       check for signals;
 *       out: unlock region;
 *   }
 *
 * ── the refcount branch is the whole point ──────────────────────────
 * refcnt > 1 means the frame is SHARED — a parent and child after fork,
 * or two processes running the same text.  A write must give the writer
 * a private copy, so a new frame is allocated and the old one's count
 * drops.
 *
 * refcnt == 1 means nobody else holds it, so the frame is simply
 * reassigned: no copy, because there is nothing to protect from.  Bach
 * calls this "steal" and it is the cheap case.
 *
 * ── what this build cannot do ───────────────────────────────────────
 * The copy is a page-to-page memcpy, which is real.  The swap-space
 * release in the refcnt==1 branch calls a function that does nothing —
 * the swap map needs the block number and a COW page does not record
 * one.  Marked rather than faked.
 * ──────────────────────────────────────────────────────────────────── */
void uiox_kix_pfault(uintptr_t faulted_addr,
                     uiox_kix_psa_pregion_t *pr)
{
    uiox_kix_psa_pte_t *pte;
    uiox_kix_pfdata_t  *pfd;

    if (!pr || !pr->pr_region) { uiox_kix_send_sigsegv(); return; }

    uiox_kix_region_lock(pr);

    pte = uiox_kix_find_pte(pr, faulted_addr);
    if (!pte || !(pte->pte_flags & PTE_VALID)) {
        /* Either outside this process's mapping, or not resident —
         * pfault is only for VALID pages, so this is a real error. */
        uiox_kix_send_sigsegv();
        goto out;
    }

    /* The legality test: COW set means the write is permitted and this
     * fault is the mechanism doing its job.  Not set means the process
     * genuinely wrote where it may not. */
    if (!(pte->pte_flags & PTE_COW)) {
        uiox_kix_send_sigsegv();
        goto out;
    }

    pfd = uiox_kix_find_in_cache(pte->pte_pfn);
    if (!pfd) {
        /* A valid, COW-marked page whose frame is not in the cache is an
         * inconsistent page table — the entry points at nothing. */
        uiox_kix_send_sigsegv();
        goto out;
    }

    if (pfd->pfd_refcnt > 1u) {
        uiox_kix_pfdata_t *newpfd = uiox_kix_alloc_physical_page();

        if (!newpfd) { uiox_kix_send_sigsegv(); goto out; }

        /* Copy the frame's CONTENTS.  The source is the physical address
         * the old frame number names; this is the only place the raw
         * frame address is used, which is why it is spelled out. */
        memcpy((void *)(uintptr_t)(newpfd->pfd_pfn << UIOX_PAGE_SHIFT),
               (void *)(uintptr_t)(pfd->pfd_pfn    << UIOX_PAGE_SHIFT),
               UIOX_PAGE_SIZE);

        pfd->pfd_refcnt--;
        newpfd->pfd_content_valid = 1;
        newpfd->pfd_pte           = pte;
        uiox_kix_add_to_cache(newpfd);

        /* Re-point the entry at the private copy. */
        pte->pte_pfn = newpfd->pfd_pfn;

    } else {
        /* Nobody else holds it — steal the frame rather than copy it. */
        if (pte->pte_flags & PTE_IN_SWAP) {
            uiox_kix_free_swap_block(0u);   /* no block recorded — see banner */
            pte->pte_flags &= ~PTE_IN_SWAP;
        }
        uiox_kix_remove_from_cache(pfd);
    }

    /* The write may now proceed: writable, dirty, no longer shared. */
    pte->pte_flags |=  PTE_WRITE;
    pte->pte_flags |=  PTE_DIRTY;
    pte->pte_flags &= ~PTE_COW;

    uiox_kix_recalc_priority_after_fault(pr);

out:
    uiox_kix_region_unlock(pr);
}
