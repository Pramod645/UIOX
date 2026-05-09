
#ifndef __SYS_UIX_MSG__H
#define __SYS_UIX_MSG__H
/*
sys/msg.h is the standard System V message queue header.  
It defines the data structures and constants used with message queues, which are one of the classic IPC (inter‑process communication) 
mechanisms in UNIX systems, along with shared memory and semaphores.

*/
/* This is for only POXIS */

#include "uix_features.h" //??


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
} uix_msqid_ds_t;                // Message queue status structure

typedef struct uix_msgbuf {
    long mtype;               // Message type — must be positive long
    char mtext[1];            // Message data — flexible array
} uix_msgbuf_t;

#define UIX_MSG_NOERROR 010000     // Truncate message if too long
#define UIX_MSG_COPY    040000
#define UIX_MSG_EXCEPT  020000

int         uix_msgget(uix_key_t key, int msgflg);       // Creates or opens message queue
int         uix_msgsnd(int msqid, const void *msgp,
                        uix_size_t msgsz, int msgflg);         // Sends message to queue
uix_ssize_t uix_msgrcv(int msqid, void *msgp, uix_size_t msgsz,
                        long msgtyp, int msgflg);                 // Receives message from queue
int         uix_msgctl(int msqid, int cmd, uix_msqid_ds_t *buf);    // Controls message queue (stat/set/remove)




#endif /* End of __SYS_UIX_MSG__H */
/* ***This is End of file, there is no more line should be added after this line*** */
