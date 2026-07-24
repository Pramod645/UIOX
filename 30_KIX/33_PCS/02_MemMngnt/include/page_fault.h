/*
 * 30_KIX/33_PCS/02_MemMngnt/include/page_fault.h  v2.0
 * System headers removed: stdint.h and stddef.h replaced by uiox_klibc.h
 */
#ifndef PAGE_FAULT_H
#define PAGE_FAULT_H

#include "uiox_klibc.h"

#define PAGE_SIZE       4096
#define PAGE_SHIFT      12
#define PHYS_PAGES      2048
#define PAGE_HASH_SZ    256

#define PTE_VALID       0x001
#define PTE_WRITE       0x002
#define PTE_USER        0x004
#define PTE_DIRTY       0x008
#define PTE_ACCESSED    0x010
#define PTE_COW         0x020
#define PTE_DEMAND_ZERO 0x040
#define PTE_DEMAND_FILL 0x080
#define PTE_IN_SWAP     0x100

typedef enum { PAGE_FREE=0, PAGE_DEMAND_FILL=1, PAGE_ON_SWAP=2, PAGE_LOCKED=3 } page_state_t;

typedef struct pte {
    uint32_t     pte_pfn;
    uint16_t     pte_flags;
    page_state_t pte_state;
} pte_t;

typedef struct disk_blk_desc {
    uint32_t dbd_blkno;
    int      dbd_type;
    int      dbd_valid;
} disk_blk_desc_t;

typedef struct pfdata {
    int      pfd_valid;
    uint16_t pfd_refcnt;
    uint32_t pfd_pfn;
    pte_t   *pfd_pte;
    int      pfd_on_hash;
    struct pfdata *pfd_hash_next;
    int      pfd_content_valid;
} pfdata_t;

typedef struct fault_region {
    uintptr_t  fr_vaddr_start;
    uint32_t   fr_size;
    int        fr_locked;
    pte_t     *fr_pgtbl;
    uint32_t   fr_npages;
} fault_region_t;

typedef struct page_cache {
    pfdata_t *pc_hash[PAGE_HASH_SZ];
    int       pc_total;
} page_cache_t;

typedef struct free_page_list {
    uint32_t fpl_pages[PHYS_PAGES];
    int      fpl_count;
    int      fpl_head;
    int      fpl_tail;
} free_page_list_t;

extern pfdata_t         pfdata_table[PHYS_PAGES];
extern page_cache_t     page_cache;
extern free_page_list_t free_pages;

#define SIGSEGV 11

void             vfault(uintptr_t faulted_addr, fault_region_t *region);
void             pfault(uintptr_t faulted_addr, fault_region_t *region);
pte_t           *find_pte(fault_region_t *region, uintptr_t vaddr);
disk_blk_desc_t *find_dbd(pte_t *pte);
pfdata_t        *find_in_cache(uint32_t pfn);
pfdata_t        *alloc_physical_page(void);
void             free_physical_page(uint32_t pfn);
void             add_to_cache(pfdata_t *pfd);
void             remove_from_cache(pfdata_t *pfd);
int              read_page_from_swap(pte_t *pte, pfdata_t *pfd);
int              read_page_from_file(pte_t *pte, pfdata_t *pfd);
void             free_swap_block(uint32_t blk);
void             recalc_priority_after_fault(void);
void             region_lock  (fault_region_t *r);
void             region_unlock(fault_region_t *r);
void             send_sigsegv(void);

#endif
