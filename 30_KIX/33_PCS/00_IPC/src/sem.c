#include "sem.h"
#include "../include/uiox_klibc.h"
/*
 * 30_KIX/33_PCS/00_IPC/src/sem.c
 *
 * Freestanding fixes (v2.0)
 * ─────────────────────────
 *   REMOVED: #include <stdio.h>  <string.h>  <stdlib.h>
 *            All provided through sem.h → ipc_types.h → uiox_klibc.h
 *
 *   FIXED: fprintf(stderr, ...) → printf(...)   (9 occurrences)
 *   FIXED: time(NULL)           → jiffies        (3 occurrences)
 *
 * No algorithm changes — all semget/semctl/semop logic unchanged.
 *
 * @version 2.0.0  @date 2026-07-23
 */
 
 /* External tick counter — replaces time(NULL) */
 extern volatile uint64_t jiffies;
 
 /* ── Semaphore set table ─────────────────────────────────────────────── */
 static SemSet sem_sets[IPC_MAX_SEM_SETS];
 
 /* ── sem_init_subsystem ──────────────────────────────────────────────── */
 void sem_init_subsystem(void)
 {
     memset(sem_sets, 0, sizeof sem_sets);
     printf("[sem] init: max_sets=%d  max_sems_per_set=%d\n",
            IPC_MAX_SEM_SETS, IPC_MAX_SEMS);
 }
 
 /* ── Algorithm semget (§9) ───────────────────────────────────────────── */
 int semget(int key, int nsems, int flag)
 {
     int i;
 
     /* Search for existing set with this key */
     for (i = 0; i < IPC_MAX_SEM_SETS; i++) {
         if (sem_sets[i].active && sem_sets[i].perm.key == key) {
             printf("[semget] found existing set id=%d key=%d\n", i, key);
             return i;
         }
     }
 
     if (!(flag & IPC_CREAT)) {
         printf("[semget] ERROR: no set key=%d and no IPC_CREAT\n", key);
         return -1;
     }
     if (nsems <= 0 || nsems > IPC_MAX_SEMS) {
         printf("[semget] ERROR: nsems=%d out of range\n", nsems);
         return -1;
     }
 
     for (i = 0; i < IPC_MAX_SEM_SETS; i++) {
         if (!sem_sets[i].active) {
             sem_sets[i].active    = true;
             sem_sets[i].perm.key  = key;
             sem_sets[i].perm.mode = (uint16_t)(flag & 0x1FF);
             sem_sets[i].nsems     = nsems;
             sem_sets[i].ctime     = (int64_t)jiffies;   /* was: time(NULL) */
             memset(sem_sets[i].sems, 0, sizeof sem_sets[i].sems);
             printf("[semget] created set id=%d key=%d nsems=%d\n",
                    i, key, nsems);
             return i;
         }
     }
 
     printf("[semget] ERROR: no free sem set slots\n");
     return -1;
 }
 
 /* ── Algorithm semctl (§10) ──────────────────────────────────────────── */
 int semctl(int semid, int semnum, int cmd, int val)
 {
     int i;
 
     if (semid < 0 || semid >= IPC_MAX_SEM_SETS || !sem_sets[semid].active) {
         printf("[semctl] ERROR: invalid semid %d\n", semid);
         return -1;
     }
     SemSet *ss = &sem_sets[semid];
 
     switch (cmd) {
     case GETVAL:
         if (semnum < 0 || semnum >= ss->nsems) return -1;
         printf("[semctl] GETVAL semid=%d [%d] = %d\n",
                semid, semnum, ss->sems[semnum].val);
         return ss->sems[semnum].val;
 
     case SETVAL:
         if (semnum < 0 || semnum >= ss->nsems) return -1;
         ss->sems[semnum].val = val;
         ss->ctime = (int64_t)jiffies;                   /* was: time(NULL) */
         printf("[semctl] SETVAL semid=%d [%d] = %d\n", semid, semnum, val);
         return 0;
 
     case GETALL:
         for (i = 0; i < ss->nsems; i++)
             printf("[semctl] GETALL semid=%d [%d]=%d\n",
                    semid, i, ss->sems[i].val);
         return 0;
 
     case SETALL:
         for (i = 0; i < ss->nsems; i++) ss->sems[i].val = val;
         ss->ctime = (int64_t)jiffies;                   /* was: time(NULL) */
         printf("[semctl] SETALL semid=%d  all=%d\n", semid, val);
         return 0;
 
     case IPC_RMID:
         memset(ss, 0, sizeof *ss);
         printf("[semctl] IPC_RMID semid=%d removed\n", semid);
         return 0;
 
     default:
         printf("[semctl] ERROR: unknown cmd %d\n", cmd);
         return -1;
     }
 }
 
 /* ── Internal: reverse operations 0..done_count-1 ───────────────────── */
 static void reverse_ops(SemSet *ss, SemBuf *ops, int done_count)
 {
     int i;
     for (i = done_count - 1; i >= 0; i--)
         ss->sems[ops[i].sem_num].val -= ops[i].sem_op;
     printf("[semop] reversed %d operations\n", done_count);
 }
 
 /* ── Algorithm semop (§11) ───────────────────────────────────────────── */
 int semop(int semid, SemBuf *ops, int nops, SimProcess *caller)
 {
     int i;
 
     if (semid < 0 || semid >= IPC_MAX_SEM_SETS || !sem_sets[semid].active) {
         printf("[semop] ERROR: invalid semid %d\n", semid);
         return -1;
     }
     if (!ops || nops <= 0) return -1;
 
     SemSet *ss = &sem_sets[semid];
 
 start:;
     /* Validate all sem_num indices */
     for (i = 0; i < nops; i++) {
         if (ops[i].sem_num < 0 || ops[i].sem_num >= ss->nsems) {
             printf("[semop] ERROR: sem_num %d out of range\n",
                    ops[i].sem_num);
             return -1;
         }
     }
 
     int done = 0;
 
     for (i = 0; i < nops; i++) {
         Semaphore *s = &ss->sems[ops[i].sem_num];
         int op       = ops[i].sem_op;
 
         if (op > 0) {
             /* ── V operation ────────────────────────────────────────── */
             s->val += op;
             if (ops[i].sem_flg & IPC_UNDO) {
                 printf("[semop] UNDO registered for +%d on sem[%d]\n",
                        op, ops[i].sem_num);
             }
             printf("[semop] V: sem[%d] += %d → %d  "
                    "(wakeup wait_incr processes)\n",
                    ops[i].sem_num, op, s->val);
             if (s->val == 0) {
                 printf("[semop] sem[%d] == 0 — wakeup wait_zero\n",
                        ops[i].sem_num);
             }
             done++;
 
         } else if (op < 0) {
             /* ── P operation ────────────────────────────────────────── */
             if (s->val >= -op) {
                 s->val += op;   /* op is negative, so this decrements */
                 printf("[semop] P: sem[%d] += %d → %d\n",
                        ops[i].sem_num, op, s->val);
                 done++;
             } else {
                 reverse_ops(ss, ops, done);
                 if (ops[i].sem_flg & IPC_NOWAIT) {
                     printf("[semop] ERROR: P blocked, NOWAIT — error\n");
                     return -1;
                 }
                 s->wait_incr++;
                 printf("[semop] pid=%d sleeps (event sem[%d] increases)\n",
                        caller ? caller->pid : -1, ops[i].sem_num);
                 sim_sleep(caller, EVENT_SEM_INCR);
                 s->wait_incr--;
                 goto start;   /* restart entire op array */
             }
 
         } else {
             /* ── Zero: wait for semaphore == 0 ─────────────────────── */
             if (s->val != 0) {
                 reverse_ops(ss, ops, done);
                 if (ops[i].sem_flg & IPC_NOWAIT) {
                     printf("[semop] ERROR: zero-wait blocked, NOWAIT\n");
                     return -1;
                 }
                 s->wait_zero++;
                 printf("[semop] pid=%d sleeps (event sem[%d] == 0)\n",
                        caller ? caller->pid : -1, ops[i].sem_num);
                 sim_sleep(caller, EVENT_SEM_ZERO);
                 s->wait_zero--;
                 goto start;
             }
             /* val already == 0 — no action needed */
             done++;
         }
     }
 
     /* All operations succeeded */
     ss->otime = (int64_t)jiffies;                       /* was: time(NULL) */
     if (caller) {
         for (i = 0; i < nops; i++)
             ss->sems[ops[i].sem_num].last_pid = caller->pid;
     }
 
     printf("[semop] semid=%d: %d operation(s) applied successfully\n",
            semid, nops);
     return 0;
 }
 