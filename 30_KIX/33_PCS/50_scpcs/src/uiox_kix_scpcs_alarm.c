/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_alarm.c
 *
 * alarm() — Group 3, Miscellaneous.
 *
 * ── the pieces exist and are not joined ─────────────────────────────
 * The scheduler implements an alarm entry point and maintains a callout
 * table whose add function takes a tick delta and a function to run.  An
 * alarm is exactly that: a callout that fires a SIGALRM at one process.
 * What does not exist is a callout BOUND to the calling process.
 *
 * ── the field that is missing is the whole semantics ────────────────
 * POSIX alarm returns the seconds REMAINING on a previously set alarm,
 * and a new alarm replaces the old one.  Both require remembering the
 * outstanding callout and its deadline — per process.  The timer record
 * on the process table entry holds the four struct-tms fields and nothing
 * else, so there is nowhere to keep an alarm.
 *
 * That makes replacing impossible AND makes the return value
 * unanswerable.  Returning 0 would claim no alarm was pending, which is
 * false the moment one is — and the caller's decision to reschedule or
 * not turns on that number.
 *
 * ── seconds versus ticks ────────────────────────────────────────────
 * The callout takes a tick delta, so a conversion is needed.  The tick
 * rate is derived from the scheduler's time quantum; this wrapper does
 * not assume a rate, because the conversion belongs with the callout
 * rather than at the syscall boundary where it would be duplicated by
 * every caller.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

int64_t uiox_kix_scpcs_alarm(uiox_uint64_t seconds)
{
    if (!uiox_kix_scps_current()) return SCPS_ESRCH;

    /* Cancelling (zero seconds) needs the outstanding callout's identity
     * just as much as setting does. */
    if (seconds == 0u) return SCPS_ENOSYS;

    return SCPS_ENOSYS;   /* no per-process alarm state */
}
