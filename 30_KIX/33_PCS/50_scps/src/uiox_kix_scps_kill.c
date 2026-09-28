/*
 * 33_PCS/50_scps/src/uiox_kix_scps_kill.c
 *
 * kill() — Group 2, Synchronization.
 *
 * In SCPS this sits with wait and signal because the kernel synchronizes
 * exit and wait THROUGH signals.  That is why the missing delivery step
 * here is not a local bug: it is the reason wait() cannot work.
 *
 * ── the delivery step is commented out ────────────────────────────────
 * kernel_kill() in signal.c:
 *
 *     printf("[kill] sending %s (%d) to pid=%u\n", ...);
 *     /* send_signal(proc_find(pid), signum); *\/     <- commented out
 *
 * Everything around it exists: send_signal() sets the pending bit,
 * proc_find() finds the entry by PID, and issig()/psig() run on the way
 * back to user mode.
 *
 * ── the PID range, and why 0 and negative are refused ─────────────────
 * POSIX kill(0, sig) signals the caller's whole process group and
 * kill(-1, sig) signals every permitted process.  Neither can be
 * expressed: there is no p_pgrp on proc_t (see getpgrp), so there is no
 * group to enumerate.  Refusing them with EINVAL is honest; silently
 * signalling only self would not be.
 *
 * ── signal numbering ──────────────────────────────────────────────────
 * This layer takes the numbers from 50_scps/include/signal.h, which is
 * what signal.c's positional descriptor table is built from.  NOTE the
 * cross-layer disagreement: process.h's list puts SIGCHLD at 17 while
 * signal.c's table puts it at 18.  Since Group 2 runs THROUGH signal.c,
 * signal.h wins here, and the disagreement is a live bug the moment
 * both headers meet in one translation unit.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"
#include "signal.h"        /* SIG* / NSIG / sig_desc_t — signal.c's table */

extern int      kernel_kill(uiox_uint32_t pid, int signum);
extern proc_t  *proc_find(uiox_uint32_t pid);

int64_t uiox_kix_scps_kill(uiox_uint64_t pid, uiox_uint64_t sig)
{
    if (!uiox_kix_scps_current()) return SCPS_ESRCH;

    if (sig == 0u)  return SCPS_EINVAL;          /* no "existence check" form */
    if (sig > (uiox_uint64_t)NSIG) return SCPS_EINVAL;
    if (pid == 0u || pid > 0x7FFFFFFFu) return SCPS_EINVAL;

    if (!proc_find((uiox_uint32_t)pid)) return SCPS_ESRCH;

    (void)kernel_kill;   /* present; does not deliver — see banner */
    return SCPS_ENOSYS;
}
