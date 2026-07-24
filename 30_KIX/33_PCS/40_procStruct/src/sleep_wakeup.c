/*
 * 30_KIX/33_PCS/40_procStruct/src/sleep_wakeup.c
 *
 * Freestanding fixes (v2.0)
 * ─────────────────────────
 *   REMOVED: #include <string.h>  <stdio.h>  <setjmp.h>
 *            All provided through proc_algo.h → uiox_klibc.h
 *
 *   FIXED: sched_pick() undeclared
 *       → extern proc_t *sched_pick(void) forward declaration
 *         (implemented in 02_MemMngnt/src/scheduler.c)
 *
 *   FIXED: longjmp(u.u_qsave, 1)  — requires <setjmp.h>
 *       → uiox_jmp_buf_t abort flag:
 *           u.u_qsave.regs[0] = 1;  (mark abort)
 *           u.u_error = EINTR;
 *           return -1;
 *         The caller checks u_error; full hardware setjmp/longjmp
 *         is the arch layer's responsibility.
 *
 * No algorithm changes — sleep/wakeup logic identical to original.
 *
 * @version 2.0.0  @date 2026-07-24
 */

 #include "../include/proc_algo.h"
 #include "../include/process.h"
 /* string.h / stdio.h / setjmp.h removed — provided transitively via
    proc_algo.h → uiox_klibc.h                                        */
 
 /* ── Error codes ─────────────────────────────────────────────── */
 #ifndef EINTR
 #define EINTR   4    /* interrupted system call               */
 #endif
 
 /* ── Forward declaration for scheduler pick-next function ───────
  * Implemented in 02_MemMngnt/src/scheduler.c.
  * Cannot be in proc_algo.h without creating a circular include.    */
 extern proc_t *sched_pick(void);
 
 /* ── Processor-level simulation stubs ───────────────────────────
  * In a real kernel these mask/unmask hardware interrupts.
  * Simulated here as no-ops — the real arch layer provides them.    */
 static inline int  splhigh(void) { return 0; }
 static inline int  splx(int lvl) { (void)lvl; return 0; }
 
 /* ── Globals ─────────────────────────────────────────────────── */
 sleep_queue_t sleep_hash[SLEEP_HASH_SZ];
 int           scheduler_flag = 0;
 int           proc_level     = 0;
 
 /* ── sleep_hash_slot — hash wchan to a queue slot ────────────── */
 static int sleep_hash_slot(uintptr_t wchan)
 {
     return (int)((wchan >> 2) % (uintptr_t)SLEEP_HASH_SZ);
 }
 
 /* ── sleep_enqueue — put process on sleep hash queue ─────────── */
 static void sleep_enqueue(proc_t *p, uintptr_t wchan)
 {
     int slot = sleep_hash_slot(wchan);
     p->p_wchan = wchan;
     p->p_next  = sleep_hash[slot].sq_head;
     p->p_prev  = (proc_t *)0;
     if (sleep_hash[slot].sq_head)
         sleep_hash[slot].sq_head->p_prev = p;
     else
         sleep_hash[slot].sq_tail = p;
     sleep_hash[slot].sq_head = p;
 }
 
 /* ── sleep_dequeue — remove process from sleep hash queue ───────*/
 static void sleep_dequeue(proc_t *p, uintptr_t wchan)
 {
     int slot = sleep_hash_slot(wchan);
     if (p->p_prev) p->p_prev->p_next          = p->p_next;
     else           sleep_hash[slot].sq_head    = p->p_next;
     if (p->p_next) p->p_next->p_prev          = p->p_prev;
     else           sleep_hash[slot].sq_tail    = p->p_prev;
     p->p_next  = (proc_t *)0;
     p->p_prev  = (proc_t *)0;
     p->p_wchan = 0;
 }
 
 /* ─────────────────────────────────────────────────────────────
  * Algorithm proc_sleep  (§10)
  *
  * input : wchan         — event address to sleep on
  *         priority      — scheduling priority while sleeping
  *         interruptible — 1 = wake on signal, 0 = uninterruptible
  *
  * output:
  *   0  — woken normally
  *   1  — woken by a caught signal (priority > PZERO)
  *  -1  — woken by uncaught signal (sets u.u_error = EINTR,
  *         marks u_qsave abort flag — caller must unwind)
  * ────────────────────────────────────────────────────────────── */
 int proc_sleep(uintptr_t wchan, int priority, int interruptible)
 {
     proc_t *p = current_proc;
     int     old_level;
 
     if (!p) return 0;
 
     /* Raise processor execution level to block interrupts */
     old_level = splhigh();
 
     /* ── Uninterruptible sleep (priority <= PZERO) ─────────── */
     if (!interruptible || priority <= PZERO) {
 
         /* Set process state to sleeping */
         proc_set_state(p, PROC_SLEEP_MEM);
 
         /* Enqueue on sleep hash */
         sleep_enqueue(p, wchan);
 
         printf("[sleep] pid=%u sleeping uninterruptibly on wchan=0x%lx\n",
                p->p_pid, (unsigned long)wchan);
 
         /* Context switch — process resumes here on wakeup */
         {
             proc_t *next = sched_pick();    /* was: undeclared */
             if (next) {
                 proc_set_state(next, PROC_KERNEL_RUNNING);
                 /* context_switch_proc(p, next); */
             }
         }
 
         splx(old_level);
         return 0;
     }
 
     /* ── Interruptible sleep (priority > PZERO) ────────────── */
 
     if (!proc_signal_pending(p)) {
         /* No signal pending — do context switch */
         proc_set_state(p, PROC_SLEEP_MEM);
         sleep_enqueue(p, wchan);
 
         printf("[sleep] pid=%u sleeping interruptibly on wchan=0x%lx\n",
                p->p_pid, (unsigned long)wchan);
 
         {
             proc_t *next = sched_pick();    /* was: undeclared */
             if (next) {
                 proc_set_state(next, PROC_KERNEL_RUNNING);
                 /* context_switch_proc(p, next); */
             }
         }
     }
 
     splx(old_level);
 
     /* Re-check for pending signal after wakeup */
     if (!proc_signal_pending(p))
         return 0;
 
     /* ── Woken by signal ────────────────────────────────────── */
 
     /* If priority set to catch signals, return 1 */
     if (priority > PZERO) {
         printf("[sleep] pid=%u woke due to caught signal\n", p->p_pid);
         return 1;
     }
 
     /* Uncaught signal — abort system call.
      * In a real kernel: longjmp(u.u_qsave, 1) unwinds to the
      * kernel entry point. Here we use the uiox_jmp_buf_t abort
      * flag and set u_error; the arch syscall entry checks this. */
     printf("[sleep] pid=%u woke due to uncaught signal, aborting syscall\n",
            p->p_pid);
 
     extern u_area_t u;
     u.u_qsave.regs[0] = 1;   /* was: longjmp(u.u_qsave, 1) */
     u.u_error         = EINTR;
     return -1;
 }
 
 /* ─────────────────────────────────────────────────────────────
  * Algorithm proc_wakeup  (§11)
  *
  * input : wchan — sleep address to wake all processes on
  * ────────────────────────────────────────────────────────────── */
 void proc_wakeup(uintptr_t wchan)
 {
     int     slot      = sleep_hash_slot(wchan);
     int     old_level = splhigh();
     proc_t *p         = sleep_hash[slot].sq_head;
     proc_t *next_p;
 
     while (p) {
         next_p = p->p_next;
 
         if (p->p_wchan == wchan) {
             sleep_dequeue(p, wchan);
 
             /* Transition to ready state */
             if (p->p_state == PROC_SLEEP_MEM)
                 proc_set_state(p, PROC_READY);
             else if (p->p_state == PROC_SLEEP_SWAPPED)
                 proc_set_state(p, PROC_READY_SWAPPED);
 
             /* Put back on run queue if in memory */
             if (p->p_state == PROC_READY)
                 sched_enqueue(p);
 
             /* Set reschedule flag if higher priority than current */
             if (current_proc &&
                 p->p_sched.p_pri < current_proc->p_sched.p_pri)
                 scheduler_flag = 1;
 
             printf("[wakeup] pid=%u woken from wchan=0x%lx\n",
                    p->p_pid, (unsigned long)wchan);
         }
 
         p = next_p;
     }
 
     splx(old_level);
 }
 