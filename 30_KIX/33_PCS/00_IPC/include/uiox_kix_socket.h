/*
 * 30KIX/33PCS/00IPC/include/uiox_kix_socket.h
 *
 * Sockets — the socket/protocol/device model.
 *
 * ── version 3.0.0 ───────────────────────────────────────────────────
 * This header names NO process type.  It carries owner_pid, an int, and
 * that is a pid COPY for the same reason msg.h's and shm.h's are: a
 * socket outlives the process that opened it, so holding a pointer to a
 * reusable table entry would be worse than holding a number.
 *
 * ── the name mismatch this version fixes ────────────────────────────
 * The header declared uiox_kix_socket_init(); the source defined
 * socket_init().  Two names for one function, so a caller including this
 * header got an undefined symbol at link.  The source now defines
 * uiox_kix_socket_init, matching what is declared here.
 *
 * ── the three layers, as the original banner described them ─────────
 *   socket   layer — the Socket struct: the system-call interface
 *   protocol layer — proto (TCP/UDP behaviour)
 *   device   layer — send_buf / recv_buf: the ring buffers that stand
 *                    in for a real Ethernet driver
 *
 * That layering is why the struct is large and flat rather than split:
 * each field belongs to exactly one layer, and the comments say which.
 *
 * @version 3.0.0  @date 2026-09-30
 */
#ifndef UIOX_SOCKET_H
#define UIOX_SOCKET_H

#include "uiox_kix_ipc_types.h"

/* ── Domains, types, protocols ─────────────────────────────────────── */
typedef enum {
    AF_UNIX = 1,   /* same machine                          */
    AF_INET = 2    /* across a network                      */
} SockDomain;

typedef enum {
    SOCK_STREAM = 1,   /* virtual circuit (TCP)             */
    SOCK_DGRAM  = 2    /* datagram (UDP)                    */
} SockType;

typedef enum {
    PROTO_TCP     = 6,
    PROTO_UDP     = 17,
    PROTO_DEFAULT = 0
} SockProto;

/* shutdown modes */
#define SHUT_RD   0
#define SHUT_WR   1
#define SHUT_RDWR 2

/* send/recv flags */
#define MSG_OOB   0x01   /* out-of-band data        */
#define MSG_PEEK  0x02   /* read without consuming  */

/* ── Address (simplified) ────────────────────────────────────────────
 * A UNIX domain path, or "ip:port" for INET.  One 64-byte field serves
 * both because this build compares paths as strings. */
typedef struct {
    SockDomain   domain;
    char         path[64];
} SockAddr;

/* ── Socket state ──────────────────────────────────────────────────── */
typedef enum {
    SS_UNCONNECTED = 0,
    SS_BOUND,
    SS_LISTENING,
    SS_CONNECTING,
    SS_CONNECTED,
    SS_DISCONNECTED
} SockState;

/* ── The socket descriptor ───────────────────────────────────────────
 * One flat struct, three logical layers, commented per section. */
#define SOCK_BUF_SIZE 4096

typedef struct Socket {
    /* ── socket layer ──────────────────────────────────────────── */
    int         sd;
    SockDomain  domain;
    SockType    type;
    SockProto   proto;
    SockState   state;
    SockAddr    local_addr;
    SockAddr    peer_addr;
    bool        active;

    /* ── protocol layer ────────────────────────────────────────── */
    bool        can_send;
    bool        can_recv;

    /* ── device layer (a real Ethernet driver's ring buffers) ──── */
    uiox_uint8_t send_buf[SOCK_BUF_SIZE];
    uiox_size_t  send_len;
    uiox_uint8_t recv_buf[SOCK_BUF_SIZE];
    uiox_size_t  recv_len;

    /* ── listen backlog ────────────────────────────────────────── */
    int          pending[IPC_MAX_PENDING];   /* awaiting accept */
    int          pending_count;
    int          backlog;

    /* ── out-of-band data ──────────────────────────────────────── */
    uiox_uint8_t oob_byte;
    bool         oob_present;

    /* ── the socket at the other end ───────────────────────────── */
    int          peer_sd;

    /* a pid COPY, not a reference */
    int          owner_pid;
} Socket;

/* ── API ───────────────────────────────────────────────────────────── */

/* Zero the socket table.  Note the name: the SOURCE used to define
 * socket_init, which nothing declared — see the banner. */
void uiox_kix_socket_init(void);

int sys_socket(SockDomain domain, SockType type, SockProto proto);
int sys_bind(int sd, const SockAddr *addr);
int sys_connect(int sd, const SockAddr *addr);
int sys_listen(int sd, int backlog);
int sys_accept(int sd, SockAddr *client_addr);

/* MSG_OOB sends a single urgent byte, delivered ahead of the stream. */
int sys_send(int sd, const void *msg, uiox_size_t length, int flags);

/* MSG_PEEK reads without consuming. */
int sys_recv(int sd, void *buf, uiox_size_t length, int flags);

/* Datagram variants — the header declared these but the source did not
 * define them, so they are listed here as DECLARED ONLY.  Callers of
 * either get an undefined symbol until they are written. */
int sys_sendto(int sd, const void *msg, uiox_size_t len, int flags,
               const SockAddr *dest);
int sys_recvfrom(int sd, void *buf, uiox_size_t len, int flags,
                 SockAddr *src);

int sys_shutdown(int sd, int how);
int sys_close_socket(int sd);

/* Query and options — also declared-only. */
int sys_getsockname(int sd, SockAddr *name);
int sys_getsockopt(int sd, int optname, void *optval, uiox_size_t *optlen);
int sys_setsockopt(int sd, int optname, const void *optval,
                   uiox_size_t optlen);

#endif /* UIOX_SOCKET_H */
