
#ifndef __SYS_SEM__H
#define __SYS_SEM__H
/*
sys/sem.h defines the System V semaphore interface, another classic IPC (interprocess communication) mechanism in UNIX systems.  
Semaphores are used to synchronize operations between processes, protecting shared resources or coordinating workloads.

*/
/* This is for only POXIS */

#include "features.h"

#include <sys/ipc.h>
#include <sys/types.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

/* Structure for semaphore permissions */
struct ipcperm {
    keyt  key;
    uidt  uid;
    gidt  gid;
    uidt  cuid;
    gidt  cgid;
    unsigned short mode;
    unsigned short seq;
};

/* Data structure describing a semaphore set */
struct semidds {
    struct ipcperm semperm; / Ownership and permissions /
    timet semotime;         / Last semop() time /
    timet semctime;         / Last change time /
    unsigned long semnsems;  / Number of semaphores in set /
};

/* Commands for semctl() */
#define GETPID 11 // Get process ID of last semop() /
#define GETVAL 12 // Get semaphore value /
#define GETALL 13 // Get all semaphore values /
#define GETNCNT 14 // Get # processes waiting for semval to increase /
#define GETZCNT 15 // Get # waiting for zero semval /
#define SETVAL 16 // Set semaphore value /
#define SETALL 17 // Set all semaphore values /
#define IPCRMID 0
#define IPCSTAT 2
#define IPCSET  1

/* Flags for semop() behavior */
#define SEMUNDO 0x1000 / Undo operation on process exit /

/* sembuf structure for semop() */
struct sembuf {
    unsigned short semnum; / semaphore index /
    short semop;           / operation (positive, negative or zero) /
    short semflg;          / operation flags /
};

/* Union used by semctl() */
union semun {
    int val;
    struct semidds buf;
    unsigned short array;
};

/* Function prototypes */
int semget(keyt key, int nsems, int semflg);
int semop(int semid, struct sembuf sops, sizet nsops);
int semctl(int semid, int semnum, int cmd, ...);



#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */

#endif /* End of __SYS_SEM__H */
/* ***This is End of file, there is no more line should be added after this line*** */