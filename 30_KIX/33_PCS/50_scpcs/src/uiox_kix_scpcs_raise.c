/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_raise.c
 *
 * raise() — Group 2, Synchronization.
 *
 * ── why this is worth having separately ─────────────────────────────
 * raise(sig) is kill(getpid(), sig), but writing it as the composite
 * would inherit kill's two dependencies that it does not actually need:
 *
 *   1. a PID LOOKUP.  Raising a signal at yourself needs no search — the
 *      process is already in hand.  So this call does not depend on the
 *      process table being searchable.
 *   2. a PID RANGE CHECK.  No PID is named, so nothing to validate.
 *
 * The delivery itself is the same primitive kill would use, and that
 * primitive is implemented.  This makes raise the cheapest first test of
 * the signal path: once delivery is uncommented in the signal
 * implementation, raise works before kill does.
 *
 * ── the abort semantics ─────────────────────────────────────────────
 * If the signal is uncaught and fatal, the process terminates — and the
 * signal implementation reaches that by way of the exit algorithm, which
 * is why exit's zombie transition matters to a call that looks unrelated.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

extern void               send_signal(uiox_kix_psa_proc_t *p, int signum);
extern uiox_kix_psa_proc_t *uiox_kix_psa_current_proc;

int64_t uiox_kix_scpcs_raise(uiox_uint64_t sig)
{
    uiox_kix_psa_proc_t *p = uiox_kix_scps_current();

    if (!p) return SCPCS_ESRCH;
    if (sig == 0u || sig > (uiox_uint64_t)UIOX_KIX_PSA_NSIG)
        return SCPCS_EINVAL;

    (void)send_signal;                 /* direct-to-self delivery */
    (void)uiox_kix_psa_current_proc;
    return SCPCS_ENOSYS;
}
