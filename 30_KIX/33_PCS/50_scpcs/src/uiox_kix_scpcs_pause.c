/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_pause.c
 *
 * pause() — suspend until a signal is delivered.
 *
 * ── why this is the cheapest first test in the layer ────────────────
 * pause needs NO timer.  Its whole body is an interruptible sleep on a
 * private wait channel, and the only thing that will ever wake it is a
 * signal — which the return-to-user path already checks for.
 *
 * That makes it the one call whose dependencies are all implemented:
 *
 *   the sleep algorithm       implemented, all three return codes
 *   the interruptible branch  implemented
 *   the signal check          implemented, before and after the switch
 *   the wakeup path           NOT needed
 *   the callout table         NOT needed
 *
 * So if anything in the process layer can be brought up first, it is
 * this.  Fixing it proves the sleep machinery, the signal check, and the
 * ready queue all at once — and it does so without touching the timer
 * work that blocks nanosleep.
 *
 * ── the priority must be ABOVE the interruptibility threshold ───────
 * That is a correctness requirement, not a tuning choice.  Sleeping at or
 * below the threshold takes the UNINTERRUPTIBLE branch, and a signal can
 * never end that sleep — so pause would become unkillable.  The value to
 * use is one step above the threshold, deliberately, not a larger number
 * that happens to work.
 *
 * ── the return ──────────────────────────────────────────────────────
 * Returning EINTR is the correct outcome rather than an error: a signal
 * arriving is exactly what the call waits for, and POSIX specifies this
 * return for it.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scps.h"

int64_t uiox_kix_scpcs_pause(void)
{
    if (!uiox_kix_scps_current()) return SCPS_ESRCH;

    /* Intended body, once delivery exists:
     *
     *   uiox_kix_psa_proc_sleep(&pause_wchan,
     *                           UIOX_KIX_PSA_PZERO + 1,   // above threshold
     *                           1);                       // interruptible
     *   return SCPS_EINTR;
     */
    return SCPS_ENOSYS;
}
