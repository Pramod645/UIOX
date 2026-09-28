/*
 * 33_PCS/50_scps/src/uiox_kix_scps_sig_pending.c
 *
 * sigpending() — Group 2, Synchronization.
 *
 * ── the field exists ──────────────────────────────────────────────────
 * proc_t has p_sig, the pending-signal bitmask.  The VALUE is available:
 * pending = p_sig & p_sigmask, which is the set of signals both pending
 * and blocked — what POSIX sigpending reports.
 *
 * ── what blocks it ────────────────────────────────────────────────────
 * set is a USER address with no copy_to_user.  The value comes back in
 * the return instead, the same stopgap clock_get_time uses, so a caller
 * can observe its blocked-pending set today.
 *
 * Note the caveat shared with sigprocmask: signal.c's issig tests a
 * CAST bitmask through sim_proc_t, not p_sig.  This reads p_sig, the
 * field this layer binds to; once both are live the two must agree.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"

int64_t uiox_kix_scps_sig_pending(uiox_uintptr_t set)
{
    proc_t *p = uiox_kix_scps_current();

    if (!p) return SCPS_ESRCH;
    if (set != 0u) return SCPS_EFAULT;   /* no copy_to_user */

    return (int64_t)(p->p_sig & p->p_sigmask);
}
