/*
 * 30_KIX/33_PCS/02_MemMngnt/include/uiox_kix_paging.h
 *
 * The paging subsystem's own types — demand paging, the page cache, and
 * the fault handler's view of a page-table entry.
 *
 * ── this header replaces page_fault.h ───────────────────────────────
 * page_fault.h carried three different kinds of thing in one file:
 *
 *   1. PAGE GEOMETRY      PAGE_SIZE, PAGE_SHIFT, PHYS_PAGES
 *   2. PTE FLAG BITS      PTE_VALID ... PTE_IN_SWAP
 *   3. THIS LAYER'S TYPES pfdata_t, fault_region_t, page_cache_t
 *
 * (1) and (2) are now in uiox_kix_pagemap.h, shared with 40_psa — which
 * is what fixed the copy-on-write defect, since the two layers had
 * disagreed about what bit 0x010 meant.
 *
 * What is left here is (3): the shapes only THIS layer needs.  A region
 * page-table entry is two fields; the fault handler's is four, because
 * it also tracks what the frame is doing.  That difference is why the
 * two structs stay separate even though they share a bit set.
 *
 * ── what is NOT here ────────────────────────────────────────────────
 * No process type.  An earlier version of the paging code reached for a
 * proc_entry_t defined in a rival scheduler header; this header names no
 * process at all, and page_fault.c binds to 40_psa's entry directly.
 *
 * ── the fault handler's two entry points ────────────────────────────
 * Bach's Algorithm 3 (vfault) and Algorithm 4 (pfault).  Both are
 * declared below; the bodies are in page_fault.c.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#ifndef UIOX_KIX_PAGING_H
#define UIOX_KIX_PAGING_H

#include "uiox_klibc.h"
#include "uiox_kix_pagemap.h"   /* geometry, flag bits, page_state_t */

/* ── Page-table entry — the FAULT HANDLER's view ─────────────────────
 * Four fields, where 40_psa's region entry has two.  The extra pair is
 * what makes this struct the handler's rather than the region's:
 *
 *   pte_pfn    the frame this entry points at
 *   pte_flags  bits from uiox_kix_pagemap.h — NOT defined here
 *   pte_state  what the frame is doing (free / filling / on swap /
 *              locked), which is a different question from what the
 *              mapping permits
 *
 * The name is deliberately the SAME as 40_psa's only in spelling, not in
 * definition — the two are guarded apart so a translation unit holding
 * both compiles. */
#ifndef UIOX_KIX_PAGING_PTE_DEFINED
#define UIOX_KIX_PAGING_PTE_DEFINED
typedef struct uiox_kix_pte {
    uint32_t     pte_pfn;
    uint16_t     pte_flags;
    page_state_t pte_state;
} uiox_kix_pte_t;
#endif

/* ── Disk block descriptor ───────────────────────────────────────────
 * Where a page's contents actually live when they are not in memory.
 * Bach's vfault consults this to decide whether a fault is legal at all:
 * "If the disk block descriptor has no record of the faulted page, the
 * attempted memory reference is invalid" — and that is the SIGSEGV case.
 *
 * dbd_type selects the source: 0 is the swap device, non-zero an
 * executable file.  That is the branch vfault takes after the legality
 * check. */
typedef struct uiox_kix_disk_blk_desc {
    uint32_t dbd_blkno;    /* block number on its device         */
    int      dbd_type;     /* 0 = swap device, else exec file    */
    int      dbd_valid;    /* 1 = this descriptor describes a page */
} uiox_kix_disk_blk_desc_t;

/* ── Page frame data ─────────────────────────────────────────────────
 * One record per frame the paging system knows about.  This is the
 * bookkeeping Bach's cache keeps separate from the page table: a frame
 * may be referenced by more than one PTE (shared text, or a COW pair
 * before either writes), and pfd_refcnt is what tells them apart.
 *
 * pfd_pte is a back-pointer to the entry, so an eviction can find what
 * referenced the frame.  pfd_on_hash reports whether the frame is
 * currently linked into the hash below. */
typedef struct uiox_kix_pfdata {
    int                         pfd_valid;          /* frame is assigned    */
    uint16_t                    pfd_refcnt;         /* how many PTEs point  */
    uint32_t                    pfd_pfn;            /* its frame number     */
    uiox_kix_pte_t             *pfd_pte;            /* back-pointer         */
    int                         pfd_on_hash;        /* linked into pc_hash  */
    struct uiox_kix_pfdata     *pfd_hash_next;      /* bucket chain         */
    int                         pfd_content_valid;  /* contents are current */
} uiox_kix_pfdata_t;

/* ── Fault region ────────────────────────────────────────────────────
 * A region as the FAULT HANDLER sees it: where it starts, how big it is,
 * whether it is locked, and its page table.
 *
 * This is not 40_psa's uiox_kix_psa_region_t.  That one carries a
 * reference count and file backing because it is the SHARED object; this
 * one carries fr_locked because the handler must lock the region while
 * it works — Bach: "lock region to prevent race conditions that would
 * occur if the page stealer attempted to swap the page out".
 *
 * A future pass should make one derive from the other rather than
 * describe it twice.  Until then the distinction is stated so a reader
 * knows which of the two they are holding. */
typedef struct uiox_kix_fault_region {
    uintptr_t           fr_vaddr_start;   /* base address               */
    uint32_t            fr_size;          /* bytes                      */
    int                 fr_locked;        /* 1 = handler is working on it */
    uiox_kix_pte_t     *fr_pgtbl;         /* the page table             */
    uint32_t            fr_npages;        /* entries in fr_pgtbl        */
} uiox_kix_fault_region_t;

/* ── Page cache ──────────────────────────────────────────────────────
 * A hash of resident frames, bucketed by frame number.  Bach's vfault
 * consults it first — "if (page in cache)" — because a frame another
 * process already faulted in needs no I/O, only a page-table update. */
typedef struct uiox_kix_page_cache {
    uiox_kix_pfdata_t *pc_hash[UIOX_PAGE_HASH_SZ];
    int                pc_total;          /* frames currently hashed */
} uiox_kix_page_cache_t;

/* ── Free page list ──────────────────────────────────────────────────
 * A ring of frame numbers, not of descriptors: the descriptors are
 * uiox_phys_alloc.c's uiox_page_t pool, and this list is the paging
 * layer's view of which of them are available.
 *
 * head/tail rather than a count-only scheme, so allocation and release
 * are both O(1) and the ring never needs compacting. */
typedef struct uiox_kix_free_page_list {
    uint32_t fpl_pages[UIOX_PHYS_PAGES];
    int      fpl_count;
    int      fpl_head;
    int      fpl_tail;
} uiox_kix_free_page_list_t;

/* ── Globals ─────────────────────────────────────────────────────────
 * Defined in page_fault.c.  The sizing uses the SHARED constants, so a
 * change to the frame count happens in one place. */
extern uiox_kix_pfdata_t           uiox_kix_pfdata_table[UIOX_PHYS_PAGES];
extern uiox_kix_page_cache_t       uiox_kix_page_cache;
extern uiox_kix_free_page_list_t   uiox_kix_free_pages;

/* ── The fault handler ───────────────────────────────────────────────
 * Bach's Algorithm 3 and Algorithm 4.  Both take the address that
 * faulted and the region it falls in; neither returns a value, because
 * a resolved fault resumes the process where it was and an unresolved
 * one signals it. */

/* Algorithm 3 — validity fault.  A page in the address space with no
 * frame assigned: bring it in from swap or a file, or from nothing if
 * it is demand-zero. */
void uiox_kix_vfault(uintptr_t faulted_addr, uiox_kix_fault_region_t *region);

/* Algorithm 4 — protection fault.  A VALID page accessed against its
 * permissions.  The one legal case is a write to a copy-on-write page,
 * which is what fork leaves behind; anything else is a real error. */
void uiox_kix_pfault(uintptr_t faulted_addr, uiox_kix_fault_region_t *region);

/* ── Helpers the two handlers share ─────────────────────────────────── */

/* Locate the page-table entry for an address, or NULL when the address
 * is outside the region — the first legality test either handler makes. */
uiox_kix_pte_t *uiox_kix_find_pte(uiox_kix_fault_region_t *region,
                                  uintptr_t vaddr);

/* The disk block descriptor for an entry, or NULL when there is no
 * record — which is what makes a reference invalid rather than merely
 * unloaded. */
uiox_kix_disk_blk_desc_t *uiox_kix_find_dbd(uiox_kix_pte_t *pte);

/* Frame lookup and cache maintenance. */
uiox_kix_pfdata_t *uiox_kix_find_in_cache(uint32_t pfn);
void               uiox_kix_add_to_cache(uiox_kix_pfdata_t *pfd);
void               uiox_kix_remove_from_cache(uiox_kix_pfdata_t *pfd);

/* Frame allocation from the free list, and its return. */
uiox_kix_pfdata_t *uiox_kix_alloc_physical_page(void);
void               uiox_kix_free_physical_page(uint32_t pfn);

/* Content transfer.  Both set pfd_content_valid on success; a real
 * kernel sleeps on the I/O, which needs a waker this build does not
 * have yet. */
int uiox_kix_read_page_from_swap(uiox_kix_pte_t *pte, uiox_kix_pfdata_t *pfd);
int uiox_kix_read_page_from_file(uiox_kix_pte_t *pte, uiox_kix_pfdata_t *pfd);
void uiox_kix_free_swap_block(uint32_t blk);

/* Region locking, held across a handler so a page stealer cannot
 * intervene. */
void uiox_kix_region_lock  (uiox_kix_fault_region_t *r);
void uiox_kix_region_unlock(uiox_kix_fault_region_t *r);

/* Deliver SIGSEGV to the running process.  Bach's both handlers reach
 * this when the fault is not legal. */
void uiox_kix_send_sigsegv(void);

/* Recalculate the faulting process's priority — Bach's Algorithms 3 and
 * 4 both end with "recalculate process priority". */
void uiox_kix_recalc_priority_after_fault(void);

/* Bring the paging system up: zero the descriptors, seed the free list
 * with every frame.  Called once, before any fault can occur. */
void uiox_kix_page_fault_init(void);

#endif /* UIOX_KIX_PAGING_H */
