/*
 * 30_KIX/33_PCS/40_procStruct/include/region.h
 *
 * Freestanding fixes (v2.0)
 * ─────────────────────────
 *   REMOVED: system headers (implicitly pulled in via process.h → uiox_klibc.h)
 *   ADDED:   ENOMEM — error code used by growreg()
 *   ADDED:   MAX_REGION_SIZE — used by attachreg() / growreg()
 *   NOTE:    pte_t is also defined in 02_MemMngnt/include/page_fault.h;
 *            guard with #ifndef to avoid redefinition.
 *
 * @version 2.0.0  @date 2026-07-24
 */
#ifndef REGION_H
#define REGION_H

#include "process.h"   /* pulls in uiox_klibc.h for uint*_t, size_t etc. */

/* ── Constants ───────────────────────────────────────────────── */
#define NREGION          128
#define PAGE_SIZE        4096
#define MAX_PAGES        1024
#define MAX_REG_PER_PROC 8
#define MAX_REGION_SIZE  (MAX_PAGES * PAGE_SIZE)  /* 4 MB per region */

/* ── Error codes used by region algorithms ────────────────────── */
#ifndef ENOMEM
#define ENOMEM  12   /* out of memory (POSIX errno value)   */
#endif
#ifndef EFAULT
#define EFAULT  14   /* bad address                         */
#endif

/* ── Region Types ────────────────────────────────────────────── */
typedef enum region_type {
    REG_TEXT  = 1,
    REG_DATA  = 2,
    REG_STACK = 3,
    REG_SHMEM = 4
} region_type_t;

/* ── Region Status Flags ─────────────────────────────────────── */
#define REG_LOCKED   0x01
#define REG_DEMAND   0x02
#define REG_LOADING  0x04
#define REG_VALID    0x08

/* ── Page Table Entry (local definition — guards against redefinition
      if page_fault.h is also included) ──────────────────────── */
#ifndef REGION_PTE_DEFINED
#define REGION_PTE_DEFINED
typedef struct region_pte {
    uint32_t pte_pfn;     /* physical frame number     */
    uint16_t pte_flags;   /* permission + status flags */
} pte_t;
#define PTE_VALID  0x001
#define PTE_WRITE  0x002
#define PTE_USER   0x004
#define PTE_DIRTY  0x008
#endif

/* ── Region descriptor ───────────────────────────────────────── */
typedef struct region {
    region_type_t  r_type;
    struct inode  *r_inode;
    uint16_t       r_refcnt;
    pte_t         *r_pgtbl;
    uint32_t       r_npages;
    uint32_t       r_size;
    uint16_t       r_status;
    struct region *r_free_next;
    struct region *r_act_next;
} region_t;

/* ── Per-process region table entry ─────────────────────────── */
typedef struct pregion {
    region_t      *pr_region;
    struct proc   *pr_proc;
    region_type_t  pr_type;
    uintptr_t      pr_vaddr;
    uint32_t       pr_size;
    int            pr_valid;
} pregion_t;

/* ── Globals ─────────────────────────────────────────────────── */
extern region_t  region_table[NREGION];
extern region_t *free_region_list;
extern region_t *active_region_list;

/* ── Physical Memory Pool (simulated) ────────────────────────── */
#define PHYS_MEM_SIZE  (64 * 1024 * 1024)   /* 64 MB */
extern uint8_t  phys_mem_pool[PHYS_MEM_SIZE];
extern uint32_t phys_mem_used;

/* ── Prototypes ──────────────────────────────────────────────── */
region_t  *allocreg (struct inode *ip, region_type_t type);
pregion_t *attachreg(region_t *rp, struct proc *p,
                     uintptr_t vaddr, region_type_t type);
void       growreg  (pregion_t *prp, int32_t delta);
void       loadreg  (pregion_t *prp, uintptr_t vaddr,
                     struct inode *ip, uint32_t file_off,
                     uint32_t byte_count);
void       freereg  (region_t *rp);
void       detachreg(pregion_t *prp);
region_t  *dupreg   (region_t *rp);

void    *pmalloc    (uint32_t size);
void     pmfree     (void *addr, uint32_t size);
pte_t   *pgtbl_alloc(uint32_t npages);
void     pgtbl_free (pte_t *tbl, uint32_t npages);
void     pgtbl_copy (pte_t *dst, pte_t *src, uint32_t npages);
void     region_init(void);

#endif /* REGION_H */
