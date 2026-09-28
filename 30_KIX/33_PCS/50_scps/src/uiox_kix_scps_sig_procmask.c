/*
 * 33_PCS/50_scps/src/uiox_kix_scps_sig_procmask.c
 *
 * sigprocmask() — Group 2, Synchronization.
 *
 * ── the field exists ──────────────────────────────────────────────────
 * proc_t has p_sigmask, a blocked-signal bitmask.  So this call is
 * closer to working than most: the storage is there.
 *
 * ── what blocks it ────────────────────────────────────────────────────
 *   1. set and oldset are USER addresses and there is no copy_to_user.
 *      A caller cannot hand in a mask, so the operation cannot be
 *      invoked in its POSIX form.
 *   2. issig() in signal.c reads a bitmask that is NOT p_sigmask — it
 *      reads sim_proc_t's sp_sigmask through a cast.  Until the cast is
 *      removed, setting p_sigmask would have no effect on the code that
 *      tests for pending signals, which is worse than not implementing
 *      it: it would look like it worked.
 *
 * TO FIX: remove the sim_proc_t cast in signal.c so issig/psig read
 * proc_t's p_sig and p_sigmask directly, then add copy_in/copy_out for
 * the mask and this becomes a real implementation.
 *
 * ── the three mask-operation values are still validated ──────────────
 * SIG_BLOCK, SIG_UNBLOCK and SIG_SETMASK are checked, so a caller that
 * passes nonsense hears EINVAL and not ENOSYS.  The two mean different
 * things.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"
#include "signal.h"

#ifndef SIG_BLOCK
#define SIG_BLOCK   0
#endif
#ifndef SIG_UNBLOCK
#define SIG_UNBLOCK 1
#endif
#ifndef SIG_SETMASK
#define SIG_SETMASK 2
#endif

int64_t uiox_kix_scps_sig_procmask(uiox_uint64_t how, uiox_uintptr_t set,
                                   uiox_uintptr_t oldset)
{
    (void)set; (void)oldset;

    if (!uiox_kix_scps_current()) return SCPS_ESRCH;
    if (how != SIG_BLOCK && how != SIG_UNBLOCK && how != SIG_SETMASK)
        return SCPS_EINVAL;

    return SCPS_ENOSYS;   /* no user copy; signal.c reads a cast, not p_sigmask */
}
