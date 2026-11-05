/* ── issig ───────────────────────────────────────────────────────────
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_issig.c
 * The lowest-numbered signal that is pending AND not blocked, or 0 when
 * there is none.
 *
 * The expression is the one uiox_kix_psa_proc_sig_pending() uses — that
 * is deliberate: two different definitions of "deliverable" is how a
 * signal ends up marked but never sent. */

 #include "../include/uiox_kix_scpcs.h"

int uiox_kix_scpcs_issig(uiox_kix_psa_proc_t *p)
{
    uiox_uint32_t set;
    int           s;

    if (!p) return 0;

    set = p->p_sig & ~p->p_sigmask;
    for (s = 1; s < (int)UIOX_KIX_PSA_NSIG; s++) {
        if (set & (uiox_uint32_t)(1u << (uiox_uint32_t)s))
            return s;
    }
    return 0;
}
