/*
 * 33_PCS/50_scps/src/uiox_kix_scps_pause.c
 *
 * pause() — POSIX.  Suspends until a signal is delivered.
 *
 * ── this is the SIMPLEST use of the sleep machinery ───────────────────
 * pause() needs no timer, which makes it the natural first proof that
 * sleep_wakeup.c is live.  Its whole body in Bach's terms is:
 *
 *     proc_sleep(&pause_wchan, PZERO + 1, 1);   // interruptible
 *     return -EINTR;                            // only a signal wakes us
 *
 * The priority must be ABOVE PZERO — that is the flag that makes the
 * sleep interruptible, and why proc_sleep can return and report EINTR.
 * Sleeping at or below PZERO would make pause() unkillable.
 *
 * So unlike nanosleep, pause() does NOT need the timed wakeup.  It needs
 * only the interrupter, which issig/psig already provide on the
 * return-to-user path.  Recorded here because it makes pause the
 * cheapest first test.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"

int64_t uiox_kix_scps_pause(void)
{
    if (!uiox_kix_scps_current()) return SCPS_ESRCH;
    return SCPS_ENOSYS;   /* proc_sleep(.., PZERO+1, 1) once wakers exist */
}
