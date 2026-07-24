/*
 * 30_KIX/33_PCS/02_MemMngnt/src/clock.c
 *
 * Freestanding fixes (v2.1)
 * ─────────────────────────
 *   REMOVED: #include <string.h>  #include <stdio.h>
 *            Both provided through clock.h → uiox_klibc.h
 *
 *   FIXED: fprintf(stderr, ...) → printf(...)
 *
 *   FIXED: gather_system_stats — ss_load_avg is double[3] (array).
 *          Replaced the broken integer EWMA with the correct expression
 *          from the original source:
 *            ss_load_avg[0] = ss_load_avg[0] * 0.9 + runnable * 0.1
 *          NOTE: this uses double arithmetic. The kernel build does NOT
 *          use -mgeneral-regs-only so the FPU is available on arm64.
 *          If a strict no-FPU build is needed, replace with integer
 *          fixed-point later.
 *
 *   FIXED: profile_kernel_tick / profile_user_tick — corrected field
 *          names to match kprof_t: kp_enabled, kp_base_pc, kp_pc_step,
 *          kp_counts, PROFILE_BUCKETS (not kp_active/kp_total/KPROF_BUCKETS).
 *
 *   FIXED: clock_tick → clock_interrupt  (correct function name per clock.h)
 *
 *   FIXED: HZ removed — not defined in clock.h or scheduler.h.
 *          Replaced with TIME_QUANTUM (defined in scheduler.h) as the
 *          per-second tick threshold.
 *
 * No algorithm changes beyond the above corrections.
 *
 * @version 2.1.0  @date 2026-07-23
 */

 #include "../include/clock.h"
 #include "../include/swapper.h"
 /* string.h / stdio.h removed — provided transitively via
    clock.h → scheduler.h → uiox_klibc.h                    */
 
 /* ── Globals ─────────────────────────────────────────────── */
 callout_t   callout_table[MAX_CALLOUT];
 sys_stats_t sys_stats;
 kprof_t     kernel_prof;
 kprof_t     user_prof;
 static uint64_t ticks_this_second = 0;
 
 /* ── clock_init ──────────────────────────────────────────── */
 void clock_init(void)
 {
     memset(callout_table, 0, sizeof(callout_table));
     memset(&sys_stats,   0, sizeof(sys_stats));
     memset(&kernel_prof, 0, sizeof(kernel_prof));
     memset(&user_prof,   0, sizeof(user_prof));
     ticks_this_second = 0;
     printf("[clock] clock subsystem initialized\n");
 }
 
 /* ── callout_add ─────────────────────────────────────────── */
 int callout_add(int delta_ticks, void (*fn)(void *), void *arg)
 {
     int i;
     for (i = 0; i < MAX_CALLOUT; i++) {
         if (!callout_table[i].co_active) {
             callout_table[i].co_active = 1;
             callout_table[i].co_delta  = delta_ticks;
             callout_table[i].co_func   = fn;
             callout_table[i].co_arg    = arg;
             printf("[clock] callout added slot=%d delta=%d\n",
                    i, delta_ticks);
             return i;
         }
     }
     printf("[clock] ERROR: callout table full\n"); /* was: fprintf(stderr,...) */
     return -1;
 }
 
 /* ── callout_del ─────────────────────────────────────────── */
 void callout_del(int index)
 {
     if (index < 0 || index >= MAX_CALLOUT) return;
     callout_table[index].co_active = 0;
     printf("[clock] callout deleted slot=%d\n", index);
 }
 
 /* ── callout_tick ────────────────────────────────────────── */
 void callout_tick(void)
 {
     int i;
     for (i = 0; i < MAX_CALLOUT; i++) {
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
 
 /* ── gather_system_stats ─────────────────────────────────── */
 void gather_system_stats(void)
 {
     int i, runnable = 0;
     sys_stats.ss_total_ticks++;
 
     for (i = 0; i < NPROC; i++) {
         proc_entry_t *p = &proc_table[i];
         if (p->pe_state == SCHED_READY ||
             p->pe_state == SCHED_RUNNING)
             runnable++;
     }
     sys_stats.ss_runnable = runnable;
 
     /* Exponential moving average for 1-min load.
      * ss_load_avg is double[3] — use floating-point as per the header.
      * α = 0.1 (simplified; real kernel uses exp(-1/60) ≈ 0.9835)     */
     sys_stats.ss_load_avg[0] =
         sys_stats.ss_load_avg[0] * 0.9 + runnable * 0.1;
 }
 
 /* ── gather_per_process_stats ────────────────────────────── */
 void gather_per_process_stats(proc_entry_t *p)
 {
     if (!p) return;
     p->pe_sched.sp_cpu_usage++;
     p->pe_tms.tms_utime++;       /* simplified: all user time */
     p->pe_sched.sp_residence++;
 }
 
 /* ── adjust_cpu_utilization ──────────────────────────────── */
 void adjust_cpu_utilization(proc_entry_t *p)
 {
     if (!p) return;
     /* UNIX decay: usage = (usage * 2) / 3 each second */
     p->pe_sched.sp_cpu_usage =
         (p->pe_sched.sp_cpu_usage * 2) / 3;
 }
 
 /* ── profile_kernel_tick ─────────────────────────────────── */
 void profile_kernel_tick(uintptr_t pc)
 {
     size_t bucket;
     /* kp_enabled, kp_base_pc, kp_pc_step, kp_counts are the
        actual field names in kprof_t (see clock.h)            */
     if (!kernel_prof.kp_enabled) return;
     if (kernel_prof.kp_pc_step == 0) return;
     if (pc >= kernel_prof.kp_base_pc) {
         uintptr_t offset = pc - kernel_prof.kp_base_pc;
         bucket = (size_t)(offset / kernel_prof.kp_pc_step);
         if (bucket < PROFILE_BUCKETS)
             kernel_prof.kp_counts[bucket]++;
     }
 }
 
 /* ── profile_user_tick ───────────────────────────────────── */
 void profile_user_tick(uintptr_t pc)
 {
     size_t bucket;
     if (!user_prof.kp_enabled) return;
     if (user_prof.kp_pc_step == 0) return;
     if (pc >= user_prof.kp_base_pc) {
         uintptr_t offset = pc - user_prof.kp_base_pc;
         bucket = (size_t)(offset / user_prof.kp_pc_step);
         if (bucket < PROFILE_BUCKETS)
             user_prof.kp_counts[bucket]++;
     }
 }
 
 /* ── clock_interrupt ─────────────────────────────────────── */
 /* Named clock_interrupt() — matches the declaration in clock.h */
 void clock_interrupt(uintptr_t kernel_pc, uintptr_t user_pc)
 {
     int i;
 
     clock_ticks++;
     ticks_this_second++;
 
     /* Adjust callout table */
     callout_tick();
 
     /* Kernel and user profiling */
     profile_kernel_tick(kernel_pc);
     profile_user_tick(user_pc);
 
     /* Gather system-wide statistics */
     gather_system_stats();
 
     /* Gather per-process statistics */
     if (current_proc)
         gather_per_process_stats(current_proc);
 
     /* Adjust CPU utilisation of running process */
     if (current_proc)
         adjust_cpu_utilization(current_proc);
 
     /* Check time-slice expiry */
     if (current_proc) {
         if (current_proc->pe_sched.sp_time_slice > 0)
             current_proc->pe_sched.sp_time_slice--;
         if (current_proc->pe_sched.sp_time_slice == 0) {
             need_resched = 1;
             current_proc->pe_sched.sp_time_slice = TIME_QUANTUM;
         }
     }
 
     /* Once per second (TIME_QUANTUM ticks): recalc priorities + swapper */
     if (ticks_this_second >= (uint64_t)TIME_QUANTUM) {
         ticks_this_second = 0;
         for (i = 0; i < NPROC; i++) {
             proc_entry_t *p = &proc_table[i];
             if (p->pe_state == SCHED_UNUSED ||
                 p->pe_state == SCHED_ZOMBIE) continue;
             adjust_cpu_utilization(p);
             if (p->pe_state == SCHED_READY ||
                 p->pe_state == SCHED_RUNNING)
                 recalc_priority(p);
         }
         wakeup_swapper();
     }
 }
 