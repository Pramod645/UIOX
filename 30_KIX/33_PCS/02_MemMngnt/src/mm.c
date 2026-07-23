/*
 * 30_KIX/33_PCS/02_MemMngnt/src/mm.c
 *
 * Physical page allocator — free-list over a contiguous DRAM region.
 *
 * Provides:
 *   uiox_mm_init()      — called by uiox_proc_init()
 *   phys_alloc_page()   — declared extern in archruntime.c
 *   phys_free_page()
 *   uiox_mm_free_pages()
 *   uiox_mm_total_pages()
 *
 * @version 2.0.0  @date 2026-07-23
 */

/*
 * uiox_klibc.h must be reachable via -I<path>/33_PCS/include.
 * It provides: uint8_t, uint32_t, uint64_t, uintptr_t, size_t, NULL.
 */
#include "uiox_klibc.h"

/* ── Memory descriptor forward declaration (for uiox_task.h compat) ─── */
struct uiox_mm_desc {
    uintptr_t  mm_pgd_phys;     /* physical addr of page-global-directory */
    uintptr_t  mm_mmap_base;    /* start of user mmap area                */
    uintptr_t  mm_mmap_top;     /* end of user mmap area                  */
    uintptr_t  mm_brk_start;    /* start of heap                          */
    uintptr_t  mm_brk_current;  /* current heap break                     */
};

/* ── Page constants ─────────────────────────────────────────────────── */
#define UIOX_PAGE_SHIFT   12
#define UIOX_PAGE_SIZE    (1U << UIOX_PAGE_SHIFT)   /* 4 KB              */
#define UIOX_PAGE_MASK    (~(uintptr_t)(UIOX_PAGE_SIZE - 1U))

/* Max pages tracked — 64 MB default (16384 × 4 KB). Raise as needed.  */
#define UIOX_MAX_PAGES    16384

/* ── Page descriptor ───────────────────────────────────────────────── */
typedef struct uiox_page {
    uintptr_t          pg_phys;      /* physical address of this page   */
    struct uiox_page  *pg_next;      /* free-list link (NULL if in use) */
    uint32_t           pg_flags;     /* reserved: dirty, pinned, …      */
    uint32_t           pg_refcount;  /* 0 = free, >0 = in use           */
} uiox_page_t;

/* ── Allocator state ────────────────────────────────────────────────── */
static uiox_page_t  s_pages[UIOX_MAX_PAGES];
static uiox_page_t *s_free_list = (uiox_page_t *)0;
static uint32_t     s_nr_free   = 0;
static uint32_t     s_nr_total  = 0;
static uint8_t      s_mm_ready  = 0;

/* ────────────────────────────────────────────────────────────────────
 * uiox_mm_init — initialise the physical page allocator.
 *
 * @dram_base  physical address of first available DRAM byte
 * @dram_size  size in bytes of available DRAM region
 *
 * Aligns base up and top down to page boundaries, then builds the
 * free list in reverse order so the lowest physical address is
 * allocated first. Called once from uiox_proc_init().
 * ──────────────────────────────────────────────────────────────────── */
void uiox_mm_init(uint64_t dram_base, uint64_t dram_size)
{
    uintptr_t base, top, addr;
    uint32_t  i;

    base = (uintptr_t)((dram_base + UIOX_PAGE_SIZE - 1U) & UIOX_PAGE_MASK);
    top  = (uintptr_t)((dram_base + dram_size) & UIOX_PAGE_MASK);

    if (top <= base) { return; }   /* degenerate / zero region */

    s_free_list = (uiox_page_t *)0;
    s_nr_free   = 0;
    s_nr_total  = 0;

    /* Build free list in reverse so lowest physical addr is at head */
    for (addr = top - UIOX_PAGE_SIZE, i = 0;
         i < UIOX_MAX_PAGES;
         addr -= UIOX_PAGE_SIZE, i++) {

        uiox_page_t *pg = &s_pages[i];
        pg->pg_phys      = addr;
        pg->pg_refcount  = 0;
        pg->pg_flags     = 0;
        pg->pg_next      = s_free_list;
        s_free_list      = pg;
        s_nr_free++;
        s_nr_total++;

        if (addr == base) { break; }  /* prevent uintptr_t underflow */
    }

    s_mm_ready = 1;
}

/* ────────────────────────────────────────────────────────────────────
 * phys_alloc_page — allocate one 4 KB physical page.
 *
 * Returns the physical address cast to void*, or NULL on exhaustion.
 * The caller maps it into virtual address space before use.
 *
 * Name matches the extern declaration in archruntime.c.
 * ──────────────────────────────────────────────────────────────────── */
void *phys_alloc_page(void)
{
    uiox_page_t *pg;

    if (!s_mm_ready || !s_free_list) { return (void *)0; }

    pg               = s_free_list;
    s_free_list      = pg->pg_next;
    pg->pg_next      = (uiox_page_t *)0;
    pg->pg_refcount  = 1;
    s_nr_free--;

    return (void *)(uintptr_t)pg->pg_phys;
}

/* ────────────────────────────────────────────────────────────────────
 * phys_free_page — return a physical page to the free list.
 * ──────────────────────────────────────────────────────────────────── */
void phys_free_page(void *page)
{
    uintptr_t phys;
    uint32_t  i;

    if (!page || !s_mm_ready) { return; }

    phys = (uintptr_t)page & UIOX_PAGE_MASK;

    /* Find the descriptor for this physical address */
    for (i = 0; i < s_nr_total; i++) {
        if (s_pages[i].pg_phys == phys) {
            if (s_pages[i].pg_refcount > 0) {
                s_pages[i].pg_refcount--;
            }
            if (s_pages[i].pg_refcount == 0) {
                s_pages[i].pg_next = s_free_list;
                s_free_list        = &s_pages[i];
                s_nr_free++;
            }
            return;
        }
    }
    /* Physical address not from our pool — silently ignore */
}

/* ────────────────────────────────────────────────────────────────────
 * uiox_mm_free_pages  — number of free pages available.
 * uiox_mm_total_pages — total pages managed by the allocator.
 * ──────────────────────────────────────────────────────────────────── */
uint32_t uiox_mm_free_pages(void)  { return s_nr_free;  }
uint32_t uiox_mm_total_pages(void) { return s_nr_total; }
