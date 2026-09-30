/*
 * 30KIX/33PCS/00IPC/src/socket.c
 *
 * Sockets — socket, bind, listen, connect, accept, send, recv, shutdown,
 * close.
 *
 * ── version 3.0.0: two fixes ────────────────────────────────────────
 * 1. socket_init() is now uiox_kix_socket_init(), matching what
 *    socket.h declares.  Two names for one function meant a caller
 *    including the header got an undefined symbol at link.
 *
 * 2. recv_head was declared in the struct and never used.  The receive
 *    buffer was therefore treated as LINEAR, and every read compacted
 *    the remainder with memmove.  The field is removed rather than
 *    left as a decoy: a reader who saw it would reasonably assume the
 *    ring was indexed, and write to it accordingly.
 *
 * ── no process type here ────────────────────────────────────────────
 * This file never took a SimProcess *.  owner_pid is an int — a pid
 * COPY, since a socket outlives the process that opened it.  So of the
 * five sources in this layer, shm.c and socket.c are the two a process
 * merge does not touch, and that is recorded so a future reader does not
 * go looking for a change that is not there.
 *
 * ── the three layers ────────────────────────────────────────────────
 *   socket   layer — sd, state, addresses
 *   protocol layer — proto, can_send, can_recv
 *   device   layer — send_buf / recv_buf, standing in for a real driver
 *
 * The loopback delivery in sys_send is the device layer's stand-in: it
 * copies into the peer's receive buffer directly, because there is no
 * driver below to hand a frame to.
 *
 * ── declared but not defined ────────────────────────────────────────
 * socket.h declares sys_sendto, sys_recvfrom, sys_getsockname,
 * sys_getsockopt and sys_setsockopt.  NONE of them is implemented here —
 * the header says so, and a caller of any gets an undefined symbol.
 * They are named at the end of this file rather than silently omitted.
 *
 * @version 3.0.0  @date 2026-09-30
 */
#include "../include/uiox_kix_socket.h"
#include "uiox_klibc.h"

/* ── Socket table ────────────────────────────────────────────────────── */
static Socket sockets[IPC_MAX_SOCKETS];

/* ── uiox_kix_socket_init ───────────────────────────────────────────── */
void uiox_kix_socket_init(void)
{
    int i;
    for (i = 0; i < IPC_MAX_SOCKETS; i++)
        memset(&sockets[i], 0, sizeof sockets[i]);
}

/* ── get_socket ──────────────────────────────────────────────────────── */
static Socket *get_socket(int sd)
{
    if (sd < 0 || sd >= IPC_MAX_SOCKETS) return (Socket *)0;
    if (!sockets[sd].active) return (Socket *)0;
    return &sockets[sd];
}

/* ── alloc_sd ────────────────────────────────────────────────────────── */
static int alloc_sd(void)
{
    int i;
    for (i = 0; i < IPC_MAX_SOCKETS; i++)
        if (!sockets[i].active) return i;
    return -1;   /* table full */
}

/* ── sys_socket ──────────────────────────────────────────────────────
 * PROTO_DEFAULT resolves from the type: STREAM implies TCP, DGRAM UDP.
 * A caller that names a protocol gets what it asked for, even if the
 * pair is unusual — this layer does not second-guess. */
int sys_socket(SockDomain domain, SockType type, SockProto proto)
{
    int     sd;
    Socket *s;

    sd = alloc_sd();
    if (sd < 0) return -1;

    s = &sockets[sd];

    s->sd        = sd;
    s->domain    = domain;
    s->type      = type;
    s->state     = SS_UNCONNECTED;
    s->active    = true;
    s->can_send  = true;
    s->can_recv  = true;
    s->peer_sd   = -1;
    s->backlog   = 0;
    s->send_len  = 0u;
    s->recv_len  = 0u;

    if (proto == PROTO_DEFAULT)
        s->proto = (type == SOCK_STREAM) ? PROTO_TCP : PROTO_UDP;
    else
        s->proto = proto;

    return sd;
}

/* ── sys_bind ────────────────────────────────────────────────────────── */
int sys_bind(int sd, const SockAddr *addr)
{
    Socket *s = get_socket(sd);
    if (!s || !addr) return -1;

    s->local_addr = *addr;
    s->state      = SS_BOUND;
    return 0;
}

/* ── sys_listen ──────────────────────────────────────────────────────
 * Only a stream socket can listen — a datagram socket has no connection
 * to queue, so the refusal is a real check rather than a formality. */
int sys_listen(int sd, int backlog)
{
    Socket *s = get_socket(sd);
    if (!s) return -1;
    if (s->type != SOCK_STREAM) return -1;

    s->state   = SS_LISTENING;
    s->backlog = backlog < IPC_MAX_PENDING ? backlog : IPC_MAX_PENDING;
    return 0;
}

/* ── sys_connect ─────────────────────────────────────────────────────
 * Finds a listening socket whose address matches, and enqueues this
 * socket in its pending list.
 *
 * The address comparison walks up to 16 bytes and stops when BOTH
 * strings have ended — which is why it checks both, not just the
 * caller's. A path prefix of a longer server path must NOT match. */
int sys_connect(int sd, const SockAddr *addr)
{
    Socket *s = get_socket(sd);
    int     i;

    if (!s || !addr) return -1;

    for (i = 0; i < IPC_MAX_SOCKETS; i++) {
        Socket *srv = &sockets[i];
        int     match = 1;
        int     j;

        if (!srv->active) continue;
        if (srv->state != SS_LISTENING) continue;
        if (srv->domain != s->domain) continue;

        for (j = 0; j < 16; j++) {
            if (addr->path[j] != srv->local_addr.path[j]) { match = 0; break; }
            if (addr->path[j] == '\0') break;      /* both ended together */
        }
        if (!match) continue;

        if (srv->pending_count >= srv->backlog) return -1;

        srv->pending[srv->pending_count++] = sd;
        s->state   = SS_CONNECTING;
        s->peer_sd = i;
        return 0;
    }
    return -1;   /* no server at that address */
}

/* ── sys_accept ──────────────────────────────────────────────────────
 * Dequeues the first pending client and creates a NEW socket for the
 * server side of the connection, inheriting domain/type/proto.
 *
 * The listening socket keeps its own state — it must remain LISTENING
 * after an accept, or the server can take exactly one connection. */
int sys_accept(int sd, SockAddr *client_addr)
{
    Socket *srv;
    Socket *ns;
    Socket *cli;
    int     client_sd;
    int     nsd;

    srv = get_socket(sd);
    if (!srv || srv->state != SS_LISTENING) return -1;
    if (srv->pending_count == 0) return -1;

    client_sd = srv->pending[0];

    memmove(&srv->pending[0], &srv->pending[1],
            (uiox_size_t)(srv->pending_count - 1) * sizeof srv->pending[0]);
    srv->pending_count--;

    nsd = alloc_sd();
    if (nsd < 0) return -1;

    ns = &sockets[nsd];
    *ns = *srv;                  /* inherit domain, type, proto, addr */
    ns->sd            = nsd;
    ns->state         = SS_CONNECTED;
    ns->peer_sd       = client_sd;
    ns->active        = true;
    ns->pending_count = 0;
    ns->send_len      = 0u;
    ns->recv_len      = 0u;
    ns->can_send      = true;
    ns->can_recv      = true;

    cli = &sockets[client_sd];
    cli->state   = SS_CONNECTED;
    cli->peer_sd = nsd;

    if (client_addr) *client_addr = cli->local_addr;

    return nsd;
}

/* ── sys_send ────────────────────────────────────────────────────────
 * Appends to the send buffer, then delivers to the peer's receive
 * buffer — the device layer's stand-in for a real driver.
 *
 * Two capacity checks, and they are different: the send buffer may be
 * full (this socket's own limit), and separately the PEER's receive
 * buffer may be short.  The second truncates the delivery rather than
 * failing the send, which is what a stream socket does. */
int sys_send(int sd, const void *msg, uiox_size_t length, int flags)
{
    Socket *s;
    uiox_size_t actual;

    s = get_socket(sd);
    if (!s || !msg) return -1;
    if (!s->can_send) return -1;              /* shut down for writing */

    if (flags & MSG_OOB) {
        /* One urgent byte, delivered ahead of the stream. */
        s->oob_byte    = ((const uiox_uint8_t *)msg)[0];
        s->oob_present = true;
        return 1;
    }

    actual = length < SOCK_BUF_SIZE - s->send_len
             ? length : SOCK_BUF_SIZE - s->send_len;
    if (actual == 0u) return -1;              /* send buffer full */

    memcpy(s->send_buf + s->send_len, msg, actual);
    s->send_len += actual;

    if (s->peer_sd >= 0) {
        Socket *peer = get_socket(s->peer_sd);
        if (peer) {
            uiox_size_t room = SOCK_BUF_SIZE - peer->recv_len;
            uiox_size_t copy = actual < room ? actual : room;

            memcpy(peer->recv_buf + peer->recv_len,
                   s->send_buf + s->send_len - actual, copy);
            peer->recv_len += copy;
        }
    }

    return (int)actual;
}

/* ── sys_recv ────────────────────────────────────────────────────────
 * OOB data is delivered FIRST when asked for, ahead of the stream.
 *
 * Without MSG_PEEK the buffer is compacted after the read — the linear
 * treatment that recv_head's removal makes explicit.  MSG_PEEK reads
 * without moving anything, so a second call sees the same bytes. */
int sys_recv(int sd, void *buf, uiox_size_t length, int flags)
{
    Socket *s;
    uiox_size_t actual;

    s = get_socket(sd);
    if (!s || !buf) return -1;
    if (!s->can_recv) return -1;

    if ((flags & MSG_OOB) && s->oob_present) {
        ((uiox_uint8_t *)buf)[0] = s->oob_byte;
        s->oob_present = false;
        return 1;
    }

    if (s->recv_len == 0u) return 0;          /* nothing available */

    actual = length < s->recv_len ? length : s->recv_len;
    memcpy(buf, s->recv_buf, actual);

    if (flags & MSG_PEEK) return (int)actual;

    memmove(s->recv_buf, s->recv_buf + actual, s->recv_len - actual);
    s->recv_len -= actual;

    return (int)actual;
}

/* ── sys_shutdown ────────────────────────────────────────────────────
 * SHUT_RD stops reading, SHUT_WR stops writing, SHUT_RDWR both.  The
 * socket is not freed — sys_close_socket does that. */
int sys_shutdown(int sd, int how)
{
    Socket *s = get_socket(sd);
    if (!s) return -1;

    if (how == SHUT_RD || how == SHUT_RDWR) s->can_recv = false;
    if (how == SHUT_WR || how == SHUT_RDWR) s->can_send = false;

    return 0;
}

/* ── sys_close_socket ────────────────────────────────────────────────
 * Clears the descriptor and tells the peer, so a process blocked reading
 * at the other end does not wait for a socket that no longer exists. */
int sys_close_socket(int sd)
{
    Socket *s = get_socket(sd);
    if (!s) return -1;

    if (s->peer_sd >= 0 && s->peer_sd < IPC_MAX_SOCKETS) {
        sockets[s->peer_sd].can_recv = false;
        sockets[s->peer_sd].peer_sd  = -1;
    }

    memset(s, 0, sizeof *s);
    return 0;
}

/* ── Declared in socket.h, NOT defined here ──────────────────────────
 *   sys_sendto      datagram send with a destination
 *   sys_recvfrom    datagram receive with a source
 *   sys_getsockname local address of a socket
 *   sys_getsockopt  option read
 *   sys_setsockopt  option write
 *
 * Each needs either a datagram path or an option table, neither of which
 * exists.  Named here so the gap is visible at the file rather than at
 * a link error.
 */
