#include "msg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static MsgQueue queues[IPC_MAX_QUEUES];

void msg_init(void)
{
    memset(queues, 0, sizeof queues);
    for (int i = 0; i < IPC_MAX_QUEUES; i++)
        queues[i].msg_qbytes = IPC_MAX_QUEUE_BYTES;
    printf("[msg] init: max_queues=%d\n", IPC_MAX_QUEUES);
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm msgget
 * ───────────────────────────────────────────────────────────── */
int msgget(int key, int flag)
{
    /* Search for existing queue with this key */
    for (int i = 0; i < IPC_MAX_QUEUES; i++) {
        if (queues[i].active && queues[i].perm.key == key) {
            printf("[msgget] found existing queue id=%d key=%d\n", i, key);
            return i;
        }
    }

    /* Not found — create if IPC_CREAT requested */
    if (!(flag & IPC_CREAT)) {
        fprintf(stderr, "[msgget] no queue for key=%d and no IPC_CREAT\n", key);
        return -1;
    }

    for (int i = 0; i < IPC_MAX_QUEUES; i++) {
        if (!queues[i].active) {
            queues[i].active        = true;
            queues[i].perm.key      = key;
            queues[i].perm.mode     = (uint16_t)(flag & 0x1FF);
            queues[i].first         = NULL;
            queues[i].last          = NULL;
            queues[i].msg_count     = 0;
            queues[i].msg_bytes     = 0;
            queues[i].msg_qbytes    = IPC_MAX_QUEUE_BYTES;
            queues[i].ctl_time      = time(NULL);
            printf("[msgget] created queue id=%d key=%d\n", i, key);
            return i;
        }
    }

    fprintf(stderr, "[msgget] no free queue slots\n");
    return -1;
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm msgctl
 * ───────────────────────────────────────────────────────────── */
int msgctl(int msgqid, int cmd, MsgQueue *buf)
{
    if (msgqid < 0 || msgqid >= IPC_MAX_QUEUES || !queues[msgqid].active) {
        fprintf(stderr, "[msgctl] invalid descriptor %d\n", msgqid);
        return -1;
    }

    MsgQueue *q = &queues[msgqid];

    switch (cmd) {
    case IPC_STAT:
        if (buf) { *buf = *q; }
        printf("[msgctl] IPC_STAT id=%d  count=%d  bytes=%zu\n",
               msgqid, q->msg_count, q->msg_bytes);
        break;

    case IPC_SET:
        if (buf) {
            q->perm.mode  = buf->perm.mode;
            q->msg_qbytes = buf->msg_qbytes;
            q->ctl_time   = time(NULL);
        }
        printf("[msgctl] IPC_SET id=%d\n", msgqid);
        break;

    case IPC_RMID: {
        /* Free all pending messages */
        MsgNode *n = q->first;
        while (n) { MsgNode *nx = n->next; free(n); n = nx; }
        memset(q, 0, sizeof *q);
        printf("[msgctl] IPC_RMID id=%d removed\n", msgqid);
        break;
    }

    default:
        fprintf(stderr, "[msgctl] unknown cmd %d\n", cmd);
        return -1;
    }
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm msgsnd  (§3)
 * ───────────────────────────────────────────────────────────── */
int msgsnd(int msgqid, const Msg *msg, size_t count, int flag,
           SimProcess *sender)
{
    if (msgqid < 0 || msgqid >= IPC_MAX_QUEUES || !queues[msgqid].active) {
        fprintf(stderr, "[msgsnd] invalid descriptor %d\n", msgqid);
        return -1;
    }
    if (!msg || count == 0 || count > IPC_MAX_MSG_BYTES) return -1;

    MsgQueue *q = &queues[msgqid];

    /*
     * While not enough space: sleep or return if NOWAIT.
     * (In simulation we just check once.)
     */
    if (q->msg_bytes + count > q->msg_qbytes) {
        if (flag & IPC_NOWAIT) {
            fprintf(stderr, "[msgsnd] queue full, NOWAIT set — returning\n");
            return -1;
        }
        printf("[msgsnd] queue full — pid=%d sleeps (event space available)\n",
               sender ? sender->pid : -1);
        sim_sleep(sender, EVENT_MSG_SPACE);
        return -1; /* caller must retry after wakeup */
    }

    /* Get a message header node */
    MsgNode *node = calloc(1, sizeof *node);
    if (!node) { perror("calloc"); return -1; }

    /* Copy message from user space to kernel */
    node->msg.mtype = msg->mtype;
    node->msg.msize = count;
    memcpy(node->msg.mtext, msg->mtext, count);
    node->next = NULL;

    /* Enqueue at tail */
    if (q->last) q->last->next = node;
    else         q->first      = node;
    q->last = node;

    q->msg_count++;
    q->msg_bytes      += count;
    q->last_send_pid   = sender ? sender->pid : 0;
    q->snd_time        = time(NULL);

    printf("[msgsnd] id=%d  type=%ld  bytes=%zu  queue_count=%d\n",
           msgqid, msg->mtype, count, q->msg_count);

    /* Wake all processes sleeping waiting to read */
    /* (simulation: annotated only) */
    printf("[msgsnd] wakeup: processes waiting to read from queue %d\n",
           msgqid);

    return (int)count;
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm msgrcv  (§4)
 * ───────────────────────────────────────────────────────────── */
int msgrcv(int msgqid, Msg *out_msg, size_t maxcount,
           long type, int flag, SimProcess *receiver)
{
    if (msgqid < 0 || msgqid >= IPC_MAX_QUEUES || !queues[msgqid].active) {
        fprintf(stderr, "[msgrcv] invalid descriptor %d\n", msgqid);
        return -1;
    }
    if (!out_msg) return -1;

    MsgQueue *q = &queues[msgqid];

loop:;
    MsgNode *chosen = NULL, *prev = NULL;
    MsgNode *cprev  = NULL;

    if (type == 0) {
        /* First message on queue */
        chosen = q->first;
        cprev  = NULL;

    } else if (type > 0) {
        /* First message of exactly this type */
        MsgNode *n = q->first, *pn = NULL;
        while (n) {
            if (n->msg.mtype == type) { chosen = n; cprev = pn; break; }
            pn = n; n = n->next;
        }

    } else {
        /* type < 0: lowest-typed message whose type <= |type| */
        long abs_type = -type;
        long best_type = abs_type + 1;
        MsgNode *n = q->first; prev = NULL;
        while (n) {
            if (n->msg.mtype <= abs_type && n->msg.mtype < best_type) {
                best_type = n->msg.mtype;
                chosen    = n;
                cprev     = prev;
            }
            prev = n; n = n->next;
        }
    }

    if (chosen) {
        /* Size check */
        if (chosen->msg.msize > maxcount) {
            fprintf(stderr, "[msgrcv] message too large (%zu > %zu)\n",
                    chosen->msg.msize, maxcount);
            return -1;
        }

        /* Copy to user space */
        out_msg->mtype = chosen->msg.mtype;
        out_msg->msize = chosen->msg.msize;
        memcpy(out_msg->mtext, chosen->msg.mtext, chosen->msg.msize);

        /* Unlink from queue */
        if (cprev)           cprev->next = chosen->next;
        else                 q->first    = chosen->next;
        if (q->last == chosen) q->last   = cprev;

        q->msg_count--;
        q->msg_bytes        -= chosen->msg.msize;
        q->last_recv_pid     = receiver ? receiver->pid : 0;
        q->rcv_time          = time(NULL);

        int received = (int)chosen->msg.msize;
        free(chosen);

        printf("[msgrcv] id=%d  type=%ld  bytes=%d  queue_count=%d\n",
               msgqid, out_msg->mtype, received, q->msg_count);
        return received;
    }

    /* No matching message */
    if (flag & IPC_NOWAIT) {
        fprintf(stderr, "[msgrcv] no message (type=%ld), NOWAIT — returning\n",
                type);
        return -1;
    }

    printf("[msgrcv] pid=%d sleeps (event message arrives on queue %d)\n",
           receiver ? receiver->pid : -1, msgqid);
    sim_sleep(receiver, EVENT_MSG_ARRIVE);
    goto loop;   /* restart after wakeup */
}
