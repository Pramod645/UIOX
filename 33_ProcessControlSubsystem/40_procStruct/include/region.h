#ifndef REGION_H
#define REGION_H

#include <stdint.h>
#include <stddef.h>

/* ── Constants ──────────────────────────────────────────────── */
#define NREGION         128     /* max regions in region table  */
#define PAGE_SIZE       4096    /* page size in bytes           */
#define MAX_PAGES       1024    /* max pages per region         */
#define MAX_REG_PER_PROC 8      /* max regions per process      */
#define MAX_REGION_SIZE (MAX_PAGES * PAGE_SIZE)

/* ── Region Types ───────────────────────────────────────────── */
typedef enum region_type {
    REG_TEXT    = 1,    /* executable code (read-only)         */
    REG_DATA    = 2,    /* initialized + BSS data (private)    */
    REG_STACK   = 3,    /* process stack (private)             */
    REG_SHMEM   = 4     /* shared memory                       */
} region_type_t;

/* ── Region Status Flags ────────────────────────────────────── */
#define REG_LOCKED      0x01    /* region is locked             */
#define REG_DEMAND      0x02    /* region in demand             */
#define REG_LOADING     0x04    /* being loaded into memory     */
#define REG_VALID       0x08    /* fully loaded, valid          */
#define REG_STICKY      0x10    /* sticky bit set               */

/* ── Page Table Entry ───────────────────────────────────────── */
typedef struct pte {
    uint32_t  pte_pfn;          /* physical frame number        */
    uint16_t  pte_flags;        /* permission + status flags    */
} pte_t;

/* Page table entry flags */
#define PTE_VALID   0x01        /* page is valid/present        */
#define PTE_WRITE   0x02        /* page is writable             */
#define PTE_USER    0x04        /* user-accessible              */
#define PTE_DIRTY   0x08        /* page has been written        */
#define PTE_ACCESSED 0x10       /* page has been accessed       */

/* ── Region Table Entry ─────────────────────────────────────── */
struct inode;                   /* forward declaration          */

typedef struct region {
    struct inode  *r_inode;     /* inode of backing file        */
    region_type_t  r_type;      /* text / data / stack / shmem  */
    uint32_t       r_size;      /* size in bytes                */
    void          *r_phys_addr; /* physical memory base address */
    uint16_t       r_status;    /* status flags (see REG_*)     */
    uint16_t       r_refcnt;    /* number of processes using it */
    pte_t         *r_pgtbl;     /* page table (phys page nums)  */
    uint32_t       r_npages;    /* number of pages in page table*/
    struct region *r_free_next; /* next on free region list     */
    struct region *r_act_next;  /* next on active region list   */
} region_t;

/* ── Per-Process Region Table Entry ────────────────────────── */
struct proc;                    /* forward declaration          */

typedef struct pregion {
    region_t      *pr_region;   /* pointer to region            */
    struct proc   *pr_proc;     /* owning process               */
    region_type_t  pr_type;     /* type of attachment           */
    uintptr_t      pr_vaddr;    /* virtual address in process   */
    uint32_t       pr_size;     /* size of this attachment      */
    int            pr_valid;    /* slot is in use               */
} pregion_t;

/* ── Region Free / Active Lists ─────────────────────────────── */
extern region_t  region_table[NREGION];
extern region_t *free_region_list;
extern region_t *active_region_list;

/* ── Physical Memory Pool (simulated) ───────────────────────── */
#define PHYS_MEM_SIZE   (64 * 1024 * 1024)   /* 64 MB pool     */
extern uint8_t phys_mem_pool[PHYS_MEM_SIZE];
extern uint32_t phys_mem_used;

/* ── Region Algorithm Prototypes ────────────────────────────── */
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

/* ── Page / Memory Helpers ───────────────────────────────────── */
void    *pmalloc(uint32_t size);
void     pmfree(void *addr, uint32_t size);
pte_t   *pgtbl_alloc(uint32_t npages);
void     pgtbl_free(pte_t *tbl, uint32_t npages);
void     pgtbl_copy(pte_t *dst, pte_t *src, uint32_t npages);

#endif /* REGION_H */
