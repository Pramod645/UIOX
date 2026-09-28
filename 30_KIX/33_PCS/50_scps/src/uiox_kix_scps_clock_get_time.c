/*
 * 33_PCS/50_scps/src/uiox_kix_scps_clock_get_time.c
 *
 * clock_gettime() — REAL.  The whole path exists:
 *
 *     01_schedular/src/syscall_time.c -> sys_clock_gettime(id, XTime *)
 *     01_schedular/src/timekeeping.c  -> xtime, monotonic_ns()
 *     01_schedular/src/clock.c        -> clock_interrupt() advances xtime
 *
 * ── the user-pointer problem, and the stopgap ─────────────────────────
 * out is a user address and there is no copy_to_user.  This fills a
 * KERNEL XTime and cannot write it out, so the two halves return in the
 * value, which needs no user write:
 *
 *     bits 63..32 : tv_nsec
 *     bits 31..0  : tv_sec, saturating at 0xFFFFFFFF
 *
 * Documented as a stopgap, replaced the moment a copy_to_user exists.
 *
 * NOTE the dispatcher's #15 is sys_clock_get_time taking ONE clock,
 * while this takes the POSIX clock_id.  clock_getres, clock_settime and
 * clock_nanosleep all exist in 01_schedular and have no syscall number.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"

#define SCPS_CLOCK_REALTIME   0u
#define SCPS_CLOCK_MONOTONIC  1u

typedef struct { int64_t tv_sec; uiox_uint32_t tv_nsec; } scps_xtime_t;

extern int sys_clock_gettime(uiox_uint64_t id, scps_xtime_t *out);

int64_t uiox_kix_scps_clock_get_time(uiox_uint64_t clock_id,
                                     uiox_uintptr_t out)
{
    scps_xtime_t ts;
    int          rc;

    if (clock_id != SCPS_CLOCK_REALTIME && clock_id != SCPS_CLOCK_MONOTONIC)
        return SCPS_EINVAL;
    if (!uiox_kix_scps_current()) return SCPS_ESRCH;

    if (out != 0u) {
        if (uiox_kix_scps_check_user_ptr(out, sizeof(scps_xtime_t), 8u) != 0)
            return SCPS_EFAULT;
        /* no copy_to_user yet — fall through to the packed return */
    }

    ts.tv_sec  = 0;
    ts.tv_nsec = 0u;

    rc = sys_clock_gettime(clock_id, &ts);
    if (rc != 0) return SCPS_EINVAL;

    if (ts.tv_sec < 0) ts.tv_sec = 0;
    if (ts.tv_sec > 0xFFFFFFFFLL) ts.tv_sec = 0xFFFFFFFFLL;

    return ((int64_t)ts.tv_nsec << 32) | (int64_t)ts.tv_sec;
}
