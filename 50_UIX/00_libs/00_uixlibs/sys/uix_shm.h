
#ifndef __SYS_SHM_H
#define __SYS_SHM_H
/*
sys/shm.h defines the System V shared memory interface — one of the classic UNIX IPC (interprocess communication) 
mechanisms. It works together with sys/ipc.h, and it’s used to allocate and attach memory segments that multiple 
processes can access concurrently.



*/
/* This is for only POXIS */

#include "features.h"

#include <sys/ipc.h>
#include <sys/types.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

/* Mode bits for shmflg in shmget() */
#define SHMRDONLY 010000  // attach read-only /
#define SHMRND    020000  // round attach address to SHMLBA /
#define SHMREMAP  040000  // take-over region on attach, Linux-specific /
#define SHMEXEC   0100000 // allow execute permissions /

/* Commands for shmctl() */
#define IPCRMID 0   // remove segment /
#define IPCSET  1   // set parameters /
#define IPCSTAT 2   // get parameters /

/* Type for shmidds fields */
typedef unsigned long shmattt;

/* Shared-memory data structure */
struct shmidds {
    struct ipcperm shmperm;    // Ownership and permissions /
    sizet          shmsegsz;   // Size of segment in bytes /
    timet          shmatime;   // Last attach time /
    timet          shmdtime;   // Last detach time /
    timet          shmctime;   // Last change time /
    pidt           shmcpid;    // PID of creator /
    pidt           shmlpid;    // PID of last shmat()/shmdt() /
    shmattt        shmnattch;  // Number of current attaches /
};

/* Function prototypes */
int shmget(keyt key, sizet size, int shmflg);
void shmat(int shmid, const void shmaddr, int shmflg);
int shmdt(const void shmaddr);
int shmctl(int shmid, int cmd, struct shmidds buf);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */

/* include/uix_shm.h */
#ifndef UIX_SHM_H
#define UIX_SHM_H

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

#endif /* UIX_SHM_H */



#endif /* End of __SYS_SHM_H */
/* ***This is End of file, there is no more line should be added after this line*** */