#ifndef UIOX_IPC_TYPES_H
#define UIOX_IPC_TYPES_H

#include "../include/uiox_klibc.h"


/* ─────────────────────────────────────────────────────────────
 * General IPC limits
 * ───────────────────────────────────────────────────────────── */
#define IPC_MAX_QUEUES      16
#define IPC_MAX_MSGS        32
#define IPC_MAX_MSG_BYTES   512
#define IPC_MAX_QUEUE_BYTES (IPC_MAX_MSGS * IPC_MAX_MSG_BYTES)
#define IPC_MAX_SHM         16
#define IPC_MAX_SHM_SIZE    (64 * 1024)   /* 64 KB per region      */
#define IPC_MAX_SEM_SETS    16
#define IPC_MAX_SEMS        16            /* semaphores per set     */
#define IPC_MAX_SOCKETS     32
#define IPC_MAX_PENDING     8             /* listen backlog         */
#define IPC_MAX_PROCESSES   32

/* IPC creation flags */
#define IPC_CREAT   0x0200
#define IPC_EXCL    0x0400
#define IPC_NOWAIT  0x0800
#define IPC_RMID    1
#define IPC_SET     2
#define IPC_STAT    3
#define IPC_UNDO    0x1000

/* Special keys */
#define IPC_PRIVATE 0

/* ─────────────────────────────────────────────────────────────
 * IPC permission block (common to all three mechanisms)
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    int      key;
    uint16_t mode;    /* permission bits                          */
    int      uid;
    int      gid;
} IpcPerm;

/* ─────────────────────────────────────────────────────────────
 * Lightweight simulated process reference
 * ───────────────────────────────────────────────────────────── */
typedef struct SimProcess {
    int  pid;
    int  uid;
    int  gid;
    bool traced;         /* trace bit set in proc table entry    */
    bool sleeping;
    int  wake_event;     /* event id this process sleeps on      */
} SimProcess;

/* Wake-event IDs */
#define EVENT_MSG_SPACE    1
#define EVENT_MSG_ARRIVE   2
#define EVENT_SEM_INCR     3
#define EVENT_SEM_ZERO     4
#define EVENT_SOCKET_CONN  5

/* Simulated sleep/wakeup stubs */
static inline void sim_sleep(SimProcess *p, int event)
{
    p->sleeping    = true;
    p->wake_event  = event;
}

static inline void sim_wakeup(SimProcess *p, int event)
{
    if (p->sleeping && p->wake_event == event) {
        p->sleeping   = false;
        p->wake_event = 0;
    }
}

#endif /* UIOX_IPC_TYPES_H */
