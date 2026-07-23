#include "socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ─────────────────────────────────────────────────────────────
 * Global socket table
 * ───────────────────────────────────────────────────────────── */
static Socket sockets[IPC_MAX_SOCKETS];

void socket_subsystem_init(void)
{
    memset(sockets, 0, sizeof sockets);
    printf("[socket] init: max_sockets=%d\n", IPC_MAX_SOCKETS);
}

/* ─────────────────────────────────────────────────────────────
 * Internal helpers
 * ───────────────────────────────────────────────────────────── */
static Socket *get_socket(int sd)
{
    if (sd < 0 || sd >= IPC_MAX_SOCKETS || !sockets[sd].active) {
        fprintf(stderr, "[socket] invalid sd %d\n", sd);
        return NULL;
    }
    return &sockets[sd];
}

static int alloc_sd(void)
{
    for (int i = 0; i < IPC_MAX_SOCKETS; i++)
        if (!sockets[i].active) return i;
    return -1;
}

/* ─────────────────────────────────────────────────────────────
 * sys_socket
 * Socket layer: allocate descriptor and select protocol.
 * ───────────────────────────────────────────────────────────── */
int sys_socket(SockDomain domain, SockType type, SockProto proto)
{
    int sd = alloc_sd();
    if (sd < 0) { fprintf(stderr,"[socket] no free descriptors\n"); return -1; }

    Socket *s    = &sockets[sd];
    s->sd        = sd;
    s->domain    = domain;
    s->type      = type;
    s->state     = SS_UNCONNECTED;
    s->active    = true;
    s->can_send  = true;
    s->can_recv  = true;
    s->peer_sd   = -1;
    s->backlog   = 0;

    /* Protocol layer: select default if PROTO_DEFAULT */
    if (proto == PROTO_DEFAULT)
        s->proto = (type == SOCK_STREAM) ? PROTO_TCP : PROTO_UDP;
    else
        s->proto = proto;

    printf("[socket] created sd=%d  domain=%s  type=%s  proto=%s\n",
           sd,
           domain == AF_UNIX ? "AF_UNIX" : "AF_INET",
           type   == SOCK_STREAM ? "STREAM" : "DGRAM",
           s->proto == PROTO_TCP ? "TCP" : "UDP");
    return sd;
}

/* ─────────────────────────────────────────────────────────────
 * sys_bind
 * Associate an address name with the socket.
 * Server processes bind to advertise themselves.
 * ───────────────────────────────────────────────────────────── */
int sys_bind(int sd, const SockAddr *addr)
{
    Socket *s = get_socket(sd);
    if (!s || !addr) return -1;

    s->local_addr = *addr;
    s->state      = SS_BOUND;

    printf("[socket] bind: sd=%d  addr='%s'\n", sd, addr->path);
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * sys_listen
 * Server: mark socket as passive, set backlog queue length.
 * ───────────────────────────────────────────────────────────── */
int sys_listen(int sd, int backlog)
{
    Socket *s = get_socket(sd);
    if (!s) return -1;
    if (s->type != SOCK_STREAM) {
        fprintf(stderr, "[socket] listen: not a stream socket\n");
        return -1;
    }

    s->backlog = backlog < IPC_MAX_PENDING ? backlog : IPC_MAX_PENDING;
    s->state   = SS_LISTENING;
    printf("[socket] listen: sd=%d  backlog=%d\n", sd, s->backlog);
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * sys_connect
 * Client: request connection.
 * For datagrams: merely records the peer address.
 * For streams (TCP): simulates the handshake.
 * ───────────────────────────────────────────────────────────── */
int sys_connect(int sd, const SockAddr *addr)
{
    Socket *s = get_socket(sd);
    if (!s || !addr) return -1;

    s->peer_addr = *addr;

    if (s->type == SOCK_DGRAM) {
        printf("[socket] connect (dgram): sd=%d  peer='%s' "
               "(address recorded only)\n", sd, addr->path);
        s->state = SS_CONNECTED;
        return 0;
    }

    /* Stream: find listening server socket */
    for (int i = 0; i < IPC_MAX_SOCKETS; i++) {
        Socket *srv = &sockets[i];
        if (!srv->active) continue;
        if (srv->state != SS_LISTENING) continue;
        if (strcmp(srv->local_addr.path, addr->path) != 0) continue;

        /* Enqueue in server's pending list */
        if (srv->pending_count >= srv->backlog) {
            fprintf(stderr, "[socket] connect: server backlog full\n");
            return -1;
        }
        srv->pending[srv->pending_count++] = sd;
        s->state   = SS_CONNECTING;
        s->peer_sd = i;
        printf("[socket] connect: sd=%d → server sd=%d  queued\n", sd, i);
        return 0;
    }

    fprintf(stderr, "[socket] connect: no server at '%s'\n", addr->path);
    return -1;
}

/* ─────────────────────────────────────────────────────────────
 * sys_accept
 * Server: dequeue one pending connection; return new socket.
 * The original listening sd remains available for more accepts.
 * ───────────────────────────────────────────────────────────── */
int sys_accept(int sd, SockAddr *client_addr)
{
    Socket *srv = get_socket(sd);
    if (!srv || srv->state != SS_LISTENING) {
        fprintf(stderr, "[socket] accept: sd=%d not listening\n", sd);
        return -1;
    }

    if (srv->pending_count == 0) {
        fprintf(stderr, "[socket] accept: no pending connections on sd=%d\n",
                sd);
        return -1;
    }

    /* Dequeue first pending client */
    int client_sd = srv->pending[0];
    memmove(&srv->pending[0], &srv->pending[1],
            (size_t)(srv->pending_count - 1) * sizeof srv->pending[0]);
    srv->pending_count--;

    /* Allocate new server-side socket for this connection */
    int nsd = alloc_sd();
    if (nsd < 0) { fprintf(stderr,"[socket] accept: no free sd\n"); return -1; }

    Socket *ns   = &sockets[nsd];
    *ns          = *srv;             /* inherit domain/proto/type  */
    ns->sd       = nsd;
    ns->state    = SS_CONNECTED;
    ns->backlog  = 0;
    ns->pending_count = 0;
    ns->peer_sd  = client_sd;
    ns->send_len = 0;
    ns->recv_len = 0;

    /* Connect the client side to this new socket */
    Socket *cl = get_socket(client_sd);
    if (cl) { cl->state = SS_CONNECTED; cl->peer_sd = nsd; }

    if (client_addr && cl)
        *client_addr = cl->local_addr;

    printf("[socket] accept: listening sd=%d  "
           "new connection sd=%d  client sd=%d\n",
           sd, nsd, client_sd);
    return nsd;
}

/* ─────────────────────────────────────────────────────────────
 * sys_send
 * Socket layer → protocol layer → device layer (send buffer).
 * MSG_OOB: bypass regular data sequence.
 * ───────────────────────────────────────────────────────────── */
int sys_send(int sd, const void *msg, size_t length, int flags)
{
    Socket *s = get_socket(sd);
    if (!s || !msg) return -1;
    if (!s->can_send) { fprintf(stderr,"[socket] send: shut down\n"); return -1; }

    if (flags & MSG_OOB) {
        /* Out-of-band: single urgent byte */
        s->oob_byte    = ((const uint8_t*)msg)[0];
        s->oob_present = true;
        printf("[socket] send OOB: sd=%d  byte=0x%02x\n", sd, s->oob_byte);
        return 1;
    }

    /* Protocol layer: TCP/UDP segmentation (simulated) */
    size_t actual = length < SOCK_BUF_SIZE - s->send_len
                    ? length : SOCK_BUF_SIZE - s->send_len;

    if (actual == 0) {
        fprintf(stderr, "[socket] send: send buffer full\n");
        return -1;
    }

    /* Device layer: copy to send ring buffer */
    memcpy(s->send_buf + s->send_len, msg, actual);
    s->send_len += actual;

    /* ── Deliver to peer's recv buffer (loopback simulation) ── */
    if (s->peer_sd >= 0) {
        Socket *peer = get_socket(s->peer_sd);
        if (peer) {
            size_t room = SOCK_BUF_SIZE - peer->recv_len;
            size_t copy = actual < room ? actual : room;
            memcpy(peer->recv_buf + peer->recv_len,
                   s->send_buf + s->send_len - actual, copy);
            peer->recv_len += copy;
        }
    }

    printf("[socket] send: sd=%d  bytes=%zu  proto=%s  total_pending=%zu\n",
           sd, actual, s->proto == PROTO_TCP ? "TCP" : "UDP", s->send_len);
    return (int)actual;
}

/* ─────────────────────────────────────────────────────────────
 * sys_recv
 * MSG_PEEK: inspect without consuming.
 * ───────────────────────────────────────────────────────────── */
int sys_recv(int sd, void *buf, size_t length, int flags)
{
    Socket *s = get_socket(sd);
    if (!s || !buf) return -1;
    if (!s->can_recv) { fprintf(stderr,"[socket] recv: shut down\n"); return -1; }

    /* Check for OOB first */
    if ((flags & MSG_OOB) && s->oob_present) {
        ((uint8_t*)buf)[0] = s->oob_byte;
        if (!(flags & MSG_PEEK)) s->oob_present = false;
        printf("[socket] recv OOB: sd=%d  byte=0x%02x\n", sd, s->oob_byte);
        return 1;
    }

    if (s->recv_len == 0) {
        printf("[socket] recv: sd=%d  no data\n", sd);
        return 0;
    }

    size_t actual = length < s->recv_len ? length : s->recv_len;
    memcpy(buf, s->recv_buf, actual);

    if (!(flags & MSG_PEEK)) {
        /* Consume data from recv buffer */
        memmove(s->recv_buf, s->recv_buf + actual, s->recv_len - actual);
        s->recv_len -= actual;
        printf("[socket] recv: sd=%d  bytes=%zu  remaining=%zu\n",
               sd, actual, s->recv_len);
    } else {
        printf("[socket] recv PEEK: sd=%d  bytes=%zu (not consumed)\n",
               sd, actual);
    }

    return (int)actual;
}

/* ─────────────────────────────────────────────────────────────
 * sys_sendto / sys_recvfrom  (datagram with explicit address)
 * ───────────────────────────────────────────────────────────── */
int sys_sendto(int sd, const void *msg, size_t len, int flags,
               const SockAddr *dest)
{
    Socket *s = get_socket(sd);
    if (!s || !dest) return -1;
    /* Record destination and delegate to sys_send */
    s->peer_addr = *dest;
    printf("[socket] sendto: sd=%d  dest='%s'\n", sd, dest->path);
    return sys_send(sd, msg, len, flags);
}

int sys_recvfrom(int sd, void *buf, size_t len, int flags, SockAddr *src)
{
    Socket *s = get_socket(sd);
    if (!s) return -1;
    int n = sys_recv(sd, buf, len, flags);
    if (src && n > 0) *src = s->peer_addr;
    return n;
}

/* ─────────────────────────────────────────────────────────────
 * sys_shutdown
 * Close one or both directions; leave descriptor intact.
 * ───────────────────────────────────────────────────────────── */
int sys_shutdown(int sd, int how)
{
    Socket *s = get_socket(sd);
    if (!s) return -1;

    if (how == SHUT_RD  || how == SHUT_RDWR) s->can_recv = false;
    if (how == SHUT_WR  || how == SHUT_RDWR) s->can_send = false;

    s->state = SS_DISCONNECTED;

    printf("[socket] shutdown: sd=%d  mode=%s\n", sd,
           how == SHUT_RD   ? "RD" :
           how == SHUT_WR   ? "WR" : "RDWR");
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * sys_close_socket
 * ───────────────────────────────────────────────────────────── */
int sys_close_socket(int sd)
{
    Socket *s = get_socket(sd);
    if (!s) return -1;
    printf("[socket] close: sd=%d\n", sd);
    memset(s, 0, sizeof *s);
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * sys_getsockname
 * ───────────────────────────────────────────────────────────── */
int sys_getsockname(int sd, SockAddr *name)
{
    Socket *s = get_socket(sd);
    if (!s || !name) return -1;
    *name = s->local_addr;
    printf("[socket] getsockname: sd=%d  name='%s'\n", sd, name->path);
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * sys_getsockopt / sys_setsockopt  (stub)
 * ───────────────────────────────────────────────────────────── */
int sys_getsockopt(int sd, int optname, void *optval, size_t *optlen)
{
    (void)optval; (void)optlen;
    printf("[socket] getsockopt: sd=%d  opt=%d\n", sd, optname);
    return 0;
}

int sys_setsockopt(int sd, int optname, const void *optval, size_t optlen)
{
    (void)optval; (void)optlen;
    printf("[socket] setsockopt: sd=%d  opt=%d\n", sd, optname);
    return 0;
}
