#ifndef UIOX_MSG_H
#define UIOX_MSG_H

#include "ipc_types.h"

/* ─────────────────────────────────────────────────────────────
 * Message structure
 * User-visible: mtype + mtext array.
 * Kernel stores a MsgNode on the queue's linked list.
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    long    mtype;                   /* user-chosen message type  */
    char    mtext[IPC_MAX_MSG_BYTES];
    size_t  msize;                   /* actual bytes in mtext     */
} Msg;

typedef struct MsgNode {
    Msg          msg;
    struct MsgNode *next;
} MsgNode;

/* ─────────────────────────────────────────────────────────────
 * Message queue header (one per descriptor)
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    IpcPerm  perm;
    MsgNode *first;           /* head of message linked list      */
    MsgNode *last;            /* tail                             */
    int      msg_count;       /* number of messages on queue      */
    size_t   msg_bytes;       /* total data bytes on queue        */
    size_t   msg_qbytes;      /* max data bytes allowed           */
    int      last_send_pid;
    int      last_recv_pid;
    time_t   snd_time;
    time_t   rcv_time;
    time_t   ctl_time;
    bool     active;
} MsgQueue;

/* ─────────────────────────────────────────────────────────────
 * Message IPC API
 * ───────────────────────────────────────────────────────────── */
void msg_init(void);

/* Algorithm msgget — create or access a message queue */
int  msgget(int key, int flag);

/* Algorithm msgctl — control operations on a queue */
int  msgctl(int msgqid, int cmd, MsgQueue *buf);

/*
 * Algorithm msgsnd — send a message  (§3)
 * Returns bytes sent, or -1.
 */
int  msgsnd(int msgqid, const Msg *msg, size_t count, int flag,
            SimProcess *sender);

/*
 * Algorithm msgrcv — receive a message  (§4)
 * type == 0  : first message
 * type >  0  : first message of that type
 * type <  0  : lowest-typed message whose type <= |type|
 * Returns bytes received, or -1.
 */
int  msgrcv(int msgqid, Msg *out_msg, size_t maxcount,
            long type, int flag, SimProcess *receiver);

#endif /* UIOX_MSG_H */
