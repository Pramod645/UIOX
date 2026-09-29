/*
 * 30_KIX/33_PCS/02_MemMngnt/src/uiox_phys_alloc.c
 *
 * Physical page allocator — free-list over a contiguous DRAM region.
 *
 * Provides:
 *   uiox_mm_init()      called by uiox_proc_init()
 *   phys_alloc_page()   declared extern in archruntime.c
 *   phys_free_page()
 *
 * No heap, no system headers.  Types from uix_types.h.
 *
 * ── this file replaces mm.c ─────────────────────────────────────────
 * mm.c was the same allocator, byte for byte: same uiox_mm_init, same
 * free list, same UIOX_MAX_PAGES.  The two differed in two ways, and
 * BOTH are now handled here so the survivor is strictly the better one:
 *
 *   1. mm.c used bare uintptr_t; this file uses uix_uintptr_t from
 *      uix_types.h, which is the BSP's own type and the one the rest of
 *      the kernel is written against.
 *
 *   2. mm.c carried PTR_TO_UINTPTR / UINTPTR_TO_PTR — memcpy helpers
 *      that move a pointer through a uintptr_t WITHOUT a cast, because
 *      on arm32 a direct cast trips -Werror=pointer-to-int-cast.  This
 *      file used direct casts, which compile on arm64 and x86_64 and
 *      FAIL on arm32.  The helpers are carried over below; see the note
 *      on UINTPTR_TO_PTR for why they are not merely a workaround.
 *
 * ── no heap ─────────────────────────────────────────────────────────
 * Page descriptors are a static pool.  UIOX_MAX_PAGES bounds how many
 * pages the allocator can DESCRIBE — 16384 x 4 KB is 64 MB, the default
 * DRAM region — which is a different quantity from how many frames a
 * machine actually has.  That second number is UIOX_PHYS_PAGES in
 * uiox_kix_pagemap.h.
 *
 * @version 3.0.0  @date 2026-09-29
 */

#include "../../../../50_UIX/00_libs/00_uixlibs/sys/uix_types.h"
#include "uiox_kix_pagemap.h"   /* UIOX_PAGE_SHIFT, UIOX_PAGE_SIZE, MASK */

/* ── Pointer <-> integer transit ─────────────────────────────────────
 * memcpy rather than a cast, and on arm32 that is not a style choice.
 *
 * A cast from void * to uintptr_t is a size change there: uintptr_t is
 * 32-bit, a pointer is 32-bit, but GCC still diagnoses the pair as
 * int-to-pointer and pointer-to-int when the intermediate types differ.
 * The (void *)(uintptr_t) doubling-out does not help either — GCC
 * evaluates each cast independently and warns on the inner one.
 *
 * memcpy is the strictly-conforming route: it copies the object
 * representation, so no conversion happens in the language's eyes and no
 * diagnostic fires.  sizeof guards length equality, which holds on every
 * target this kernel builds for. */
#define PTR_TO_UINTPTR(dst, src)                                \
    do { const void *_q = (const void *)(src);                  \
         memcpy(&(dst), &_q, sizeof(dst)); } while (0)

#define UINTPTR_TO_PTR(dst, src)                                \
    do { uix_uintptr_t _u = (uix_uintptr_t)(src);               \
         memcpy(&(dst), &_u, sizeof(dst)); } while (0)

/* ── Page constants ─────────────────────────────────────────────────
 * Taken from uiox_kix_pagemap.h rather than restated.  UIOX_PAGE_SHIFT
 * is the authority there and UIOX_PAGE_SIZE derives from it, so the two
 * cannot drift. */
#define UIOX_MAX_PAGES      16384u

/* ── Page descriptor ───────────────────────────────────────────────── */
typedef struct uiox_page {
    uix_uintptr_t      pg_phys;      /* physical address of this page  */
    struct uiox_page  *pg_next;      /* free-list link                 */
    uix_uint32_t       pg_flags;     /* reserved (dirty, pinned ...)   */
    uix_uint32_t       pg_refcount;  /* 0 = free                       */
} uiox_page_t;

/* ── Allocator state ────────────────────────────────────────────────── */
static uiox_page_t  s_pages[UIOX_MAX_PAGES];
static uiox_page_t *s_free_list = (uiox_page_t *)0;
static uix_uint32_t s_nr_free   = 0u;
static uix_uint32_t s_nr_total  = 0u;
static uix_uint8_t  s_mm_ready  = 0u;

/* ── Memory descriptor ───────────────────────────────────────────────
 * Forward-declared here, and this IS the definition: uiox_task_t holds a
 * pointer to it.  Kept as a bare struct rather than a typedef because
 * that is how uiox_task.h refers to it. */
struct uiox_mm_desc {
    uix_uintptr_t  mm_pgd_phys;     /* physical addr of page-global-dir */
    uix_uintptr_t  mm_mmap_base;    /* start of user mmap area          */
    uix_uintptr_t  mm_mmap_top;     /* end of user mmap area            */
    uix_uintptr_t  mm_brk_start;    /* start of heap                    */
    uix_uintptr_t  mm_brk_current;  /* current heap break               */
};

/* ────────────────────────────────────────────────────────────────────
 * uiox_mm_init — initialise the physical page allocator.
 *
 * @dram_base  physical address of first available DRAM byte
 * @dram_size  size in bytes of the available DRAM region
 *
 * Aligns base up and top down to page boundaries, then builds the free
 * list in REVERSE so the lowest physical address ends up at the head —
 * which makes the first allocation the lowest frame, the ordering a
 * reader of a page dump expects.
 *
 * Called once from uiox_proc_init() before any allocation.
 * ──────────────────────────────────────────────────────────────────── */
void uiox_mm_init(uix_uint64_t dram_base, uix_uint64_t dram_size)
{
    uix_uintptr_t base, top, addr;
    uix_uint32_t  i;

    base = (uix_uintptr_t)((dram_base + UIOX_PAGE_SIZE - 1u) & UIOX_PAGE_MASK);
    top  = (uix_uintptr_t)((dram_base + dram_size) & UIOX_PAGE_MASK);

    if (top <= base) return;   /* region smaller than one page */

    s_free_list = (uiox_page_t *)0;
    s_nr_free   = 0u;
    s_nr_total  = 0u;

    for (addr = top - UIOX_PAGE_SIZE, i = 0u;
         i < UIOX_MAX_PAGES;
         addr -= UIOX_PAGE_SIZE, i++) {

        uiox_page_t *pg = &s_pages[i];
        pg->pg_phys     = addr;
        pg->pg_refcount = 0u;
        pg->pg_flags    = 0u;
        pg->pg_next     = s_free_list;
        s_free_list     = pg;
        s_nr_free++;
        s_nr_total++;

        if (addr == base) break;   /* stop before underflowing addr */
    }

    s_mm_ready = 1u;
}

/* ────────────────────────────────────────────────────────────────────
 * phys_alloc_page — allocate one physical page.
 *
 * Returns the physical address as a void*, or NULL when the pool is
 * empty.  Name matches the extern declaration in archruntime.c.
 *
 * The return goes through UINTPTR_TO_PTR rather than a cast — see the
 * macro's note.  A caller in a hosted build would dereference the
 * result; in this kernel it is a physical address being carried across
 * a void * parameter, which is the interface archruntime.c declares.
 * ──────────────────────────────────────────────────────────────────── */
void *phys_alloc_page(void)
{
    uiox_page_t *pg;
    void        *ret;

    if (!s_mm_ready || !s_free_list) return (void *)0;

    pg              = s_free_list;
    s_free_list     = pg->pg_next;
    pg->pg_next     = (uiox_page_t *)0;
    pg->pg_refcount = 1u;
    s_nr_free--;

    UINTPTR_TO_PTR(ret, pg->pg_phys);
    return ret;
}

/* ────────────────────────────────────────────────────────────────────
 * phys_free_page — return a page to the free list.
 *
 * Masks the address to a page boundary first, so a pointer into the
 * middle of a page still matches its descriptor rather than failing to
 * find one.  A pointer that matches nothing is ignored: freeing
 * something not from this pool is a caller error, and inventing a
 * descriptor for it would corrupt the list.
 * ──────────────────────────────────────────────────────────────────── */
void phys_free_page(void *page)
{
    uix_uintptr_t phys;
    uix_uint32_t  i;

    if (!page || !s_mm_ready) return;

    PTR_TO_UINTPTR(phys, page);
    phys &= UIOX_PAGE_MASK;

    for (i = 0u; i < s_nr_total; i++) {
        if (s_pages[i].pg_phys == phys) {
            if (s_pages[i].pg_refcount > 0u)
                s_pages[i].pg_refcount--;
            if (s_pages[i].pg_refcount == 0u) {
                s_pages[i].pg_next = s_free_list;
                s_free_list = &s_pages[i];
                s_nr_free++;
            }
            return;
        }
    }
}

/* ────────────────────────────────────────────────────────────────────
 * uiox_mm_free_pages / uiox_mm_total_pages — diagnostics.
 * ──────────────────────────────────────────────────────────────────── */
uix_uint32_t uiox_mm_free_pages(void)  { return s_nr_free;  }
uix_uint32_t uiox_mm_total_pages(void) { return s_nr_total; }
