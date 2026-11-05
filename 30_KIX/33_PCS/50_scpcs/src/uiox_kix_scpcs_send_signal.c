/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_send_signal.c
 *
 * send_signal() — mark a signal pending on a process.
 *
 * ── what this is, and what it is NOT ────────────────────────────────
 * It SETS the pending bit.  Delivery happens on the way back to EL0, in
 * the per-arch signal check (arm64/arm32/riscv64/x86_64_signal_check)
 * which calls issig() then psig() just before the registers are
 * restored.  Marking without delivering inside the sender's call is the
 * correct split: a signal to a RUNNING process takes effect at its next
 * return, not in the middle of someone else's syscall.
 *
 * ── the bit convention ──────────────────────────────────────────────
 *   bit number == signal number       p_sig |= (1u << signum)
 *
 * This file established that convention; sigprocmask writes p_sigmask
 * with the same shift, because uiox_kix_psa_proc_sig_pending() masks one
 * against the other bit-for-bit:
 *
 *     return (int)(p->p_sig & ~p->p_sigmask);
 *
 * A different shift in either field makes every signal report as
 * deliverable, or none — silently, and in the same direction for every
 * process.
 *
 * ── why signum is bounded ───────────────────────────────────────────
 * UIOX_KIX_PSA_NSIG bounds it.  A signum at or above it would shift past
 * the width of p_sig, which is undefined behaviour rather than simply
 * "no such signal" — so the bound is a correctness check, not a courtesy.
 *
 * ── the wake, and the one state deliberately left alone ─────────────
 * A resident sleeper is enqueued so the scheduler re-tests it; without
 * that, the pending bit would sit unread until something else woke the
 * process.  A SWAPPED sleeper is NOT enqueued: the ready queue means
 * "resident and runnable", and claiming that for a process whose pages
 * are out would have the scheduler switch to it before the swap-in.  Its
 * signal is still marked, and the swap-in path re-tests it — the same
 * re-test sleep_wakeup.c already performs.
 *
 * @version 1.0.0
 */
#include "../include/uiox_kix_scpcs.h"

void uiox_kix_scpcs_send_signal(uiox_kix_psa_proc_t *p, int signum)
{
    if (!p) return;
    if (signum <= 0 || signum >= (int)UIOX_KIX_PSA_NSIG) return;

    p->p_sig |= (uiox_uint32_t)(1u << (uiox_uint32_t)signum);

    if (p->p_state == UIOX_KIX_PSA_PROC_SLEEP_MEM)
        uiox_kix_psa_sched_enqueue(p);
}

