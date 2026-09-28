/*
 * 33_PCS/50_scps/src/uiox_kix_scps_nice.c
 *
 * nice() — POSIX.  REAL: p_sched.p_nice exists on proc_t, and the
 * scheduler's priority computation already reads it.
 *
 * Returns the NEW nice value, which is what POSIX specifies (nice(2)
 * returns the resulting nice value, not the old one — unlike brk).
 *
 * The kernel's nice range is 0..NICE_MAX where a HIGHER number means
 * LOWER priority and NICE_MAX is the default.  POSIX increments a
 * user-visible nice in [-20, 19].  The mapping is documented rather
 * than silently applied:
 *
 *     p_nice_new = clamp(p_nice_old - inc, 1, NICE_MAX)
 *
 * The subtraction is the POSIX inversion: nice(+5) LOWERS priority, and
 * in this kernel lowering priority means a SMALLER p_nice.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"

#ifndef NICE_MAX
#define NICE_MAX 20          /* kernels default; must match process.h */
#endif

int64_t uiox_kix_scps_nice(uiox_uint64_t inc)
{
    proc_t *p = uiox_kix_scps_current();
    int32_t n, d;

    if (!p) return SCPS_ESRCH;
    if (inc > 40u) return SCPS_EINVAL;     /* POSIX range is -20..19 */

    d = (int32_t)inc;
    n = (int32_t)p->p_sched.p_nice - d;    /* POSIX inversion */

    if (n < 1)        n = 1;
    if (n > NICE_MAX) n = NICE_MAX;

    p->p_sched.p_nice = (uiox_uint32_t)n;
    return (int64_t)n;
}
