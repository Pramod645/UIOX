/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_clock_get_time.c
 *
 * clock_gettime() — the one call in this layer that is REAL end to end.
 *
 * ── the whole path exists ───────────────────────────────────────────
 *   the scheduler's time syscall   takes an id and a kernel time struct
 *   the timekeeping source         maintains the wall clock and a
 *                                  monotonic counter
 *   the periodic clock interrupt    advances the source
 *
 * Nothing in that chain is a stub, which makes this the layer's working
 * reference: every other ENOSYS in this directory is blocked by something
 * this call does not need.
 *
 * ── identifier validation, and why two are enough ───────────────────
 * Only the realtime and monotonic clocks are accepted.  The others POSIX
 * defines either need per-process CPU accounting that is not yet charged,
 * or need a timer facility that does not exist.  Accepting an identifier
 * and then returning wall-clock time for it would be worse than refusing:
 * a caller asking for process CPU time would receive the time of day.
 *
 * ── the stopgap, and why it is a stopgap ────────────────────────────
 * There is no copy_to_user, so the two halves come back packed into the
 * return value: nanoseconds in the high word, seconds in the low.  That
 * is not the shape a caller expects and it is labelled as temporary.
 *
 * Seconds saturate rather than wrap.  A wrapped second count would read
 * as a time in the distant past, which a caller could act on; a
 * saturated one reads as "very large", which is obviously not a real
 * reading.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

#define SCPCS_CLOCK_REALTIME   0u
#define SCPCS_CLOCK_MONOTONIC  1u

/* The kernel time shape.  Restated so this file needs no second include
 * path — it MUST match what the scheduler's time syscall declares. */
typedef struct { int64_t tv_sec; uiox_uint32_t tv_nsec; } scps_xtime_t;

extern int sys_clock_gettime(uiox_uint64_t id, scps_xtime_t *out);

int64_t uiox_kix_scpcs_clock_get_time(uiox_uint64_t clock_id,
                                      uiox_uintptr_t out)
{
    scps_xtime_t ts;
    int          rc;

    if (clock_id != (uiox_uint64_t)SCPCS_CLOCK_REALTIME &&
        clock_id != (uiox_uint64_t)SCPCS_CLOCK_MONOTONIC)
        return SCPS_EINVAL;

    if (!uiox_kix_scps_current()) return SCPCS_ESRCH;

    /* A named output buffer is validated even though it cannot be
     * written: refusing a bad address is still the right answer, and it
     * keeps this consistent with the rest of the layer. */
    if (out != 0u) {
        if (uiox_kix_scps_check_user_ptr(out, sizeof(scps_xtime_t), 8u) != 0)
            return SCPCS_EFAULT;
    }

    ts.tv_sec  = 0;
    ts.tv_nsec = 0u;

    rc = sys_clock_gettime(clock_id, &ts);
    if (rc != 0) return SCPCS_EINVAL;

    if (ts.tv_sec < 0)            ts.tv_sec = 0;
    if (ts.tv_sec > 0xFFFFFFFFLL) ts.tv_sec = 0xFFFFFFFFLL;

    return ((int64_t)ts.tv_nsec << 32) | (int64_t)ts.tv_sec;
}
