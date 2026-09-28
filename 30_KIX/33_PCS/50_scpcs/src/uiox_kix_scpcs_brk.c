/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_brk.c
 *
 * brk() — Group 1, memory Management.
 *
 * ── the algorithm, verbatim ─────────────────────────────────────────
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
 * The grouping lists brk as: growreg — uiox_kix_psa_growreg(), PSA
 * Algorithm 5, implemented, with both bounds checked.
 *
 * ── what the callee already does ────────────────────────────────────
 * Aligns the request to a page, rejects a value below the minimum or
 * above the maximum, and reports expand vs shrink.  That part is real
 * and its verdict is trustworthy.
 *
 * ── what it does not do ─────────────────────────────────────────────
 * It does not lock the region, does not call growreg, and does not zero
 * the new space.  Its notion of the current break is a fixed constant
 * because there is no pregion walk, so the value it compares against is
 * not this process's actual break.
 *
 * ── why the return is still correct ─────────────────────────────────
 * brk's contract is "the PREVIOUS break", not "the new one" — which is
 * how it differs from nice(), where POSIX specifies the NEW value.  A
 * failure is signalled by (uintptr_t)-1, and that is translated to
 * ENOMEM here rather than passed through as a negative size.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

extern uintptr_t kernel_brk(uintptr_t new_brk);

int64_t uiox_kix_scpcs_brk(uiox_uint64_t new_brk)
{
    uintptr_t old;

    if (!uiox_kix_scps_current()) return SCPS_ESRCH;

    /* the model is 32-bit; a wider request cannot be honoured and must
     * not be silently truncated into an address that is merely wrong */
    if (new_brk > 0xFFFFFFFFu) return SCPS_EINVAL;

    old = kernel_brk((uintptr_t)new_brk);
    if (old == (uintptr_t)-1) return SCPS_ENOMEM;

    return (int64_t)old;
}
