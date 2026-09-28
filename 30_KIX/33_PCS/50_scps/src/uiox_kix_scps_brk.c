/*
 * 33_PCS/50_scps/src/uiox_kix_scps_brk.c
 *
 * brk() — Group 1, Memory Management.
 *
 * ── SCPS Algorithm 9, verbatim ────────────────────────────────────────
 *   input: new break value
 *   output: old break value
 *   {
 *       lock process data region;
 *       if (region size increasing)
 *       {
 *           if (new region size is illegal)
 *           { unlock data region; return (error); }
 *       }
 *       change region size (algorithm growreg);
 *       zero out addresses in new data space;
 *       unlock process data region;
 *   }
 *
 * The SCPS grouping lists brk as: growreg  (region.c, PSA Algorithm 5)
 *
 * ── what already works ────────────────────────────────────────────────
 * kernel_brk() implements the validation and the arithmetic: it aligns
 * to BRK_ALIGN (4096), rejects below MIN_BRK_ADDR and above
 * MAX_BRK_ADDR, and reports expand vs shrink.
 *
 * ── what it does not do ───────────────────────────────────────────────
 * It does not lock the region, does not call growreg(), and does not
 * zero the new space.  get_current_brk() returns a fixed MIN_BRK_ADDR
 * because there is no pregion to walk.
 *
 * Returning kernel_brk's value is still correct: its contract is "the
 * PREVIOUS break", which is what POSIX brk requires, and failure is
 * signalled by (uintptr_t)-1.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"

extern uintptr_t kernel_brk(uintptr_t new_brk);   /* 40_procStruct/src/brk.c */

int64_t uiox_kix_scps_brk(uiox_uint64_t new_brk)
{
    uintptr_t old;

    if (!uiox_kix_scps_current()) return SCPS_ESRCH;
    if (new_brk > 0xFFFFFFFFu) return SCPS_EINVAL;   /* 32-bit VA model */

    old = kernel_brk((uintptr_t)new_brk);
    if (old == (uintptr_t)-1) return SCPS_ENOMEM;

    return (int64_t)old;
}
