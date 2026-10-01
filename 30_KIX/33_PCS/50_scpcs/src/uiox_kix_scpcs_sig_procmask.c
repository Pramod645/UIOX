/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_sig_procmask.c
 *
 * sigprocmask() — Group 2, Synchronization.
 *
 * ── the storage is real ─────────────────────────────────────────────
 * The process table entry carries a blocked-signal mask, so unlike
 * sigaction this call has somewhere to put its result.  That makes it
 * closer to working than most of the group.
 *
 * ── the two things that block it ────────────────────────────────────
 *   1. set and oldset are USER addresses and there is no copy_to_user.
 *      A caller cannot hand in a mask, so the operation cannot be
 *      invoked in its POSIX form.
 *   2. The signal implementation tests a CAST mask, not the real field.
 *      Until that cast is removed, setting the real field would have no
 *      effect on the code that decides whether a signal is deliverable —
 *      which is worse than not implementing it, because it would LOOK
 *      like it worked while signals continued to arrive.
 *
 * That second point is why this returns ENOSYS rather than doing the
 * partial job: a write that nothing reads is a silent no-op dressed as
 * success.
 *
 * ── the operation codes are still validated ─────────────────────────
 * A caller passing nonsense hears EINVAL, not ENOSYS.  The two mean
 * different things: EINVAL blames the argument, ENOSYS reports the
 * kernel.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

#ifndef SIG_BLOCK
#define SIG_BLOCK   0
#endif
#ifndef SIG_UNBLOCK
#define SIG_UNBLOCK 1
#endif
#ifndef SIG_SETMASK
#define SIG_SETMASK 2
#endif

int64_t uiox_kix_scpcs_sig_procmask(uiox_uint64_t how, uiox_uintptr_t set,
                                    uiox_uintptr_t oldset)
{
    (void)set;
    (void)oldset;

    if (!uiox_kix_scpcs_current()) return SCPCS_ESRCH;

    if (how != (uiox_uint64_t)SIG_BLOCK   &&
        how != (uiox_uint64_t)SIG_UNBLOCK &&
        how != (uiox_uint64_t)SIG_SETMASK)
        return SCPCS_EINVAL;

    return SCPCS_ENOSYS;
}
