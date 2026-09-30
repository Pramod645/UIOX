/*
 * 30_KIX/33_PCS/include/uiox_kix_pagemap.h
 *
 * Page geometry and page-table-entry flag bits — ONE definition, shared
 * by every layer that walks a page table.
 *
 * ── why this file exists ────────────────────────────────────────────
 * Four headers had grown their own idea of what a page is:
 *
 *   page_fault.h (02_MemMngnt)   PAGE_SIZE 4096   PHYS_PAGES 2048
 *   swapper.h    (02_MemMngnt)   PHYS_PAGES 2048
 *   mm.c         (02_MemMngnt)   UIOX_PAGE_SIZE 4096  UIOX_MAX_PAGES 16384
 *   40_psa region header         UIOX_KIX_PSA_PAGE_SIZE 4096
 *                                UIOX_KIX_PSA_MAX_PAGES 1024
 *
 * Two of those disagreed about the page COUNT, and two disagreed about
 * what the flag bit 0x010 MEANS:
 *
 *   page_fault.h : PTE_ACCESSED 0x010   PTE_COW 0x020
 *   40_psa       : UIOX_KIX_PSA_PTE_COW 0x010
 *
 * That second disagreement was a live defect, not a tidiness problem.
 * dupreg() set the region's COW bit (0x010); the fault handler tested
 * its own COW bit (0x020); the test failed, the handler took the
 * "real program error" branch, and a child process died on its first
 * write after fork.  Copy-on-write never fired.
 *
 * ── the rule this file establishes ──────────────────────────────────
 * A flag bit has ONE value, and it lives in ONE place.  Both structs
 * keep their own fields — page_fault.h's pte_t carries pte_state and
 * four flags the region entry does not need — but neither may define a
 * bit.  They include this header instead.
 *
 * ── the values are page_fault.h's, and that is deliberate ───────────
 * Where the two disagreed the fault handler's set won, because it is
 * the LARGER: it already uses 0x010 through 0x100 and the region only
 * claimed 0x001 through 0x010.  Adopting the smaller set would have
 * meant renumbering bits the fault handler is already testing.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#ifndef UIOX_KIX_PAGEMAP_H
#define UIOX_KIX_PAGEMAP_H

#include "uiox_klibc.h"

/* ── Page geometry ─────────────────────────────────────────────────
 * PAGE_SHIFT is the authority; PAGE_SIZE is derived from it, so the
 * two cannot drift.  A 4 KB page on every target this kernel builds for. */
#define UIOX_PAGE_SHIFT     12u
#define UIOX_PAGE_SIZE      (1u << UIOX_PAGE_SHIFT)     /* 4096          */
#define UIOX_PAGE_MASK      (~(uintptr_t)(UIOX_PAGE_SIZE - 1u))

/* The bare names, for the headers that used them before this one
 * existed.  Both are the SAME value as UIOX_PAGE_*; a source may use
 * either spelling and mean one thing. */
#ifndef PAGE_SHIFT
#define PAGE_SHIFT          UIOX_PAGE_SHIFT
#endif
#ifndef PAGE_SIZE
#define PAGE_SIZE           UIOX_PAGE_SIZE
#endif

/* ── How many pages a pool describes ─────────────────────────────────
 * TWO counts, and both are needed — they are not a disagreement:
 *
 *   UIOX_MAX_PAGES    how many page DESCRIPTORS the allocator pools.
 *                     16384 x 4 KB is 64 MB, the default DRAM region.
 *
 *   UIOX_PHYS_PAGES   how many FRAMES the simulated physical memory
 *                     holds. 2048 x 4 KB is 8 MB.
 *
 * The allocator can describe more pages than a given machine has; the
 * fault handler works within what the machine reports.  An earlier
 * version had three different numbers for these two concepts, which is
 * why they are named apart here rather than merged into one. */
#define UIOX_MAX_PAGES      16384u
#define UIOX_PHYS_PAGES     2048u

#ifndef PHYS_PAGES
#define PHYS_PAGES          UIOX_PHYS_PAGES
#endif

#define UIOX_PAGE_HASH_SZ   256u
#ifndef PAGE_HASH_SZ
#define PAGE_HASH_SZ        UIOX_PAGE_HASH_SZ
#endif

/* ── Page-table-entry flag bits ──────────────────────────────────────
 * One bit per meaning, no aliasing.  Low nibble is permissions, next is
 * status, then the paging states. */

/* permissions and basic state */
#define PTE_VALID        0x001u   /* mapping is usable               */
#define PTE_WRITE        0x002u   /* writable                        */
#define PTE_USER         0x004u   /* user-mode accessible            */
#define PTE_DIRTY        0x008u   /* written since last load         */

/* status — 0x010 is ACCESSED, and NOT copy-on-write.  This is the bit
 * the region header used to call COW; see the banner. */
#define PTE_ACCESSED     0x010u   /* referenced since last clear     */

/* copy-on-write — set by dupreg at fork, cleared by pfault on first
 * write.  The handler that tests it and the function that sets it now
 * agree because they read the same constant. */
#define PTE_COW          0x020u   /* private copy on write           */

/* fill policy */
#define PTE_DEMAND_ZERO  0x040u   /* zero-fill on first touch        */
#define PTE_DEMAND_FILL  0x080u   /* fault-fill, contents unknown    */

/* paging state */
#define PTE_IN_SWAP      0x100u   /* contents live on the swap device */

/* ── Page states ─────────────────────────────────────────────────────
 * What a frame is doing, as opposed to what its PTE says.  Used by
 * pfdata_t's bookkeeping. */
typedef enum {
    PAGE_FREE        = 0,
    PAGE_DEMAND_FILL = 1,
    PAGE_ON_SWAP     = 2,
    PAGE_LOCKED      = 3
} page_state_t;

#endif /* UIOX_KIX_PAGEMAP_H */
