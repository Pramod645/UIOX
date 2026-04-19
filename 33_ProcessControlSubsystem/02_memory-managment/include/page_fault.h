#ifndef PAGE_FAULT_H
#define PAGE_FAULT_H

#include <stdint.h>
#include <stddef.h>

/* ── Page size ───────────────────────────────────────────────── */
#define PAGE_SIZE       4096
#define PAGE_SHIFT      12
#define PHYS_PAGES      2048
#define PAGE_HASH_SZ    256

/* ── Page table entry flags ─────────────────────────────────── */
#define PTE_VALID       0x001   /* page is present in memory     */
#define PTE_WRITE       0x002   /* page is writable              */
#define PTE_USER        0x004   /* user-accessible               */
#define PTE_DIRTY       0x008   /* page has been written (modify)*/
#define PTE_ACCESSED    0x010   /* page has been read            */
#define PTE_COW         0x020   /* copy-on-write                 */
#define PTE_DEMAND_ZERO 0x040   /* fill with zeros on first use  */
#define PTE_DEMAND_FILL 0x080   /* fill from file on first use   */
#define PTE_IN_SWAP     0x100   /* page is on swap device        */

/* ── Page states (five states from the description) ─────────── */
typedef enum page_state {
    PAGE_ON_SWAP       = 1,  /* on swap device, not in memory   */
    PAGE_ON_FREE_LIST  = 2,  /* on free page list in memory     */
    PAGE_IN_EXEC_FILE  = 3,  /* in executable file              */
    PAGE_DEMAND_ZERO   = 4,  /* marked demand-zero              */
    PAGE_DEMAND_FILL   = 5   /* marked demand-fill              */
} page_state_t;

/* ── Page table entry ────────────────────────────────────────── */
typedef struct pte {
    uint32_t    pte_pfn;       /* physical frame number          */
    uint16_t    pte_flags;     /* PTE_* flags                    */
    uint32_t    pte_swap_blk;  /* swap block if PTE_IN_SWAP      */
    page_state_t pte_state;    /* logical page state             */
} pte_t;

/* ── Disk block descriptor ───────────────────────────────────── */
typedef struct disk_blk_desc {
    int      dbd_valid;        /* descriptor has a record        */
    uint32_t dbd_swap_blk;     /* block on swap device           */
    uint32_t dbd_file_blk;     /* block in executable file       */
    int      dbd_type;         /* 0 = swap, 1 = file             */
} disk_blk_desc_t;

/* ── Physical frame data (pfdata) ────────────────────────────── */
typedef struct pfdata {
    int      pfd_valid;        /* frame is in use                */
    uint16_t pfd_refcnt;       /* number of ptes referencing it  */
    uint32_t pfd_pfn;          /* physical frame number          */
    pte_t   *pfd_pte;          /* back-pointer to owning pte     */
    int      pfd_on_hash;      /* 1 = on page hash queue         */
    struct pfdata *pfd_hash_next;  /* page hash chain            */
    int      pfd_content_valid;   /* 1 = contents filled in      */
} pfdata_t;

/* ── Region (simplified, for fault handlers) ────────────────── */
typedef struct fault_region {
    uintptr_t   fr_vaddr_start; /* region base virtual address   */
    uint32_t    fr_size;        /* region size in bytes          */
    int         fr_locked;      /* 1 = region is locked          */
    pte_t      *fr_pgtbl;       /* region page table             */
    uint32_t    fr_npages;
} fault_region_t;

/* ── Page cache ──────────────────────────────────────────────── */
typedef struct page_cache {
    pfdata_t *pc_hash[PAGE_HASH_SZ];  /* hash chains             */
    int       pc_total;
} page_cache_t;

/* ── Free page list ──────────────────────────────────────────── */
typedef struct free_page_list {
    uint32_t   fpl_pages[PHYS_PAGES];
    int        fpl_count;
    int        fpl_head;
    int        fpl_tail;
} free_page_list_t;

/* ── Globals ────────────────────────────────────────────────── */
extern pfdata_t        pfdata_table[PHYS_PAGES];
extern page_cache_t    page_cache;
extern free_page_list_t free_pages;

/* ── Signal numbers needed here ─────────────────────────────── */
#define SIGSEGV     11

/* ── Function prototypes ────────────────────────────────────── */

/* Algorithm vfault — validity fault handler */
void vfault(uintptr_t faulted_addr, fault_region_t *region);

/* Algorithm pfault — protection fault handler */
void pfault(uintptr_t faulted_addr, fault_region_t *region);

/* Page table / physical memory helpers */
pte_t    *find_pte(fault_region_t *region, uintptr_t vaddr);
disk_blk_desc_t *find_dbd(pte_t *pte);
pfdata_t *find_in_cache(uint32_t pfn);
pfdata_t *alloc_physical_page(void);
void      free_physical_page(uint32_t pfn);
void      add_to_cache(pfdata_t *pfd);
void      remove_from_cache(pfdata_t *pfd);
int       read_page_from_swap(pte_t *pte, pfdata_t *pfd);
int       read_page_from_file(pte_t *pte, pfdata_t *pfd);
void      free_swap_block(uint32_t blk);
void      recalc_priority_after_fault(void);

/* Region lock / unlock */
void region_lock  (fault_region_t *r);
void region_unlock(fault_region_t *r);

/* Signal helper */
void send_sigsegv(void);

#endif /* PAGE_FAULT_H */
