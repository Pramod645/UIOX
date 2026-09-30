/*
 * 30KIX/33PCS/00IPC/include/uiox_kix_msg.h
 *
 * Message queues — Bach's Algorithms 1-4.
 *
 * ── version 3.0.0: the sleep/wakeup binding ─────────────────────────
 * uiox_kix_msg_snd and uiox_kix_msg_rcv took SimProcess * — a fourth process model with its
 * own pid spelling, defined in ipc_types.h and now deleted.  They take
 * uiox_kix_psa_proc_t * — the ONE process type.
 *
 * This header moved with its source, not after it: a header left
 * declaring the old type is a declaration/definition mismatch, which is
 * a compile failure rather than a warning.
 *
 * ── the fields that stayed int ──────────────────────────────────────
 * MsgQueue's last_send_pid and last_recv_pid are int, and remain so.
 * They record WHO last touched the queue for IPC_STAT to report, and the
 * value stored is a copy of the process's pid — not a reference to it.
 * Casting to int on the way in is deliberate: the queue outlives the
 * process, and holding a pointer to a recycled entry would be worse
 * than holding a stale number.
 *
 * @version 3.0.0  @date 2026-09-30
 */
#ifndef UIOX_MSG_H
#define UIOX_MSG_H

#include "uiox_kix_ipc_types.h"           /* limits, IpcPerm, ipc_sleep/wakeup */
#include "uiox_kix_psa_process.h" /* uiox_kix_psa_proc_t */

/* ── Message ─────────────────────────────────────────────────────────
 * mtext is a fixed 512-byte array because the kernel stores nodes in a
 * static pool; a variable-length message would need a heap.  msize says
 * how much of it is in use, which is what uiox_kix_msg_snd counts and uiox_kix_msg_rcv
 * copies. */
typedef struct {
    long         mtype;                    /* user-chosen type    */
    char         mtext[IPC_MAX_MSG_BYTES];
    uiox_size_t  msize;                    /* bytes in use        */
} Msg;

typedef struct MsgNode {
    Msg              msg;
    struct MsgNode  *next;
} MsgNode;

/* ── Queue header ────────────────────────────────────────────────────
 * first/last make the queue a FIFO for type 0, and uiox_kix_msg_rcv's typed
 * selection walks the same list.  msg_bytes against msg_qbytes is the
 * space test uiox_kix_msg_snd waits on. */
typedef struct {
    IpcPerm      perm;
    MsgNode     *first;
    MsgNode     *last;
    int          msg_count;
    uiox_size_t  msg_bytes;
    uiox_size_t  msg_qbytes;
    int          last_send_pid;
    int          last_recv_pid;
    time_t       snd_time;
    time_t       rcv_time;
    time_t       ctl_time;
    bool         active;
} MsgQueue;

/* ── API ─────────────────────────────────────────────────────────────
 * All three return a descriptor, 0 for success, or -1 — the System V
 * convention, and the reason uiox_kix_msg_get's return is an index rather than a
 * pointer: a caller can hold it across a restart. */

/* Zero the node pool and every queue. */
void uiox_kix_msg_init(void);

/* Algorithm 1 — find by key, or create with IPC_CREAT.
 * IPC_EXCL makes an existing key a failure.  Returns the queue index. */
int uiox_kix_msg_get(int key, int flag);

/* Algorithm 2 — IPC_STAT, IPC_SET, IPC_RMID.  IPC_RMID drains the queue
 * and wakes everyone waiting on it. */
int uiox_kix_msg_ctl(int msgqid, int cmd, MsgQueue *buf);

/* Algorithm 3 — append a message, or sleep on the queue's SPACE channel
 * until there is room.  A message larger than IPC_MAX_MSG_BYTES is
 * refused before any waiting happens.
 *
 * Returns the byte count, or -1 for an invalid descriptor, a queue that
 * was removed while waiting, or an uncaught signal. */
int uiox_kix_msg_snd(int msgqid, const Msg *msg, uiox_size_t count, int flag,
           uiox_kix_psa_proc_t *sender);

/* Algorithm 4 — take a message, or sleep on the queue's ARRIVE channel.
 *
 *   type == 0   the first message
 *   type >  0   the first of that type
 *   type <  0   the LOWEST type <= |type| — the smallest such, not any
 *
 * An oversized message is consumed truncated when MSG_NOERROR is set,
 * and left on the queue with -1 returned when it is not.
 *
 * Returns the byte count copied, or -1. */
int uiox_kix_msg_rcv(int msgqid, Msg *out_msg, uiox_size_t maxcount,
           long type, int flag, uiox_kix_psa_proc_t *receiver);

#endif /* UIOX_MSG_H */
