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
 * ── what changed in 1.1.0 ───────────────────────────────────────────
 * The local uiox_kix_pte_t and uiox_kix_fault_region_t are GONE.  This
 * header used to define a second PTE and a second region type, described
 * as "the fault handler's view" with extra fields.  That reading was
 * wrong at the boundary: page_fault.c does not take a handler-private
 * region, it takes 40_psa's uiox_kix_psa_pregion_t — the PRIVATE
 * per-process attachment — and reaches the shared uiox_kix_psa_region_t
 * through it.  Two structs describing one concept is the defect class
 * this tree keeps producing, and it is what produced the 14
 * conflicting-type errors: the .c used the psa_ vocabulary, this header
 * declared a rival one, and neither ever met until the linker did.
 *
 * 40_psa OWNS the type.  uiox_kix_psa_region.h guards its PTE with
 * UIOX_KIX_PSA_PTE_DEFINED so another header may include it safely —
 * that guard exists for exactly this case, and this file now uses it
 * instead of defining a rival.
 *
 * The handler's extra need — knowing what a FRAME is doing, not just
 * what a mapping permits — lives on uiox_kix_pfdata_t, where it already
 * did (pfd_content_valid), rather than on a duplicate PTE.
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
 * @version 1.1.0  @date 2026-10-11
 */
#ifndef UIOX_KIX_PAGING_H
#define UIOX_KIX_PAGING_H

#include "uiox_klibc.h"
#include "uiox_kix_pagemap.h"   /* geometry, flag bits, page_state_t */

/* 40_psa owns the PTE.  The guard UIOX_KIX_PSA_PTE_DEFINED means this
 * include is safe here and in every translation unit that also reaches
 * uiox_kix_psa_region.h directly. */
#include "uiox_kix_psa_region.h"

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
 * currently linked into the hash below.
 *
 * pfd_content_valid carries the "what is this FRAME doing" question that
 * an earlier version of this header tried to answer with a duplicate PTE
 * and a pte_state field.  It belongs here: a frame's currency is a
 * property of the frame, not of any one mapping that points at it. */
typedef struct uiox_kix_pfdata {
    int                         pfd_valid;          /* frame is assigned    */
    uint16_t                    pfd_refcnt;         /* how many PTEs point  */
    uint32_t                    pfd_pfn;            /* its frame number     */
    uiox_kix_psa_pte_t         *pfd_pte;            /* back-pointer         */
    int                         pfd_on_hash;        /* linked into pc_hash  */
    struct uiox_kix_pfdata     *pfd_hash_next;      /* bucket chain         */
    int                         pfd_content_valid;  /* contents are current */
} uiox_kix_pfdata_t;

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
 * faulted and the PROCESS'S ATTACHMENT to the region it falls in —
 * 40_psa's uiox_kix_psa_pregion_t, the private half — and reach the
 * shared uiox_kix_psa_region_t through its pr_region field.
 *
 * Neither returns a value, because a resolved fault resumes the process
 * where it was and an unresolved one signals it. */

/* Algorithm 3 — validity fault.  A page in the address space with no
 * frame assigned: bring it in from swap or a file, or from nothing if
 * it is demand-zero. */
void uiox_kix_vfault(uintptr_t faulted_addr, uiox_kix_psa_pregion_t *region);

/* Algorithm 4 — protection fault.  A VALID page accessed against its
 * permissions.  The one legal case is a write to a copy-on-write page,
 * which is what fork leaves behind; anything else is a real error. */
void uiox_kix_pfault(uintptr_t faulted_addr, uiox_kix_psa_pregion_t *region);

/* ── Helpers the two handlers share ─────────────────────────────────── */

/* Locate the page-table entry for an address, or NULL when the address
 * is outside the region — the first legality test either handler makes. */
uiox_kix_psa_pte_t *uiox_kix_find_pte(uiox_kix_psa_pregion_t *region,
                                      uintptr_t vaddr);

/* The disk block descriptor for an entry, or NULL when there is no
 * record — which is what makes a reference invalid rather than merely
 * unloaded. */
uiox_kix_disk_blk_desc_t *uiox_kix_find_dbd(uiox_kix_psa_pte_t *pte);

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
int uiox_kix_read_page_from_swap(uiox_kix_psa_pte_t *pte, uiox_kix_pfdata_t *pfd);
int uiox_kix_read_page_from_file(uiox_kix_psa_pte_t *pte, uiox_kix_pfdata_t *pfd);
void uiox_kix_free_swap_block(uint32_t blk);

/* Region locking, held across a handler so a page stealer cannot
 * intervene.
 *
 * The lock lives on the SHARED region as uiox_kix_psa_region_t.r_status,
 * bit UIOX_KIX_PSA_REG_LOCKED — not on the attachment.  Two processes
 * faulting on one region must contend for ONE lock; a per-attachment
 * lock would let both proceed into the same page table.  The attachment
 * parameter is kept because a caller holds an attachment, not a region. */
void uiox_kix_region_lock  (uiox_kix_psa_pregion_t *r);
void uiox_kix_region_unlock(uiox_kix_psa_pregion_t *r);

/* Deliver SIGSEGV to the running process.  Bach's both handlers reach
 * this when the fault is not legal. */
void uiox_kix_send_sigsegv(void);

/* Recalculate the faulting process's priority — Bach's Algorithms 3 and
 * 4 both end with "recalculate process priority".
 *
 * Takes the attachment (v1.1.0 — the parameter was dropped at some point
 * after 1.0.0, leaving the declaration at (void) while the definition in
 * page_fault.c kept the argument).  Recalculate the priority OF THE
 * FAULTING PROCESS, so the argument is not optional. */
void uiox_kix_recalc_priority_after_fault(uiox_kix_psa_pregion_t *region);

/* Bring the paging system up: zero the descriptors, seed the free list
 * with every frame.  Called once, before any fault can occur. */
void uiox_kix_page_fault_init(void);

#endif /* UIOX_KIX_PAGING_H */
