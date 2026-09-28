/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_sig_action.c
 *
 * sigaction() — Group 2, Synchronization.
 *
 * ── where the disposition lives right now ───────────────────────────
 * The signal implementation keeps handler dispositions on a PRIVATE
 * structure of its own, reached by casting a process pointer to it.  The
 * process table entry carries no handler array, so once the cast is
 * removed there is nowhere for a handler to be stored.
 *
 * ── the one thing already correct ───────────────────────────────────
 * SIGKILL and SIGSTOP cannot be caught or ignored, and the callee
 * enforces that.  The check is not duplicated in the wrapper: two copies
 * of a rule are two things to keep in step.
 *
 * ── why ENOSYS rather than 0 ────────────────────────────────────────
 * Returning success would tell the caller a handler was installed when
 * nothing was stored.  That is the same class of false report as a
 * verification that checks its own fabricated input.
 *
 * TO FINISH: add a disposition array to the process table entry in
 * 40psa, delete the cast in the signal implementation so issig/psig read
 * that entry directly, and uncomment the handler assignment.  With
 * copy_in/copy_out for act and oldact this becomes complete.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

int64_t uiox_kix_scpcs_sig_action(uiox_uint64_t sig, uiox_uintptr_t act,
                                  uiox_uintptr_t oldact)
{
    (void)act;      /* no copy in  — see banner */
    (void)oldact;   /* no copy out */

    if (!uiox_kix_scpcs_current()) return SCPCS_ESRCH;

    if (sig == 0u || sig > (uiox_uint64_t)UIOX_KIX_PSA_NSIG)
        return SCPCS_EINVAL;

    /* the two dispositions nothing may change */
    if (sig == (uiox_uint64_t)UIOX_KIX_PSA_SIGKILL ||
        sig == (uiox_uint64_t)UIOX_KIX_PSA_SIGSTOP)
        return SCPCS_EINVAL;

    return SCPCS_ENOSYS;
}
