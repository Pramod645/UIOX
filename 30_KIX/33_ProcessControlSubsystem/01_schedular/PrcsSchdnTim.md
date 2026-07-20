| File | Concept |
| --- | --- |
| sched_types.h | All shared types: TaskState, Process, ProcTimes, XTime, jiffies, constants |
| scheduler.h/c | Algorithm 1 — schedule_process, multilevel feedback run queue, priority recalc, fair-share, tick/preempt |
| clock.h/c | Algorithm 2 — clock_tick, callout table, per-second work (alarm adjust, priority adjust, load avg, swapper wakeup) |
| timekeeping.h/c | Timer source selection (timer_opts), time_init, get_cmos_time, update_times, TSC calibration, udelay/ndelay |
| timer.h/c | Dynamic timers — timer wheel (tvec_base_t), timer_add/timer_del/timer_run |
| profiler.h/c | Algorithm 3 — profile_tick, PC histogram, hot-spot report, NMI watchdog |
| syscall_time.h/c | All timing system calls: sys_time, sys_gettimeofday, sys_adjtimex, sys_alarm, sys_setitimer, full POSIX timer/clock API |