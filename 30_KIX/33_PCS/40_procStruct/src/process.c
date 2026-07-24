/*
 * 30_KIX/33_PCS/40_procStruct/src/process.c
 *
 * Freestanding fixes (v2.0)
 * ─────────────────────────
 *   REMOVED: implicit <stdio.h> dependency
 *            All provided through proc_algo.h → uiox_klibc.h
 *
 *   FIXED: fprintf(stderr, "[proc_alloc] process table full\n")
 *       → printf("[proc_alloc] ERROR: process table full\n")
 *
 *   FIXED: fprintf(stderr, "[proc_set_state] invalid transition...")
 *       → printf("[proc_set_state] ERROR: invalid transition...")
 *
 *   FIXED: NULL → (proc_t *)0  (no stdlib.h in freestanding)
 *
 *   FIXED: for (int i...) → int i; for (i...) — strict C11 freestanding
 *
 * No algorithm changes.
 *
 * @version 2.0.0  @date 2026-07-24
 */

 #include "../include/process.h"
 #include "../include/proc_algo.h"
 /* stdio.h removed — provided transitively via proc_algo.h → uiox_klibc.h */
 
 /* ── Globals ─────────────────────────────────────────────────── */
 proc_t  proc_table[NPROC];
 proc_t *current_proc = (proc_t *)0;
 
 /* ── proc_alloc ──────────────────────────────────────────────── */
 proc_t *proc_alloc(uint32_t pid, uint32_t ppid)
 {
     int i;
     for (i = 0; i < NPROC; i++) {
         if (proc_table[i].p_state == PROC_UNUSED) {
             memset(&proc_table[i], 0, sizeof(proc_t));
             proc_table[i].p_state = PROC_CREATED;
             proc_table[i].p_pid   = pid  ? pid  : (uint32_t)(i + 1);
             proc_table[i].p_ppid  = ppid;
             printf("[proc_alloc] allocated pid=%u\n",
                    proc_table[i].p_pid);
             return &proc_table[i];
         }
     }
     printf("[proc_alloc] ERROR: process table full\n"); /* was: fprintf(stderr,...) */
     return (proc_t *)0;
 }
 
 /* ── proc_free ───────────────────────────────────────────────── */
 void proc_free(proc_t *p)
 {
     if (!p) return;
     printf("[proc_free] freeing pid=%u\n", p->p_pid);
     memset(p, 0, sizeof(proc_t));
     p->p_state = PROC_UNUSED;
 }
 
 /* ── proc_find ───────────────────────────────────────────────── */
 proc_t *proc_find(uint32_t pid)
 {
     int i;
     for (i = 0; i < NPROC; i++) {
         if (proc_table[i].p_state != PROC_UNUSED &&
             proc_table[i].p_pid   == pid)
             return &proc_table[i];
     }
     return (proc_t *)0;
 }
 
 /* ── proc_set_state ──────────────────────────────────────────── */
 int proc_set_state(proc_t *p, proc_state_t new_state)
 {
     int valid = 0;
     proc_state_t old;
 
     if (!p) return -1;
     old = p->p_state;
 
     switch (old) {
     case PROC_USER_RUNNING:
         valid = (new_state == PROC_KERNEL_RUNNING ||
                  new_state == PROC_PREEMPTED);
         break;
     case PROC_KERNEL_RUNNING:
         valid = (new_state == PROC_USER_RUNNING   ||
                  new_state == PROC_PREEMPTED       ||
                  new_state == PROC_SLEEP_MEM       ||
                  new_state == PROC_READY           ||
                  new_state == PROC_ZOMBIE);
         break;
     case PROC_READY:
         valid = (new_state == PROC_KERNEL_RUNNING ||
                  new_state == PROC_READY_SWAPPED);
         break;
     case PROC_SLEEP_MEM:
         valid = (new_state == PROC_READY          ||
                  new_state == PROC_SLEEP_SWAPPED);
         break;
     case PROC_READY_SWAPPED:
         valid = (new_state == PROC_READY          ||
                  new_state == PROC_READY_SWAPPED);
         break;
     case PROC_SLEEP_SWAPPED:
         valid = (new_state == PROC_SLEEP_MEM      ||
                  new_state == PROC_READY_SWAPPED);
         break;
     case PROC_PREEMPTED:
         valid = (new_state == PROC_USER_RUNNING   ||
                  new_state == PROC_READY);
         break;
     case PROC_CREATED:
         valid = (new_state == PROC_READY          ||
                  new_state == PROC_READY_SWAPPED);
         break;
     case PROC_ZOMBIE:
         valid = 0;   /* terminal state */
         break;
     default:
         valid = 1;
         break;
     }
 
     if (!valid) {
         printf("[proc_set_state] ERROR: invalid transition %d -> %d for pid=%u\n",
                old, new_state, p->p_pid); /* was: fprintf(stderr,...) */
         return -1;
     }
 
     printf("[proc_set_state] pid=%u  state %d -> %d\n",
            p->p_pid, old, new_state);
     p->p_state = new_state;
 
     /* Update P_SWAPPED flag */
     if (new_state == PROC_READY_SWAPPED ||
         new_state == PROC_SLEEP_SWAPPED)
         p->p_flag |=  P_SWAPPED;
     else
         p->p_flag &= ~P_SWAPPED;
 
     return 0;
 }
 
 /* ── proc_signal_pending ─────────────────────────────────────── */
 int proc_signal_pending(proc_t *p)
 {
     if (!p) return 0;
     return (int)(p->p_sig & ~p->p_sigmask);
 }
 
 /* ── sched_enqueue ───────────────────────────────────────────── */
 void sched_enqueue(proc_t *p)
 {
     proc_t *cur, *prev;
     if (!p) return;
 
     /* Insert in priority order (lower p_pri = higher priority) */
     cur  = current_proc;
     prev = (proc_t *)0;
 
     while (cur && cur->p_sched.p_pri <= p->p_sched.p_pri) {
         prev = cur;
         cur  = cur->p_next;
     }
 
     p->p_next = cur;
     p->p_prev = prev;
     if (prev) prev->p_next = p;
     else      current_proc = p;
     if (cur)  cur->p_prev  = p;
 
     printf("[sched_enqueue] pid=%u  pri=%d\n",
            p->p_pid, p->p_sched.p_pri);
 }
 
 /* ── sched_dequeue ───────────────────────────────────────────── */
 void sched_dequeue(proc_t *p)
 {
     if (!p) return;
 
     if (p->p_prev) p->p_prev->p_next = p->p_next;
     else           current_proc       = p->p_next;
     if (p->p_next) p->p_next->p_prev = p->p_prev;
 
     p->p_next = (proc_t *)0;
     p->p_prev = (proc_t *)0;
 
     printf("[sched_dequeue] pid=%u\n", p->p_pid);
 }
 