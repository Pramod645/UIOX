/*
 * 33_PCS/50_scps/src/uiox_kix_scps_alarm.c
 *
 * alarm() — POSIX.  Schedules SIGALRM.
 *
 * ── the pieces exist and are not joined ───────────────────────────────
 * 01_schedular has sys_alarm() implemented, and a callout table —
 * callout_add(delta_ticks, fn, arg) in clock.c — which is the mechanism
 * an alarm is built from.  What does not exist is a callout bound to the
 * CALLING process that fires send_signal(p, SIGALRM).
 *
 * ── the one field that is missing ─────────────────────────────────────
 * POSIX: alarm() returns the seconds REMAINING on a previously set
 * alarm, and a new alarm replaces the old.  That needs per-process
 * state — the outstanding callout id and its deadline.  proc_t has
 * p_timers (the four struct-tms fields) but no alarm field, so the
 * replacement semantics cannot be expressed.  Returning 0 would claim
 * "no previous alarm" even when one is pending.
 *
 * ── seconds vs ticks ──────────────────────────────────────────────────
 * callout_add takes delta_ticks.  The conversion needs the tick rate,
 * which clock.c derives from TIME_QUANTUM (HZ was removed as undefined).
 * This wrapper does not guess the rate; the conversion belongs with the
 * callout, not at the syscall boundary.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"

int64_t uiox_kix_scps_alarm(uiox_uint64_t seconds)
{
    if (!uiox_kix_scps_current()) return SCPS_ESRCH;
    if (seconds == 0u) return SCPS_ENOSYS;   /* cancel: needs the alarm id */
    return SCPS_ENOSYS;                      /* no per-proc alarm state */
}
