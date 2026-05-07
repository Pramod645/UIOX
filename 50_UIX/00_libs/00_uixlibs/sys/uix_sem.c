#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SEMKEY 5678

// Define the union semun here if not provided by the system header /
union semun {
    int val;
    struct semidds buf;
    unsigned short array;
};

// P and V operations (classical semaphore down/up) /
void semwaitop(int semid);
void semsignalop(int semid);

int sem(void) {
    int semid;
    struct sembuf op;
    union semun arg;

    // Create a semaphore set containing one semaphore /
    semid = semget(SEMKEY, 1, IPCCREAT | 0666);
    if (semid == -1) {
        perror("semget");
        exit(EXITFAILURE);
    }

    // Initialize semaphore to 1 /
    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl");
        exit(EXITFAILURE);
    }

    printf("Semaphore created with ID = %d\n", semid);
    printf("Entering critical section...\n");

    // Perform P (wait) operation /
    semwaitop(semid);
    printf("Inside critical section (PID=%d)\n", getpid());
    sleep(3); / simulate work /
    printf("Leaving critical section...\n");

    // Perform V (signal) operation /
    semsignalop(semid);

    // Clean up after one run /
    if (semctl(semid, 0, IPCRMID) == -1)
        perror("semctl(IPCRMID)");

    return 0;
}

void semwaitop(int semid) {
    struct sembuf op = {0, -1, 0}; / decrement semaphore /
    if (semop(semid, &op, 1) == -1) {
        perror("semop - wait");
        exit(EXITFAILURE);
    }
}

void semsignalop(int semid) {
    struct sembuf op = {0, 1, 0}; / increment semaphore */
    if (semop(semid, &op, 1) == -1) {
        perror("semop - signal");
        exit(EXITFAILURE);
    }
}
//only one process enters the “critical section” at a time — the semaphore enforces mutual exclusion.
//////////////////////////////////
/* src/uix_sem.c */
#include "uix_sem.h"
#include "uix_errno.h"
#include "uix_stdarg.h"

int uix_semget(uix_key_t key, int nsems, int semflg)
{
    extern int sys_semget(uix_key_t,int,int) __attribute__((weak));
    if (sys_semget) return sys_semget(key, nsems, semflg);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_semop(int semid, uix_sembuf_t *sops, uix_size_t nsops)
{
    extern int sys_semop(int,void*,uix_size_t) __attribute__((weak));
    if (sys_semop) return sys_semop(semid, sops, nsops);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_semctl(int semid, int semnum, int cmd, ...)
{
    uix_va_list ap; uix_va_start(ap, cmd);
    void *arg = uix_va_arg(ap, void *);
    uix_va_end(ap);
    extern int sys_semctl(int,int,int,void*) __attribute__((weak));
    if (sys_semctl) return sys_semctl(semid, semnum, cmd, arg);
    uix_errno = UIX_ENOSYS; return -1;
}



