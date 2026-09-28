/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_nano_sleep.c
 *
 * nanosleep() — Group 2 territory, but reached through the sleep
 * algorithm rather than a signal.
 *
 * ── the sleeping half is complete ───────────────────────────────────
 * The sleep algorithm is implemented, including all three return codes:
 * woken normally, woken by a caught signal, and woken by an uncaught one
 * with the abort flag raised so the syscall entry unwinds.
 *
 * ── the half that is missing is the WAKEUP ──────────────────────────
 * Something must call the wakeup routine when the deadline passes.  The
 * scheduler has a callout table and a periodic clock interrupt — the
 * mechanism exists.  What does not exist is a link between a callout and
 * the wait channel this sleep enqueued on.  So a process that sleeps with
 * no other waker never runs again.
 *
 * ── why a busy-wait was not the answer ──────────────────────────────
 * The platform delay routine would honour the requested duration while
 * occupying the processor for its whole length.  On a system whose entire
 * purpose is running several processes at once, that is strictly worse
 * than not sleeping: it starves every other runnable process for the same
 * interval and then returns.
 *
 * ── the higher-value sibling ────────────────────────────────────────
 * pause() needs the sleeping half only, so it can be made to work before
 * this does.  Wiring the callout to the wakeup unblocks both.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

int64_t uiox_kix_scpcs_nano_sleep(uiox_uintptr_t req, uiox_uintptr_t rem)
{
    if (!uiox_kix_scpcs_current()) return SCPCS_ESRCH;

    /* req is mandatory: a sleep with no requested duration is not a
     * request, and the struct is read by the caller's own code. */
    if (uiox_kix_scpcs_check_user_ptr(req, 16u, 8u) != 0) return SCPCS_EFAULT;

    /* rem is optional — it receives the unslept remainder — so NULL is
     * legal and only a non-NULL value is validated. */
    if (rem != 0u &&
        uiox_kix_scpcs_check_user_ptr(rem, 16u, 8u) != 0) return SCPCS_EFAULT;

    return SCPCS_ENOSYS;   /* nothing wakes a timed sleep */
}
