/*
 * 30_KIX/33_PCS/40psa/include/uiox_kix_psa_region.h
 *
 * PSA region side.  A region is the SHARED half of a process's memory;
 * a pregion is one process's attachment to that region.
 *
 * ── why the split exists ────────────────────────────────────────────
 * Text may be shared between processes that executed the same file.
 * PSA therefore puts the page table and a reference count on the REGION,
 * and keeps only the per-process mapping (where it sits in THIS process's
 * address space) on the pregion.  That is what makes fork cheap: dupreg
 * increments r_refcnt and the two processes share frames until one writes.
 *
 * ── copy-on-write ───────────────────────────────────────────────────
 * The write flag is cleared and PTE_COW set on every writable page at
 * fork time.  02_MemMngnt's fault handler tests that bit and copies the
 * frame on first write.  Without the flag, a fork would give both
 * processes the same writable frame and they would corrupt each other.
 *
 * ── the pte_t guard ─────────────────────────────────────────────────
 * page_fault.h defines a pte_t of its own and the two are NOT identical.
 * A named guard is used rather than a merge, because merging them would
 * silently change the layout one layer already depends on.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#ifndef UIOX_KIX_PSA_REGION_H
#define UIOX_KIX_PSA_REGION_H

#include "uiox_kix_psa_process.h"

/* ── Limits ────────────────────────────────────────────────────────── */
#define UIOX_KIX_PSA_NREGION          128
#define UIOX_KIX_PSA_PAGE_SIZE        4096
#define UIOX_KIX_PSA_MAX_PAGES        1024
#define UIOX_KIX_PSA_MAX_REG_PER_PROC 8
#define UIOX_KIX_PSA_MAX_REGION_SIZE \
    (UIOX_KIX_PSA_MAX_PAGES * UIOX_KIX_PSA_PAGE_SIZE)   /* 4 MB */

/* ── Error codes set in u_error by the algorithms ──────────────────── */
#ifndef ENOMEM
#define ENOMEM  12   /* no free region, or growth past MAX_REGION_SIZE */
#endif
#ifndef EFAULT
#define EFAULT  14   /* bad address                                     */
#endif

/* ── Region types ──────────────────────────────────────────────────────
 * The type decides sharing.  TEXT is the one that is normally shared
 * across processes; DATA and STACK are private after the first write. */
typedef enum uiox_kix_psa_region_type {
    UIOX_KIX_PSA_REG_TEXT  = 1,
    UIOX_KIX_PSA_REG_DATA  = 2,
    UIOX_KIX_PSA_REG_STACK = 3,
    UIOX_KIX_PSA_REG_SHMEM = 4
} uiox_kix_psa_region_type_t;

/* ── Region status bits ────────────────────────────────────────────────
 * LOCKED   — the region is being modified; a concurrent fault must wait
 * DEMAND   — pages are created on demand rather than loaded up front
 * LOADING  — a loadreg is in progress and the pages are NOT yet valid
 * VALID    — the region's page table is populated */
#define UIOX_KIX_PSA_REG_LOCKED   0x01
#define UIOX_KIX_PSA_REG_DEMAND   0x02
#define UIOX_KIX_PSA_REG_LOADING  0x04
#define UIOX_KIX_PSA_REG_VALID    0x08

/* ── Page table entry ──────────────────────────────────────────────── */
#ifndef UIOX_KIX_PSA_PTE_DEFINED
#define UIOX_KIX_PSA_PTE_DEFINED
typedef struct uiox_kix_psa_pte {
    uint32_t pte_pfn;     /* physical frame number                     */
    uint16_t pte_flags;   /* permission and status bits below          */
} uiox_kix_psa_pte_t;

#define UIOX_KIX_PSA_PTE_VALID  0x001   /* mapping is usable          */
#define UIOX_KIX_PSA_PTE_WRITE  0x002   /* writable                   */
#define UIOX_KIX_PSA_PTE_USER   0x004   /* user-mode accessible       */
#define UIOX_KIX_PSA_PTE_DIRTY  0x008   /* written since last load    */
#define UIOX_KIX_PSA_PTE_COW    0x010   /* copy-on-write after fork   */
#endif

/* ── Region descriptor — the SHARED half ─────────────────────────────
 * r_refcnt counts attachments.  It is incremented by attachreg and
 * decremented by detachreg; the region is freed when it reaches zero,
 * which is why freereg refuses to run while the count is non-zero. */
typedef struct uiox_kix_psa_region {
    uiox_kix_psa_region_type_t  r_type;      /* text/data/stack/shmem     */
    struct inode               *r_inode;     /* backing file, may be NULL */
    uint16_t                    r_refcnt;    /* how many processes share  */
    uiox_kix_psa_pte_t         *r_pgtbl;     /* shared page table         */
    uint32_t                    r_npages;    /* entries in r_pgtbl        */
    uint32_t                    r_size;      /* bytes, not pages          */
    uint16_t                    r_status;    /* UIOX_KIX_PSA_REG_* bits   */
    struct uiox_kix_psa_region *r_free_next; /* free-list linkage         */
    struct uiox_kix_psa_region *r_act_next;  /* active-list linkage       */
} uiox_kix_psa_region_t;

/* ── Per-process attachment — the PRIVATE half ───────────────────────
 * pr_vaddr is the address THIS process sees the region at; two processes
 * sharing one region may map it at different addresses, which is exactly
 * why the address is stored here and not on the region. */
typedef struct uiox_kix_psa_pregion {
    uiox_kix_psa_region_t     *pr_region;  /* the shared region            */
    struct uiox_kix_psa_proc  *pr_proc;    /* owning process               */
    uiox_kix_psa_region_type_t pr_type;    /* cached copy of r_type        */
    uintptr_t                  pr_vaddr;   /* base address for this proc   */
    uint32_t                   pr_size;    /* cached copy of r_size        */
    int                        pr_valid;   /* slot in use                  */
} uiox_kix_psa_pregion_t;

/* ── Globals ────────────────────────────────────────────────────────── */
extern uiox_kix_psa_region_t  uiox_kix_psa_region_table[UIOX_KIX_PSA_NREGION];
extern uiox_kix_psa_region_t *uiox_kix_psa_free_region_list;
extern uiox_kix_psa_region_t *uiox_kix_psa_active_region_list;

/* ── Physical memory arena ───────────────────────────────────────────
 * A flat 64 MB array, bump-allocated.  This is one view of the machine's
 * RAM; 31BufferCache maps the same physical memory through the SoC memory
 * map instead.  Two views of one thing, which is why neither may assume
 * it owns the arena. */
#define UIOX_KIX_PSA_PHYS_MEM_SIZE  (64 * 1024 * 1024)
extern uint8_t  uiox_kix_psa_phys_mem_pool[UIOX_KIX_PSA_PHYS_MEM_SIZE];
extern uint32_t uiox_kix_psa_phys_mem_used;

/* ── PSA region algorithms 3..9 ────────────────────────────────────── */

/* PSA Algorithm 3 — take a free region and mark it active.
 * Sets refcnt to ZERO: a fresh region belongs to nobody until attachreg
 * is called.  Returns NULL when no region is free (u_error should be
 * set to ENOMEM by the caller's convention). */
uiox_kix_psa_region_t *uiox_kix_psa_allocreg(
        struct inode *ip, uiox_kix_psa_region_type_t type);

/* PSA Algorithm 4 — attach a region to a process at a given address.
 * Finds a free pregion slot in the caller's u area, records the mapping,
 * and increments the region's reference count.  Returns the pregion, or
 * NULL when the caller already holds MAX_REG_PER_PROC regions. */
uiox_kix_psa_pregion_t *uiox_kix_psa_attachreg(
        uiox_kix_psa_region_t *rp, struct uiox_kix_psa_proc *p,
        uintptr_t vaddr, uiox_kix_psa_region_type_t type);

/* PSA Algorithm 5 — change a region's size by delta bytes.
 * Refuses growth past MAX_REGION_SIZE (u_error = ENOMEM) and refuses a
 * shrink past zero (u_error = EINVAL) — the second check is the one the
 * original lacked, and without it r_npages wraps to a nonsense value.
 * Allocates a page table on first growth into a region that had none. */
void uiox_kix_psa_growreg(uiox_kix_psa_pregion_t *prp, int32_t delta);

/* PSA Algorithm 6 — load a file's contents into a region.
 * Marks the region LOADING and stops.  It does NOT mark pages VALID and
 * does NOT claim to have read anything, because no readi()-equivalent is
 * wired from 01_fsa yet.  Claiming success on unread data is the same
 * defect as fabricating a header and then validating it. */
void uiox_kix_psa_loadreg(uiox_kix_psa_pregion_t *prp, uintptr_t vaddr,
                          struct inode *ip, uint32_t file_off,
                          uint32_t byte_count);

/* PSA Algorithm 7 — return a region to the free list.
 * Refuses while r_refcnt is non-zero: another process still maps it, so
 * freeing now would leave a dangling page table.  Frees the page table,
 * unlinks from the active list, and pushes onto the free list. */
void uiox_kix_psa_freereg(uiox_kix_psa_region_t *rp);

/* PSA Algorithm 8 — remove one process's attachment to a region.
 * Decrements the reference count and, when it reaches zero, frees the
 * region itself.  The pregion slot is then cleared so attachreg can
 * reuse it.  Safe to call on an already-detached pregion. */
void uiox_kix_psa_detachreg(uiox_kix_psa_pregion_t *prp);

/* PSA Algorithm 9 — duplicate a region for fork.
 * Copies the descriptor and the page table, then marks every writable
 * page copy-on-write so parent and child share frames until one writes.
 * Returns an UNATTACHED region: the caller attaches it to the child with
 * attachreg, which is the order PSA's fork algorithm uses. */
uiox_kix_psa_region_t *uiox_kix_psa_dupreg(uiox_kix_psa_region_t *rp);

/* ── Pool and page-table helpers ───────────────────────────────────── */

/* Bump-allocate size bytes from the physical arena.  Returns NULL when
 * the arena is exhausted.  There is no matching general free: the arena
 * is reclaimed as a whole, which is why pmfree is a no-op. */
void *uiox_kix_psa_pmalloc(uint32_t size);

/* Reclaim an allocation.  A no-op, deliberately: a bump allocator cannot
 * return an interior block, and pretending otherwise would let a caller
 * believe memory was returned when it was not. */
void uiox_kix_psa_pmfree(void *addr, uint32_t size);

/* Take a page table of npages entries from the static pool.
 * The pool exists because freestanding has no heap.  Returns NULL if
 * npages is zero, exceeds MAX_PAGES, or every slot is taken. */
uiox_kix_psa_pte_t *uiox_kix_psa_pgtbl_alloc(uint32_t npages);

/* Return a page table to the pool by matching its base address. */
void uiox_kix_psa_pgtbl_free(uiox_kix_psa_pte_t *tbl, uint32_t npages);

/* Copy npages entries from src to dst.  Used by dupreg to give the child
 * its own page table over the shared frames. */
void uiox_kix_psa_pgtbl_copy(uiox_kix_psa_pte_t *dst,
                             uiox_kix_psa_pte_t *src, uint32_t npages);

/* Thread every region onto the free list and zero the arena counter.
 * Must run before any allocreg; calling it twice is harmless. */
void uiox_kix_psa_region_init(void);

#endif /* UIOX_KIX_PSA_REGION_H */
