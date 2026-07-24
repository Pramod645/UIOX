/*
 * 30_KIX/33_PCS/02_MemMngnt/src/page_fault.c
 * v2.1 — all field names verified from live page_fault.h
 */
#include "../include/page_fault.h"
#include "../include/scheduler.h"

pfdata_t         pfdata_table[PHYS_PAGES];
page_cache_t     page_cache;
free_page_list_t free_pages;

void page_fault_init(void)
{
    uint32_t i;
    memset(pfdata_table, 0, sizeof pfdata_table);
    memset(&page_cache,  0, sizeof page_cache);
    memset(&free_pages,  0, sizeof free_pages);
    for (i = 0; i < PHYS_PAGES; i++) {
        free_pages.fpl_pages[free_pages.fpl_tail] = i;
        free_pages.fpl_tail = (free_pages.fpl_tail + 1) % PHYS_PAGES;
        free_pages.fpl_count++;
    }
    printf("[fault] init: %u pages\n", PHYS_PAGES);
}

void send_sigsegv(void)
{
    printf("[fault] SIGSEGV pid=%u\n",
           current_proc ? current_proc->pe_pid : 0u);
}

void region_lock  (fault_region_t *r) { if (r) r->fr_locked = 1; }
void region_unlock(fault_region_t *r) { if (r) r->fr_locked = 0; }
void recalc_priority_after_fault(void) { if (current_proc) recalc_priority(current_proc); }

pte_t *find_pte(fault_region_t *region, uintptr_t vaddr)
{
    uint32_t page; uintptr_t offset;
    if (!region || !region->fr_pgtbl) return (pte_t *)0;
    if (vaddr < region->fr_vaddr_start) return (pte_t *)0;
    offset = vaddr - region->fr_vaddr_start;
    page   = (uint32_t)(offset >> PAGE_SHIFT);
    if (page >= region->fr_npages) return (pte_t *)0;
    return &region->fr_pgtbl[page];
}

disk_blk_desc_t *find_dbd(pte_t *pte) { (void)pte; return (disk_blk_desc_t *)0; }

pfdata_t *find_in_cache(uint32_t pfn)
{
    uint32_t slot = pfn % PAGE_HASH_SZ;
    pfdata_t *p   = page_cache.pc_hash[slot];
    while (p) { if (p->pfd_pfn == pfn) return p; p = p->pfd_hash_next; }
    return (pfdata_t *)0;
}

void add_to_cache(pfdata_t *pfd)
{
    uint32_t slot;
    if (!pfd) return;
    slot = pfd->pfd_pfn % PAGE_HASH_SZ;
    pfd->pfd_hash_next       = page_cache.pc_hash[slot];
    page_cache.pc_hash[slot] = pfd;
    pfd->pfd_on_hash         = 1;
    page_cache.pc_total++;
}

void remove_from_cache(pfdata_t *pfd)
{
    uint32_t slot; pfdata_t *prev, *cur;
    if (!pfd || !pfd->pfd_on_hash) return;
    slot = pfd->pfd_pfn % PAGE_HASH_SZ;
    prev = (pfdata_t *)0; cur = page_cache.pc_hash[slot];
    while (cur) {
        if (cur == pfd) {
            if (prev) prev->pfd_hash_next      = cur->pfd_hash_next;
            else      page_cache.pc_hash[slot] = cur->pfd_hash_next;
            pfd->pfd_hash_next = (pfdata_t *)0;
            pfd->pfd_on_hash   = 0;
            page_cache.pc_total--;
            return;
        }
        prev = cur; cur = cur->pfd_hash_next;
    }
}

pfdata_t *alloc_physical_page(void)
{
    uint32_t pfn; pfdata_t *pfd;
    if (free_pages.fpl_count == 0) {
        printf("[fault] ERROR: out of pages\n");
        return (pfdata_t *)0;
    }
    pfn = free_pages.fpl_pages[free_pages.fpl_head];
    free_pages.fpl_head  = (free_pages.fpl_head + 1) % PHYS_PAGES;
    free_pages.fpl_count--;
    pfd = &pfdata_table[pfn % PHYS_PAGES];
    memset(pfd, 0, sizeof(pfdata_t));
    pfd->pfd_valid  = 1; pfd->pfd_pfn = pfn; pfd->pfd_refcnt = 1;
    printf("[fault] alloc pfn=%u\n", pfn);
    return pfd;
}

void free_physical_page(uint32_t pfn)
{
    pfdata_t *pfd = &pfdata_table[pfn % PHYS_PAGES];
    memset(pfd, 0, sizeof(pfdata_t));
    free_pages.fpl_pages[free_pages.fpl_tail] = pfn;
    free_pages.fpl_tail  = (free_pages.fpl_tail + 1) % PHYS_PAGES;
    free_pages.fpl_count++;
    printf("[fault] free pfn=%u\n", pfn);
}

void free_swap_block(uint32_t blk) { printf("[fault] free swap blk=%u\n", blk); }

int read_page_from_swap(pte_t *pte, pfdata_t *pfd)
{ (void)pte; pfd->pfd_content_valid = 1; printf("[fault] read swap pfn=%u\n", pfd->pfd_pfn); return 0; }

int read_page_from_file(pte_t *pte, pfdata_t *pfd)
{ (void)pte; pfd->pfd_content_valid = 1; printf("[fault] read file pfn=%u\n", pfd->pfd_pfn); return 0; }

void vfault(uintptr_t faulted_addr, fault_region_t *region)
{
    pte_t *pte; disk_blk_desc_t *dbd; pfdata_t *pfd;
    if (!region) { send_sigsegv(); return; }
    region_lock(region);
    pte = find_pte(region, faulted_addr);
    if (!pte) { send_sigsegv(); goto out; }
    dbd = find_dbd(pte);
    if (pte->pte_flags & PTE_VALID) goto out;
    if (pte->pte_flags & PTE_IN_SWAP) {
        pfd = find_in_cache(pte->pte_pfn);
        if (pfd) { remove_from_cache(pfd); pte->pte_pfn = pfd->pfd_pfn; }
        else {
            pfd = alloc_physical_page();
            if (!pfd) { send_sigsegv(); goto out; }
            add_to_cache(pfd);
            if (!dbd || !dbd->dbd_valid) {
                if (pte->pte_flags & PTE_DEMAND_ZERO)
                    memset((void *)(uintptr_t)(pfd->pfd_pfn << PAGE_SHIFT), 0, PAGE_SIZE);
            } else {
                if (dbd->dbd_type == 0) read_page_from_swap(pte, pfd);
                else                    read_page_from_file(pte, pfd);
            }
            pfd->pfd_content_valid = 1;
            pte->pte_pfn = pfd->pfd_pfn;
        }
    } else {
        pfd = alloc_physical_page();
        if (!pfd) { send_sigsegv(); goto out; }
        add_to_cache(pfd);
        if (pte->pte_flags & PTE_DEMAND_ZERO) pfd->pfd_content_valid = 1;
        else read_page_from_file(pte, pfd);
        pte->pte_pfn = pfd->pfd_pfn;
    }
    pte->pte_flags |= PTE_VALID; pte->pte_flags &= ~PTE_DIRTY;
    pte->pte_state  = PAGE_DEMAND_FILL;
    recalc_priority_after_fault();
    printf("[vfault] resolved pfn=%u\n", pte->pte_pfn);
out:
    region_unlock(region);
}

void pfault(uintptr_t faulted_addr, fault_region_t *region)
{
    pte_t *pte;
    if (!region) { send_sigsegv(); return; }
    region_lock(region);
    pte = find_pte(region, faulted_addr);
    if (!pte || !(pte->pte_flags & PTE_VALID)) { send_sigsegv(); goto out; }
    if (pte->pte_flags & PTE_COW) {
        pte->pte_flags &= ~PTE_COW; pte->pte_flags |= PTE_WRITE;
    } else { send_sigsegv(); }
out:
    region_unlock(region);
}
