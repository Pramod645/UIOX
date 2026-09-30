/*
 * 30KIX/33PCS/00IPC/src/msg.c
 *
 * Bach's Algorithms 1-4 — uiox_kix_msg_get, uiox_kix_msg_ctl, uiox_kix_msg_snd, uiox_kix_msg_rcv (typed message
 * selection).
 *
 * ── bound to 40_psa ─────────────────────────────────────────────────
 * The sender and receiver arguments were SimProcess * — a fourth process
 * model with its own pid spelling, now deleted.  They are
 * uiox_kix_psa_proc_t * — the ONE process type.
 *
 * ── the wakeup that never happened ──────────────────────────────────
 * The version this replaces called sim_sleep(sender, EVENT_MSG_SPACE)
 * when the queue was full, and uiox_kix_msg_rcv printed:
 *
 *     "[uiox_kix_msg_snd] wakeup: processes waiting to read from queue %d"
 *
 * — a message describing a wakeup that nothing performed.  sim_sleep set
 * a flag on a private struct that only this subsystem could see, and no
 * path ever called sim_wakeup for a sender.
 *
 * So a sender that hit a full queue blocked permanently.  Two now call
 * ipc_wakeup: uiox_kix_msg_rcv after it dequeues a message (space appeared), and
 * uiox_kix_msg_ctl's IPC_RMID after it drains a queue (the queue is gone, so a
 * waiting sender must not keep waiting for it).
 *
 * ── channels are addresses now ──────────────────────────────────────
 * 40_psa's sleep takes a wait CHANNEL, not an event number.  Queue 3's
 * "space available" and queue 7's are different addresses, so a wakeup
 * on one no longer reaches the other — which the old shared
 * EVENT_MSG_SPACE could not distinguish.
 *
 * ── what is unchanged ───────────────────────────────────────────────
 * The message selection logic, the three type modes, the queue-space
 * accounting and the static node pool are all as they were.  This is a
 * port, not a redesign.
 *
 * @version 3.0.0  @date 2026-09-30
 */
#include "../include/uiox_kix_msg.h"
#include "../include/uiox_kix_ipc_types.h"
#include "uiox_klibc.h"

/* Wall clock, maintained by 01_schedular's timekeeping.  Replaces
 * time(NULL), which freestanding does not have. */
extern volatile uiox_uint64_t jiffies;

/* ── Static MsgNode pool ─────────────────────────────────────────────
 * IPC_MAX_MSGS nodes per queue, for every queue.  No heap. */
#define MSG_NODE_POOL_SIZE  (IPC_MAX_QUEUES * IPC_MAX_MSGS)

static MsgNode s_node_pool[MSG_NODE_POOL_SIZE];
static uiox_uint8_t s_node_used[MSG_NODE_POOL_SIZE];
static uiox_uint8_t s_pool_ready = 0;

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
    uiox_uint32_t i;
    pool_init();
    for (i = 0; i < MSG_NODE_POOL_SIZE; i++) {
        if (!s_node_used[i]) {
            uiox_uint8_t *p = (uiox_uint8_t *)&s_node_pool[i];
            uiox_uint64_t n = sizeof(MsgNode);
            while (n--) *p++ = 0u;
            s_node_used[i] = 1;
            return &s_node_pool[i];
        }
    }
    return (MsgNode *)0;   /* pool exhausted */
}

static void node_free(MsgNode *n)
{
    uiox_uint32_t i;
    if (!n) return;
    for (i = 0; i < MSG_NODE_POOL_SIZE; i++) {
        if (&s_node_pool[i] == n) {
            uiox_uint8_t *p = (uiox_uint8_t *)n;
            uiox_uint64_t k = sizeof(MsgNode);
            while (k--) *p++ = 0u;
            s_node_used[i] = 0;
            return;
        }
    }
    /* Not from this pool — ignored.  Freeing something we did not
     * allocate would corrupt the list. */
}

/* ── Queue table ────────────────────────────────────────────────────── */
static MsgQueue queues[IPC_MAX_QUEUES];

/* ── uiox_kix_msg_init ────────────────────────────────────────────────────────── */
void uiox_kix_msg_init(void)
{
    int i;
    pool_init();
    memset(queues, 0, sizeof queues);
    for (i = 0; i < IPC_MAX_QUEUES; i++)
        queues[i].msg_qbytes = IPC_MAX_QUEUE_BYTES;
}

/* ── Algorithm 1 — uiox_kix_msg_get ────────────────────────────────────────────
 * Find by key, or create.  IPC_CREAT is required to create, and
 * IPC_EXCL turns that into "fail if it already exists".
 *
 * The key search runs first, so uiox_kix_msg_get on an existing key always returns
 * the existing descriptor rather than making a second one — which is what
 * lets two unrelated processes reach the same queue. */
int uiox_kix_msg_get(int key, int flag)
{
    int i;

    for (i = 0; i < IPC_MAX_QUEUES; i++) {
        if (queues[i].active && queues[i].perm.key == key) {
            if (flag & IPC_EXCL) return -1;    /* asked for exclusive */
            return i;
        }
    }

    if (!(flag & IPC_CREAT)) return -1;

    for (i = 0; i < IPC_MAX_QUEUES; i++) {
        if (!queues[i].active) {
            queues[i].active     = true;
            queues[i].perm.key   = key;
            queues[i].perm.mode  = (uiox_uint16_t)(flag & 0x1FF);
            queues[i].first      = (MsgNode *)0;
            queues[i].last       = (MsgNode *)0;
            queues[i].msg_count  = 0;
            queues[i].msg_bytes  = 0;
            queues[i].msg_qbytes = IPC_MAX_QUEUE_BYTES;
            queues[i].ctl_time   = (time_t)jiffies;
            return i;
        }
    }
    return -1;   /* no free slots */
}

/* ── Algorithm 2 — uiox_kix_msg_ctl ────────────────────────────────────────────
 * IPC_RMID drains the queue and then WAKES ANYONE WAITING ON IT.  That
 * second part is the addition: a sender blocked on "space available"
 * would otherwise wait forever for a queue that no longer exists.
 *
 * The wakeup happens after the drain, because the waiters re-check their
 * condition on waking and must see the queue already empty. */
int uiox_kix_msg_ctl(int msgqid, int cmd, MsgQueue *buf)
{
    MsgQueue *q;

    if (msgqid < 0 || msgqid >= IPC_MAX_QUEUES) return -1;
    if (!queues[msgqid].active) return -1;

    q = &queues[msgqid];

    switch (cmd) {
    case IPC_STAT:
        if (buf) *buf = *q;
        return 0;

    case IPC_SET:
        if (buf) {
            q->perm.mode  = buf->perm.mode;
            q->msg_qbytes = buf->msg_qbytes;
            q->ctl_time   = (time_t)jiffies;
        }
        return 0;

    case IPC_RMID: {
        MsgNode *n = q->first;

        while (n) {
            MsgNode *nx = n->next;
            node_free(n);
            n = nx;
        }
        memset(q, 0, sizeof *q);

        /* The queue is gone — release every waiter on it, both kinds. */
        ipc_wakeup(IPC_WCHAN_MSG_SPACE(q));
        ipc_wakeup(IPC_WCHAN_MSG_ARRIVE(q));
        return 0;
    }

    default:
        return -1;
    }
}

/* ── Algorithm 3 — uiox_kix_msg_snd ────────────────────────────────────────────
 * Append a message, or wait for space.
 *
 * ── the retry loop ──────────────────────────────────────────────────
 * The sleep is taken in a LOOP because a wakeup does not guarantee the
 * condition now holds: Bach's wakeup releases every waiter, and the
 * first to run takes the space.  The others wake to find the queue still
 * full and sleep again.
 *
 * That is why the sleep's return code matters.  A negative return means
 * an UNCAUGHT signal ended the wait — the syscall must unwind rather
 * than re-check, or an unkillable process sits here. */
int uiox_kix_msg_snd(int msgqid, const Msg *msg, uiox_size_t count, int flag,
           uiox_kix_psa_proc_t *sender)
{
    MsgQueue *q;
    MsgNode  *node;

    if (msgqid < 0 || msgqid >= IPC_MAX_QUEUES) return -1;
    if (!queues[msgqid].active) return -1;
    if (!msg || count == 0 || count > IPC_MAX_MSG_BYTES) return -1;

    q = &queues[msgqid];

    /* Wait for room. */
    while (q->msg_bytes + count > q->msg_qbytes) {
        int rc;

        if (flag & IPC_NOWAIT) return -1;

        rc = ipc_sleep(sender, IPC_WCHAN_MSG_SPACE(q));

        if (rc < 0) return -1;          /* uncaught signal — unwind */
        if (!queues[msgqid].active) return -1;   /* queue was removed */
    }

    node = node_alloc();
    if (!node) return -1;               /* pool exhausted */

    node->msg.mtype = msg->mtype;
    node->msg.msize = count;
    memcpy(node->msg.mtext, msg->mtext, count);
    node->next = (MsgNode *)0;

    if (q->last) q->last->next = node;
    else         q->first      = node;
    q->last = node;

    q->msg_count++;
    q->msg_bytes     += count;
    q->last_send_pid  = sender ? (int)sender->p_pid : 0;
    q->snd_time       = (time_t)jiffies;

    /* Data arrived — wake the readers.  This is the wakeup the old
     * version only printed. */
    ipc_wakeup(IPC_WCHAN_MSG_ARRIVE(q));

    return (int)count;
}

/* ── Algorithm 4 — uiox_kix_msg_rcv ────────────────────────────────────────────
 * Bach's three selection modes, verbatim:
 *
 *   type == 0   the FIRST message on the queue
 *   type >  0   the first message of THAT type
 *   type <  0   the LOWEST-typed message whose type <= |type|
 *
 * The third is the one people get wrong. It is not "any message below
 * the threshold" — it is the smallest such type, so a queue holding
 * types 3, 7 and 9 with type=-8 returns the 7, not the 3.
 *
 * ── what changed ────────────────────────────────────────────────────
 * An oversized message no longer jams the queue.  If MSG_NOERROR is set
 * the message is TRUNCATED and consumed, which is the whole point of the
 * flag; without it the message stays and -1 is returned, as POSIX
 * requires.  The old version had neither branch, so a single message
 * larger than every reader's buffer blocked the queue permanently. */
int uiox_kix_msg_rcv(int msgqid, Msg *out_msg, uiox_size_t maxcount,
           long type, int flag, uiox_kix_psa_proc_t *receiver)
{
    MsgQueue *q;
    MsgNode  *chosen = (MsgNode *)0;
    MsgNode  *cprev  = (MsgNode *)0;
    int       received;

    if (msgqid < 0 || msgqid >= IPC_MAX_QUEUES) return -1;
    if (!queues[msgqid].active || !out_msg) return -1;

    q = &queues[msgqid];

    /* Wait for a message that matches. */
    for (;;) {
        MsgNode *n;
        MsgNode *pn = (MsgNode *)0;

        chosen = (MsgNode *)0;
        cprev  = (MsgNode *)0;

        if (type == 0) {
            chosen = q->first;
        } else if (type > 0) {
            for (n = q->first; n; pn = n, n = n->next) {
                if (n->msg.mtype == type) { chosen = n; cprev = pn; break; }
            }
        } else {
            long best = -type + 1;
            for (n = q->first; n; pn = n, n = n->next) {
                if (n->msg.mtype <= -type && n->msg.mtype < best) {
                    best   = n->msg.mtype;
                    chosen = n;
                    cprev  = pn;
                }
            }
        }

        if (chosen) break;

        if (flag & IPC_NOWAIT) return -1;

        {
            int rc = ipc_sleep(receiver, IPC_WCHAN_MSG_ARRIVE(q));
            if (rc < 0) return -1;                    /* signal — unwind */
            if (!queues[msgqid].active) return -1;    /* queue removed   */
        }
    }

    /* The selected message may be larger than the caller's buffer. */
    if (chosen->msg.msize > maxcount) {
        if (!(flag & MSG_NOERROR)) return -1;   /* leave it, report -1 */

        /* Truncate and consume — MSG_NOERROR's purpose.  Returning -1
         * here instead would jam every reader behind it. */
        received = (int)maxcount;
    } else {
        received = (int)chosen->msg.msize;
    }

    out_msg->mtype = chosen->msg.mtype;
    out_msg->msize = (uiox_size_t)received;
    memcpy(out_msg->mtext, chosen->msg.mtext, (uiox_size_t)received);

    if (cprev) cprev->next = chosen->next;
    else       q->first    = chosen->next;
    if (q->last == chosen) q->last = cprev;

    q->msg_count--;
    q->msg_bytes     -= chosen->msg.msize;
    q->last_recv_pid  = receiver ? (int)receiver->p_pid : 0;
    q->rcv_time       = (time_t)jiffies;

    node_free(chosen);

    /* Space appeared — wake the senders.  This is the call the old
     * version's printed message described and nothing made. */
    ipc_wakeup(IPC_WCHAN_MSG_SPACE(q));

    return received;
}
