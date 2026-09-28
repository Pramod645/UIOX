/*
 * 33_PCS/50_scps/src/uiox_kix_scps_nano_sleep.c
 *
 * nanosleep() — req and rem point at user structs
 * { int64 tv_sec; uint32 tv_nsec; }.
 *
 * ── the algorithm exists ──────────────────────────────────────────────
 * SCPS Algorithm 10 (sleep), implemented in 40_procStruct/src/
 * sleep_wakeup.c as proc_sleep(wchan, priority, interruptible).  Three
 * return codes, verified from the source:
 *
 *     0  woken normally
 *     1  woken by a CAUGHT signal      (priority > PZERO)
 *    -1  woken by an UNCAUGHT signal, u.u_error = EINTR and the u_qsave
 *        abort flag set so the syscall entry unwinds
 *
 * The same file has proc_wakeup (Algorithm 11).  Both are complete.
 *
 * ── what is missing: the TIMED half ───────────────────────────────────
 * Something must call proc_wakeup(wchan) when the deadline passes.
 * 01_schedular has timer_add()/timer_run() and a callout table.  Nothing
 * connects them to sleep_wakeup.c's wait channel, so a process that
 * sleeps with no waker never runs again.
 *
 * A busy-wait through ndelay() would honour the delay while blocking the
 * CPU — worse than not sleeping, on a system whose point is
 * multiprogramming.  Not done.
 *
 * This is the highest-value integration in the layer: proc_sleep,
 * proc_wakeup and the callout table are all written, and one waker joins
 * them.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"

int64_t uiox_kix_scps_nano_sleep(uiox_uintptr_t req, uiox_uintptr_t rem)
{
    if (!uiox_kix_scps_current()) return SCPS_ESRCH;
    if (uiox_kix_scps_check_user_ptr(req, 16u, 8u) != 0) return SCPS_EFAULT;
    if (rem != 0u &&
        uiox_kix_scps_check_user_ptr(rem, 16u, 8u) != 0) return SCPS_EFAULT;

    return SCPS_ENOSYS;   /* proc_sleep exists; the timed wakeup does not */
}
