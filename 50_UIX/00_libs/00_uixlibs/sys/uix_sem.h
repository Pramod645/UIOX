
#ifndef __SYS_UIX_SEM__H
#define __SYS_UIX_SEM__H
/*
sys/sem.h defines the System V semaphore interface, another classic IPC (interprocess communication) mechanism in UNIX systems.  
Semaphores are used to synchronize operations between processes, protecting shared resources or coordinating workloads.

*/
/* This is for only POXIS */

#include "uix_features.h" //?


#include "uix_ipc.h"
#include "uix_time.h"

typedef struct uix_semid_ds {
    uix_ipc_perm_t sem_perm;
    uix_time_t     sem_otime;
    uix_time_t     sem_ctime;
    uix_size_t     sem_nsems;
} uix_semid_ds_t;

typedef struct uix_sembuf {
    unsigned short sem_num;  // Semaphore number in set
    short          sem_op;   // Operation: positive=V, negative=P, zero=wait-for-zero
    short          sem_flg;   // Flags: IPC_NOWAIT, SEM_UNDO
} uix_sembuf_t;

union uix_semun {
    int              val;
    uix_semid_ds_t  *buf;
    unsigned short  *array;
};

#define UIX_GETVAL  12    // Get value of one semaphore
#define UIX_SETVAL  16     // Set value of one semaphore
#define UIX_GETALL  13
#define UIX_SETALL  17
#define UIX_GETNCNT 14
#define UIX_GETPID  11
#define UIX_GETZCNT 15

int uix_semget(uix_key_t key, int nsems, int semflg);         // Creates or opens semaphore set
int uix_semop (int semid, uix_sembuf_t *sops, uix_size_t nsops);  // Performs array of semaphore operations atomically
int uix_semctl(int semid, int semnum, int cmd, ...);              // Controls semaphore (query/set/remove)


#endif /* End of __SYS_UIX_SEM__H */
/* ***This is End of file, there is no more line should be added after this line*** */
