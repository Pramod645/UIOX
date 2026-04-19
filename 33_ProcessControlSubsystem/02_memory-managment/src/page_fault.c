#include "../include/page_fault.h"
#include "../include/scheduler.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ── Globals ────────────────────────────────────────────────── */
pfdata_t         pfdata_table[PHYS_PAGES];
page_cache_t     page_cache;
free_page_list_t free_pages;

/* ── region_lock / region_unlock ─────────────────────────────
 * Lock a region to prevent the page stealer from racing.
 */
void region_lock(fault_region_t *r)
{
    if (r) { r->fr_locked = 1; }
}

void region_unlock(fault_region_t *r)
{
    if (r) { r->fr_locked = 0; }
}

/* ── send_sigsegv ────────────────────────────────────────────
 * Deliver SIGSEGV to the current process.
 */
void send_sigsegv(void)
{
    printf("[fault] SIGSEGV sent to pid=%u\n",
           current_proc ? current_proc->pe_pid : 0);
    /* send_signal(current_proc, SIGSEGV); */
}

/* ── find_pte ────────────────────────────────────────────────
 * Locate the page table entry for a given virtual address
 * within a region.
 */
pte_t *find_pte(fault_region_t *region, uintptr_t vaddr)
{
    if (!region || !region->fr_pgtbl) return NULL;
    if (vaddr < region->fr_vaddr_start) return NULL;

    uintptr_t offset = vaddr - region->fr_vaddr_start;
    uint32_t  page   = (uint32_t)(offset >> PAGE_SHIFT);

    if (page >= region->fr_npages) return NULL;
    return &region->fr_pgtbl[page];
}

/* ── find_dbd ────────────────────────────────────────────────
 * Return the disk block descriptor for a page table entry.
 * (In a real kernel this is stored alongside the PTE.)
 */
disk_blk_desc_t *find_dbd(pte_t *pte)
{
    if (!pte) return NULL;
    /* Simulated: construct descriptor from PTE fields */
    static disk_blk_desc_t dbd;
    dbd.dbd_valid    = (pte->pte_swap_blk != 0);
    dbd.dbd_swap_blk = pte->pte_swap_blk;
    dbd.dbd_file_blk = 0;
    dbd.dbd_type     = 0;
    return &dbd;
}

/* ── Page hash helpers ───────────────────────────────────────
 */
static int page_hash(uint32_t pfn)
{
    return (int)(pfn % PAGE_HASH_SZ);
}

pfdata_t *find_in_cache(uint32_t pfn)
{
    int bucket = page_hash(pfn);
    for (pfdata_t *p = page_cache.pc_hash[bucket];
         p; p = p->pfd_hash_next)
        if (p->pfd_pfn == pfn && p->pfd_valid) return p;
    return NULL;
}

void add_to_cache(pfdata_t *pfd)
{
    if (!pfd) return;
    int bucket = page_hash(pfd->pfd_pfn);
    pfd->pfd_hash_next           = page_cache.pc_hash[bucket];
    page_cache.pc_hash[bucket]   = pfd;
    pfd->pfd_on_hash             = 1;
    page_cache.pc_total++;
}

void remove_from_cache(pfdata_t *pfd)
{
    if (!pfd || !pfd->pfd_on_hash) return;
    int bucket = page_hash(pfd->pfd_pfn);
    pfdata_t **pp = &page_cache.pc_hash[bucket];
    while (*pp) {
        if (*pp == pfd) {
            *pp = pfd->pfd_hash_next;
            pfd->pfd_on_hash = 0;
            page_cache.pc_total--;
            return;
        }
        pp = &(*pp)->pfd_hash_next;
    }
}

/* ── alloc_physical_page ─────────────────────────────────────
 * Remove and return one page from the free page list.
 */
pfdata_t *alloc_physical_page(void)
{
    if (free_pages.fpl_count == 0) {
        fprintf(stderr, "[fault] out of physical pages\n");
        return NULL;
    }
    uint32_t pfn = free_pages.fpl_pages[free_pages.fpl_head];
    free_pages.fpl_head =
        (free_pages.fpl_head + 1) % PHYS_PAGES;
    free_pages.fpl_count--;

    pfdata_t *pfd = &pfdata_table[pfn % PHYS_PAGES];
    memset(pfd, 0, sizeof(pfdata_t));
    pfd->pfd_valid  = 1;
    pfd->pfd_pfn    = pfn;
    pfd->pfd_refcnt = 1;

    printf("[fault] allocated physical page pfn=%u\n", pfn);
    return pfd;
}

/* ── free_physical_page ──────────────────────────────────────
 * Return a page to the free list.
 */
void free_physical_page(uint32_t pfn)
{
    pfdata_t *pfd = &pfdata_table[pfn % PHYS_PAGES];
    memset(pfd, 0, sizeof(pfdata_t));
    free_pages.fpl_pages[free_pages.fpl_tail] = pfn;
    free_pages.fpl_tail =
        (free_pages.fpl_tail + 1) % PHYS_PAGES;
    free_pages.fpl_count++;
    printf("[fault] freed physical page pfn=%u\n", pfn);
}

/* ── free_swap_block ─────────────────────────────────────────
 * Release a swap block back to the swap map.
 */
void free_swap_block(uint32_t blk)
{
    printf("[fault] freed swap block=%u\n", blk);
    /* map_free(&swap_map, blk, 1); */
}

/* ── read_page_from_swap ─────────────────────────────────────
 * Simulate reading a page from the swap device.
 */
int read_page_from_swap(pte_t *pte, pfdata_t *pfd)
{
    printf("[fault] reading page pfn=%u from "
           "swap blk=%u\n", pfd->pfd_pfn, pte->pte_swap_blk);
    /* In real kernel: initiate block I/O and sleep */
    pfd->pfd_content_valid = 1;
    return 0;
}

/* ── read_page_from_file ─────────────────────────────────────
 * Simulate reading a page from the executable file.
 */
int read_page_from_file(pte_t *pte, pfdata_t *pfd)
{
    (void)pte;
    printf("[fault] reading page pfn=%u from exec file\n",
           pfd->pfd_pfn);
    pfd->pfd_content_valid = 1;
    return 0;
}

/* ── recalc_priority_after_fault ─────────────────────────────
 * Boost the priority of the current process slightly after
 * handling a page fault (it just did I/O-style work).
 */
void recalc_priority_after_fault(void)
{
    if (!current_proc) return;
    recalc_priority(current_proc);
}

/* ─────────────────────────────────────────────────────────────
 * 3. Algorithm vfault
 *    input : virtual address that caused the fault
 *    output: none
 *
 *    Validity fault handler — page not present in memory.
 */
void vfault(uintptr_t faulted_addr, fault_region_t *region)
{
    pte_t           *pte = NULL;
    disk_blk_desc_t *dbd = NULL;
    pfdata_t        *pfd = NULL;

    printf("[vfault] fault at vaddr=0x%lx\n",
           (unsigned long)faulted_addr);

    /* Find region, page table entry, disk block descriptor;
     * lock region to prevent race with page stealer          */
    region_lock(region);

    pte = find_pte(region, faulted_addr);

    /* Address outside virtual address space? */
    if (!pte) {
        printf("[vfault] address outside VAS — SIGSEGV\n");
        send_sigsegv();
        goto out;
    }

    dbd = find_dbd(pte);

    /* If address is now valid (another process resolved it) */
    if (pte->pte_flags & PTE_VALID) {
        printf("[vfault] page already valid (race resolved)\n");
        goto out;
    }

    if (pte->pte_flags & PTE_IN_SWAP) {
        /* ── Page is on swap device ───────────────────────── */

        /* Check page cache first */
        pfd = find_in_cache(pte->pte_pfn);
        if (pfd) {
            /* Remove page from cache */
            remove_from_cache(pfd);

            /* Adjust page table entry */
            pte->pte_pfn = pfd->pfd_pfn;

            /* Wait while another process fills page contents */
            while (!pfd->pfd_content_valid) {
                printf("[vfault] waiting for page contents\n");
                /* proc_sleep(event_page_valid, PRIBIO, 0); */
                /* Simulation: break after one check         */
                break;
            }
        } else {
            /* ── Page not in cache ── */

            /* Assign new physical page to region */
            pfd = alloc_physical_page();
            if (!pfd) { send_sigsegv(); goto out; }

            /* Place in cache and update pfdata entry */
            add_to_cache(pfd);

            if (!dbd || !dbd->dbd_valid) {
                /* Page not previously loaded — demand zero */
                if (pte->pte_flags & PTE_DEMAND_ZERO) {
                    printf("[vfault] demand zero fill\n");
                    memset((void *)(uintptr_t)(pfd->pfd_pfn
                           << PAGE_SHIFT), 0, PAGE_SIZE);
                }
            } else {
                /* Read page from swap device or exec file */
                if (dbd->dbd_type == 0)
                    read_page_from_swap(pte, pfd);
                else
                    read_page_from_file(pte, pfd);

                /* Sleep until I/O complete (simulated) */
                printf("[vfault] I/O complete\n");
            }

            /* Awaken processes waiting for this page */
            /* wakeup(event_page_contents_valid); */
            pfd->pfd_content_valid = 1;

            pte->pte_pfn = pfd->pfd_pfn;
        }
    } else {
        /* Page in executable file or demand-fill */
        pfd = alloc_physical_page();
        if (!pfd) { send_sigsegv(); goto out; }
        add_to_cache(pfd);

        if (pte->pte_flags & PTE_DEMAND_ZERO) {
            printf("[vfault] demand zero (not swap)\n");
            pfd->pfd_content_valid = 1;
        } else {
            read_page_from_file(pte, pfd);
        }
        pte->pte_pfn = pfd->pfd_pfn;
    }

    /* ── Set valid bit; clear modify bit and page age ────── */
    pte->pte_flags |=  PTE_VALID;
    pte->pte_flags &= ~PTE_DIRTY;
    pte->pte_state  =  PAGE_DEMAND_FILL;   /* now resident     */

    /* Recalculate process priority */
    recalc_priority_after_fault();

    printf("[vfault] fault resolved: pte valid pfn=%u\n",
           pte->pte_pfn);

out:
    region_unlock(region);
}

/* ─────────────────────────────────────────────────────────────
 * 4. Algorithm pfault
 *    input : virtual address that caused the protection fault
 *    output: none
 *
 *    Protection fault handler — page present but COW or
 *    genuinely illegal access.
 */
void pfault(uintptr_t faulted_addr, fault_region_t *region)
{
    pte_t    *pte = NULL;
    pfdata_t *pfd = NULL;

    printf("[pfault] protection fault at vaddr=0x%lx\n",
           (unsigned long)faulted_addr);

    /* Find region, PTE, disk block descriptor, page frame;
     * lock region                                           */
    region_lock(region);

    pte = find_pte(region, faulted_addr);
    if (!pte) {
        printf("[pfault] no PTE found\n");
        goto out;
    }

    /* If page not valid in memory — nothing to do here */
    if (!(pte->pte_flags & PTE_VALID)) {
        printf("[pfault] page not valid, ignoring "
               "(vfault should handle)\n");
        goto out;
    }

    /* If copy-on-write bit not set — genuine permission error */
    if (!(pte->pte_flags & PTE_COW)) {
        printf("[pfault] COW bit not set — "
               "real permission violation, SIGSEGV\n");
        send_sigsegv();
        goto out;
    }

    /* ── Copy-on-write handling ─────────────────────────── */
    pfd = find_in_cache(pte->pte_pfn);
    if (!pfd) pfd = &pfdata_table[pte->pte_pfn % PHYS_PAGES];

    if (pfd->pfd_refcnt > 1) {
        /* More than one process sharing this page — copy it */
        printf("[pfault] COW: copying page pfn=%u "
               "(refcnt=%d)\n",
               pte->pte_pfn, pfd->pfd_refcnt);

        /* Allocate a new physical page */
        pfdata_t *new_pfd = alloc_physical_page();
        if (!new_pfd) { goto out; }

        /* Copy contents of old page to new page */
        memcpy(
            (void *)(uintptr_t)(new_pfd->pfd_pfn << PAGE_SHIFT),
            (void *)(uintptr_t)(pfd->pfd_pfn     << PAGE_SHIFT),
            PAGE_SIZE
        );

        /* Decrement old page frame reference count */
        pfd->pfd_refcnt--;

        /* Update PTE to point to new physical page */
        pte->pte_pfn = new_pfd->pfd_pfn;
        new_pfd->pfd_pte = pte;

        printf("[pfault] COW copy done: old pfn=%u "
               "new pfn=%u\n",
               pfd->pfd_pfn, new_pfd->pfd_pfn);

    } else {
        /* ── Steal the page (only reference) ───────────── */
        printf("[pfault] COW: stealing page pfn=%u "
               "(sole owner)\n", pte->pte_pfn);

        /* If a copy exists on swap device — free it */
        if (pte->pte_swap_blk) {
            free_swap_block(pte->pte_swap_blk);
            pte->pte_swap_blk = 0;
            pte->pte_flags   &= ~PTE_IN_SWAP;
        }

        /* Remove from page hash queue */
        if (pfd->pfd_on_hash)
            remove_from_cache(pfd);
    }

    /* Set modify bit; clear copy-on-write bit */
    pte->pte_flags |=  PTE_DIRTY;
    pte->pte_flags &= ~PTE_COW;

    /* Recalculate process priority */
    recalc_priority_after_fault();

    /* Check for signals */
    if (current_proc && current_proc->pe_pid != 0) {
        /* issig(current_proc) → handle pending signals */
    }

    printf("[pfault] COW resolved at vaddr=0x%lx\n",
           (unsigned long)faulted_addr);

out:
    region_unlock(region);
}
