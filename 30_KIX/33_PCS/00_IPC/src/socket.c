#include "socket.h"
#include "../include/uiox_klibc.h"
/*
 * 30_KIX/33_PCS/00_IPC/src/socket.c
 *
 * Freestanding fixes (v2.0)
 * ─────────────────────────
 *   REMOVED: #include <stdio.h>  <stdlib.h>  <string.h>
 *            All provided through socket.h → ipc_types.h → uiox_klibc.h
 *
 *   FIXED: fprintf(stderr, ...) → printf(...)   (all occurrences)
 *   FIXED: %zu → %llu + (unsigned long long) cast (5 occurrences)
 *
 * No algorithm changes — all socket logic identical to original.
 *
 * @version 2.0.0  @date 2026-07-23
 */
 
 /* ── Global socket table ─────────────────────────────────────────────── */
 static Socket sockets[IPC_MAX_SOCKETS];
 
 /* ── socket_init ─────────────────────────────────────────────────────── */
 void socket_init(void)
 {
     memset(sockets, 0, sizeof sockets);
     printf("[socket] init: max_sockets=%d\n", IPC_MAX_SOCKETS);
 }
 
 /* ── Internal: get socket by descriptor ──────────────────────────────── */
 static Socket *get_socket(int sd)
 {
     if (sd < 0 || sd >= IPC_MAX_SOCKETS || !sockets[sd].active) {
         printf("[socket] ERROR: invalid sd %d\n", sd); /* was: fprintf(stderr,...) */
         return (Socket *)0;
     }
     return &sockets[sd];
 }
 
 /* ── Internal: allocate a free descriptor ────────────────────────────── */
 static int alloc_sd(void)
 {
     int i;
     for (i = 0; i < IPC_MAX_SOCKETS; i++)
         if (!sockets[i].active) return i;
     return -1;
 }
 
 /* ── sys_socket ──────────────────────────────────────────────────────── */
 int sys_socket(SockDomain domain, SockType type, SockProto proto)
 {
     int sd = alloc_sd();
     if (sd < 0) {
         printf("[socket] ERROR: no free descriptors\n"); /* was: fprintf(stderr,...) */
         return -1;
     }
     Socket *s   = &sockets[sd];
     s->sd       = sd;
     s->domain   = domain;
     s->type     = type;
     s->state    = SS_UNCONNECTED;
     s->active   = true;
     s->can_send = true;
     s->can_recv = true;
     s->peer_sd  = -1;
     s->backlog  = 0;
 
     if (proto == PROTO_DEFAULT)
         s->proto = (type == SOCK_STREAM) ? PROTO_TCP : PROTO_UDP;
     else
         s->proto = proto;
 
     printf("[socket] sys_socket: sd=%d  domain=%d  type=%d  proto=%s\n",
            sd, (int)domain, (int)type,
            s->proto == PROTO_TCP ? "TCP" : "UDP");
     return sd;
 }
 
 /* ── sys_bind ────────────────────────────────────────────────────────── */
 int sys_bind(int sd, const SockAddr *addr)
 {
     Socket *s = get_socket(sd);
     if (!s || !addr) return -1;
     s->local_addr = *addr;
     s->state      = SS_BOUND;
     printf("[socket] bind: sd=%d  path='%s'\n", sd, addr->path);
     return 0;
 }
 
 /* ── sys_listen ──────────────────────────────────────────────────────── */
 int sys_listen(int sd, int backlog)
 {
     Socket *s = get_socket(sd);
     if (!s) return -1;
     if (s->type != SOCK_STREAM) {
         printf("[socket] ERROR: listen: not a stream socket\n"); /* was: fprintf(stderr,...) */
         return -1;
     }
     s->state   = SS_LISTENING;
     s->backlog = backlog < IPC_MAX_PENDING ? backlog : IPC_MAX_PENDING;
     printf("[socket] listen: sd=%d  backlog=%d\n", sd, s->backlog);
     return 0;
 }
 
 /* ── sys_connect ─────────────────────────────────────────────────────── */
 int sys_connect(int sd, const SockAddr *addr)
 {
     int i;
     Socket *s = get_socket(sd);
     if (!s || !addr) return -1;
 
     /* Find listening server with matching address */
     for (i = 0; i < IPC_MAX_SOCKETS; i++) {
         Socket *srv = &sockets[i];
         if (!srv->active || srv->state != SS_LISTENING) continue;
         if (srv->domain != s->domain) continue;
 
         /* Address match (simplified: compare path string) */
         int match = 1;
         int j;
         for (j = 0; j < 16 && (addr->path[j] || srv->local_addr.path[j]); j++) {
             if (addr->path[j] != srv->local_addr.path[j]) { match = 0; break; }
         }
         if (!match) continue;
 
         /* Enqueue in server's pending list */
         if (srv->pending_count >= srv->backlog) {
             printf("[socket] ERROR: connect: server backlog full\n"); /* was: fprintf(stderr,...) */
             return -1;
         }
         srv->pending[srv->pending_count++] = sd;
         s->state   = SS_CONNECTING;
         s->peer_sd = i;
 
         /* Wake the server (simulation) */
         printf("[socket] connect: sd=%d → server sd=%d  path='%s'\n",
                sd, i, addr->path);
         return 0;
     }
 
     printf("[socket] ERROR: connect: no server at '%s'\n", addr->path); /* was: fprintf(stderr,...) */
     return -1;
 }
 
 /* ── sys_accept ──────────────────────────────────────────────────────── */
 int sys_accept(int sd, SockAddr *client_addr)
 {
     Socket *srv = get_socket(sd);
     if (!srv || srv->state != SS_LISTENING) {
         printf("[socket] ERROR: accept: sd=%d not listening\n", sd); /* was: fprintf(stderr,...) */
         return -1;
     }
     if (srv->pending_count == 0) {
         printf("[socket] ERROR: accept: no pending connections on sd=%d\n", sd);
         return -1;
     }
 
     /* Dequeue first pending client */
     int client_sd = srv->pending[0];
     memmove(&srv->pending[0], &srv->pending[1],
             (size_t)(srv->pending_count - 1) * sizeof srv->pending[0]);
     srv->pending_count--;
 
     /* Allocate new server-side socket for this connection */
     int nsd = alloc_sd();
     if (nsd < 0) {
         printf("[socket] ERROR: accept: no free sd\n"); /* was: fprintf(stderr,...) */
         return -1;
     }
     Socket *ns  = &sockets[nsd];
     *ns         = *srv;            /* inherit domain/proto/type */
     ns->sd      = nsd;
     ns->state   = SS_CONNECTED;
     ns->peer_sd = client_sd;
     ns->active  = true;
     ns->pending_count = 0;
 
     /* Complete the client connection */
     Socket *cli = &sockets[client_sd];
     cli->state   = SS_CONNECTED;
     cli->peer_sd = nsd;
 
     if (client_addr) *client_addr = cli->local_addr;
 
     printf("[socket] accept: listening_sd=%d  new_sd=%d  client_sd=%d\n",
            sd, nsd, client_sd);
     return nsd;
 }
 
 /* ── sys_send ────────────────────────────────────────────────────────── */
 int sys_send(int sd, const void *msg, size_t length, int flags)
 {
     Socket *s = get_socket(sd);
     if (!s || !msg) return -1;
     if (!s->can_send) {
         printf("[socket] ERROR: send: shut down\n"); /* was: fprintf(stderr,...) */
         return -1;
     }
 
     if (flags & MSG_OOB) {
         s->oob_byte    = ((const uint8_t *)msg)[0];
         s->oob_present = true;
         printf("[socket] send OOB: sd=%d  byte=0x%02x\n", sd, s->oob_byte);
         return 1;
     }
 
     /* TCP/UDP segmentation (simulated) */
     size_t actual = length < SOCK_BUF_SIZE - s->send_len
                     ? length : SOCK_BUF_SIZE - s->send_len;
     if (actual == 0) {
         printf("[socket] ERROR: send: send buffer full\n"); /* was: fprintf(stderr,...) */
         return -1;
     }
 
     /* Copy to send ring buffer */
     memcpy(s->send_buf + s->send_len, msg, actual);
     s->send_len += actual;
 
     /* Loopback: deliver to peer's recv buffer */
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
 
     printf("[socket] send: sd=%d  bytes=%llu  proto=%s  total_pending=%llu\n",
            sd,
            (unsigned long long)actual,                     /* was: %zu */
            s->proto == PROTO_TCP ? "TCP" : "UDP",
            (unsigned long long)s->send_len);               /* was: %zu */
     return (int)actual;
 }
 
 /* ── sys_recv ────────────────────────────────────────────────────────── */
 int sys_recv(int sd, void *buf, size_t length, int flags)
 {
     Socket *s = get_socket(sd);
     if (!s || !buf) return -1;
     if (!s->can_recv) {
         printf("[socket] ERROR: recv: shut down\n"); /* was: fprintf(stderr,...) */
         return -1;
     }
 
     /* OOB data first */
     if ((flags & MSG_OOB) && s->oob_present) {
         ((uint8_t *)buf)[0] = s->oob_byte;
         s->oob_present = false;
         printf("[socket] recv OOB: sd=%d  byte=0x%02x\n", sd, s->oob_byte);
         return 1;
     }
 
     if (s->recv_len == 0) {
         printf("[socket] recv: sd=%d  no data available\n", sd);
         return 0;
     }
 
     size_t actual = length < s->recv_len ? length : s->recv_len;
     memcpy(buf, s->recv_buf, actual);
 
     if (!(flags & MSG_PEEK)) {
         memmove(s->recv_buf, s->recv_buf + actual, s->recv_len - actual);
         s->recv_len -= actual;
         printf("[socket] recv: sd=%d  bytes=%llu  remaining=%llu\n",
                sd,
                (unsigned long long)actual,                 /* was: %zu */
                (unsigned long long)s->recv_len);           /* was: %zu */
     } else {
         printf("[socket] recv PEEK: sd=%d  bytes=%llu (not consumed)\n",
                sd,
                (unsigned long long)actual);                /* was: %zu */
     }
     return (int)actual;
 }
 
 /* ── sys_shutdown ────────────────────────────────────────────────────── */
 int sys_shutdown(int sd, int how)
 {
     Socket *s = get_socket(sd);
     if (!s) return -1;
     if (how == 0 || how == 2) s->can_recv = false;
     if (how == 1 || how == 2) s->can_send = false;
     printf("[socket] shutdown: sd=%d  how=%d\n", sd, how);
     return 0;
 }
 
 /* ── sys_close_socket ────────────────────────────────────────────────── */
 int sys_close_socket(int sd)
 {
     Socket *s = get_socket(sd);
     if (!s) return -1;
 
     /* Notify peer */
     if (s->peer_sd >= 0 && s->peer_sd < IPC_MAX_SOCKETS) {
         sockets[s->peer_sd].can_recv = false;
         sockets[s->peer_sd].peer_sd  = -1;
     }
 
     memset(s, 0, sizeof *s);
     printf("[socket] close: sd=%d\n", sd);
     return 0;
 }
 