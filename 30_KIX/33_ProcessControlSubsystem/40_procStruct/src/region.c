#include "../include/region.h"
#include "../include/proc_algo.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ── Globals ────────────────────────────────────────────────── */
region_t  region_table[NREGION];
region_t *free_region_list   = NULL;
region_t *active_region_list = NULL;

uint8_t   phys_mem_pool[PHYS_MEM_SIZE];
uint32_t  phys_mem_used = 0;

/* ── Physical Memory Helpers ────────────────────────────────── */
void *pmalloc(uint32_t size)
{
    uint32_t aligned = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    if (phys_mem_used + aligned > PHYS_MEM_SIZE) {
        fprintf(stderr, "[pmalloc] out of physical memory\n");
        return NULL;
    }
    void *ptr = phys_mem_pool + phys_mem_used;
    phys_mem_used += aligned;
    memset(ptr, 0, aligned);
    return ptr;
}

void pmfree(void *addr, uint32_t size)
{
    /* Simplified: in real kernel use a buddy allocator */
    (void)addr; (void)size;
}

pte_t *pgtbl_alloc(uint32_t npages)
{
    pte_t *tbl = (pte_t *)calloc(npages, sizeof(pte_t));
    return tbl;
}

void pgtbl_free(pte_t *tbl, uint32_t npages)
{
    (void)npages;
    free(tbl);
}

void pgtbl_copy(pte_t *dst, pte_t *src, uint32_t npages)
{
    memcpy(dst, src, npages * sizeof(pte_t));
}

/* ── Region List Initialisation ─────────────────────────────── */
void region_init(void)
{
    memset(region_table, 0, sizeof(region_table));
    /* Chain all regions onto the free list */
    for (int i = 0; i < NREGION - 1; i++)
        region_table[i].r_free_next = &region_table[i + 1];
    region_table[NREGION - 1].r_free_next = NULL;
    free_region_list   = &region_table[0];
    active_region_list = NULL;
}

/* ── Region Lock / Unlock ───────────────────────────────────── */
static void region_lock(region_t *rp)
{
    rp->r_status |= REG_LOCKED;
}

static void region_unlock(region_t *rp)
{
    rp->r_status &= ~REG_LOCKED;
}

/* ─────────────────────────────────────────────────────────────
 * 3. Algorithm allocreg
 *    input : inode pointer, region type
 *    output: locked region
 */
region_t *allocreg(struct inode *ip, region_type_t type)
{
    /* Remove region from linked list of free regions */
    if (!free_region_list) {
        fprintf(stderr, "[allocreg] no free regions\n");
        return NULL;
    }

    region_t *rp    = free_region_list;
    free_region_list = rp->r_free_next;
    rp->r_free_next  = NULL;

    /* Assign region type */
    rp->r_type   = type;

    /* Assign region inode pointer */
    rp->r_inode  = ip;

    /* If inode pointer not null, increment inode reference count */
    if (ip) {
        /* ip->i_count++; */   /* would call iget in real kernel */
        printf("[allocreg] inode ref count incremented\n");
    }

    rp->r_refcnt = 0;
    rp->r_size   = 0;
    rp->r_status = 0;
    rp->r_pgtbl  = NULL;
    rp->r_npages = 0;

    /* Place region on linked list of active regions */
    rp->r_act_next   = active_region_list;
    active_region_list = rp;

    /* Return locked region */
    region_lock(rp);
    printf("[allocreg] allocated region type=%d\n", type);
    return rp;
}

/* ─────────────────────────────────────────────────────────────
 * 4. Algorithm attachreg
 *    input : locked region, process, virtual address, type
 *    output: per-process region table entry
 */
pregion_t *attachreg(region_t *rp, struct proc *p,
                     uintptr_t vaddr, region_type_t type)
{
    extern u_area_t u;

    /* Allocate per-process region table entry */
    pregion_t *prp = NULL;
    for (int i = 0; i < MAX_REG_PER_PROC; i++) {
        if (!u.u_pregs[i].pr_valid) {
            prp = &u.u_pregs[i];
            break;
        }
    }
    if (!prp) {
        fprintf(stderr, "[attachreg] per-proc region table full\n");
        return NULL;
    }

    /* Check legality of virtual address and region size */
    if (vaddr == 0 || rp->r_size > MAX_REGION_SIZE) {
        fprintf(stderr, "[attachreg] illegal vaddr or region size\n");
        return NULL;
    }

    /* Initialize per-process region table entry */
    prp->pr_region = rp;           /* pointer to region         */
    prp->pr_type   = type;         /* type field                */
    prp->pr_vaddr  = vaddr;        /* virtual address field     */
    prp->pr_proc   = p;
    prp->pr_size   = rp->r_size;
    prp->pr_valid  = 1;

    /* Increment region reference count */
    rp->r_refcnt++;

    /* Increment process size according to attached region */
    if (p) p->p_size += rp->r_size;

    /* Initialize hardware register triple (TLB / page-table base) */
    /* In real kernel: load page table base register for segment   */

    printf("[attachreg] attached region type=%d at vaddr=0x%lx "
           "refcnt=%d\n", type, (unsigned long)vaddr, rp->r_refcnt);
    return prp;
}

/* ─────────────────────────────────────────────────────────────
 * 5. Algorithm growreg
 *    input : per-proc region entry, delta (positive or negative)
 *    output: none
 */
void growreg(pregion_t *prp, int32_t delta)
{
    if (!prp || !prp->pr_region) return;

    region_t *rp      = prp->pr_region;
    int32_t   new_size = (int32_t)rp->r_size + delta;

    if (new_size < 0) new_size = 0;

    if (delta > 0) {
        /* Region size increasing */

        /* Check legality of new region size */
        if ((uint32_t)new_size > MAX_REGION_SIZE) {
            fprintf(stderr, "[growreg] region size exceeds max\n");
            u.u_error = ENOMEM;
            return;
        }

        uint32_t new_pages = ((uint32_t)new_size + PAGE_SIZE - 1)
                             / PAGE_SIZE;
        uint32_t old_pages = rp->r_npages;

        /* Allocate auxiliary tables (page tables) */
        pte_t *new_tbl = pgtbl_alloc(new_pages);
        if (!new_tbl) {
            u.u_error = ENOMEM;
            return;
        }

        /* Copy old entries; new entries are zeroed */
        if (rp->r_pgtbl && old_pages > 0)
            pgtbl_copy(new_tbl, rp->r_pgtbl, old_pages);

        pgtbl_free(rp->r_pgtbl, old_pages);
        rp->r_pgtbl  = new_tbl;
        rp->r_npages = new_pages;

        /* Allocate physical memory (no demand paging in this sim) */
        void *new_phys = pmalloc((uint32_t)delta);
        if (!new_phys) {
            u.u_error = ENOMEM;
            return;
        }

        /* Initialize page table entries for new pages */
        for (uint32_t i = old_pages; i < new_pages; i++) {
            uintptr_t frame = ((uintptr_t)new_phys +
                               (i - old_pages) * PAGE_SIZE);
            rp->r_pgtbl[i].pte_pfn   = (uint32_t)(frame / PAGE_SIZE);
            rp->r_pgtbl[i].pte_flags  = PTE_VALID | PTE_USER;
            if (rp->r_type != REG_TEXT)
                rp->r_pgtbl[i].pte_flags |= PTE_WRITE;
        }

    } else {
        /* Region size decreasing */
        uint32_t new_pages = ((uint32_t)new_size + PAGE_SIZE - 1)
                             / PAGE_SIZE;
        uint32_t old_pages = rp->r_npages;

        /* Free physical memory for removed pages */
        for (uint32_t i = new_pages; i < old_pages; i++) {
            if (rp->r_pgtbl && rp->r_pgtbl[i].pte_flags & PTE_VALID) {
                uintptr_t frame_addr =
                    (uintptr_t)rp->r_pgtbl[i].pte_pfn * PAGE_SIZE;
                pmfree((void *)frame_addr, PAGE_SIZE);
                rp->r_pgtbl[i].pte_flags = 0;
            }
        }

        /* Free auxiliary page table entries */
        if (new_pages == 0) {
            pgtbl_free(rp->r_pgtbl, old_pages);
            rp->r_pgtbl  = NULL;
            rp->r_npages = 0;
        } else {
            rp->r_npages = new_pages;
        }
    }

    /* Set size fields */
    rp->r_size    = (uint32_t)new_size;
    prp->pr_size  = (uint32_t)new_size;

    /* Update process size in process table */
    if (prp->pr_proc)
        prp->pr_proc->p_size =
            (uint32_t)((int32_t)prp->pr_proc->p_size + delta);

    printf("[growreg] region new size=%u bytes, pages=%u\n",
           rp->r_size, rp->r_npages);
}

/* ─────────────────────────────────────────────────────────────
 * 6. Algorithm loadreg
 *    input : pregion entry, vaddr, inode, file offset, byte count
 *    output: none
 */
void loadreg(pregion_t *prp, uintptr_t vaddr,
             struct inode *ip, uint32_t file_off,
             uint32_t byte_count)
{
    if (!prp || !prp->pr_region) return;

    region_t *rp = prp->pr_region;

    /* Increase region size to eventual size (algorithm growreg) */
    int32_t delta = (int32_t)byte_count - (int32_t)rp->r_size;
    if (delta != 0)
        growreg(prp, delta);

    /* Mark region state: being loaded into memory */
    rp->r_status |= REG_LOADING;
    rp->r_status &= ~REG_VALID;

    /* Unlock region during I/O */
    region_unlock(rp);

    /* Set up u area parameters for reading file */
    u.u_io.io_base   = (char *)vaddr;      /* target vaddr     */
    u.u_io.io_offset = file_off;           /* start offset     */
    u.u_io.io_count  = byte_count;         /* bytes to read    */
    u.u_io.io_seg    = 0;                  /* user space       */

    /* Read file into region (internal read algorithm) */
    /* In real kernel: call bread / iread loop here    */
    if (ip) {
        printf("[loadreg] reading %u bytes from inode "
               "offset=%u into vaddr=0x%lx\n",
               byte_count, file_off, (unsigned long)vaddr);
        /* Simulated read: zero-fill the region memory */
        if (rp->r_phys_addr)
            memset(rp->r_phys_addr, 0, byte_count);
    }

    /* Lock region again */
    region_lock(rp);

    /* Mark region state: completely loaded into memory */
    rp->r_status &= ~REG_LOADING;
    rp->r_status |=  REG_VALID;

    /* Awaken all processes waiting for this region to be loaded */
    /* In real kernel: wakeup(rp) */
    printf("[loadreg] region loaded, waking waiters\n");
}

/* ─────────────────────────────────────────────────────────────
 * 7. Algorithm freereg
 *    input : locked region pointer
 *    output: none
 */
void freereg(region_t *rp)
{
    if (!rp) return;

    /* If reference count non-zero, some process still using region */
    if (rp->r_refcnt > 0) {
        region_unlock(rp);
        /* If region has associated inode, release inode lock */
        if (rp->r_inode) {
            /* iunlock(rp->r_inode); */
        }
        printf("[freereg] region still in use (refcnt=%d), "
               "not freed\n", rp->r_refcnt);
        return;
    }

    /* If region has associated inode, release it (algorithm iput) */
    if (rp->r_inode) {
        /* iput(rp->r_inode); */
        printf("[freereg] releasing inode\n");
        rp->r_inode = NULL;
    }

    /* Free physical memory still associated with region */
    if (rp->r_phys_addr) {
        pmfree(rp->r_phys_addr, rp->r_size);
        rp->r_phys_addr = NULL;
    }

    /* Free auxiliary tables (page tables) */
    if (rp->r_pgtbl) {
        pgtbl_free(rp->r_pgtbl, rp->r_npages);
        rp->r_pgtbl  = NULL;
        rp->r_npages = 0;
    }

    /* Remove from active region list */
    region_t **cur = &active_region_list;
    while (*cur) {
        if (*cur == rp) { *cur = rp->r_act_next; break; }
        cur = &(*cur)->r_act_next;
    }

    /* Clear region fields */
    memset(rp, 0, sizeof(region_t));

    /* Place region on region free list */
    rp->r_free_next  = free_region_list;
    free_region_list = rp;

    /* Unlock region */
    region_unlock(rp);

    printf("[freereg] region freed and returned to free list\n");
}

/* ─────────────────────────────────────────────────────────────
 * 8. Algorithm detachreg
 *    input : per-process region table entry pointer
 *    output: none
 */
void detachreg(pregion_t *prp)
{
    if (!prp || !prp->pr_valid) return;

    region_t *rp = prp->pr_region;
    if (!rp) return;

    /* Get and release auxiliary memory management tables */
    if (rp->r_pgtbl) {
        /* hardware: invalidate TLB entries for this region */
    }

    /* Decrement process size */
    if (prp->pr_proc)
        prp->pr_proc->p_size -= prp->pr_size;

    /* Decrement region reference count */
    rp->r_refcnt--;
    printf("[detachreg] region refcnt now %d\n", rp->r_refcnt);

    /* Invalidate this per-process entry */
    prp->pr_valid = 0;

    if (rp->r_refcnt == 0 && !(rp->r_status & REG_STICKY)) {
        /* Free region (algorithm freereg) */
        freereg(rp);
    } else {
        /* Either reference count non-0 or sticky bit set */
        if (rp->r_inode) {
            /* iunlock(rp->r_inode); */
        }
        region_unlock(rp);
        printf("[detachreg] region kept (refcnt=%d sticky=%d)\n",
               rp->r_refcnt,
               (rp->r_status & REG_STICKY) ? 1 : 0);
    }
}

/* ─────────────────────────────────────────────────────────────
 * 9. Algorithm dupreg
 *    input : region table entry pointer
 *    output: pointer to new region identical to input
 */
region_t *dupreg(region_t *rp)
{
    if (!rp) return NULL;

    /* If region type is shared, caller increments refcnt via
     * a subsequent attachreg call — return input region directly */
    if (rp->r_type == REG_SHMEM) {
        printf("[dupreg] shared region, returning same region\n");
        return rp;
    }

    /* Allocate new region (algorithm allocreg) */
    region_t *new_rp = allocreg(rp->r_inode, rp->r_type);
    if (!new_rp) return NULL;

    /* Set up auxiliary memory management structures */
    if (rp->r_npages > 0) {
        new_rp->r_pgtbl = pgtbl_alloc(rp->r_npages);
        if (!new_rp->r_pgtbl) {
            freereg(new_rp);
            return NULL;
        }
        new_rp->r_npages = rp->r_npages;

        /* Allocate physical memory for new region contents */
        new_rp->r_phys_addr = pmalloc(rp->r_size);
        if (!new_rp->r_phys_addr) {
            freereg(new_rp);
            return NULL;
        }

        /* Copy region contents from input to new region */
        memcpy(new_rp->r_phys_addr, rp->r_phys_addr, rp->r_size);

        /* Copy page table entries; update physical addresses */
        pgtbl_copy(new_rp->r_pgtbl, rp->r_pgtbl, rp->r_npages);
        for (uint32_t i = 0; i < rp->r_npages; i++) {
            uintptr_t frame =
                (uintptr_t)new_rp->r_phys_addr + i * PAGE_SIZE;
            new_rp->r_pgtbl[i].pte_pfn = (uint32_t)(frame / PAGE_SIZE);
        }
    }

    new_rp->r_size   = rp->r_size;
    new_rp->r_status = rp->r_status & ~REG_LOCKED;

    printf("[dupreg] duplicated region size=%u bytes\n", rp->r_size);
    return new_rp;
}
