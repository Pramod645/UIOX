#ifndef UIOX_SEM_H
#define UIOX_SEM_H

#include "ipc_types.h"

/* ─────────────────────────────────────────────────────────────
 * Single semaphore
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    int  val;           /* current semaphore value               */
    int  last_pid;      /* pid of last process to operate        */
    int  wait_incr;     /* processes waiting for value to rise   */
    int  wait_zero;     /* processes waiting for value == 0      */
} Semaphore;

/* ─────────────────────────────────────────────────────────────
 * Semaphore set (one per semget descriptor)
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    IpcPerm    perm;
    Semaphore  sems[IPC_MAX_SEMS];
    int        nsems;
    time_t     otime;   /* last semop time                       */
    time_t     ctime;   /* last semctl time                      */
    bool       active;
} SemSet;

/* ─────────────────────────────────────────────────────────────
 * Single operation in the semop array
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    int   sem_num;   /* which semaphore in the set               */
    int   sem_op;    /* operation value (positive, negative, 0)  */
    int   sem_flg;   /* IPC_NOWAIT | IPC_UNDO                    */
} SemBuf;

/* semctl command parameters */
#define GETVAL   4
#define SETVAL   5
#define GETALL   6
#define SETALL   7

/* ─────────────────────────────────────────────────────────────
 * Semaphore IPC API
 * ───────────────────────────────────────────────────────────── */
void sem_init_subsystem(void);

/* Algorithm semget (§9) */
int  semget(int key, int nsems, int flag);

/* Algorithm semctl (§10) */
int  semctl(int semid, int semnum, int cmd, int val);

/*
 * Algorithm semop (§11)
 * Atomically apply 'nops' operations from 'ops' array.
 * Returns value of last semaphore operated on before success, or -1.
 */
int  semop(int semid, SemBuf *ops, int nops, SimProcess *caller);

#endif /* UIOX_SEM_H */
