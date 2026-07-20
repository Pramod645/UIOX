#include <stdio.h>
#include "sched_types.h"
#include "scheduler.h"
#include "clock.h"
#include "timekeeping.h"
#include "timer.h"
#include "profiler.h"
#include "syscall_time.h"

/* ─────────────────────────────────────────────────────────────
 * Helpers
 * ───────────────────────────────────────────────────────────── */
static void banner(const char *s)
{
    printf("\n══════════════════════════════════════════\n");
    printf("  %s\n", s);
    printf("══════════════════════════════════════════\n");
}

static Process mkproc(int pid, int spri, int nice, int gid)
{
    Process p      = {0};
    p.pid          = pid;
    p.state        = TASK_RUNNING;
    p.policy       = SCHED_NORMAL;
    p.static_priority  = spri;
    p.dynamic_priority = spri;
    p.nice         = nice;
    p.time_slice   = TIME_QUANTUM;
    p.in_memory    = true;
    p.group_id     = gid;
    p.cpu_usage    = 0;
    return p;
}

/* Callout test callback */
static void callout_cb(void *arg)
{
    printf("  [callout_cb] fired! arg=%p\n", arg);
}

/* Dynamic timer callback */
static void timer_cb(unsigned long data)
{
    printf("  [timer_cb] fired! data=%lu  jiffies=%llu\n",
           data, (unsigned long long)jiffies);
}

/* ─────────────────────────────────────────────────────────────
 * main
 * ───────────────────────────────────────────────────────────── */
int main(void)
{
    /* ── 1. Timekeeping init ─────────────────────────────────── */
    banner("Timekeeping Initialisation");
    timekeeping_init();
    timer_init();
    clock_init();
    profiler_init(/*kernel=*/true, /*user=*/true);
    nmi_watchdog_enable(5 * HZ);  /* watchdog: 5-second freeze threshold */

    /* ── 2. Scheduler init ──────────────────────────────────── */
    banner("Scheduler Init");
    scheduler_init();

    Process p1 = mkproc(1, 20, -5,  1);
    Process p2 = mkproc(2, 60,  0,  1);
    Process p3 = mkproc(3, 40, +5,  2);
    Process p4 = mkproc(4, 10, -10, 2);

    enqueue_process(&p1);
    enqueue_process(&p2);
    enqueue_process(&p3);
    enqueue_process(&p4);
    scheduler_print();

    /* ── 3. Algorithm schedule_process ──────────────────────── */
    banner("Algorithm: schedule_process");
    Process *running = schedule_process();
    if (running)
        printf("Running: pid=%d  prio=%d\n",
               running->pid, running->dynamic_priority);

    /* ── 4. Tick simulation + Algorithm clock ────────────────── */
    banner("Algorithm: clock (15 ticks)");

    /* Register a callout to fire in 5 ticks */
    callout_add(5, callout_cb, (void*)0xCAFE);

    /* Register a dynamic timer to fire at jiffies+8 */
    TimerNode *dyn = timer_add(jiffies + 8, timer_cb, 42UL);

    for (int tick = 0; tick < 15; tick++) {
        printf("\n── tick %d  jiffies=%llu ──\n",
               tick, (unsigned long long)jiffies);

        /* Simulate one clock interrupt */
        clock_tick();
        timer_run();
        update_times();

        /* Tick the running process; preempt if slice expires */
        if (running && tick_process(running)) {
            running = schedule_process();
            if (running)
                printf("  Preempted → new running: pid=%d\n", running->pid);
        }

        nmi_watchdog_check();
    }

    /* ── 5. Per-second effects ───────────────────────────────── */
    banner("Per-second priority readjust");
    readjust_all_priorities();

    /* ── 6. Fair-share scheduling ────────────────────────────── */
    banner("Fair-share adjust (group 1)");
    p1.total_ticks = 80;
    p2.total_ticks = 20;
    enqueue_process(&p2);
    fair_share_adjust(1);
    scheduler_print();

    /* ── 7. Profiler report ──────────────────────────────────── */
    banner("Profiler Report");
    profiler_report(3);

    /* ── 8. System calls ─────────────────────────────────────── */
    banner("System Calls — time / gettimeofday");
    sys_time();
    TimeVal tv;
    sys_gettimeofday(&tv);

    banner("System Calls — alarm / setitimer");
    sys_alarm(&p1, 3);
    ItimerVal itv = { {2, 0}, {5, 0} };
    sys_setitimer(&p1, &itv, NULL);

    banner("System Calls — adjtimex");
    sys_adjtimex(1, 500000);

    banner("System Calls — POSIX clocks");
    XTime xt_out, xt_res;
    sys_clock_gettime(UIOX_CLOCK_REALTIME,  &xt_out);
    sys_clock_gettime(UIOX_CLOCK_MONOTONIC, &xt_out);
    sys_clock_getres(UIOX_CLOCK_REALTIME,   &xt_res);

    banner("System Calls — POSIX timers");
    PosixTimer *pt = NULL;
    sys_timer_create(UIOX_CLOCK_REALTIME, &pt);
    ItimerVal pt_spec = { {1, 0}, {2, 0} };
    sys_timer_settime(pt, &pt_spec);
    ItimerVal pt_cur;
    sys_timer_gettime(pt, &pt_cur);

    /* Run a few more ticks so the POSIX timer fires */
    printf("\n── 10 more ticks for POSIX timer ──\n");
    for (int i = 0; i < 10; i++) {
        jiffies++; jiffies_64++;
        timer_run();
    }

    sys_timer_getoverrun(pt);
    sys_timer_delete(pt);

    banner("sys_clock_nanosleep");
    sys_clock_nanosleep(UIOX_CLOCK_MONOTONIC, 500000);

    /* Cleanup */
    if (dyn) timer_free(dyn);

    return 0;
}
