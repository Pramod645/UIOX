/*
 * 30_KIX/33_PCS/01_schedular/src/uiox_kix_scheduler.c
 *
 * Bach's Algorithm 1 — schedule_process — and the multilevel feedback
 * run queue it picks from.
 *
 * ── the queue holds 40_psa entries ──────────────────────────────────
 * Every band link is a uiox_kix_psa_proc_t *, which is the type p_next
 * and p_prev actually are.  There is no cast in this file: to reach the
 * scheduler's wrapper for a queued entry, scp_to_wrapper() states the
 * layout fact once, in scheduler.h.
 *
 * Two prefixes, one owner each:
 *   e->p_state, e->p_sched, e->p_timers, e->p_next ...  40_psa's fields
 *   s->s_time_slice, s->s_total_ticks, ...              scheduler's
 *
 * ── the state model is PSA's ten ────────────────────────────────────
 *   TASK_RUNNING          -> UIOX_KIX_PSA_PROC_READY
 *   TASK_SLEEPING         -> UIOX_KIX_PSA_PROC_SLEEP_MEM
 *   TASK_INTERRUPTIBLE    -> UIOX_KIX_PSA_PROC_SLEEP_MEM
 *   TASK_UNINTERRUPTIBLE  -> UIOX_KIX_PSA_PROC_SLEEP_MEM
 *   TASK_ZOMBIE           -> UIOX_KIX_PSA_PROC_ZOMBIE
 *   TASK_STOPPED          -> UIOX_KIX_PSA_PROC_PREEMPTED
 *
 * RUNNING became READY because a process ON a run queue is Bach's state
 * 3 — "ready to run as soon as the kernel schedules it".  Only the one
 * on the CPU is USER_RUNNING or KERNEL_RUNNING, and it is not queued.
 *
 * The three sleeping values collapsed into one because 40_psa encodes
 * interruptibility in the PRIORITY a sleep is taken at, not in the
 * state — see uiox_kix_psa_proc_sleep.  That is the older model, and the
 * collapse is faithful rather than lossy.
 *
 * @version 4.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scheduler.h"

/* ── Global state ────────────────────────────────────────────────────── */
static RunQueue              rq;
static uiox_sched_proc_t    *s_current = (uiox_sched_proc_t *)0;

volatile uint64_t jiffies    = 0;
volatile uint64_t jiffies_64 = 0;
XTime             xtime      = {0, 0};

/* ── Priority to band ────────────────────────────────────────────────── */
static int priority_to_band(int prio)
{
    int band = MAX_PRIORITY / MAX_PRIORITY_QUEUES;   /* 28 */

    if (prio < 0)             prio = 0;
    if (prio >= MAX_PRIORITY) prio = MAX_PRIORITY - 1;

    return prio / band;
}

/* ── Eligibility ─────────────────────────────────────────────────────── */
static bool sched_eligible(const uiox_kix_psa_proc_t *e, bool in_memory)
{
    if (!e) return false;
    if (!in_memory) return false;
    return e->p_state == UIOX_KIX_PSA_PROC_READY;
}

/* ── scheduler_init ──────────────────────────────────────────────────── */
void scheduler_init(void)
{
    int q;
    for (q = 0; q < MAX_PRIORITY_QUEUES; q++) rq.heads[q] = (uiox_kix_psa_proc_t *)0;
    rq.count  = 0;
    s_current = (uiox_sched_proc_t *)0;
}

/* ── enqueue_process ─────────────────────────────────────────────────
 * Head of band, because within one band the processes have equal
 * priority and a stack is as fair as a queue there while costing less. */
void enqueue_process(uiox_sched_proc_t *p)
{
    uiox_kix_psa_proc_t *e;
    int                  q;

    if (!p) return;
    if (!sched_eligible(&p->p, p->s_in_memory != 0)) return;

    e = scp_to_entry(p);
    q = priority_to_band(e->p_sched.p_pri);

    e->p_next = rq.heads[q];
    e->p_prev = (uiox_kix_psa_proc_t *)0;
    if (rq.heads[q]) rq.heads[q]->p_prev = e;
    rq.heads[q] = e;
    rq.count++;
}

/* ── dequeue_process ─────────────────────────────────────────────────
 * No casts: the band head and the link are the same type.  Fixes up
 * both neighbours, which the old single-linked version could not. */
void dequeue_process(uiox_sched_proc_t *p)
{
    uiox_kix_psa_proc_t *e;
    int                  q;

    if (!p) return;

    e = scp_to_entry(p);
    q = priority_to_band(e->p_sched.p_pri);

    if (e->p_prev) e->p_prev->p_next = e->p_next;
    else           rq.heads[q]       = e->p_next;

    if (e->p_next) e->p_next->p_prev = e->p_prev;

    e->p_next = (uiox_kix_psa_proc_t *)0;
    e->p_prev = (uiox_kix_psa_proc_t *)0;
    rq.count--;
}

/* ── recalculate_priority ────────────────────────────────────────────
 * p_pri = clamp(p_cpu + p_nice, 1, MAX_PRIORITY - 1)
 *
 * A larger p_pri is a LOWER priority, and p_nice is ADDED, so a positive
 * nice lowers priority.  The two agree in sign. */
void recalculate_priority(uiox_sched_proc_t *p)
{
    int new_prio;

    if (!p) return;

    new_prio = (int)p->p.p_sched.p_cpu + (int)p->p.p_sched.p_nice;

    if (new_prio < 1)             new_prio = 1;
    if (new_prio >= MAX_PRIORITY) new_prio = MAX_PRIORITY - 1;

    p->p.p_sched.p_pri = new_prio;
}

/* ── readjust_all_priorities ─────────────────────────────────────────── */
void readjust_all_priorities(void)
{
    int q;
    uiox_kix_psa_proc_t *e;

    for (q = 0; q < MAX_PRIORITY_QUEUES; q++)
        for (e = rq.heads[q]; e; e = e->p_next)
            if (e->p_state == UIOX_KIX_PSA_PROC_READY)
                recalculate_priority(scp_to_wrapper(e));
}

/* ── Algorithm schedule_process — Bach, Algorithm 1 ──────────────────
 * The scan is over bands, highest first.  The idle branch returns NULL
 * rather than spinning: this build has no halt instruction, and a spin
 * would burn the CPU the next interrupt needs. */
uiox_sched_proc_t *schedule_process(void)
{
    uiox_kix_psa_proc_t *e;
    uiox_sched_proc_t   *chosen = (uiox_sched_proc_t *)0;
    int                  q;

    for (q = 0; q < MAX_PRIORITY_QUEUES && !chosen; q++) {
        for (e = rq.heads[q]; e; e = e->p_next) {
            uiox_sched_proc_t *s = scp_to_wrapper(e);
            if (sched_eligible(e, s->s_in_memory != 0)) { chosen = s; break; }
        }
    }

    if (!chosen) return (uiox_sched_proc_t *)0;    /* machine idle */

    dequeue_process(chosen);

    /* 3 -> 2 "reschedule process".  Validated by 40_psa's table rather
     * than forced, so an invalid transition leaves the state alone. */
    (void)uiox_kix_psa_proc_set_state(&chosen->p,
                                      UIOX_KIX_PSA_PROC_KERNEL_RUNNING);

    chosen->s_time_slice = TIME_QUANTUM;
    s_current            = chosen;

    return chosen;
}

/* ── tick_process ────────────────────────────────────────────────────
 * Charging the RUNNING process is done here rather than in clock.c,
 * because a running process is not on any queue and clock.c only walks
 * the bands.  The division is deliberate: uiox_kix_scheduler.c counts who runs,
 * clock.c counts who waits.
 *
 * Returns TRUE when the slice is exhausted and the caller must
 * reschedule. */
bool tick_process(uiox_sched_proc_t *running)
{
    if (!running) return false;

    running->s_total_ticks++;
    running->p.p_timers.p_utime++;          /* struct tms — user time */

    running->s_cpu_usage =
        (uint32_t)((running->s_cpu_usage * 7u + 255u) / 8u);
    running->p.p_sched.p_cpu = running->s_cpu_usage;

    if (running->s_time_slice > 0)
        running->s_time_slice--;

    if (running->s_time_slice == 0) {
        if (running->p.p_sched.p_pri < MAX_PRIORITY - 1)
            running->p.p_sched.p_pri++;     /* fed to a lower band */

        (void)uiox_kix_psa_proc_set_state(&running->p,
                                          UIOX_KIX_PSA_PROC_READY);
        enqueue_process(running);
        return true;
    }
    return false;
}

/* ── fair_share_adjust ───────────────────────────────────────────────── */
void fair_share_adjust(int group_id)
{
    uint64_t             group_total = 0;
    int                  group_size  = 0;
    int                  q;
    uiox_kix_psa_proc_t *e;
    uint64_t             avg;

    for (q = 0; q < MAX_PRIORITY_QUEUES; q++)
        for (e = rq.heads[q]; e; e = e->p_next) {
            uiox_sched_proc_t *s = scp_to_wrapper(e);
            if (s->s_group_id == group_id) {
                group_total += s->s_total_ticks;
                group_size++;
            }
        }

    if (!group_size) return;
    avg = group_total / (uint64_t)group_size;

    for (q = 0; q < MAX_PRIORITY_QUEUES; q++) {
        for (e = rq.heads[q]; e; e = e->p_next) {
            uiox_sched_proc_t *s = scp_to_wrapper(e);

            if (s->s_group_id != group_id) continue;

            if (s->s_total_ticks > avg && e->p_sched.p_pri < MAX_PRIORITY - 1)
                e->p_sched.p_pri++;        /* penalise: lower priority */
            else if (s->s_total_ticks < avg && e->p_sched.p_pri > 1)
                e->p_sched.p_pri--;        /* reward: higher priority   */
        }
    }
}

/* ── scheduler_print ─────────────────────────────────────────────────
 * Dumps the run queue, one band per line.
 *
 * It prints through uiox_printf — the freestanding primitive declared in
 * uiox_klibc.h — because this layer has no console of its own and the
 * alternative was a loop that walked every band and produced nothing.
 * A diagnostic that does not report is not a diagnostic.
 *
 * Note the reporting contract: an EMPTY band is skipped entirely rather
 * than printed as a bare header.  On a queue with processes in two of
 * five bands, four empty headers are noise around the two lines that
 * matter — and at one tick per second that noise is most of the output.
 *
 * p_pri is printed alongside the pid because the band index alone does
 * not say WHERE in the band a process sits, and a priority that has
 * drifted to the band edge is the thing a reader is usually looking for. */
void scheduler_print(void)
{
    int                  q;
    uiox_kix_psa_proc_t *e;

    uiox_printf("[sched] run queue, %d entries\n", rq.count);

    for (q = 0; q < MAX_PRIORITY_QUEUES; q++) {
        if (!rq.heads[q]) continue;          /* skip empty bands */

        uiox_printf("  band %d:", q);

        for (e = rq.heads[q]; e; e = e->p_next) {
            uiox_sched_proc_t *s = scp_to_wrapper(e);
            uiox_printf(" pid=%u(pri=%d,slice=%d)",
                        (unsigned)e->p_pid,
                        e->p_sched.p_pri,
                        s->s_time_slice);
        }
        uiox_printf("\n");
    }
}

RunQueue *get_run_queue(void) { return &rq; }
