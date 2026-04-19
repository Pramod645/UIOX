#include "../include/scheduler.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ── Globals ────────────────────────────────────────────────── */
proc_entry_t  proc_table[NPROC];
proc_entry_t *current_proc  = NULL;
run_queue_t   run_queue      = { .rq_count = 0 };
uint64_t      clock_ticks    = 0;
int           need_resched   = 0;

/* ── sched_init ──────────────────────────────────────────────
 * Zero the process table and reset the run queue.
 */
void sched_init(void)
{
    memset(proc_table, 0, sizeof(proc_table));
    memset(&run_queue, 0, sizeof(run_queue));
    current_proc = NULL;
    clock_ticks  = 0;
    printf("[sched] scheduler initialized\n");
}

/* ── run_queue_add ───────────────────────────────────────────
 * Insert process into run queue if not already present.
 */
void run_queue_add(proc_entry_t *p)
{
    if (!p) return;
    for (int i = 0; i < run_queue.rq_count; i++)
        if (run_queue.rq_procs[i] == p) return;

    if (run_queue.rq_count < NPROC) {
        run_queue.rq_procs[run_queue.rq_count++] = p;
        p->pe_state = SCHED_READY;
        printf("[sched] pid=%u added to run queue "
               "(pri=%d)\n", p->pe_pid,
               p->pe_sched.sp_priority);
    }
}

/* ── run_queue_remove ────────────────────────────────────────
 * Remove process from run queue.
 */
void run_queue_remove(proc_entry_t *p)
{
    if (!p) return;
    for (int i = 0; i < run_queue.rq_count; i++) {
        if (run_queue.rq_procs[i] == p) {
            /* Compact the array */
            for (int j = i; j < run_queue.rq_count - 1; j++)
                run_queue.rq_procs[j] = run_queue.rq_procs[j + 1];
            run_queue.rq_procs[--run_queue.rq_count] = NULL;
            printf("[sched] pid=%u removed from run queue\n",
                   p->pe_pid);
            return;
        }
    }
}

/* ── pick_highest_priority ───────────────────────────────────
 * Scan the run queue and return the process with the
 * numerically lowest priority value (= highest CPU priority)
 * that is currently loaded in memory.
 *
 * Round-robin multilevel feedback: among equal-priority
 * processes the one that has used least CPU recently wins.
 */
proc_entry_t *pick_highest_priority(void)
{
    proc_entry_t *best = NULL;

    for (int i = 0; i < run_queue.rq_count; i++) {
        proc_entry_t *p = run_queue.rq_procs[i];

        if (!p || !p->pe_in_memory)  continue;
        if (p->pe_state != SCHED_READY) continue;

        if (!best ||
            p->pe_sched.sp_priority < best->pe_sched.sp_priority ||
            (p->pe_sched.sp_priority == best->pe_sched.sp_priority &&
             p->pe_sched.sp_cpu_usage < best->pe_sched.sp_cpu_usage))
            best = p;
    }
    return best;
}

/* ── recalc_priority ─────────────────────────────────────────
 * Recalculate effective priority for one process.
 *
 * Classic UNIX formula (simplified):
 *   priority = PRIO_USER_BASE + (cpu_usage / 2) + nice
 *
 * Lower numeric value = higher scheduling priority.
 */
void recalc_priority(proc_entry_t *p)
{
    if (!p) return;
    int old = p->pe_sched.sp_priority;
    p->pe_sched.sp_priority =
        PRIO_USER_BASE +
        (p->pe_sched.sp_cpu_usage / 2) +
        p->pe_sched.sp_nice;

    /* Clamp to valid range */
    if (p->pe_sched.sp_priority < PRIO_KERNEL)
        p->pe_sched.sp_priority = PRIO_KERNEL;
    if (p->pe_sched.sp_priority > PRIO_IDLE)
        p->pe_sched.sp_priority = PRIO_IDLE;

    if (p->pe_sched.sp_priority != old)
        printf("[sched] pid=%u priority %d -> %d\n",
               p->pe_pid, old, p->pe_sched.sp_priority);
}

/* ── recalc_all_priorities ───────────────────────────────────
 * Decay CPU usage and recalculate priority for every ready
 * process in user mode.  Called once per second.
 */
void recalc_all_priorities(void)
{
    for (int i = 0; i < NPROC; i++) {
        proc_entry_t *p = &proc_table[i];
        if (p->pe_state == SCHED_UNUSED) continue;
        if (p->pe_state == SCHED_ZOMBIE) continue;

        /* Decay CPU usage (exponential moving average) */
        p->pe_sched.sp_cpu_usage =
            (p->pe_sched.sp_cpu_usage * 3) / 4;

        /* Only adjust priority for user-mode ready processes */
        if (p->pe_state == SCHED_READY ||
            p->pe_state == SCHED_RUNNING)
            recalc_priority(p);
    }
}

/* ── context_switch_to ───────────────────────────────────────
 * Perform a context switch to the chosen process.
 * In a real kernel this saves/restores hardware registers.
 */
void context_switch_to(proc_entry_t *next)
{
    if (!next) return;
    proc_entry_t *prev = current_proc;

    if (prev && prev->pe_state == SCHED_RUNNING)
        prev->pe_state = SCHED_READY;

    next->pe_state             = SCHED_RUNNING;
    next->pe_sched.sp_time_slice = TIME_QUANTUM;
    current_proc               = next;

    printf("[sched] context switch: "
           "pid=%u -> pid=%u\n",
           prev ? prev->pe_pid : 0,
           next->pe_pid);
}

/* ── fairshare_update ────────────────────────────────────────
 * Fair-share scheduling: distribute CPU budget equally among
 * groups, then proportionally within each group.
 * (simplified — real implementation uses group CPU counts)
 */
void fairshare_update(void)
{
    /* Count active processes per group */
    int group_count[NPROC] = {0};
    for (int i = 0; i < NPROC; i++) {
        proc_entry_t *p = &proc_table[i];
        if (p->pe_state != SCHED_UNUSED &&
            p->pe_state != SCHED_ZOMBIE &&
            p->pe_group_id >= 0)
            group_count[p->pe_group_id]++;
    }

    /* Adjust nice value based on group share */
    for (int i = 0; i < NPROC; i++) {
        proc_entry_t *p = &proc_table[i];
        if (p->pe_state == SCHED_UNUSED) continue;
        int gc = group_count[p->pe_group_id];
        if (gc > 1)
            p->pe_sched.sp_nice = gc - 1; /* penalise busy groups */
        recalc_priority(p);
    }
}

/* ─────────────────────────────────────────────────────────────
 * 1. Algorithm schedule_process
 *    input : none
 *    output: none
 *
 *    Round-robin multilevel feedback scheduler.
 *    Picks the highest-priority memory-resident process and
 *    switches context to it.  Idles the CPU if nothing is
 *    runnable.
 */
void schedule_process(void)
{
    proc_entry_t *chosen = NULL;

    /* Loop until a process is picked */
    while (!chosen) {

        /* For every process on run queue:
         * pick highest-priority process loaded in memory */
        chosen = pick_highest_priority();

        if (!chosen) {
            /* No process eligible — idle the CPU */
            printf("[sched] no runnable process, "
                   "CPU idle (ticks=%llu)\n",
                   (unsigned long long)clock_ticks);
            /*
             * In a real kernel the CPU executes a HLT or
             * WAIT instruction here; an interrupt will bring
             * it out of the idle state.
             */
            return;     /* simulation: return to caller       */
        }
    }

    /* Remove chosen process from run queue */
    run_queue_remove(chosen);

    /* Switch context to chosen process, resume its execution */
    context_switch_to(chosen);

    printf("[sched] scheduled pid=%u pri=%d cpu_use=%d\n",
           chosen->pe_pid,
           chosen->pe_sched.sp_priority,
           chosen->pe_sched.sp_cpu_usage);
}
