
#ifndef __SYS_IPC__H
#define __SYS_IPC__H
/*
sys/ipc.h is a standard System V IPC (Inter‑Process Communication) header found on Unix-like systems. 
It defines key constants and the key type used with System V shared memory, message queues, and semaphores. 
It’s often used together with headers like sys/shm.h, sys/msg.h, and sys/sem.h.

*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>   // for keyt, uidt, gidt /

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

/* Permission structure for IPC objects */
struct ipcperm {
    keyt  _key;    // Key supplied to msgget(), semget(), shmget() /
    uidt  uid;      // Owner's user ID /
    gidt  gid;      // Owner's group ID /
    uidt  cuid;     // Creator's user ID /
    gidt  cgid;     // Creator's group ID /
    unsigned short mode; // Read/write permission bits /
    unsigned short seq; // Sequence number (internal use) /
};

/* Special key values for ipc mechanisms */
#define IPCCREAT  01000    // Create entry if key doesn't exist /
#define IPCEXCL   02000    // Fail if key exists /
#define IPCNOWAIT 04000    // Return error on wait /

/* Authorization commands for msgctl(), semctl(), shmctl() */
#define IPCRMID 0   // Remove identifier /
#define IPCSET  1   // Set ipcperm options /
#define IPCSTAT 2   // Get ipcperm options /

/* Reserved still used in some code */
#define IPCPRIVATE ((keyt)0)

/* Function to generate a key from pathname and project ID */
keyt ftok(const char pathname, int projid);


#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */



/* include/uix_ipc.h */
#ifndef UIX_IPC_H
#define UIX_IPC_H

#include "uix_types.h"

typedef int uix_key_t;

#define UIX_IPC_PRIVATE  ((uix_key_t)0)      // Key meaning private IPC object
#define UIX_IPC_CREAT    01000        // Create IPC object if key doesn't exist
#define UIX_IPC_EXCL     02000      // Fail if object already exists
#define UIX_IPC_NOWAIT   04000
#define UIX_IPC_RMID     0             // Remove IPC object
#define UIX_IPC_SET      1
#define UIX_IPC_STAT     2      // Get IPC object status
#define UIX_IPC_INFO     3

typedef struct uix_ipc_perm {
    uix_key_t  __key;
    uix_uid_t  uid;
    uix_gid_t  gid;
    uix_uid_t  cuid;
    uix_gid_t  cgid;
    uix_uint16_t mode;
    uix_uint16_t __seq;
} uix_ipc_perm_t;            // Permission structure shared by msg/sem/shm

uix_key_t uix_ftok(const char *path, int id); // Generates IPC key from path and id — POSIX

#endif /* UIX_IPC_H */




#endif /* End of __SYS_IPC__H */
/* ***This is End of file, there is no more line should be added after this line*** */