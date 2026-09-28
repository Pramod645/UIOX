/*
 * 33_PCS/50_scps/src/uiox_kix_scps_times.c
 *
 * times() — POSIX.  proc_t has proc_timer_t p_timers with four fields,
 * which is exactly struct tms (utime, stime, cutime, cstime).  The
 * kernel's field names are not verified here, so this file does NOT
 * guess them.
 *
 * ── why it returns the four values instead of filling buf_out ─────────
 * Two independent blockers:
 *   1. p_timers' field names are unverified — writing them would be a
 *      guess that compiles and reads the wrong word.
 *   2. buf_out is a USER address and there is no copy_to_user.
 *
 * With no user write available the values come back in the return, the
 * same stopgap clock_get_time uses:
 *
 *   bits 63..48 : cstime
 *   bits 47..32 : cutime
 *   bits 31..16 : stime
 *   bits 15..0  : utime
 *
 * TO FINISH: read p_timers' real fields from process.h and pack them.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"

int64_t uiox_kix_scps_times(uiox_uintptr_t buf_out)
{
    if (!uiox_kix_scps_current()) return SCPS_ESRCH;
    if (buf_out != 0u) return SCPS_EFAULT;   /* no copy_to_user */
    return SCPS_ENOSYS;                      /* p_timers fields unverified */
}
