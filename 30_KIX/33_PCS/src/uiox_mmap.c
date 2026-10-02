/*
 * 30_KIX/33_PCS/src/uiox_mmap.c
 *
 * The address half of mmap — insert a physical page into a user page
 * table, and take it back out again.
 *
 * ── WHAT IS NOT IMPLEMENTED, STATED PLAINLY ───────────────────────────
 * uiox_mm_map_user_phys() returns 0 today.  It needs three things
 * 02_MemMngnt does not yet export:
 *
 *     uiox_vma_alloc(proc, va_hint, size, prot)
 *     uiox_pte_set(proc->mm.pgd, va, pte)
 *     uiox_tlb_flush_user(proc)
 *
 * Returning 0 is the honest answer rather than a mapping a first read
 * would fault into nothing.
 *
 * @version 1.0.0  @date 2026-10-03
 */
#include "uiox_kix_pagemap.h"
#include "uiox_uaccess.h"
#include "uiox_klibc.h"

#define EINVAL  22

struct uiox_proc;

extern struct uiox_proc *uiox_current_proc(void);
extern uintptr_t uiox_vma_alloc(struct uiox_proc *proc, uintptr_t va_hint,
                                size_t size, uint32_t prot);
extern int  uiox_pte_set(struct uiox_proc *proc, uintptr_t va, uintptr_t pa,
                         uint32_t prot);
extern int  uiox_pte_clear(struct uiox_proc *proc, uintptr_t va);
extern void uiox_tlb_flush_user(struct uiox_proc *proc);

uintptr_t uiox_mm_map_user_phys(struct uiox_proc *proc,
                                uintptr_t         va_hint,
                                uintptr_t         pa,
                                size_t            size,
                                uint32_t          prot)
{
    uintptr_t pa_aligned;
    size_t    size_aligned;
    uintptr_t va;
    size_t    off;

    if (!proc)      return 0u;
    if (size == 0u) return 0u;
    if (pa == 0u)   return 0u;

    pa_aligned   = pa & UIOX_PAGE_MASK;
    size_aligned = (size + (pa - pa_aligned) + (UIOX_PAGE_SIZE - 1u))
                   & UIOX_PAGE_MASK;

    (void)va; (void)off;

#if 0
    va = uiox_vma_alloc(proc, va_hint, size_aligned, prot);
    if (va == 0u) return 0u;

    for (off = 0u; off < size_aligned; off += UIOX_PAGE_SIZE) {
        if (uiox_pte_set(proc, va + off, pa_aligned + off, prot) != 0)
            return 0u;
    }

    uiox_tlb_flush_user(proc);
    return va;
#else
    return 0u;
#endif
}

int uiox_mm_unmap_user(struct uiox_proc *proc, uintptr_t va, size_t size)
{
    uintptr_t end;
    uintptr_t v;

    if (!proc)      return -EINVAL;
    if (size == 0u) return -EINVAL;
    if (!uiox_uaccess_ok((const void *)va, size)) return -EINVAL;

    end = va + ((size + UIOX_PAGE_SIZE - 1u) & UIOX_PAGE_MASK);

    for (v = va; v < end; v += UIOX_PAGE_SIZE)
        (void)uiox_pte_clear(proc, v);

    uiox_tlb_flush_user(proc);
    return 0;
}
