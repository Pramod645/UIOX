/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_times.c
 *
 * times() — Group 3, Miscellaneous.
 *
 * ── the four values exist ───────────────────────────────────────────
 * The process table entry carries a timer record whose four fields match
 * struct tms one for one: user time, system time, and the summed user and
 * system time of REAPED children.  The last two are why wait adds a
 * child's usage to its parent — a parent's children's time is not lost
 * when the child is freed, it moves.
 *
 * ── the two blockers ────────────────────────────────────────────────
 *   1. There is no copy_to_user, so the record cannot be written to
 *      buf_out in its proper shape.
 *   2. Nothing yet CHARGES the fields.  No tick accounting writes them,
 *      so today they hold whatever the allocator's zeroing left — which
 *      for a clock is not a value, it is an absence.
 *
 * The second is the important one.  Packing four zeros into the return
 * would be a well-formed answer to a question nobody has answered: a
 * caller would read 0.0s of CPU and believe it had been idle rather than
 * unbilled.  That is the same false report as a verification passing on
 * its own fabricated input.
 *
 * ── what a return would look like, when it can ──────────────────────
 * Sixteen bits per field, cstime in the top quarter down to utime in the
 * bottom, is representable on this model.  It is written down here so
 * the packing is decided in one place when the accounting lands.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

int64_t uiox_kix_scpcs_times(uiox_uintptr_t buf_out)
{
    if (!uiox_kix_scpcs_current()) return SCPCS_ESRCH;

    /* A NULL buffer is legal per POSIX and asks only whether times() is
     * callable — but answering it with success would claim the timer
     * record holds measured values. */
    if (buf_out != 0u) return SCPCS_EFAULT;   /* no copy_to_user */

    return SCPCS_ENOSYS;   /* nothing charges the timer fields yet */
}
