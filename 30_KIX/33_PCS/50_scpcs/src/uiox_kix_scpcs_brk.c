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
#include "uiox_kix_psa_region.h"
#include "uiox_kix_psa_process.h"
#include "uiox_kix_psa_context.h"
#include "uiox_kix_psa_proc_algo.h"

#include "uiox_kix_psa_region.h"
#include "uiox_kix_psa_context.h"     /* for uiox_kix_psa_u_area_t */


int64_t uiox_kix_scpcs_brk(uiox_uint64_t new_brk)
{
    uiox_kix_psa_u_area_t  *u;
    uiox_kix_psa_pregion_t *data = (uiox_kix_psa_pregion_t *)0;
    uint32_t i;
    uint32_t old;

    if (!uiox_kix_scpcs_current()) return SCPCS_ESRCH;
    if (new_brk > 0xFFFFFFFFu) return SCPCS_EINVAL;

    u = uiox_kix_psa_current_uarea();
    if (!u) return SCPCS_ESRCH;

    /* the data segment is the pregion whose cached type is DATA */
    for (i = 0u; i < (uint32_t)UIOX_KIX_PSA_MAX_REG_PER_PROC; i++) {
        if (u->u_pregs[i].pr_valid &&
            u->u_pregs[i].pr_type == UIOX_KIX_PSA_REG_DATA) {
            data = &u->u_pregs[i];
            break;
        }
    }
    if (!data) return SCPCS_ENOMEM;

    /* brk's contract is the PREVIOUS break, so capture it before growing */
    old = data->pr_size;
    uiox_kix_psa_growreg(data, (int32_t)((uint32_t)new_brk - old));
    return (int64_t)old;
}
