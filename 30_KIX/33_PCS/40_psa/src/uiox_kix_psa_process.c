/*
 * 30_KIX/33_PCS/40psa/src/uiox_kix_psa_process.c
 *
 * Process table: allocate, free, find, state transitions, run queue.
 *
 * ── the violation this file fixes ───────────────────────────────────
 * proc_set_state() implemented PSA's transition table but then had a
 * "default: valid = 1" clause AFTER the enumerated cases, so any state
 * the function had not been taught was silently allowed through.  PSA is
 * explicit that the model is closed — nine states and the arrows drawn
 * between them are the whole of it — so an unrecognised state is a bug,
 * not a transition.  This version enumerates all ten values and refuses
 * everything outside the diagram.
 *
 * ── the run-queue head ──────────────────────────────────────────────
 * The original used current_proc as the head of the ready queue.  That
 * is wrong: current_proc is whoever is executing NOW, and a running
 * process is by definition not on the ready list.  Enqueuing walked a
 * list that began with the running process, so its priority compared
 * against a process that was not a candidate.  A separate s_runq_head is
 * kept here.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_psa_process.h"
#include "../include/uiox_kix_psa_region.h"
#include "../include/uiox_kix_psa_proc_algo.h"

/* ── Globals ───────────────────────────────────────────────────────── */
uiox_kix_psa_proc_t  uiox_kix_psa_proc_table[UIOX_KIX_PSA_NPROC];
uiox_kix_psa_proc_t *uiox_kix_psa_current_proc = (uiox_kix_psa_proc_t *)0;

/* The READY queue head.  Deliberately NOT uiox_kix_psa_current_proc:
 * see the banner.  File-static because nothing outside this file has any
 * business walking the ready list — sched_pick() is the reader. */
static uiox_kix_psa_proc_t *s_runq_head = (uiox_kix_psa_proc_t *)0;

/* PID allocator.  Starts at 2 because PID 0 is the swapper and PID 1 is
 * init, both of which are created explicitly rather than allocated. */
static uint32_t s_next_pid = 2u;

/* ── uiox_kix_psa_proc_alloc ─────────────────────────────────────────
 * PSA Algorithm 1: "get free proc table slot, unique PID number".
 *
 * Scans for a slot in state UNUSED, zeroes it so no field carries over
 * from the previous occupant, and marks it CREATED.  The caller moves it
 * to READY with proc_set_state once the process is actually runnable.
 *
 * pid == 0 means "assign me one"; the counter wraps below 2 so the
 * reserved PIDs are never handed out.
 *
 * Returns NULL when the table is full.  The caller is expected to treat
 * that as PSA's "check that user not running too many processes" having
 * failed, not as a memory error. */
uiox_kix_psa_proc_t *uiox_kix_psa_proc_alloc(uint32_t pid, uint32_t ppid)
{
    int i;

    for (i = 0; i < UIOX_KIX_PSA_NPROC; i++) {
        if (uiox_kix_psa_proc_table[i].p_state != UIOX_KIX_PSA_PROC_UNUSED)
            continue;

        memset(&uiox_kix_psa_proc_table[i], 0, sizeof(uiox_kix_psa_proc_t));
        uiox_kix_psa_proc_table[i].p_state = UIOX_KIX_PSA_PROC_CREATED;
        uiox_kix_psa_proc_table[i].p_ppid  = ppid;

        if (pid != 0u) {
            uiox_kix_psa_proc_table[i].p_pid = pid;
        } else {
            uiox_kix_psa_proc_table[i].p_pid = ++s_next_pid;
            if (s_next_pid >= (uint32_t)UIOX_KIX_PSA_NPROC)
                s_next_pid = 2u;
        }
        return &uiox_kix_psa_proc_table[i];
    }
    return (uiox_kix_psa_proc_t *)0;
}

/* ── uiox_kix_psa_proc_free ──────────────────────────────────────────
 * Return a slot to the free pool.
 *
 * Zeroes the whole entry, which also clears p_next and p_prev — so a
 * process that was still linked on a queue cannot drag a freed slot back
 * onto it.  Then sets UNUSED, which is the state proc_find and proc_alloc
 * both test.
 *
 * This does NOT reap children, close files, or release regions.  Those
 * are PSA exit's steps and belong to the caller. */
void uiox_kix_psa_proc_free(uiox_kix_psa_proc_t *p)
{
    if (!p) return;
    memset(p, 0, sizeof(uiox_kix_psa_proc_t));
    p->p_state = UIOX_KIX_PSA_PROC_UNUSED;
}

/* ── uiox_kix_psa_proc_find ──────────────────────────────────────────
 * Locate a live process by PID.
 *
 * Skips UNUSED slots, so a freed slot is never returned even if its
 * stale PID happened to match.  Returns NULL when no live process has
 * the PID — callers such as kill() turn that into ESRCH. */
uiox_kix_psa_proc_t *uiox_kix_psa_proc_find(uint32_t pid)
{
    int i;

    for (i = 0; i < UIOX_KIX_PSA_NPROC; i++) {
        if (uiox_kix_psa_proc_table[i].p_state != UIOX_KIX_PSA_PROC_UNUSED &&
            uiox_kix_psa_proc_table[i].p_pid   == pid)
            return &uiox_kix_psa_proc_table[i];
    }
    return (uiox_kix_psa_proc_t *)0;
}

/* ── uiox_kix_psa_proc_set_state ─────────────────────────────────────
 * The CLOSED PSA transition table.
 *
 * Every arrow in PSA's state diagram appears as one line below.  Anything
 * not listed is refused, including an out-of-range target state.
 *
 * Refusals return -1 and change nothing — a partially applied transition
 * is worse than none, because the flag update and the state would then
 * disagree.
 *
 * Side effect: P_SWAPPED is set whenever the new state is one of the two
 * swapped states and cleared otherwise.  Keeping it in one place is what
 * stops the flag drifting out of step with p_state. */
int uiox_kix_psa_proc_set_state(uiox_kix_psa_proc_t *p,
                                uiox_kix_psa_proc_state_t new_state)
{
    int                       valid = 0;
    uiox_kix_psa_proc_state_t old;

    if (!p) return -1;

    /* Target must be a state that exists. */
    if (new_state < UIOX_KIX_PSA_PROC_UNUSED ||
        new_state > UIOX_KIX_PSA_PROC_ZOMBIE)
        return -1;

    old = p->p_state;

    switch (old) {
    case UIOX_KIX_PSA_PROC_UNUSED:
        /* A freed slot may only be re-created. */
        valid = (new_state == UIOX_KIX_PSA_PROC_CREATED);
        break;

    case UIOX_KIX_PSA_PROC_CREATED:
        /* PSA 8: "fork, enough memory -> 3; not enough memory -> 5" */
        valid = (new_state == UIOX_KIX_PSA_PROC_READY          ||
                 new_state == UIOX_KIX_PSA_PROC_READY_SWAPPED);
        break;

    case UIOX_KIX_PSA_PROC_USER_RUNNING:
        /* PSA 1: syscall or interrupt -> 2; preempted -> 7 */
        valid = (new_state == UIOX_KIX_PSA_PROC_KERNEL_RUNNING ||
                 new_state == UIOX_KIX_PSA_PROC_PREEMPTED);
        break;

    case UIOX_KIX_PSA_PROC_KERNEL_RUNNING:
        /* PSA 2: return -> 1; preempt -> 7; exit -> 9;
         *        sleep -> 4; reschedule -> 3 */
        valid = (new_state == UIOX_KIX_PSA_PROC_USER_RUNNING ||
                 new_state == UIOX_KIX_PSA_PROC_PREEMPTED    ||
                 new_state == UIOX_KIX_PSA_PROC_ZOMBIE       ||
                 new_state == UIOX_KIX_PSA_PROC_SLEEP_MEM    ||
                 new_state == UIOX_KIX_PSA_PROC_READY);
        break;

    case UIOX_KIX_PSA_PROC_READY:
        /* PSA 3: reschedule -> 2; wakeup/sleep -> 4; swap out -> 5 */
        valid = (new_state == UIOX_KIX_PSA_PROC_KERNEL_RUNNING ||
                 new_state == UIOX_KIX_PSA_PROC_SLEEP_MEM      ||
                 new_state == UIOX_KIX_PSA_PROC_READY_SWAPPED);
        break;

    case UIOX_KIX_PSA_PROC_SLEEP_MEM:
        /* PSA 4: wakeup -> 3; swap out -> 6 */
        valid = (new_state == UIOX_KIX_PSA_PROC_READY ||
                 new_state == UIOX_KIX_PSA_PROC_SLEEP_SWAPPED);
        break;

    case UIOX_KIX_PSA_PROC_READY_SWAPPED:
        /* PSA 5: swap in -> 3; stays swapped */
        valid = (new_state == UIOX_KIX_PSA_PROC_READY ||
                 new_state == UIOX_KIX_PSA_PROC_READY_SWAPPED);
        break;

    case UIOX_KIX_PSA_PROC_SLEEP_SWAPPED:
        /* PSA 6: swap out -> 4; wakeup -> 5 */
        valid = (new_state == UIOX_KIX_PSA_PROC_SLEEP_MEM ||
                 new_state == UIOX_KIX_PSA_PROC_READY_SWAPPED);
        break;

    case UIOX_KIX_PSA_PROC_PREEMPTED:
        /* PSA 7: return to user -> 1; ready -> 3 */
        valid = (new_state == UIOX_KIX_PSA_PROC_USER_RUNNING ||
                 new_state == UIOX_KIX_PSA_PROC_READY);
        break;

    case UIOX_KIX_PSA_PROC_ZOMBIE:
        /* PSA: "the zombie state is the FINAL state of a process."
         * Nothing leaves it except proc_free, which sets UNUSED
         * directly rather than transitioning. */
        valid = 0;
        break;

    default:
        /* Not a PSA state at all.  Refuse; do not wave it through. */
        valid = 0;
        break;
    }

    if (!valid) return -1;

    p->p_state = new_state;

    if (new_state == UIOX_KIX_PSA_PROC_READY_SWAPPED ||
        new_state == UIOX_KIX_PSA_PROC_SLEEP_SWAPPED)
        p->p_flag |=  UIOX_KIX_PSA_P_SWAPPED;
    else
        p->p_flag &= ~UIOX_KIX_PSA_P_SWAPPED;

    return 0;
}

/* ── uiox_kix_psa_proc_sig_pending ───────────────────────────────────
 * Signals that are pending AND not blocked.
 *
 * The mask-out is the whole point: p_sig alone says a signal arrived,
 * p_sigmask says the process asked not to be disturbed by it, and the
 * interruptible branch of proc_sleep must test the COMBINATION or it
 * will wake for a signal it is supposed to be ignoring.
 *
 * Returns a bitmask, so 0 means "nothing deliverable". */
int uiox_kix_psa_proc_sig_pending(uiox_kix_psa_proc_t *p)
{
    if (!p) return 0;
    return (int)(p->p_sig & ~p->p_sigmask);
}

/* ── uiox_kix_psa_sched_enqueue ──────────────────────────────────────
 * Insert a process into the READY queue in priority order.
 *
 * p_sched.p_pri is LOWER-is-higher, so the walk advances past every
 * process whose priority is numerically less-or-equal and stops at the
 * first one that outranks the newcomer.  Equal priorities therefore
 * append AFTER existing equals, which gives round-robin behaviour among
 * them rather than starving the newcomer or the incumbent.
 *
 * The caller is responsible for the process being in state READY; this
 * function only manages linkage. */
void uiox_kix_psa_sched_enqueue(uiox_kix_psa_proc_t *p)
{
    uiox_kix_psa_proc_t *cur;
    uiox_kix_psa_proc_t *prev;

    if (!p) return;

    cur  = s_runq_head;
    prev = (uiox_kix_psa_proc_t *)0;

    while (cur && cur->p_sched.p_pri <= p->p_sched.p_pri) {
        prev = cur;
        cur  = cur->p_next;
    }

    p->p_next = cur;
    p->p_prev = prev;
    if (prev) prev->p_next = p;
    else      s_runq_head   = p;
    if (cur)  cur->p_prev  = p;
}

/* ── uiox_kix_psa_sched_dequeue ──────────────────────────────────────
 * Unlink a process from the ready queue.
 *
 * Handles all four linkage cases: middle, head, tail, and the sole
 * element.  When the process was at the front, s_runq_head advances —
 * forgetting that case is how a queue loses its front and any later walk
 * starts inside the list.
 *
 * Clears both link fields so a stale p_next cannot resurrect the entry
 * on a later scan. */
void uiox_kix_psa_sched_dequeue(uiox_kix_psa_proc_t *p)
{
    if (!p) return;

    if (p->p_prev) p->p_prev->p_next = p->p_next;
    else           s_runq_head       = p->p_next;

    if (p->p_next) p->p_next->p_prev = p->p_prev;

    p->p_next = (uiox_kix_psa_proc_t *)0;
    p->p_prev = (uiox_kix_psa_proc_t *)0;
}

/* ── uiox_kix_psa_sched_pick ─────────────────────────────────────────
 * PSA's "reschedule process": the highest-priority READY process.
 *
 * The queue is priority-ordered, so the head IS the answer — no scan.
 * Read-only: it does not dequeue, change state, or switch context.  The
 * caller performs those, which keeps this safe to call for a priority
 * comparison without side effects. */
uiox_kix_psa_proc_t *uiox_kix_psa_sched_pick(void)
{
    return s_runq_head;
}
