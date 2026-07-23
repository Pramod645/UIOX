/*
 * 33_PCS/02_MemMngnt/src/mm.c
 *
 * Physical page allocator — free list allocator over a contiguous DRAM
 * region. Provides the physallocpage() / physfreepage() symbols that
 * archruntime.c declares extern, and uioxmminit() called by
 * uioxprocinit().
 *
 * Design
 * ──────
 * Each physical 4 KB page has a one-word descriptor in a statically
 * allocated array (s_pages). Free pages are linked into a singly-linked
 * free list through the descriptor's .next field. No buddy system yet —
 * that can be layered on top of this allocator later.
 *
 * Freestanding: no system headers. Types from uioxbasetypes.h.
 *
 * @version 1.0.0  @date 2026-07-23
 */

#ifndef UIOXBASETYPESCOMPAT
#define UIOXBASETYPESCOMPAT
#endif
#include "uioxbasetypes.h"

/* ── Memory-management descriptor (forward for uioxtask.h compat) ───── */
/*    Full mm_desc: virtual address space, VMA list, page-table root.    */
/*    For now we only need the physical allocator to unblock the link.   */
struct uioxmmdesc {
    uioxuintptrt  pgdphys;   /* physical address of page-global-directory */
    uioxuintptrt  mmapbase;  /* start of user mmap area                   */
    uioxuintptrt  mmaptop;   /* end of user mmap area                     */
    uioxuintptrt  brkstart;  /* start of heap                             */
    uioxuintptrt  brkcurrent;/* current heap break                        */
};

/* ── Page constants ─────────────────────────────────────────────────── */
#define UIOXPAGESHIFT   12u
#define UIOXPAGESIZE    (1u << UIOXPAGESHIFT)   /* 4 KB                  */
#define UIOXPAGEMASK    (~(uioxuintptrt)(UIOXPAGESIZE - 1u))

/* ── Maximum pages we can track (64 MB default = 16384 pages) ─────────  */
/*    Raise UIOXMAXPAGES to support more RAM.                             */
#define UIOXMAXPAGES    16384u

/* ── Page descriptor ───────────────────────────────────────────────── */
typedef struct uioxpage {
    uioxuintptrt    phys;     /* physical address of this page           */
    struct uioxpage *next;    /* free-list link (NULL when allocated)     */
    uioxuint32t     flags;    /* reserved for future (dirty, pinned, ...) */
    uioxuint32t     refcount; /* 0 = free, >0 = in use                   */
} uioxpaget;

/* ── Page descriptor array and free list head ───────────────────────── */
static uioxpaget   s_pages[UIOXMAXPAGES];
static uioxpaget  *s_freelist  = (uioxpaget *)0;
static uioxuint32t s_nrfree    = 0u;
static uioxuint32t s_nrtotal   = 0u;
static uioxuint8t  s_mmready   = 0u;

/* ────────────────────────────────────────────────────────────────────
 * uioxmminit — initialise the physical page allocator.
 *
 * @drambase   physical address of the first byte of available DRAM
 * @dramsize   size in bytes of the available DRAM region
 *
 * Aligns the base up to a page boundary and builds the free list.
 * Called once from uioxprocinit() before any allocation can occur.
 * ──────────────────────────────────────────────────────────────────── */
void uioxmminit(uioxuint64t drambase, uioxuint64t dramsize)
{
    uioxuintptrt base, top, addr;
    uioxuint32t  i;

    /* Align base up, top down */
    base = (uioxuintptrt)((drambase + UIOXPAGESIZE - 1u) & UIOXPAGEMASK);
    top  = (uioxuintptrt)((drambase + dramsize) & UIOXPAGEMASK);

    if (top <= base) { return; }   /* degenerate region */

    s_freelist = (uioxpaget *)0;
    s_nrfree   = 0u;
    s_nrtotal  = 0u;

    /* Build free list — chain pages in reverse order so first page
       is at the head (lowest physical address allocated first).       */
    for (addr = top - UIOXPAGESIZE, i = 0u;
         addr >= base && i < UIOXMAXPAGES;
         addr -= UIOXPAGESIZE, i++) {

        uioxpaget *pg = &s_pages[i];
        pg->phys      = addr;
        pg->refcount  = 0u;
        pg->flags     = 0u;
        pg->next      = s_freelist;
        s_freelist    = pg;
        s_nrfree++;
        s_nrtotal++;

        if (addr == base) { break; }  /* prevent underflow on uintptr */
    }

    s_mmready = 1u;
}

/* ────────────────────────────────────────────────────────────────────
 * physallocpage — allocate one 4 KB physical page.
 *
 * Returns a pointer to the PHYSICAL address as a void*, or NULL.
 * The caller is responsible for mapping it into virtual address space.
 *
 * Declared extern in archruntime.c — this is the definition.
 * ──────────────────────────────────────────────────────────────────── */
void *physallocpage(void)
{
    uioxpaget *pg;

    if (!s_mmready || !s_freelist) { return (void *)0; }

    pg             = s_freelist;
    s_freelist     = pg->next;
    pg->next       = (uioxpaget *)0;
    pg->refcount   = 1u;
    s_nrfree--;

    return (void *)(uioxuintptrt)pg->phys;
}

/* ────────────────────────────────────────────────────────────────────
 * physfreepage — return a physical page to the free list.
 * ──────────────────────────────────────────────────────────────────── */
void physfreepage(void *page)
{
    uioxuintptrt phys;
    uioxuint32t  i;

    if (!page || !s_mmready) { return; }

    phys = (uioxuintptrt)page & UIOXPAGEMASK;

    /* Find the descriptor for this physical address */
    for (i = 0u; i < s_nrtotal; i++) {
        if (s_pages[i].phys == phys) {
            if (s_pages[i].refcount > 0u) {
                s_pages[i].refcount--;
            }
            if (s_pages[i].refcount == 0u) {
                s_pages[i].next = s_freelist;
                s_freelist      = &s_pages[i];
                s_nrfree++;
            }
            return;
        }
    }
    /* Physical address not from our pool — ignore */
}

/* ────────────────────────────────────────────────────────────────────
 * uioxmmfreepages — return number of free pages (debug / info).
 * ──────────────────────────────────────────────────────────────────── */
uioxuint32t uioxmmfreepages(void)
{
    return s_nrfree;
}

uioxuint32t uioxmmtotalpages(void)
{
    return s_nrtotal;
}
