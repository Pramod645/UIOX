/*
 * 33_PCS/50_scps/src/uiox_kix_scps_sig_action.c
 *
 * sigaction() — Group 2, Synchronization.
 *
 * ── where the disposition actually lives right now ────────────────────
 * signal.c stores it on its PRIVATE sim_proc_t:
 *
 *     sigaction_t  sp_sigact[NSIG + 1];
 *
 * reached by casting a struct proc * to sim_proc_t *.  proc_t has no
 * disposition array, so there is nowhere on the model everything else
 * binds to for a handler to be installed.
 *
 * kernel_signal() has its assignment commented out:
 *
 *     /* current_proc sigact[signum].sa_handler = handler; *\/
 *
 * ── what is already correct ───────────────────────────────────────────
 * The SIGKILL / SIGSTOP refusal.  Those two cannot be caught or ignored
 * whatever the storage looks like, and kernel_signal() enforces that.
 * The rule is not duplicated in the wrapper.
 *
 * TO FIX: add sigaction_t sa_sigact[NSIG + 1] to proc_t (or to the
 * u_area it points at), delete the sim_proc_t cast in signal.c, then
 * *uncomment the assignment*.  With copy_in/copy_out for act and oldact
 * this becomes complete.  Until then ENOSYS is honest: a zero return
 * would claim a handler was installed.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"
#include "signal.h"

extern int kernel_signal(int signum, sig_handler_t handler);

int64_t uiox_kix_scps_sig_action(uiox_uint64_t sig, uiox_uintptr_t act,
                                 uiox_uintptr_t oldact)
{
    (void)act; (void)oldact;      /* no user copy in or out */

    if (!uiox_kix_scps_current()) return SCPS_ESRCH;
    if (sig == 0u || sig > (uiox_uint64_t)NSIG) return SCPS_EINVAL;
    if (sig == SIGKILL || sig == SIGSTOP) return SCPS_EINVAL;

    (void)kernel_signal;
    return SCPS_ENOSYS;           /* nowhere on proc_t to store it */
}
