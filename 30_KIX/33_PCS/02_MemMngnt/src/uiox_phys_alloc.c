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
 * No heap, no system headers. Types from uix_types.h.
 *
 * @version 2.0.0  @date 2026-07-23
 */

#include "../../../../50_UIX/00_libs/00_uixlibs/sys/uix_types.h"

/* ── Page constants ─────────────────────────────────────────────────── */
#define UIOX_PAGE_SHIFT     12u
#define UIOX_PAGE_SIZE      (1u << UIOX_PAGE_SHIFT)     /* 4 KB         */
#define UIOX_PAGE_MASK      (~(uix_uintptr_t)(UIOX_PAGE_SIZE - 1u))

/* Max pages tracked — 64 MB default (16384 × 4 KB). Raise as needed.  */
#define UIOX_MAX_PAGES      16384u

/* ── Page descriptor ───────────────────────────────────────────────── */
typedef struct uiox_page {
    uix_uintptr_t      pg_phys;      /* physical address of this page   */
    struct uiox_page  *pg_next;      /* free-list link                  */
    uix_uint32_t       pg_flags;     /* reserved (dirty, pinned …)      */
    uix_uint32_t       pg_refcount;  /* 0 = free                        */
} uiox_page_t;

/* ── Allocator state ────────────────────────────────────────────────── */
static uiox_page_t  s_pages[UIOX_MAX_PAGES];
static uiox_page_t *s_free_list = (uiox_page_t *)0;
static uix_uint32_t s_nr_free   = 0u;
static uix_uint32_t s_nr_total  = 0u;
static uix_uint8_t  s_mm_ready  = 0u;

/* ── Memory descriptor (forward declaration for uiox_task_t compat) ─── */
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
 * @dram_size  size in bytes of available DRAM region
 *
 * Aligns base up / top down to page boundaries, builds free list.
 * Called once from uiox_proc_init() before any allocation.
 * ──────────────────────────────────────────────────────────────────── */
void uiox_mm_init(uix_uint64_t dram_base, uix_uint64_t dram_size)
{
    uix_uintptr_t base, top, addr;
    uix_uint32_t  i;

    base = (uix_uintptr_t)((dram_base + UIOX_PAGE_SIZE - 1u) & UIOX_PAGE_MASK);
    top  = (uix_uintptr_t)((dram_base + dram_size) & UIOX_PAGE_MASK);

    if (top <= base) return;

    s_free_list = (uiox_page_t *)0;
    s_nr_free   = 0u;
    s_nr_total  = 0u;

    /* Build free list in reverse so lowest physical addr is at head */
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

        if (addr == base) break;   /* prevent underflow */
    }

    s_mm_ready = 1u;
}

/* ────────────────────────────────────────────────────────────────────
 * phys_alloc_page — allocate one 4 KB physical page.
 *
 * Returns the physical address cast to void*, or NULL.
 * Name matches the extern declaration in archruntime.c.
 * ──────────────────────────────────────────────────────────────────── */
void *phys_alloc_page(void)
{
    uiox_page_t *pg;
    if (!s_mm_ready || !s_free_list) return (void *)0;
    pg              = s_free_list;
    s_free_list     = pg->pg_next;
    pg->pg_next     = (uiox_page_t *)0;
    pg->pg_refcount = 1u;
    s_nr_free--;
    return (void *)(uix_uintptr_t)pg->pg_phys;
}

/* ────────────────────────────────────────────────────────────────────
 * phys_free_page — return a physical page to the free list.
 * ──────────────────────────────────────────────────────────────────── */
void phys_free_page(void *page)
{
    uix_uintptr_t phys;
    uix_uint32_t  i;
    if (!page || !s_mm_ready) return;
    phys = (uix_uintptr_t)page & UIOX_PAGE_MASK;
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
