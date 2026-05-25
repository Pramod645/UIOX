/*
 * mmu.c  —  x86_64 4-level page table management.
 *
 * Mirrors: 10_Arch/arm32/src/mmu.c
 *
 * Physical page allocator is a simple bump allocator
 * initialised by phys_mem_init().  Replace with a proper
 * free-list allocator when integrating with the full kernel.
 */

 #include "../include/arch.h"
 #include <string.h>
 #include <stdint.h>
 
 /* ── Simple physical page bump allocator ─────────────────── */
 
 static phys_addr_t phys_bump_ptr = 0;
 static phys_addr_t phys_bump_end = 0;
 
 void phys_mem_init(phys_addr_t start, phys_addr_t end)
 {
     /* align start up to page boundary */
     phys_bump_ptr = ARCH_ALIGN_UP(start, ARCH_PAGE_SIZE);
     phys_bump_end = ARCH_ALIGN_DOWN(end,  ARCH_PAGE_SIZE);
 }
 
 phys_addr_t phys_alloc_page(void)
 {
     if (phys_bump_ptr + ARCH_PAGE_SIZE > phys_bump_end)
         return 0;   /* OOM */
     phys_addr_t pa  = phys_bump_ptr;
     phys_bump_ptr  += ARCH_PAGE_SIZE;
     /* zero the page through the direct-map window */
     memset((void *)PHYS_TO_KVIRT(pa), 0, ARCH_PAGE_SIZE);
     return pa;
 }
 
 void phys_free_page(phys_addr_t paddr)
 {
     /* bump allocator — no free; replace with real allocator */
     (void)paddr;
 }
 
 /* ── Internal helpers ────────────────────────────────────── */
 
 /*
  * get_or_create_table()
  * Given a page-table entry *e (PDPT/PD/PT pointer entry),
  * return the virtual address of the next-level table.
  * If the entry is not present, allocate a new page,
  * install it, and return its kernel virtual address.
  */
 static pte_t *get_or_create_table(pte_t *entry, uint64_t user_flag)
 {
     if (*entry & PTE_PRESENT)
         return (pte_t *)PHYS_TO_KVIRT(*entry & PTE_ADDR_MASK);
 
     phys_addr_t pa = phys_alloc_page();
     if (!pa) return (pte_t *)0;
 
     *entry = pa | PTE_PRESENT | PTE_WRITABLE | user_flag;
     return (pte_t *)PHYS_TO_KVIRT(pa);
 }
 
 /* ── mmu_create_pml4 ─────────────────────────────────────── */
 pml4_t *mmu_create_pml4(void)
 {
     phys_addr_t pa = phys_alloc_page();
     if (!pa) return (pml4_t *)0;
     return (pml4_t *)PHYS_TO_KVIRT(pa);
 }
 
 /* ── mmu_destroy_pml4 ────────────────────────────────────── */
 void mmu_destroy_pml4(pml4_t *pml4)
 {
     /* walk and free all user-space tables */
     for (int i = 0; i < 256; i++) {           /* user half     */
         if (!((*pml4)[i] & PTE_PRESENT)) continue;
         pdpt_t *pdpt = (pdpt_t *)PHYS_TO_KVIRT(
                            (*pml4)[i] & PTE_ADDR_MASK);
         for (int j = 0; j < MMU_ENTRIES_PER_TABLE; j++) {
             if (!((*pdpt)[j] & PTE_PRESENT)) continue;
             pd_t *pd = (pd_t *)PHYS_TO_KVIRT(
                            (*pdpt)[j] & PTE_ADDR_MASK);
             for (int k = 0; k < MMU_ENTRIES_PER_TABLE; k++) {
                 if (!((*pd)[k] & PTE_PRESENT)) continue;
                 if ((*pd)[k] & PTE_HUGE) continue;   /* 2 MiB  */
                 phys_free_page((*pd)[k] & PTE_ADDR_MASK);
             }
             phys_free_page((*pdpt)[j] & PTE_ADDR_MASK);
         }
         phys_free_page((*pml4)[i] & PTE_ADDR_MASK);
     }
     phys_free_page(KVIRT_TO_PHYS((virt_addr_t)pml4));
 }
 
 /* ── mmu_map_page ────────────────────────────────────────── */
 int mmu_map_page(pml4_t      *pml4,
                   virt_addr_t  vaddr,
                   phys_addr_t  paddr,
                   uint64_t     flags)
 {
     uint64_t user = (flags & PTE_USER) ? PTE_USER : 0;
 
     /* PML4 → PDPT */
     pte_t *pml4e = &(*pml4)[PML4_IDX(vaddr)];
     pdpt_t *pdpt = (pdpt_t *)get_or_create_table(pml4e, user);
     if (!pdpt) return -1;
 
     /* PDPT → PD */
     pte_t *pdpte = &(*pdpt)[PDPT_IDX(vaddr)];
     pd_t  *pd    = (pd_t *)get_or_create_table(pdpte, user);
     if (!pd) return -1;
 
     /* PD → PT */
     pte_t *pde = &(*pd)[PD_IDX(vaddr)];
     pt_t  *pt  = (pt_t *)get_or_create_table(pde, user);
     if (!pt) return -1;
 
     /* PT entry */
     (*pt)[PT_IDX(vaddr)] = (paddr & PTE_ADDR_MASK) | flags;
     mmu_tlb_flush_page(vaddr);
     return 0;
 }
 
 /* ── mmu_map_range ───────────────────────────────────────── */
 int mmu_map_range(pml4_t      *pml4,
                    virt_addr_t  vaddr,
                    phys_addr_t  paddr,
                    uint64_t     size,
                    uint64_t     flags)
 {
     virt_addr_t va = ARCH_ALIGN_DOWN(vaddr, ARCH_PAGE_SIZE);
     phys_addr_t pa = ARCH_ALIGN_DOWN(paddr, ARCH_PAGE_SIZE);
     uint64_t    end = ARCH_ALIGN_UP(vaddr + size, ARCH_PAGE_SIZE);
 
     for (; va < end; va += ARCH_PAGE_SIZE, pa += ARCH_PAGE_SIZE) {
         int r = mmu_map_page(pml4, va, pa, flags);
         if (r) return r;
     }
     return 0;
 }
 
 /* ── mmu_unmap_page ──────────────────────────────────────── */
 int mmu_unmap_page(pml4_t *pml4, virt_addr_t vaddr)
 {
     pte_t *pml4e = &(*pml4)[PML4_IDX(vaddr)];
     if (!(*pml4e & PTE_PRESENT)) return -1;
 
     pdpt_t *pdpt = (pdpt_t *)PHYS_TO_KVIRT(*pml4e & PTE_ADDR_MASK);
     pte_t  *pdpte = &(*pdpt)[PDPT_IDX(vaddr)];
     if (!(*pdpte & PTE_PRESENT)) return -1;
 
     pd_t  *pd  = (pd_t *)PHYS_TO_KVIRT(*pdpte & PTE_ADDR_MASK);
     pte_t *pde = &(*pd)[PD_IDX(vaddr)];
     if (!(*pde & PTE_PRESENT)) return -1;
 
     pt_t  *pt  = (pt_t *)PHYS_TO_KVIRT(*pde & PTE_ADDR_MASK);
     pte_t *pte = &(*pt)[PT_IDX(vaddr)];
     if (!(*pte & PTE_PRESENT)) return -1;
 
     *pte = 0;
     mmu_tlb_flush_page(vaddr);
     return 0;
 }
 
 /* ── mmu_virt_to_phys ────────────────────────────────────── */
 phys_addr_t mmu_virt_to_phys(pml4_t *pml4, virt_addr_t vaddr)
 {
     pte_t pml4e = (*pml4)[PML4_IDX(vaddr)];
     if (!(pml4e & PTE_PRESENT)) return 0;
 
     pdpt_t *pdpt  = (pdpt_t *)PHYS_TO_KVIRT(pml4e & PTE_ADDR_MASK);
     pte_t   pdpte = (*pdpt)[PDPT_IDX(vaddr)];
     if (!(pdpte & PTE_PRESENT)) return 0;
     /* 1 GiB huge page? */
     if (pdpte & PTE_HUGE)
         return (pdpte & PTE_ADDR_MASK) | (vaddr & (ARCH_PAGE_SIZE * 512 * 512 - 1));
 
     pd_t  *pd  = (pd_t *)PHYS_TO_KVIRT(pdpte & PTE_ADDR_MASK);
     pte_t  pde = (*pd)[PD_IDX(vaddr)];
     if (!(pde & PTE_PRESENT)) return 0;
     /* 2 MiB huge page? */
     if (pde & PTE_HUGE)
         return (pde & PTE_ADDR_MASK) | (vaddr & (ARCH_PAGE_SIZE * 512 - 1));
 
     pt_t  *pt  = (pt_t *)PHYS_TO_KVIRT(pde & PTE_ADDR_MASK);
     pte_t  pte = (*pt)[PT_IDX(vaddr)];
     if (!(pte & PTE_PRESENT)) return 0;
 
     return (pte & PTE_ADDR_MASK) | PAGE_OFF(vaddr);
 }
 
 /* ── mmu_switch ──────────────────────────────────────────── */
 void mmu_switch(pml4_t *pml4)
 {
     phys_addr_t pa = KVIRT_TO_PHYS((virt_addr_t)pml4);
     __asm__ volatile("movq %0, %%cr3" :: "r"(pa) : "memory");
 }
 
 /* ── TLB flush ───────────────────────────────────────────── */
 void mmu_tlb_flush_all(void)
 {
     uint64_t cr3 = 0;
     __asm__ volatile("movq %%cr3,%0; movq %0,%%cr3"
                      : "=r"(cr3) :: "memory");
 }
 
 void mmu_tlb_flush_page(virt_addr_t vaddr)
 {
     __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
 }
 
 /* ── mmu_init ────────────────────────────────────────────── */
 void mmu_init(void)
 {
     uint64_t cr4 = cpu_read_cr4();
     cr4 |= CR4_PGE;   /* enable global pages */
     cpu_write_cr4(cr4);
 }
 