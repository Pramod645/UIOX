/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_nice.c
 *
 * nice() — Group 3, Miscellaneous.
 *
 * ── returns the NEW value ───────────────────────────────────────────
 * POSIX specifies nice returns the resulting nice value, not the previous
 * one.  That is the opposite of brk, which returns the OLD break.  Both
 * are deliberate in the standard, and getting them backwards produces a
 * caller that misreads its own priority.
 *
 * ── the inversion ───────────────────────────────────────────────────
 * The user convention is that a positive increment LOWERS priority: a
 * cooperative process makes itself less favoured.  This kernel's
 * scheduling field is the other way up — a LOWER number is a HIGHER
 * scheduling priority, which is the comparison the ready-queue
 * ordering relies on.  So the user's increment is SUBTRACTED.
 *
 * The conversion is written out rather than hidden in a macro, because
 * a sign error here would quietly invert every caller's intent.
 *
 * ── the clamp ───────────────────────────────────────────────────────
 * The result is bounded at both ends.  Bounding only the high end would
 * let an aggressive caller drive its value below the range the scheduler
 * expects, which would make it outrank every process including the
 * swapper.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

/* Upper bound on the internal (lower-is-better) priority scale.  If
 * 40psa ever publishes a named constant for this, use it instead. */
#ifndef UIOX_KIX_PSA_PRIO_MAX
#define UIOX_KIX_PSA_PRIO_MAX 139
#endif

int64_t uiox_kix_scpcs_nice(uiox_uint64_t inc)
{
    uiox_kix_psa_proc_t *p = uiox_kix_scpcs_current();
    int32_t n;
    int32_t d;

    if (!p) return SCPCS_ESRCH;

    /* POSIX's user-visible range is -20..19; 40 is the run of it and
     * refuses anything wider rather than clamping silently. */
    if (inc > 40u) return SCPCS_EINVAL;

    d = (int32_t)inc;
    n = (int32_t)p->p_sched.p_nice - d;   /* the inversion, made visible */

    if (n < 1)                     n = 1;
    if (n > UIOX_KIX_PSA_PRIO_MAX) n = UIOX_KIX_PSA_PRIO_MAX;

    p->p_sched.p_nice = n;
    return (int64_t)n;                    /* the NEW value */
}
