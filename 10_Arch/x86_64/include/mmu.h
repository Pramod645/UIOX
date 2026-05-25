#ifndef __ARCH_X86_64_MMU_H
#define __ARCH_X86_64_MMU_H

/*
 * mmu.h  —  x86_64 4-level paging (PML4) definitions.
 *
 * Mirrors: 10_Arch/arm32/include/mmu.h
 *
 * Virtual address layout (48-bit canonical form):
 *   [63:48] sign extension
 *   [47:39] PML4 index   (9 bits)
 *   [38:30] PDPT index   (9 bits)
 *   [29:21] PD   index   (9 bits)
 *   [20:12] PT   index   (9 bits)
 *   [11: 0] page offset  (12 bits)
 */

#include <stdint.h>
#include "arch.h"

/* ── Page-table entry flags ──────────────────────────────── */
#define PTE_PRESENT     (1UL <<  0)   /* P    — page present    */
#define PTE_WRITABLE    (1UL <<  1)   /* R/W  — read/write      */
#define PTE_USER        (1UL <<  2)   /* U/S  — user accessible */
#define PTE_PWT         (1UL <<  3)   /* write-through          */
#define PTE_PCD         (1UL <<  4)   /* cache disable          */
#define PTE_ACCESSED    (1UL <<  5)   /* A    — accessed        */
#define PTE_DIRTY       (1UL <<  6)   /* D    — dirty           */
#define PTE_HUGE        (1UL <<  7)   /* PS   — huge page (PD)  */
#define PTE_GLOBAL      (1UL <<  8)   /* G    — global          */
#define PTE_NX          (1UL << 63)   /* NX/XD — no execute     */

#define PTE_ADDR_MASK   0x000FFFFFFFFFF000UL

/* ── Index extraction from virtual address ───────────────── */
#define PML4_IDX(va)  (((va) >> 39) & 0x1FFUL)
#define PDPT_IDX(va)  (((va) >> 30) & 0x1FFUL)
#define PD_IDX(va)    (((va) >> 21) & 0x1FFUL)
#define PT_IDX(va)    (((va) >> 12) & 0x1FFUL)
#define PAGE_OFF(va)  ((va) & (ARCH_PAGE_SIZE - 1))

/* ── Table sizes ─────────────────────────────────────────── */
#define MMU_ENTRIES_PER_TABLE   512
#define MMU_TABLE_SIZE          (MMU_ENTRIES_PER_TABLE * sizeof(uint64_t))

/* ── Physical address types ──────────────────────────────── */
typedef uint64_t phys_addr_t;
typedef uint64_t virt_addr_t;
typedef uint64_t pte_t;        /* one page-table entry        */

/* ── Page-table page types ───────────────────────────────── */
typedef pte_t pml4_t[MMU_ENTRIES_PER_TABLE];
typedef pte_t pdpt_t[MMU_ENTRIES_PER_TABLE];
typedef pte_t pd_t  [MMU_ENTRIES_PER_TABLE];
typedef pte_t pt_t  [MMU_ENTRIES_PER_TABLE];

/* ── MMU protection flags (POSIX-style) ──────────────────── */
#define MMU_PROT_NONE   0x0
#define MMU_PROT_READ   0x1
#define MMU_PROT_WRITE  0x2
#define MMU_PROT_EXEC   0x4
#define MMU_PROT_USER   0x8
#define MMU_PROT_KERN   0x10

/* ── Physical memory map (typical x86_64 layout) ────────── */
#define PHYS_BASE           0x0000000000000000UL
#define PHYS_KERN_LOAD      0x0000000000100000UL  /* 1 MiB      */
#define PHYS_MEM_MAX        0x0000000100000000UL  /* 4 GiB      */

/* ── Kernel virtual memory map ───────────────────────────── */
#define KVIRT_DIRECT_MAP    0xFFFF800000000000UL  /* phys → virt*/
#define KVIRT_KERN_BASE     0xFFFFFFFF80000000UL  /* kernel image*/
#define KVIRT_HEAP_START    0xFFFF900000000000UL
#define KVIRT_HEAP_END      0xFFFFA00000000000UL

/* ── Convert between physical and kernel virtual ────────── */
#define PHYS_TO_KVIRT(pa)  ((virt_addr_t)(pa) + KVIRT_DIRECT_MAP)
#define KVIRT_TO_PHYS(va)  ((phys_addr_t)(va) - KVIRT_DIRECT_MAP)

/* ── Function prototypes ─────────────────────────────────── */
void       mmu_init(void);
pml4_t    *mmu_create_pml4(void);
void       mmu_destroy_pml4(pml4_t *pml4);

int        mmu_map_page(pml4_t *pml4,
                         virt_addr_t vaddr,
                         phys_addr_t paddr,
                         uint64_t    flags);

int        mmu_map_range(pml4_t *pml4,
                          virt_addr_t vaddr,
                          phys_addr_t paddr,
                          uint64_t    size,
                          uint64_t    flags);

int        mmu_unmap_page(pml4_t *pml4, virt_addr_t vaddr);

phys_addr_t mmu_virt_to_phys(pml4_t *pml4, virt_addr_t vaddr);

void       mmu_switch(pml4_t *pml4);
void       mmu_tlb_flush_all(void);
void       mmu_tlb_flush_page(virt_addr_t vaddr);

phys_addr_t phys_alloc_page(void);
void        phys_free_page(phys_addr_t paddr);
void        phys_mem_init(phys_addr_t start, phys_addr_t end);

#endif /* __ARCH_X86_64_MMU_H */
