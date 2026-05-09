
#ifndef __SYS_UIX_SHM__H
#define __SYS_UIX_SHM__H
/*
sys/shm.h defines the System V shared memory interface — one of the classic UNIX IPC (interprocess communication) 
mechanisms. It works together with sys/ipc.h, and it’s used to allocate and attach memory segments that multiple 
processes can access concurrently.



*/
/* This is for only POXIS */

#include "uix_features.h"  //?


#include "uix_ipc.h"
#include "uix_time.h"

typedef struct uix_shmid_ds {
    uix_ipc_perm_t shm_perm;
    uix_size_t     shm_segsz;     // Shared memory segment size in bytes
    uix_time_t     shm_atime;
    uix_time_t     shm_dtime;
    uix_time_t     shm_ctime;
    uix_pid_t      shm_cpid;
    uix_pid_t      shm_lpid;
    uix_size_t     shm_nattch;    // Current number of attached processes
} uix_shmid_ds_t;

#define UIX_SHM_RDONLY  010000        // Attach read-only
#define UIX_SHM_RND     020000
#define UIX_SHM_REMAP   040000
#define UIX_SHM_EXEC    0100000

int   uix_shmget(uix_key_t key, uix_size_t size, int shmflg);  // Creates or opens shared memory segment
void *uix_shmat (int shmid, const void *shmaddr, int shmflg);  // Attaches segment to process address space
int   uix_shmdt (const void *shmaddr);                      /// Detaches shared memory segment
int   uix_shmctl(int shmid, int cmd, uix_shmid_ds_t *buf);    // Controls shared memory (stat/set/remove)



#endif /* End of __SYS_UIX_SHM__H */
/* ***This is End of file, there is no more line should be added after this line*** */
