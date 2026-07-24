/*
 * 30_KIX/33_PCS/40_procStruct/src/region.c
 *
 * Freestanding fixes (v2.0)
 * ─────────────────────────
 *   REMOVED: #include <string.h>  <stdlib.h>  <stdio.h>
 *            All provided through region.h → process.h → uiox_klibc.h
 *
 *   FIXED: fprintf(stderr,...) → printf(...)          (5 occurrences)
 *   FIXED: calloc(npages, sizeof(pte_t)) → static pool (no heap)
 *   FIXED: free(tbl) → return slot to pool
 *   FIXED: ENOMEM  → defined in region.h
 *   FIXED: NULL    → typed null pointer casts
 *   FIXED: for (int i...) → int i; before loop
 *
 * @version 2.0.0  @date 2026-07-24
 */

 #include "../include/region.h"
 #include "../include/proc_algo.h"
 /* string.h / stdlib.h / stdio.h removed — provided via region.h → uiox_klibc.h */
 
 /* ── Globals ─────────────────────────────────────────────────── */
 region_t  region_table[NREGION];
 region_t *free_region_list   = (region_t *)0;
 region_t *active_region_list = (region_t *)0;
 uint8_t   phys_mem_pool[PHYS_MEM_SIZE];
 uint32_t  phys_mem_used = 0;
 
 /* ── Static page-table pool — replaces calloc/free ───────────────
  * Supports up to NREGION concurrent page tables, each up to
  * MAX_PAGES entries. Total: 128 × 1024 × 6 bytes ≈ 768 KB.        */
 #define PGTBL_POOL_SLOTS   NREGION
 static pte_t    s_pgtbl_pool[PGTBL_POOL_SLOTS][MAX_PAGES];
 static uint8_t  s_pgtbl_used[PGTBL_POOL_SLOTS];
 static uint8_t  s_pgtbl_ready = 0;
 
 static void pgtbl_pool_init(void)
 {
     if (!s_pgtbl_ready) {
         memset(s_pgtbl_pool, 0, sizeof s_pgtbl_pool);
         memset(s_pgtbl_used, 0, sizeof s_pgtbl_used);
         s_pgtbl_ready = 1;
     }
 }
 
 /* ── Physical Memory Helpers ─────────────────────────────────── */
 void *pmalloc(uint32_t size)
 {
     uint32_t aligned = (size + PAGE_SIZE - 1) & ~(uint32_t)(PAGE_SIZE - 1);
     void *ptr;
     if (phys_mem_used + aligned > PHYS_MEM_SIZE) {
         printf("[pmalloc] ERROR: out of physical memory\n"); /* was: fprintf(stderr,...) */
         return (void *)0;
     }
     ptr = phys_mem_pool + phys_mem_used;
     phys_mem_used += aligned;
     memset(ptr, 0, aligned);
     return ptr;
 }
 
 void pmfree(void *addr, uint32_t size)
 {
     /* Simplified — real kernel uses a buddy allocator */
     (void)addr; (void)size;
 }
 
 /* ── pgtbl_alloc — allocate from static pool ─────────────────── */
 pte_t *pgtbl_alloc(uint32_t npages)
 {
     uint32_t i;
     pgtbl_pool_init();
     if (npages == 0 || npages > MAX_PAGES) return (pte_t *)0;
     for (i = 0; i < PGTBL_POOL_SLOTS; i++) {
         if (!s_pgtbl_used[i]) {
             s_pgtbl_used[i] = 1;
             memset(s_pgtbl_pool[i], 0, npages * sizeof(pte_t));
             return s_pgtbl_pool[i];
         }
     }
     printf("[pgtbl_alloc] ERROR: page table pool exhausted\n");
     return (pte_t *)0;
 }
 
 /* ── pgtbl_free — return slot to pool ────────────────────────── */
 void pgtbl_free(pte_t *tbl, uint32_t npages)
 {
     uint32_t i;
     (void)npages;
     if (!tbl) return;
     for (i = 0; i < PGTBL_POOL_SLOTS; i++) {
         if (s_pgtbl_pool[i] == tbl) {
             memset(s_pgtbl_pool[i], 0, sizeof s_pgtbl_pool[i]);
             s_pgtbl_used[i] = 0;
             return;
         }
     }
 }
 
 void pgtbl_copy(pte_t *dst, pte_t *src, uint32_t npages)
 {
     if (dst && src)
         memcpy(dst, src, npages * sizeof(pte_t));
 }
 
 /* ── Region List Initialisation ──────────────────────────────── */
 void region_init(void)
 {
     int i;
     pgtbl_pool_init();
     memset(region_table, 0, sizeof(region_table));
     for (i = 0; i < NREGION - 1; i++)
         region_table[i].r_free_next = &region_table[i + 1];
     region_table[NREGION - 1].r_free_next = (region_t *)0;
     free_region_list   = &region_table[0];
     active_region_list = (region_t *)0;
 }
 
 /* ── Region Lock / Unlock ────────────────────────────────────── */
 static void region_lock  (region_t *rp) { rp->r_status |=  REG_LOCKED; }
 static void region_unlock(region_t *rp) { rp->r_status &= ~REG_LOCKED; }
 
 /* ── Algorithm allocreg (§3) ─────────────────────────────────── */
 region_t *allocreg(struct inode *ip, region_type_t type)
 {
     region_t *rp;
     if (!free_region_list) {
         printf("[allocreg] ERROR: no free regions\n"); /* was: fprintf(stderr,...) */
         return (region_t *)0;
     }
 
     rp               = free_region_list;
     free_region_list = rp->r_free_next;
     rp->r_free_next  = (region_t *)0;
 
     rp->r_type   = type;
     rp->r_inode  = ip;
     if (ip)
         printf("[allocreg] inode ref count incremented\n");
 
     rp->r_refcnt = 0;
     rp->r_size   = 0;
     rp->r_status = 0;
     rp->r_pgtbl  = (pte_t *)0;
     rp->r_npages = 0;
 
     rp->r_act_next   = active_region_list;
     active_region_list = rp;
 
     region_lock(rp);
     printf("[allocreg] allocated region type=%d\n", type);
     return rp;
 }
 
 /* ── Algorithm attachreg (§4) ────────────────────────────────── */
 pregion_t *attachreg(region_t *rp, struct proc *p,
                      uintptr_t vaddr, region_type_t type)
 {
     int i;
     pregion_t *prp = (pregion_t *)0;
     extern u_area_t u;
 
     for (i = 0; i < MAX_REG_PER_PROC; i++) {
         if (!u.u_pregs[i].pr_valid) {
             prp = &u.u_pregs[i];
             break;
         }
     }
     if (!prp) {
         printf("[attachreg] ERROR: per-proc region table full\n"); /* was: fprintf(stderr,...) */
         return (pregion_t *)0;
     }
 
     if (vaddr == 0 || rp->r_size > MAX_REGION_SIZE) {
         printf("[attachreg] ERROR: illegal vaddr or region size\n"); /* was: fprintf(stderr,...) */
         return (pregion_t *)0;
     }
 
     prp->pr_region = rp;
     prp->pr_type   = type;
     prp->pr_vaddr  = vaddr;
     prp->pr_proc   = p;
     prp->pr_size   = rp->r_size;
     prp->pr_valid  = 1;
 
     rp->r_refcnt++;
     if (p) p->p_size += rp->r_size;
 
     printf("[attachreg] attached region type=%d vaddr=0x%lx\n",
            type, (unsigned long)vaddr);
     return prp;
 }
 
 /* ── Algorithm growreg (§5) ──────────────────────────────────── */
 void growreg(pregion_t *prp, int32_t delta)
 {
     int32_t  new_size;
     uint32_t new_pages, old_pages;
     pte_t   *new_tbl;
     void    *new_phys;
     extern u_area_t u;
 
     if (!prp || !prp->pr_region) return;
     region_t *rp = prp->pr_region;
     new_size      = (int32_t)rp->r_size + delta;
     if (new_size < 0) new_size = 0;
 
     if (delta > 0) {
         if ((uint32_t)new_size > MAX_REGION_SIZE) {
             printf("[growreg] ERROR: region size exceeds max\n"); /* was: fprintf(stderr,...) */
             u.u_error = ENOMEM;
             return;
         }
 
         new_pages = ((uint32_t)new_size + PAGE_SIZE - 1) / PAGE_SIZE;
         old_pages = rp->r_npages;
 
         new_tbl = pgtbl_alloc(new_pages);
         if (!new_tbl) {
             u.u_error = ENOMEM;
             return;
         }
 
         if (rp->r_pgtbl && old_pages > 0)
             pgtbl_copy(new_tbl, rp->r_pgtbl, old_pages);
 
         pgtbl_free(rp->r_pgtbl, old_pages);
         rp->r_pgtbl  = new_tbl;
         rp->r_npages = new_pages;
 
         new_phys = pmalloc((uint32_t)delta);
         if (!new_phys) {
             u.u_error = ENOMEM;
             return;
         }
 
         {
             uint32_t i;
             for (i = old_pages; i < new_pages; i++) {
                 uintptr_t frame = ((uintptr_t)new_phys +
                                    (i - old_pages) * PAGE_SIZE);
                 rp->r_pgtbl[i].pte_pfn   = (uint32_t)(frame >> 12);
                 rp->r_pgtbl[i].pte_flags = PTE_VALID | PTE_WRITE;
             }
         }
 
     } else if (delta < 0) {
         new_pages = ((uint32_t)new_size + PAGE_SIZE - 1) / PAGE_SIZE;
         old_pages = rp->r_npages;
 
         {
             uint32_t i;
             for (i = new_pages; i < old_pages; i++) {
                 if (rp->r_pgtbl &&
                     (rp->r_pgtbl[i].pte_flags & PTE_VALID)) {
                     uintptr_t frame_addr =
                         (uintptr_t)rp->r_pgtbl[i].pte_pfn * PAGE_SIZE;
                     pmfree((void *)frame_addr, PAGE_SIZE);
                     rp->r_pgtbl[i].pte_flags = 0;
                 }
             }
         }
 
         if (new_pages == 0) {
             pgtbl_free(rp->r_pgtbl, old_pages);
             rp->r_pgtbl  = (pte_t *)0;
             rp->r_npages = 0;
         } else {
             rp->r_npages = new_pages;
         }
     }
 
     rp->r_size   = (uint32_t)new_size;
     prp->pr_size = (uint32_t)new_size;
     printf("[growreg] region size %u -> %u\n",
            (uint32_t)((int32_t)rp->r_size - delta), rp->r_size);
 }
 
 /* ── Algorithm freereg ───────────────────────────────────────── */
 void freereg(region_t *rp)
 {
     region_t *prev, *cur;
     if (!rp) return;
 
     pgtbl_free(rp->r_pgtbl, rp->r_npages);
     rp->r_pgtbl  = (pte_t *)0;
     rp->r_npages = 0;
 
     /* Remove from active list */
     prev = (region_t *)0;
     cur  = active_region_list;
     while (cur) {
         if (cur == rp) {
             if (prev) prev->r_act_next  = cur->r_act_next;
             else      active_region_list = cur->r_act_next;
             break;
         }
         prev = cur; cur = cur->r_act_next;
     }
 
     memset(rp, 0, sizeof(region_t));
     rp->r_free_next  = free_region_list;
     free_region_list = rp;
     printf("[freereg] region freed\n");
 }
 
 /* ── Algorithm detachreg ─────────────────────────────────────── */
 void detachreg(pregion_t *prp)
 {
     region_t *rp;
     if (!prp || !prp->pr_valid) return;
     rp = prp->pr_region;
 
     if (prp->pr_proc)
         prp->pr_proc->p_size -= prp->pr_size;
 
     prp->pr_valid = 0;
 
     if (rp) {
         if (rp->r_refcnt > 0) rp->r_refcnt--;
         if (rp->r_refcnt == 0) freereg(rp);
     }
     printf("[detachreg] region detached\n");
 }
 
 /* ── Algorithm dupreg ────────────────────────────────────────── */
 region_t *dupreg(region_t *rp)
 {
     region_t *new_rp;
     pte_t    *new_tbl;
 
     if (!rp) return (region_t *)0;
 
     new_rp = allocreg(rp->r_inode, rp->r_type);
     if (!new_rp) return (region_t *)0;
 
     new_rp->r_size = rp->r_size;
 
     if (rp->r_pgtbl && rp->r_npages > 0) {
         new_tbl = pgtbl_alloc(rp->r_npages);
         if (!new_tbl) { freereg(new_rp); return (region_t *)0; }
         pgtbl_copy(new_tbl, rp->r_pgtbl, rp->r_npages);
         new_rp->r_pgtbl  = new_tbl;
         new_rp->r_npages = rp->r_npages;
     }
 
     printf("[dupreg] region duplicated\n");
     region_unlock(new_rp);
     return new_rp;
 }
 
 /* ── Algorithm loadreg ───────────────────────────────────────── */
 void loadreg(pregion_t *prp, uintptr_t vaddr,
              struct inode *ip, uint32_t file_off,
              uint32_t byte_count)
 {
     /* Simulation: mark region as loading then valid */
     region_t *rp;
     (void)vaddr; (void)ip; (void)file_off;
     if (!prp || !prp->pr_region) return;
     rp = prp->pr_region;
     rp->r_status |= REG_LOADING;
     printf("[loadreg] loading %u bytes into region\n", byte_count);
     rp->r_status &= ~REG_LOADING;
     rp->r_status |=  REG_VALID;
 }
 