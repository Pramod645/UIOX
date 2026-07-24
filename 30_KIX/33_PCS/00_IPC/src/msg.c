#include "msg.h"
#include "../include/uiox_klibc.h"
/*
 * 30_KIX/33_PCS/00_IPC/src/msg.c
 *
 * Freestanding fixes (v2.0)
 * ─────────────────────────
 *   REMOVED: #include <stdio.h>  <stdlib.h>  <string.h>
 *            All provided through msg.h → ipc_types.h → uiox_klibc.h
 *
 *   FIXED: fprintf(stderr, ...) → printf(...)
 *          No stderr in freestanding; uiox_klibc.h maps printf → uiox_printf.
 *
 *   FIXED: time(NULL) → xtime.tv_sec
 *          xtime is the kernel wall-clock maintained by timekeeping.c.
 *
 *   FIXED: calloc(1, sizeof *node) → static node pool (no heap)
 *          free(node) → return to pool
 *          perror("calloc") → printf(...)
 *
 *   FIXED: %zu → %llu with cast to (unsigned long long)
 *          size_t = uint64_t in uiox_klibc.h; %zu requires system libc.
 *
 * No algorithm changes — all msgget/msgctl/msgsnd/msgrcv logic unchanged.
 *
 * @version 2.0.0  @date 2026-07-23
 */

 /* External wall-clock from timekeeping.c (replaces time(NULL)) */
 extern volatile uint64_t jiffies;
 
 /* ── Static MsgNode pool — replaces calloc/free ──────────────────────
  * IPC_MAX_MSGS nodes total; enough for all queues simultaneously.     */
 #define MSG_NODE_POOL_SIZE  (IPC_MAX_QUEUES * IPC_MAX_MSGS)
 
 static MsgNode  s_node_pool[MSG_NODE_POOL_SIZE];
 static uint8_t  s_node_used[MSG_NODE_POOL_SIZE];
 static uint8_t  s_pool_ready = 0;
 
 static void pool_init(void)
 {
     if (!s_pool_ready) {
         memset(s_node_pool, 0, sizeof s_node_pool);
         memset(s_node_used, 0, sizeof s_node_used);
         s_pool_ready = 1;
     }
 }
 
 static MsgNode *node_alloc(void)
 {
     uint32_t i;
     pool_init();
     for (i = 0; i < MSG_NODE_POOL_SIZE; i++) {
         if (!s_node_used[i]) {
             s_node_used[i] = 1;
             memset(&s_node_pool[i], 0, sizeof s_node_pool[i]);
             return &s_node_pool[i];
         }
     }
     return (MsgNode *)0;
 }
 
 static void node_free(MsgNode *n)
 {
     uint32_t i;
     if (!n) return;
     for (i = 0; i < MSG_NODE_POOL_SIZE; i++) {
         if (&s_node_pool[i] == n) {
             memset(n, 0, sizeof *n);
             s_node_used[i] = 0;
             return;
         }
     }
 }
 
 /* ── Queue table ────────────────────────────────────────────────────── */
 static MsgQueue queues[IPC_MAX_QUEUES];
 
 /* ── msg_init ───────────────────────────────────────────────────────── */
 void msg_init(void)
 {
     pool_init();
     memset(queues, 0, sizeof queues);
     int i;
     for (i = 0; i < IPC_MAX_QUEUES; i++)
         queues[i].msg_qbytes = IPC_MAX_QUEUE_BYTES;
     printf("[msg] init: max_queues=%d\n", IPC_MAX_QUEUES);
 }
 
 /* ── Algorithm msgget ───────────────────────────────────────────────── */
 int msgget(int key, int flag)
 {
     int i;
     for (i = 0; i < IPC_MAX_QUEUES; i++) {
         if (queues[i].active && queues[i].perm.key == key) {
             printf("[msgget] found existing queue id=%d key=%d\n", i, key);
             return i;
         }
     }
     if (!(flag & IPC_CREAT)) {
         printf("[msgget] ERROR: no queue for key=%d and no IPC_CREAT\n", key);
         return -1;
     }
     for (i = 0; i < IPC_MAX_QUEUES; i++) {
         if (!queues[i].active) {
             queues[i].active        = true;
             queues[i].perm.key      = key;
             queues[i].perm.mode     = (uint16_t)(flag & 0x1FF);
             queues[i].first         = (MsgNode *)0;
             queues[i].last          = (MsgNode *)0;
             queues[i].msg_count     = 0;
             queues[i].msg_bytes     = 0;
             queues[i].msg_qbytes    = IPC_MAX_QUEUE_BYTES;
             queues[i].ctl_time      = (int64_t)jiffies;   /* was: time(NULL) */
             printf("[msgget] created queue id=%d key=%d\n", i, key);
             return i;
         }
     }
     printf("[msgget] ERROR: no free queue slots\n");
     return -1;
 }
 
 /* ── Algorithm msgctl ───────────────────────────────────────────────── */
 int msgctl(int msgqid, int cmd, MsgQueue *buf)
 {
     if (msgqid < 0 || msgqid >= IPC_MAX_QUEUES || !queues[msgqid].active) {
         printf("[msgctl] ERROR: invalid descriptor %d\n", msgqid);
         return -1;
     }
     MsgQueue *q = &queues[msgqid];
     switch (cmd) {
     case IPC_STAT:
         if (buf) { *buf = *q; }
         printf("[msgctl] IPC_STAT id=%d  count=%d  bytes=%llu\n",
                msgqid, q->msg_count, (unsigned long long)q->msg_bytes);
         break;
     case IPC_SET:
         if (buf) {
             q->perm.mode  = buf->perm.mode;
             q->msg_qbytes = buf->msg_qbytes;
             q->ctl_time   = (int64_t)jiffies;             /* was: time(NULL) */
         }
         printf("[msgctl] IPC_SET id=%d\n", msgqid);
         break;
     case IPC_RMID: {
         MsgNode *n = q->first;
         while (n) {
             MsgNode *nx = n->next;
             node_free(n);                                  /* was: free(n)    */
             n = nx;
         }
         memset(q, 0, sizeof *q);
         printf("[msgctl] IPC_RMID id=%d removed\n", msgqid);
         break;
     }
     default:
         printf("[msgctl] ERROR: unknown cmd %d\n", cmd);
         return -1;
     }
     return 0;
 }
 
 /* ── Algorithm msgsnd ───────────────────────────────────────────────── */
 int msgsnd(int msgqid, const Msg *msg, size_t count, int flag,
            SimProcess *sender)
 {
     if (msgqid < 0 || msgqid >= IPC_MAX_QUEUES || !queues[msgqid].active) {
         printf("[msgsnd] ERROR: invalid descriptor %d\n", msgqid);
         return -1;
     }
     if (!msg || count == 0 || count > IPC_MAX_MSG_BYTES) return -1;
 
     MsgQueue *q = &queues[msgqid];
 
     /* Check queue space */
     if (q->msg_bytes + count > q->msg_qbytes) {
         if (flag & IPC_NOWAIT) {
             printf("[msgsnd] ERROR: queue full, NOWAIT set — returning\n");
             return -1;
         }
         printf("[msgsnd] queue full — pid=%d sleeps (event space available)\n",
                sender ? sender->pid : -1);
         sim_sleep(sender, EVENT_MSG_SPACE);
     }
 
     /* Allocate node from static pool — was: calloc(1, sizeof *node) */
     MsgNode *node = node_alloc();
     if (!node) {
         printf("[msgsnd] ERROR: message node pool exhausted\n"); /* was: perror */
         return -1;
     }
 
     node->msg.mtype = msg->mtype;
     node->msg.msize = count;
     memcpy(node->msg.mtext, msg->mtext, count);
     node->next = (MsgNode *)0;
 
     if (q->last) q->last->next = node;
     else         q->first      = node;
     q->last = node;
     q->msg_count++;
     q->msg_bytes      += count;
     q->last_send_pid   = sender ? sender->pid : 0;
     q->snd_time        = (int64_t)jiffies;                /* was: time(NULL) */
 
     printf("[msgsnd] id=%d  type=%ld  bytes=%llu  queue_count=%d\n",
            msgqid, msg->mtype,
            (unsigned long long)count,                      /* was: %zu        */
            q->msg_count);
     printf("[msgsnd] wakeup: processes waiting to read from queue %d\n",
            msgqid);
     return (int)count;
 }
 
 /* ── Algorithm msgrcv ───────────────────────────────────────────────── */
 int msgrcv(int msgqid, Msg *out_msg, size_t maxcount,
            long type, int flag, SimProcess *receiver)
 {
     if (msgqid < 0 || msgqid >= IPC_MAX_QUEUES || !queues[msgqid].active) {
         printf("[msgrcv] ERROR: invalid descriptor %d\n", msgqid);
         return -1;
     }
 
     MsgQueue *q      = &queues[msgqid];
     MsgNode  *chosen = (MsgNode *)0;
     MsgNode  *cprev  = (MsgNode *)0;
 
     if (type == 0) {
         chosen = q->first;
         cprev  = (MsgNode *)0;
     } else if (type > 0) {
         MsgNode *n = q->first, *pn = (MsgNode *)0;
         while (n) {
             if (n->msg.mtype == type) { chosen = n; cprev = pn; break; }
             pn = n; n = n->next;
         }
     } else {
         /* type < 0: lowest-typed message whose type <= |type| */
         long     best = -type + 1;
         MsgNode *n    = q->first, *pn = (MsgNode *)0;
         while (n) {
             if (n->msg.mtype <= -type && n->msg.mtype < best) {
                 best   = n->msg.mtype;
                 chosen = n;
                 cprev  = pn;
             }
             pn = n; n = n->next;
         }
     }
 
     if (chosen) {
         if (chosen->msg.msize > maxcount) {
             printf("[msgrcv] ERROR: message too large (%llu > %llu)\n",
                    (unsigned long long)chosen->msg.msize,  /* was: %zu */
                    (unsigned long long)maxcount);
             return -1;
         }
 
         out_msg->mtype = chosen->msg.mtype;
         out_msg->msize = chosen->msg.msize;
         memcpy(out_msg->mtext, chosen->msg.mtext, chosen->msg.msize);
 
         /* Dequeue */
         if (cprev) cprev->next  = chosen->next;
         else       q->first     = chosen->next;
         if (q->last == chosen)  q->last = cprev;
 
         q->msg_count--;
         q->msg_bytes        -= chosen->msg.msize;
         q->last_recv_pid     = receiver ? receiver->pid : 0;
         q->rcv_time          = (int64_t)jiffies;           /* was: time(NULL) */
 
         int received = (int)chosen->msg.msize;
         node_free(chosen);                                  /* was: free(chosen) */
 
         printf("[msgrcv] id=%d  type=%ld  bytes=%d  queue_count=%d\n",
                msgqid, out_msg->mtype, received, q->msg_count);
         return received;
     }
 
     /* No matching message */
     if (flag & IPC_NOWAIT) {
         printf("[msgrcv] ERROR: no message (type=%ld), NOWAIT — returning\n",
                type);
         return -1;
     }
 
     printf("[msgrcv] pid=%d sleeps (event message arrives on queue %d)\n",
            receiver ? receiver->pid : -1, msgqid);
     sim_sleep(receiver, EVENT_MSG_ARRIVE);
     return -1;
 }
 