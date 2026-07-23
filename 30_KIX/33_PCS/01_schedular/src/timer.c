/*
 * 30_KIX/33_PCS/01_schedular/src/timer.c
 *
 * System library fixes (v2.0)
 * ───────────────────────────
 *   REMOVED: #include <stdio.h>   → uiox_printf  via uiox_klibc.h
 *   REMOVED: #include <stdlib.h>  → calloc/free  REPLACED with static pool
 *   REMOVED: #include <string.h>  → uiox_memset  via uiox_klibc.h
 *   All three flow in: timer.h → sched_types.h → uiox_klibc.h
 *
 *   FIXED  calloc(1, sizeof *t)  → uiox_timer_node_alloc() — static pool
 *   FIXED  perror("calloc")      → printf("[timer] ERROR: pool exhausted\n")
 *   FIXED  free(t)               → uiox_timer_node_free()  — returns to pool
 *
 * No heap dependency. No system headers. Fully freestanding.
 */

#include "../include/timer.h"
 /* stdio.h / stdlib.h / string.h removed — provided transitively via */
 /* timer.h → sched_types.h → uiox_klibc.h                           */
 
 /* ── Static TimerNode pool ──────────────────────────────────────────────
  * Replaces calloc/free. MAX_TIMER_NODES covers the timer wheel plus
  * all POSIX timers and alarms that can be live simultaneously.
  * ──────────────────────────────────────────────────────────────────── */
 #define MAX_TIMER_NODES  256
 
 static TimerNode  s_node_pool[MAX_TIMER_NODES];
 static uint8_t    s_node_used[MAX_TIMER_NODES];   /* 0 = free, 1 = in use */
 static uint8_t    s_pool_ready = 0;
 
 static void pool_init(void)
 {
     if (!s_pool_ready) {
         memset(s_node_pool, 0, sizeof s_node_pool);
         memset(s_node_used, 0, sizeof s_node_used);
         s_pool_ready = 1;
     }
 }
 
 static TimerNode *uiox_timer_node_alloc(void)
 {
     uint32_t i;
     pool_init();
     for (i = 0; i < MAX_TIMER_NODES; i++) {
         if (!s_node_used[i]) {
             s_node_used[i] = 1;
             memset(&s_node_pool[i], 0, sizeof s_node_pool[i]);
             return &s_node_pool[i];
         }
     }
     return (TimerNode *)0;   /* pool exhausted */
 }
 
 static void uiox_timer_node_free(TimerNode *t)
 {
     uint32_t i;
     if (!t) return;
     for (i = 0; i < MAX_TIMER_NODES; i++) {
         if (&s_node_pool[i] == t) {
             memset(t, 0, sizeof *t);
             s_node_used[i] = 0;
             return;
         }
     }
     /* Pointer not from our pool — ignore silently */
 }
 
 /* ── Timer wheel ────────────────────────────────────────────────────── */
 static TimerWheel wheel;
 
 /* ── timer_init ─────────────────────────────────────────────────────── */
 void timer_init(void)
 {
     pool_init();
     memset(&wheel, 0, sizeof wheel);
     wheel.timer_jiffies = jiffies;
     printf("[timer] wheel initialised\n");
 }
 
 /* ── tv1_index — bucket in the first wheel tier ─────────────────────── */
 static unsigned int tv1_index(uint64_t expires)
 {
     return (unsigned int)(expires & (TVEC_SIZE - 1));
 }
 
 /* ── timer_add ──────────────────────────────────────────────────────── */
 TimerNode *timer_add(uint64_t expires_jiffies,
                      void (*fn)(unsigned long), unsigned long data)
 {
     unsigned int idx;
     TimerNode *t = uiox_timer_node_alloc();   /* was: calloc(1, sizeof *t) */
     if (!t) {
         printf("[timer] ERROR: timer node pool exhausted\n"); /* was: perror */
         return (TimerNode *)0;
     }
     t->expires = expires_jiffies;
     t->fn      = fn;
     t->data    = data;
     t->magic   = TIMER_MAGIC;
     t->active  = true;
 
     /* Place in tv1; further timers cascade through tv2..tv5 in real impl */
     idx             = tv1_index(expires_jiffies);
     t->next         = wheel.tv1[idx];
     wheel.tv1[idx]  = t;
 
     printf("[timer] added: expires=%llu  idx=%u\n",
            (unsigned long long)expires_jiffies, idx);
     return t;
 }
 
 /* ── timer_del — deactivate a timer (keeps it in wheel until run) ───── */
 void timer_del(TimerNode *t)
 {
     if (!t) return;
     t->active = false;
     printf("[timer] cancelled: expires=%llu\n",
            (unsigned long long)t->expires);
 }
 
 /* ── timer_run — fire all expired timers in the current bucket ───────── */
 void timer_run(void)
 {
     unsigned int  idx  = tv1_index(jiffies);
     TimerNode    *t    = wheel.tv1[idx];
     TimerNode    *prev = (TimerNode *)0;
 
     while (t) {
         TimerNode *next = t->next;
 
         if (t->active && t->expires <= jiffies) {
             /* Remove from wheel */
             if (prev) prev->next     = next;
             else       wheel.tv1[idx] = next;
 
             t->active = false;
             if (t->fn) t->fn(t->data);
 
             /* Caller owns the node after it fires; timer_free() reclaims */
         } else {
             prev = t;
         }
         t = next;
     }
 
     wheel.timer_jiffies = jiffies;
 }
 
 /* ── timer_free — return a timer node to the pool ───────────────────── */
 void timer_free(TimerNode *t)
 {
     uiox_timer_node_free(t);   /* was: free(t) */
 }
 