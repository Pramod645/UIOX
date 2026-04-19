#include "../include/clock.h"
#include "../include/swapper.h"
#include <string.h>
#include <stdio.h>

/* ── Globals ────────────────────────────────────────────────── */
callout_t  callout_table[MAX_CALLOUT];
sys_stats_t sys_stats;
kprof_t    kernel_prof;
kprof_t    user_prof;

static uint64_t ticks_this_second = 0;

/* ── clock_init ──────────────────────────────────────────────
 * Zero all clock-related data structures.
 */
void clock_init(void)
{
    memset(callout_table, 0, sizeof(callout_table));
    memset(&sys_stats,    0, sizeof(sys_stats));
    memset(&kernel_prof,  0, sizeof(kernel_prof));
    memset(&user_prof,    0, sizeof(user_prof));
    ticks_this_second = 0;
    printf("[clock] clock subsystem initialized\n");
}

/* ── callout_add ─────────────────────────────────────────────
 * Register a function to be called after delta_ticks ticks.
 * Returns index in callout table, or -1 on failure.
 */
int callout_add(int delta_ticks, void (*fn)(void *), void *arg)
{
    for (int i = 0; i < MAX_CALLOUT; i++) {
        if (!callout_table[i].co_active) {
            callout_table[i].co_active  = 1;
            callout_table[i].co_delta   = delta_ticks;
            callout_table[i].co_func    = fn;
            callout_table[i].co_arg     = arg;
            printf("[clock] callout added slot=%d "
                   "delta=%d\n", i, delta_ticks);
            return i;
        }
    }
    fprintf(stderr, "[clock] callout table full\n");
    return -1;
}

/* ── callout_del ─────────────────────────────────────────────
 * Remove a callout entry by index.
 */
void callout_del(int index)
{
    if (index < 0 || index >= MAX_CALLOUT) return;
    callout_table[index].co_active = 0;
    printf("[clock] callout deleted slot=%d\n", index);
}

/* ── callout_tick ────────────────────────────────────────────
 * Decrement all callout timers; fire any that reach zero.
 */
void callout_tick(void)
{
    for (int i = 0; i < MAX_CALLOUT; i++) {
        callout_t *c = &callout_table[i];
        if (!c->co_active) continue;

        c->co_delta--;
        if (c->co_delta <= 0) {
            printf("[clock] callout firing slot=%d\n", i);
            c->co_active = 0;
            if (c->co_func) c->co_func(c->co_arg);
        }
    }
}

/* ── gather_system_stats ─────────────────────────────────────
 * Accumulate global tick counters and compute load average.
 */
void gather_system_stats(void)
{
    sys_stats.ss_total_ticks++;

    /* Count runnable processes */
    int runnable = 0;
    for (int i = 0; i < NPROC; i++) {
        proc_entry_t *p = &proc_table[i];
        if (p->pe_state == SCHED_READY  ||
            p->pe_state == SCHED_RUNNING)
            runnable++;
    }
    sys_stats.ss_runnable = runnable;

    /* Exponential moving average for 1-min load */
    /* load_avg[0] = load_avg[0] * (1-α) + runnable * α
     * α = 1 / (60 * HZ) but we simplify:               */
    sys_stats.ss_load_avg[0] =
        sys_stats.ss_load_avg[0] * 0.9 + runnable * 0.1;
}

/* ── gather_per_process_stats ────────────────────────────────
 * Charge one tick of CPU time to the current process.
 */
void gather_per_process_stats(proc_entry_t *p)
{
    if (!p) return;
    p->pe_sched.sp_cpu_usage++;
    p->pe_tms.tms_utime++;          /* simplified: all user time */
    p->pe_sched.sp_residence++;
}

/* ── adjust_cpu_utilization ──────────────────────────────────
 * Per-tick decay of CPU usage counter (used in priority calc).
 */
void adjust_cpu_utilization(proc_entry_t *p)
{
    if (!p) return;
    /* UNIX decay: usage = (usage * 2) / 3   each second */
    p->pe_sched.sp_cpu_usage =
        (p->pe_sched.sp_cpu_usage * 2) / 3;
}

/* ── profile_kernel_tick ─────────────────────────────────────
 * Record one hit in the kernel profiling histogram
 * at the given program counter.
 */
void profile_kernel_tick(uintptr_t pc)
{
    if (!kernel_prof.kp_enabled) return;
    if (kernel_prof.kp_pc_step == 0) return;

    if (pc >= kernel_prof.kp_base_pc) {
        uintptr_t offset  = pc - kernel_prof.kp_base_pc;
        size_t    bucket  = offset / kernel_prof.kp_pc_step;
        if (bucket < PROFILE_BUCKETS)
            kernel_prof.kp_counts[bucket]++;
    }
}

/* ── profile_user_tick ───────────────────────────────────────
 * Record one hit in the user profiling histogram.
 */
void profile_user_tick(uintptr_t pc)
{
    if (!user_prof.kp_enabled) return;
    if (user_prof.kp_pc_step == 0) return;

    if (pc >= user_prof.kp_base_pc) {
        uintptr_t offset = pc - user_prof.kp_base_pc;
        size_t    bucket = offset / user_prof.kp_pc_step;
        if (bucket < PROFILE_BUCKETS)
            user_prof.kp_counts[bucket]++;
    }
}

/* ─────────────────────────────────────────────────────────────
 * 2. Algorithm clock
 *    input : kernel_pc — program counter at time of interrupt
 *            user_pc   — user-mode PC if applicable
 *    output: none
 *
 *    Called on every hardware clock interrupt (every tick).
 */
void clock_interrupt(uintptr_t kernel_pc, uintptr_t user_pc)
{
    /* ── Restart the clock (re-arm hardware timer) ──────────── */
    /* In a real kernel: reprogram PIT/APIC.  Simulated here.   */
    clock_ticks++;
    ticks_this_second++;

    /* ── Adjust callout table ───────────────────────────────── */
    callout_tick();

    /* ── Kernel and user profiling ──────────────────────────── */
    profile_kernel_tick(kernel_pc);
    profile_user_tick(user_pc);

    /* ── Gather system-wide statistics ─────────────────────── */
    gather_system_stats();

    /* ── Gather per-process statistics ─────────────────────── */
    if (current_proc)
        gather_per_process_stats(current_proc);

    /* ── Adjust CPU utilization of running process ──────────── */
    if (current_proc)
        adjust_cpu_utilization(current_proc);

    /* ── Check time-slice expiry ────────────────────────────── */
    if (current_proc) {
        if (current_proc->pe_sched.sp_time_slice > 0)
            current_proc->pe_sched.sp_time_slice--;

        if (current_proc->pe_sched.sp_time_slice == 0) {
            /* Time quantum expired — request reschedule */
            need_resched = 1;
            printf("[clock] pid=%u quantum expired\n",
                   current_proc->pe_pid);
        }
    }

    /* ── Per-second processing ──────────────────────────────── */
    if (ticks_this_second >= (uint64_t)(1000000000UL / 1000000UL)) {
        ticks_this_second = 0;

        for (int i = 0; i < NPROC; i++) {
            proc_entry_t *p = &proc_table[i];
            if (p->pe_state == SCHED_UNUSED) continue;

            /* Adjust alarm countdown */
            if (p->pe_alarm > 0) {
                p->pe_alarm--;
                if (p->pe_alarm == 0) {
                    printf("[clock] SIGALRM for pid=%u\n",
                           p->pe_pid);
                    /* send_signal(p, SIGALRM); */
                }
            }

            /* Adjust CPU utilization measure */
            adjust_cpu_utilization(p);

            /* Adjust priority for user-mode processes */
            if (p->pe_state == SCHED_READY  ||
                p->pe_state == SCHED_RUNNING)
                recalc_priority(p);
        }

        /* Wake swapper if memory pressure requires it */
        wakeup_swapper();
    }
}
