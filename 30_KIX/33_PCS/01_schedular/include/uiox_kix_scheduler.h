/*
 * 30_KIX/33_PCS/01_schedular/include/uiox_kix_scheduler.h
 *
 * The scheduler — Bach's Algorithm 1 (schedule_process), and the
 * multilevel feedback run queue it picks from.
 *
 * ── the run queue holds 40_psa entries, not wrappers ────────────────
 * An earlier version of this header made the bands hold \`Process *\` —
 * the scheduler's wrapper struct — while the LINK FIELDS inside it are
 * \`uiox_kix_psa_proc_t *\`.  That mismatch forced a cast on every walk,
 * because the band head had one type and the links had another.
 *
 * The bands now hold \`uiox_kix_psa_proc_t *\` directly: the type the
 * links actually are, and the type 40_psa's own sched_enqueue /
 * sched_dequeue / sched_pick already operate on.  No casts, and one
 * fewer way for the two to disagree.
 *
 * ── how this relates to 40_psa's queue routines ─────────────────────
 * Bach: "the scheduler ... belongs to the general class known as round
 * robin with multilevel feedback, meaning that the kernel allocates the
 * CPU to a process for a time quantum, preempts a process that exceeds
 * its time quantum, and feeds it back into one of several priority
 * queues."
 *
 * 40_psa's routines maintain a SINGLE ordered list.  This header declares
 * the MULTILEVEL queue: five bands, highest priority first, which is the
 * structure the algorithm needs.  The two are not rivals — same type,
 * same p_next / p_prev links.  A sleep and a wakeup use the single-list
 * case; a tick uses the banded case.
 *
 * ── why the scheduler wrapper still exists ──────────────────────────
 * \`uiox_sched_proc_t\` carries six fields 40_psa has no reason to know:
 * time slice, total ticks, CPU usage, policy, group, residency.  Those
 * are scheduling POLICY rather than process identity, so they live in a
 * wrapper — and the wrapper is what a caller holds, while the queue
 * links the inner entry.  To reach the wrapper from a link, use
 * scp_to_wrapper() below.
 *
 * @version 4.0.0  @date 2026-09-29
 */
#ifndef UIOX_SCHEDULER_H
#define UIOX_SCHEDULER_H

#include "sched_types.h"

/* ── Multilevel feedback run queue ───────────────────────────────────
 * MAX_PRIORITY_QUEUES bands; band 0 is highest priority.  A process is
 * placed by its p_sched.p_pri, which is LOWER-is-higher — the same
 * convention the band order uses, so the mapping is direct and needs no
 * inversion. */
typedef struct {
    uiox_kix_psa_proc_t *heads[MAX_PRIORITY_QUEUES];
    int                  count;      /* total entries queued */
} RunQueue;

/* ── Reaching the wrapper from a queued entry ────────────────────────
 * The queue links uiox_kix_psa_proc_t, which is the FIRST member of
 * uiox_sched_proc_t.  So the wrapper's address equals the inner entry's
 * address — but that is a fact about the layout, and this helper states
 * it once instead of scattering casts through the walk loops.
 *
 * If the member order in sched_types.h ever changes so the inner entry
 * is no longer first, this function must change with it.  That is the
 * one place to look. */
static inline uiox_sched_proc_t *scp_to_wrapper(uiox_kix_psa_proc_t *p)
{
    return (uiox_sched_proc_t *)(void *)p;
}

/* The reverse: from a wrapper to the entry the queue links. */
static inline uiox_kix_psa_proc_t *scp_to_entry(uiox_sched_proc_t *s)
{
    return &s->p;
}

/* ── Scheduler API ─────────────────────────────────────────────────── */

/* Initialise the run queue.  Does NOT touch the process table — that is
 * 40_psa's uiox_kix_psa_proc_alloc. */
void  scheduler_init(void);

/* ────────────────────────────────────────────────────────────────────
 * Algorithm schedule_process — Bach, Algorithm 1, verbatim:
 *
 *   input: none
 *   output: none
 *   {
 *       while (no process picked to execute)
 *       {
 *           for (every process on run queue)
 *               pick highest priority process that is loaded in memory;
 *           if (no process eligible to execute)
 *               idle the machine;
 *               // interrupt takes machine out of idle state
 *       }
 *       remove chosen process from run queue;
 *       switch context to that of chosen process, resume its execution;
 *   }
 *
 * Returns the chosen wrapper, or NULL when nothing is eligible.  The
 * inner scan is over the BANDS, highest first — and within a band over
 * the entries in it.  "loaded in memory" is a presence test: a process
 * that is swapped out is skipped, not chosen and then failed.
 * ──────────────────────────────────────────────────────────────────── */
uiox_sched_proc_t *schedule_process(void);

/* Place a process in the band its priority names.  Refuses one that is
 * not READY or not resident — a queue holding something unrunnable would
 * be chosen and then fail. */
void  enqueue_process(uiox_sched_proc_t *p);

/* Remove a process from whichever band holds it. */
void  dequeue_process(uiox_sched_proc_t *p);

/* ────────────────────────────────────────────────────────────────────
 * Recalculate dynamic priority.
 *
 * Bach: "Every active process has a scheduling priority; the kernel ...
 * recalculates the priority of the running process when it returns from
 * kernel mode to user mode."
 *
 *   p_sched.p_pri = clamp(p_sched.p_cpu + p_sched.p_nice, 1, PRIO_MAX)
 *
 * A process that has used more CPU ends up with a LARGER p_pri, which is
 * a LOWER priority — the feedback that makes the queue fair.  p_nice is
 * ADDED: a positive nice lowers priority, and here lowering priority
 * means a larger number, so the two agree in sign.
 * ──────────────────────────────────────────────────────────────────── */
void  recalculate_priority(uiox_sched_proc_t *p);

/* Recalculate every READY process in every band.  Bach: "it periodically
 * readjusts the priority of every ready-to-run process."  Called once
 * per second by the clock interrupt. */
void  readjust_all_priorities(void);

/* ────────────────────────────────────────────────────────────────────
 * Tick the running process.
 *
 * Decrements the time slice; when it reaches zero the process is fed
 * back into a lower band and the caller should reschedule.  Returns TRUE
 * when the slice is exhausted.
 * ──────────────────────────────────────────────────────────────────── */
bool  tick_process(uiox_sched_proc_t *running);

/* Fair-share: penalise processes in a group that over-consumed CPU
 * relative to their peers, and reward those that under-consumed. */
void  fair_share_adjust(int group_id);

/* Dump queue state (debug). */
void  scheduler_print(void);

/* Access the global run queue. */
RunQueue *get_run_queue(void);

#endif /* UIOX_SCHEDULER_H */
