/*
 * 30_KIX/33_PCS/02_MemMngnt/src/mm.c
 *
 * Physical page allocator — free-list over a contiguous DRAM region.
 *
 * @version 2.0.0  @date 2026-07-23
 */

 #include "uiox_klibc.h"

 /*
  * Freestanding-safe pointer <-> integer helpers via memcpy.
  * sizeof(uintptr_t) == sizeof(void *) on every target — no
  * -Werror=pointer-to-int-cast or -Werror=int-to-pointer-cast on
  * arm32 or arm64.
  */
 #define PTR_TO_UINTPTR(dst, src)                        \
     do { const void *_q = (const void *)(src);          \
          memcpy(&(dst), &_q, sizeof(dst)); } while (0)
 
 #define UINTPTR_TO_PTR(dst, src)                        \
     do { uintptr_t _u = (src);                          \
          memcpy(&(dst), &_u, sizeof(dst)); } while (0)
 
 /* ── Memory descriptor ──────────────────────────────────────────────── */
 struct uiox_mm_desc {
     uintptr_t  mm_pgd_phys;
     uintptr_t  mm_mmap_base;
     uintptr_t  mm_mmap_top;
     uintptr_t  mm_brk_start;
     uintptr_t  mm_brk_current;
 };
 
 /* ── Page constants ─────────────────────────────────────────────────── */
 #define UIOX_PAGE_SHIFT   12
 #define UIOX_PAGE_SIZE    (1U << UIOX_PAGE_SHIFT)
 #define UIOX_PAGE_MASK    (~(uintptr_t)(UIOX_PAGE_SIZE - 1U))
 
 #define UIOX_MAX_PAGES    16384
 
 /* ── Page descriptor ────────────────────────────────────────────────── */
 typedef struct uiox_page {
     uintptr_t          pg_phys;
     struct uiox_page  *pg_next;
     uint32_t           pg_flags;
     uint32_t           pg_refcount;
 } uiox_page_t;
 
 /* ── Allocator state ─────────────────────────────────────────────────── */
 static uiox_page_t  s_pages[UIOX_MAX_PAGES];
 static uiox_page_t *s_free_list = (uiox_page_t *)0;
 static uint32_t     s_nr_free   = 0;
 static uint32_t     s_nr_total  = 0;
 static uint8_t      s_mm_ready  = 0;
 
 /* ─────────────────────────────────────────────────────────────────────
  * uiox_mm_init
  * ───────────────────────────────────────────────────────────────────── */
 void uiox_mm_init(uint64_t dram_base, uint64_t dram_size)
 {
     uintptr_t base, top, addr;
     uint32_t  i;
 
     base = (uintptr_t)((dram_base + UIOX_PAGE_SIZE - 1U) & UIOX_PAGE_MASK);
     top  = (uintptr_t)((dram_base + dram_size) & UIOX_PAGE_MASK);
 
     if (top <= base) { return; }
 
     s_free_list = (uiox_page_t *)0;
     s_nr_free   = 0;
     s_nr_total  = 0;
 
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
 
         if (addr == base) { break; }
     }
 
     s_mm_ready = 1;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * phys_alloc_page
  * ───────────────────────────────────────────────────────────────────── */
 void *phys_alloc_page(void)
 {
     uiox_page_t *pg;
     void        *ret;
 
     if (!s_mm_ready || !s_free_list) { return (void *)0; }
 
     pg               = s_free_list;
     s_free_list      = pg->pg_next;
     pg->pg_next      = (uiox_page_t *)0;
     pg->pg_refcount  = 1;
     s_nr_free--;
 
     /* line 118 fix: uintptr_t → void * via memcpy, no cast warning */
     UINTPTR_TO_PTR(ret, pg->pg_phys);
     return ret;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * phys_free_page
  * ───────────────────────────────────────────────────────────────────── */
 void phys_free_page(void *page)
 {
     uintptr_t phys;
     uint32_t  i;
 
     if (!page || !s_mm_ready) { return; }
 
     /* line 131 fix: void * → uintptr_t via memcpy, no cast warning */
     PTR_TO_UINTPTR(phys, page);
     phys &= UIOX_PAGE_MASK;
 
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
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * uiox_mm_free_pages / uiox_mm_total_pages
  * ───────────────────────────────────────────────────────────────────── */
 uint32_t uiox_mm_free_pages(void)  { return s_nr_free;  }
 uint32_t uiox_mm_total_pages(void) { return s_nr_total; }
 