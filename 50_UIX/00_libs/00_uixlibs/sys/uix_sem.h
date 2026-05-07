
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


/* include/uix_sem.h */
#ifndef UIX_SEM_H
#define UIX_SEM_H

#include "uix_ipc.h"
#include "uix_time.h"

typedef struct uix_semid_ds {
    uix_ipc_perm_t sem_perm;
    uix_time_t     sem_otime;
    uix_time_t     sem_ctime;
    uix_size_t     sem_nsems;
} uix_semid_ds_t;

typedef struct uix_sembuf {
    unsigned short sem_num;
    short          sem_op;
    short          sem_flg;
} uix_sembuf_t;

union uix_semun {
    int              val;
    uix_semid_ds_t  *buf;
    unsigned short  *array;
};

#define UIX_GETVAL  12
#define UIX_SETVAL  16
#define UIX_GETALL  13
#define UIX_SETALL  17
#define UIX_GETNCNT 14
#define UIX_GETPID  11
#define UIX_GETZCNT 15

int uix_semget(uix_key_t key, int nsems, int semflg);
int uix_semop (int semid, uix_sembuf_t *sops, uix_size_t nsops);
int uix_semctl(int semid, int semnum, int cmd, ...);

#endif /* UIX_SEM_H */




#endif /* End of __SYS_SEM__H */
/* ***This is End of file, there is no more line should be added after this line*** */