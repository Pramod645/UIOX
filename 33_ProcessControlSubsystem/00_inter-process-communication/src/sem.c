#include "sem.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static SemSet sem_sets[IPC_MAX_SEM_SETS];

void sem_init_subsystem(void)
{
    memset(sem_sets, 0, sizeof sem_sets);
    printf("[sem] init: max_sets=%d  max_sems_per_set=%d\n",
           IPC_MAX_SEM_SETS, IPC_MAX_SEMS);
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm semget  (§9)
 * ───────────────────────────────────────────────────────────── */
int semget(int key, int nsems, int flag)
{
    for (int i = 0; i < IPC_MAX_SEM_SETS; i++) {
        if (sem_sets[i].active && sem_sets[i].perm.key == key) {
            printf("[semget] found existing set id=%d key=%d\n", i, key);
            return i;
        }
    }

    if (!(flag & IPC_CREAT)) {
        fprintf(stderr, "[semget] no set key=%d and no IPC_CREAT\n", key);
        return -1;
    }

    if (nsems <= 0 || nsems > IPC_MAX_SEMS) {
        fprintf(stderr, "[semget] nsems=%d out of range\n", nsems);
        return -1;
    }

    for (int i = 0; i < IPC_MAX_SEM_SETS; i++) {
        if (!sem_sets[i].active) {
            sem_sets[i].active    = true;
            sem_sets[i].perm.key  = key;
            sem_sets[i].perm.mode = (uint16_t)(flag & 0x1FF);
            sem_sets[i].nsems     = nsems;
            sem_sets[i].ctime     = time(NULL);
            memset(sem_sets[i].sems, 0, sizeof sem_sets[i].sems);
            printf("[semget] created set id=%d key=%d nsems=%d\n",
                   i, key, nsems);
            return i;
        }
    }

    fprintf(stderr, "[semget] no free sem set slots\n");
    return -1;
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm semctl  (§10)
 * ───────────────────────────────────────────────────────────── */
int semctl(int semid, int semnum, int cmd, int val)
{
    if (semid < 0 || semid >= IPC_MAX_SEM_SETS || !sem_sets[semid].active) {
        fprintf(stderr, "[semctl] invalid semid %d\n", semid);
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
        ss->ctime = time(NULL);
        printf("[semctl] SETVAL semid=%d [%d] = %d\n", semid, semnum, val);
        return 0;

    case GETALL:
        for (int i = 0; i < ss->nsems; i++)
            printf("[semctl] GETALL semid=%d [%d]=%d\n",
                   semid, i, ss->sems[i].val);
        return 0;

    case SETALL:
        for (int i = 0; i < ss->nsems; i++) ss->sems[i].val = val;
        ss->ctime = time(NULL);
        printf("[semctl] SETALL semid=%d  all=%d\n", semid, val);
        return 0;

    case IPC_RMID:
        memset(ss, 0, sizeof *ss);
        printf("[semctl] IPC_RMID semid=%d removed\n", semid);
        return 0;

    default:
        fprintf(stderr, "[semctl] unknown cmd %d\n", cmd);
        return -1;
    }
}

/* ─────────────────────────────────────────────────────────────
 * Internal: reverse operations 0..done_count-1
 * ───────────────────────────────────────────────────────────── */
static void reverse_ops(SemSet *ss, SemBuf *ops, int done_count)
{
    for (int i = done_count - 1; i >= 0; i--)
        ss->sems[ops[i].sem_num].val -= ops[i].sem_op;
    printf("[semop] reversed %d operations\n", done_count);
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm semop  (§11)
 * ───────────────────────────────────────────────────────────── */
int semop(int semid, SemBuf *ops, int nops, SimProcess *caller)
{
    if (semid < 0 || semid >= IPC_MAX_SEM_SETS || !sem_sets[semid].active) {
        fprintf(stderr, "[semop] invalid semid %d\n", semid);
        return -1;
    }
    if (!ops || nops <= 0) return -1;

    SemSet *ss = &sem_sets[semid];
    int last_val = 0;

start:;
    /* Check permissions for all operations */
    for (int i = 0; i < nops; i++) {
        if (ops[i].sem_num < 0 || ops[i].sem_num >= ss->nsems) {
            fprintf(stderr, "[semop] sem_num %d out of range\n",
                    ops[i].sem_num);
            return -1;
        }
    }

    int done = 0; /* how many ops applied so far this iteration */

    for (int i = 0; i < nops; i++) {
        Semaphore *s = &ss->sems[ops[i].sem_num];
        int op       = ops[i].sem_op;

        if (op > 0) {
            /* ── Positive: V operation ────────────────────── */
            s->val += op;
            if (ops[i].sem_flg & IPC_UNDO) {
                printf("[semop] UNDO registered for +%d on sem[%d]\n",
                       op, ops[i].sem_num);
            }
            /* Wake processes waiting for value to increase */
            printf("[semop] V: sem[%d] += %d → %d  "
                   "(wakeup wait_incr processes)\n",
                   ops[i].sem_num, op, s->val);
            if (s->val == 0)
                printf("[semop] V: sem[%d] == 0  "
                       "(wakeup wait_zero processes)\n",
                       ops[i].sem_num);
            done++;

        } else if (op < 0) {
            /* ── Negative: P operation ────────────────────── */
            if (s->val + op >= 0) {
                last_val = s->val;
                s->val  += op;
                if (ops[i].sem_flg & IPC_UNDO) {
                    printf("[semop] UNDO registered for %d on sem[%d]\n",
                           op, ops[i].sem_num);
                }
                if (s->val == 0)
                    printf("[semop] P: sem[%d] == 0  "
                           "(wakeup wait_zero)\n", ops[i].sem_num);
                printf("[semop] P: sem[%d] += %d → %d\n",
                       ops[i].sem_num, op, s->val);
                done++;
            } else {
                /* Cannot decrement — reverse and sleep */
                reverse_ops(ss, ops, done);
                if (ops[i].sem_flg & IPC_NOWAIT) {
                    fprintf(stderr, "[semop] P blocked, NOWAIT — error\n");
                    return -1;
                }
                s->wait_incr++;
                printf("[semop] pid=%d sleeps (event sem[%d] increases)\n",
                       caller ? caller->pid : -1, ops[i].sem_num);
                sim_sleep(caller, EVENT_SEM_INCR);
                s->wait_incr--;
                goto start; /* restart entire op array */
            }

        } else {
            /* ── Zero: wait for semaphore == 0 ───────────── */
            if (s->val != 0) {
                reverse_ops(ss, ops, done);
                if (ops[i].sem_flg & IPC_NOWAIT) {
                    fprintf(stderr, "[semop] zero-wait blocked, NOWAIT\n");
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
    ss->otime = time(NULL);
    if (caller) {
        for (int i = 0; i < nops; i++)
            ss->sems[ops[i].sem_num].last_pid = caller->pid;
    }

    printf("[semop] all %d operations succeeded  last_val=%d\n",
           nops, last_val);
    return last_val;
}
