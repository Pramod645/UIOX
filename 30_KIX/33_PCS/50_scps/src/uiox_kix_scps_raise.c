/*
 * 33_PCS/50_scps/src/uiox_kix_scps_raise.c
 *
 * raise() — POSIX.  Trivially kill(getpid(), sig), and it is written as
 * exactly that so it inherits kill's state rather than duplicating it.
 *
 * Raising a signal at YOURSELF does not need proc_find or a PID at all:
 * send_signal(current_proc, sig) is the whole operation, and
 * send_signal() is implemented in signal.c.  So this call can be made
 * to work the moment the delivery path is uncommented — it does not
 * depend on the process table being searchable.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"
#include "signal.h"

extern void send_signal(struct proc *p, int signum);   /* signal.c */

int64_t uiox_kix_scps_raise(uiox_uint64_t sig)
{
    proc_t *p = uiox_kix_scps_current();

    if (!p) return SCPS_ESRCH;
    if (sig == 0u || sig > (uiox_uint64_t)NSIG) return SCPS_EINVAL;

    (void)send_signal;   /* direct-to-self delivery, once wired */
    return SCPS_ENOSYS;
}
