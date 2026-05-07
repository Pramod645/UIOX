
#ifndef __SYS_MSG__H
#define __SYS_MSG__H
/*
sys/msg.h is the standard System V message queue header.  
It defines the data structures and constants used with message queues, which are one of the classic IPC (inter‑process communication) 
mechanisms in UNIX systems, along with shared memory and semaphores.

*/
/* This is for only POXIS */

#include "features.h"

#include <sys/ipc.h>
#include <sys/types.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

/* Message buffer used by msgsnd() and msgrcv() */
struct msgbuf {
    long mtype;       // Message type, must be > 0 /
    char mtext[1];    // Message data (variable length) /
};

/* Data structure describing a message queue — used internally */
struct msqidds {
    struct ipcperm msgperm;  // Ownership and permissions /
    timet          msgstime; // Time of last msgsnd() /
    timet          msgrtime; // Time of last msgrcv() /
    timet          msgctime; // Time of last change /
    unsigned long   _msgcbytes; // Current number of bytes on queue /
    unsigned long   msgqnum;     // Number of messages currently on queue /
    unsigned long   msgqbytes;   // Max bytes allowed on queue /
    pidt           msglspid;    // PID of last msgsnd() /
    pidt           msglrpid;    // PID of last msgrcv() /
};

/* Commands for msgctl() */
#define IPCRMID  0    // Remove queue /
#define IPCSET   1    // Set queue options /
#define IPCSTAT  2    // Get queue status /

/* Function prototypes */
int msgget(keyt key, int msgflg);
int msgctl(int msqid, int cmd, struct msqidds buf);
int msgsnd(int msqid, const void msgp, sizet msgsz, int msgflg);
ssizet msgrcv(int msqid, void msgp, sizet msgsz, long msgtyp, int msgflg);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */



/* include/uix_msg.h */
#ifndef UIX_MSG_H
#define UIX_MSG_H

#include "uix_ipc.h"
#include "uix_time.h"

typedef struct uix_msqid_ds {
    uix_ipc_perm_t msg_perm;
    uix_time_t     msg_stime;
    uix_time_t     msg_rtime;
    uix_time_t     msg_ctime;
    uix_size_t     msg_cbytes;
    uix_size_t     msg_qnum;
    uix_size_t     msg_qbytes;
    uix_pid_t      msg_lspid;
    uix_pid_t      msg_lrpid;
} uix_msqid_ds_t;

typedef struct uix_msgbuf {
    long mtype;
    char mtext[1];
} uix_msgbuf_t;

#define UIX_MSG_NOERROR 010000
#define UIX_MSG_COPY    040000
#define UIX_MSG_EXCEPT  020000

int         uix_msgget(uix_key_t key, int msgflg);
int         uix_msgsnd(int msqid, const void *msgp,
                        uix_size_t msgsz, int msgflg);
uix_ssize_t uix_msgrcv(int msqid, void *msgp, uix_size_t msgsz,
                        long msgtyp, int msgflg);
int         uix_msgctl(int msqid, int cmd, uix_msqid_ds_t *buf);

#endif /* UIX_MSG_H */





#endif /* End of __SYS_MSG__H */
/* ***This is End of file, there is no more line should be added after this line*** */