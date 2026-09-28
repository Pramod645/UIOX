/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_sig_pending.c
 *
 * sigpending() — Group 2, Synchronization.
 *
 * ── this one can answer today ───────────────────────────────────────
 * The value POSIX asks for is the set of signals BOTH pending and
 * blocked, and both masks are real fields on the process table entry.
 * So the answer is computed here and returned directly.
 *
 * The intersection is the whole meaning: a signal that is pending but
 * NOT blocked is about to be delivered, so it is not "waiting" for
 * anything.  Returning the pending mask alone would overstate the set.
 *
 * ── the user pointer ────────────────────────────────────────────────
 * There is no copy_to_user, so the mask comes back in the return value
 * instead of being written to set.  A caller can therefore observe its
 * blocked-pending set today, which is more than the sibling calls in this
 * group can offer.
 *
 * A non-NULL set is refused rather than ignored: accepting it would tell
 * the caller a write happened.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

int64_t uiox_kix_scpcs_sig_pending(uiox_uintptr_t set)
{
    uiox_kix_psa_proc_t *p = uiox_kix_scpcs_current();

    if (!p) return SCPCS_ESRCH;
    if (set != 0u) return SCPCS_EFAULT;   /* no copy_to_user */

    return (int64_t)(p->p_sig & p->p_sigmask);
}
