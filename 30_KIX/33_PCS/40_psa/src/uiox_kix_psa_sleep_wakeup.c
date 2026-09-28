/*
 * 30_KIX/33_PCS/40psa/src/uiox_kix_psa_sleep_wakeup.c
 *
 * PSA Algorithm 10 (sleep) and Algorithm 11 (wakeup).
 *
 * ── the defect this file fixes ──────────────────────────────────────
 * The original sleep_enqueue maintained the queue head but never the
 * tail:
 *
 *     p->p_next = sleep_hash[slot].sq_head;
 *     p->p_prev = (proc_t *)0;
 *     if (sleep_hash[slot].sq_head)
 *         sleep_hash[slot].sq_head->p_prev = p;
 *     else
 *         sleep_hash[slot].sq_tail = p;      <- only on the EMPTY path
 *     sleep_hash[slot].sq_head = p;
 *
 * So sq_tail is written once, on the first insert, and goes stale forever
 * after.  Nothing reads it today — proc_wakeup walks from sq_head — which
 * makes the bug LATENT rather than harmless: the first future append that
 * trusts sq_tail corrupts the queue silently.  Both enqueue and dequeue
 * maintain head, tail and p_prev here.
 *
 * ── the two-test sleep ──────────────────────────────────────────────
 * proc_sleep checks for a pending signal BOTH before and after the
 * context switch.  The order matters: a signal that arrives between the
 * two checks is not lost, because the second check sees it.  Skipping the
 * first check would let the process sleep with a signal already pending,
 * which on a single-processor system means it never wakes.
 *
 * ── what a wakeup cannot yet do ─────────────────────────────────────
 * proc_wakeup moves a sleeper to READY and enqueues it, and that part is
 * complete.  Nothing CALLS proc_wakeup on a timer, so a timed sleep never
 * ends.  The waker is 01_schedular's callout table; joining the two is
 * the highest-value integration in this layer.  pause() is the cheapest
 * first test because it needs no timer at all.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_psa_proc_algo.h"
#include "../include/uiox_kix_psa_process.h"

/* ── Globals ───────────────────────────────────────────────────────── */
uiox_kix_psa_sleep_queue_t
    uiox_kix_psa_sleep_hash[UIOX_KIX_PSA_SLEEP_HASH_SZ];

int uiox_kix_psa_scheduler_flag = 0;
int uiox_kix_psa_proc_level     = 0;

/* The running process's u area.  PSA allocates this per running process,
 * not per table slot, so there is exactly one and it belongs to whoever
 * is executing. */
uiox_kix_psa_u_area_t uiox_kix_psa_u;

/* ── Processor-level stubs ───────────────────────────────────────────
 * In a real kernel these mask and unmask hardware interrupts around a
 * critical section.  The arch layer provides the real ones; these keep
 * the layer linkable standalone, and returning 0 means "no prior level",
 * which splx then restores harmlessly. */
static inline int splhigh(void) { return 0; }
static inline int splx(int lvl) { (void)lvl; return 0; }

/* ── sleep_hash_slot ─────────────────────────────────────────────────
 * Hash a wait channel to a bucket index.
 *
 * Shifts right by 2 first, because event addresses are typically
 * word-aligned and the low two bits would otherwise be constant — with
 * them included, every channel in a structure would land in one bucket
 * and the "hash" would degrade to a list.
 *
 * The modulo bounds the result to SLEEP_HASH_SZ buckets. */
static int sleep_hash_slot(uintptr_t wchan)
{
    return (int)((wchan >> 2) % (uintptr_t)UIOX_KIX_PSA_SLEEP_HASH_SZ);
}

/* ── sleep_enqueue ───────────────────────────────────────────────────
 * Put a process on the sleep queue for its wait channel.
 *
 * Inserts at the HEAD, so a wakeup walks newest first.  Order among
 * sleepers on one channel is not significant — proc_wakeup wakes them
 * all — so a stack is as good as a queue and cheaper to maintain.
 *
 * Maintains sq_tail on the empty path only, because an insert at the
 * head does not move the tail: the existing tail is still the last
 * element.  Recording p_wchan here is what proc_wakeup compares against,
 * so a bucket holding several channels can tell its members apart.
 *
 * Sets p_prev to NULL so the new head is a well-formed list start. */
static void sleep_enqueue(uiox_kix_psa_proc_t *p, uintptr_t wchan)
{
    int slot = sleep_hash_slot(wchan);
    uiox_kix_psa_proc_t *head = uiox_kix_psa_sleep_hash[slot].sq_head;

    p->p_wchan = wchan;
    p->p_next  = head;
    p->p_prev  = (uiox_kix_psa_proc_t *)0;

    if (head) {
        head->p_prev = p;          /* tail unchanged: last element stays */
    } else {
        uiox_kix_psa_sleep_hash[slot].sq_tail = p;   /* first element */
    }

    uiox_kix_psa_sleep_hash[slot].sq_head = p;
}

/* ── sleep_dequeue ───────────────────────────────────────────────────
 * Remove a process from the sleep queue.
 *
 * Fixes up all three references: the forward neighbour, the backward
 * neighbour, and both the head and the tail when the process was at
 * either end.  The tail fix-up is the half the original was missing
 * everywhere except enqueue's empty path.
 *
 * Clears p_wchan as well as the links, so a process that is no longer
 * sleeping cannot be matched by a later wakeup on the same channel. */
static void sleep_dequeue(uiox_kix_psa_proc_t *p, uintptr_t wchan)
{
    int slot = sleep_hash_slot(wchan);

    if (p->p_prev) p->p_prev->p_next = p->p_next;
    else           uiox_kix_psa_sleep_hash[slot].sq_head = p->p_next;

    if (p->p_next) p->p_next->p_prev = p->p_prev;
    else           uiox_kix_psa_sleep_hash[slot].sq_tail = p->p_prev;

    p->p_next  = (uiox_kix_psa_proc_t *)0;
    p->p_prev  = (uiox_kix_psa_proc_t *)0;
    p->p_wchan = 0;
}

/* ── PSA Algorithm 10 — uiox_kix_psa_proc_sleep ──────────────────────
 * Sleep on a wait channel until woken or signalled.
 *
 * Branch selection is by priority against PZERO:
 *
 *   priority <= PZERO, or interruptible == 0
 *     UNINTERRUPTIBLE.  The process moves to SLEEP_MEM and is enqueued.
 *     No signal can end this sleep; only proc_wakeup can.  Returning 0
 *     unconditionally is correct because no other outcome is reachable.
 *
 *   priority > PZERO
 *     INTERRUPTIBLE.  It sleeps only if nothing is deliverable, then
 *     re-checks on wake.  Three outcomes: 0 woken normally, 1 woken by
 *     a signal the process catches, -1 woken by one it does not.
 *
 * The -1 path sets u_error to EINTR and raises the abort flag so the
 * syscall entry unwinds the call.  Both are needed: u_error is what a
 * caller inspects, and the flag is what makes the dispatcher abandon
 * work it has already started.
 *
 * The state change goes through proc_set_state, so an invalid transition
 * is refused rather than forced — which is why the return value is
 * ignored here but the call is not. */
int uiox_kix_psa_proc_sleep(uintptr_t wchan, int priority, int interruptible)
{
    uiox_kix_psa_proc_t *p = uiox_kix_psa_current_proc;
    int                  old_level;

    if (!p) return 0;

    old_level = splhigh();

    /* ── Uninterruptible ─────────────────────────────────────────── */
    if (!interruptible || priority <= UIOX_KIX_PSA_PZERO) {

        (void)uiox_kix_psa_proc_set_state(p, UIOX_KIX_PSA_PROC_SLEEP_MEM);
        sleep_enqueue(p, wchan);

        {
            uiox_kix_psa_proc_t *next = uiox_kix_psa_sched_pick();

            if (next && next != p) {
                (void)uiox_kix_psa_proc_set_state(
                        next, UIOX_KIX_PSA_PROC_KERNEL_RUNNING);
                uiox_kix_psa_sched_dequeue(next);
                uiox_kix_psa_current_proc = next;
                /* The register switch is uiox_kix_psa_swtch(), which the
                 * arch layer defines.  Bookkeeping only, here. */
            }
        }

        splx(old_level);
        return 0;
    }

    /* ── Interruptible: first test ───────────────────────────────── */
    if (!uiox_kix_psa_proc_sig_pending(p)) {

        (void)uiox_kix_psa_proc_set_state(p, UIOX_KIX_PSA_PROC_SLEEP_MEM);
        sleep_enqueue(p, wchan);

        {
            uiox_kix_psa_proc_t *next = uiox_kix_psa_sched_pick();

            if (next && next != p) {
                (void)uiox_kix_psa_proc_set_state(
                        next, UIOX_KIX_PSA_PROC_KERNEL_RUNNING);
                uiox_kix_psa_sched_dequeue(next);
                uiox_kix_psa_current_proc = next;
            }
        }
    }

    splx(old_level);

    /* ── Second test: nothing pending means an ordinary wakeup ───── */
    if (!uiox_kix_psa_proc_sig_pending(p))
        return 0;

    /* A caught signal simply returns to the caller, which resumes the
     * interrupted system call. */
    if (priority > UIOX_KIX_PSA_PZERO)
        return 1;

    /* An uncaught signal ends the call: raise the abort flag and report
     * EINTR so the syscall entry unwinds rather than resuming. */
    uiox_kix_psa_u.u_qsave.regs[0] = 1u;
    uiox_kix_psa_u.u_error         = EINTR;
    return -1;
}

/* ── PSA Algorithm 11 — uiox_kix_psa_proc_wakeup ─────────────────────
 * Wake every process sleeping on a wait channel.
 *
 * The bucket is keyed on the channel, but several channels can hash to
 * one slot — so each candidate is tested with p_wchan == wchan before
 * being woken.  Waking by bucket alone would wake processes waiting on
 * unrelated events, which they would then re-enter, in a loop.
 *
 * next_p is captured BEFORE the dequeue, because dequeue clears p_next.
 * Reading p->p_next after unlinking would walk into NULL and stop early,
 * leaving later sleepers on a queue whose tail no longer points at them.
 *
 * Each woken process moves from its SLEEP state to the matching READY
 * state — SLEEP_MEM to READY, SLEEP_SWAPPED to READY_SWAPPED — and only a
 * resident process is enqueued on the ready queue, because a swapped one
 * must be brought in first.
 *
 * scheduler_flag is set when a woken process outranks the running one.
 * That is a request to reschedule, not a switch: the kernel checks the
 * flag at its next safe point. */
void uiox_kix_psa_proc_wakeup(uintptr_t wchan)
{
    int                  slot      = sleep_hash_slot(wchan);
    int                  old_level = splhigh();
    uiox_kix_psa_proc_t *p         = uiox_kix_psa_sleep_hash[slot].sq_head;
    uiox_kix_psa_proc_t *next_p;

    while (p) {
        next_p = p->p_next;          /* capture before the unlink */

        if (p->p_wchan == wchan) {
            sleep_dequeue(p, wchan);

            if (p->p_state == UIOX_KIX_PSA_PROC_SLEEP_MEM)
                (void)uiox_kix_psa_proc_set_state(
                        p, UIOX_KIX_PSA_PROC_READY);
            else if (p->p_state == UIOX_KIX_PSA_PROC_SLEEP_SWAPPED)
                (void)uiox_kix_psa_proc_set_state(
                        p, UIOX_KIX_PSA_PROC_READY_SWAPPED);

            if (p->p_state == UIOX_KIX_PSA_PROC_READY)
                uiox_kix_psa_sched_enqueue(p);

            if (uiox_kix_psa_current_proc &&
                p->p_sched.p_pri < uiox_kix_psa_current_proc->p_sched.p_pri)
                uiox_kix_psa_scheduler_flag = 1;
        }

        p = next_p;
    }

    splx(old_level);
}
