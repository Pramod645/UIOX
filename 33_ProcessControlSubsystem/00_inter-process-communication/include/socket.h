#ifndef UIOX_SOCKET_H
#define UIOX_SOCKET_H

#include "ipc_types.h"

/* ─────────────────────────────────────────────────────────────
 * Socket domains, types, protocols
 * ───────────────────────────────────────────────────────────── */
typedef enum {
    AF_UNIX    = 1,   /* UNIX system domain (same machine)       */
    AF_INET    = 2    /* Internet domain (across network)        */
} SockDomain;

typedef enum {
    SOCK_STREAM = 1,  /* virtual circuit (TCP)                   */
    SOCK_DGRAM  = 2   /* datagram (UDP)                          */
} SockType;

typedef enum {
    PROTO_TCP = 6,
    PROTO_UDP = 17,
    PROTO_DEFAULT = 0
} SockProto;

/* shutdown mode */
#define SHUT_RD   0
#define SHUT_WR   1
#define SHUT_RDWR 2

/* send/recv flags */
#define MSG_OOB   0x01   /* out-of-band data                     */
#define MSG_PEEK  0x02   /* peek without consuming               */

/* ─────────────────────────────────────────────────────────────
 * Socket address (simplified)
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    SockDomain  domain;
    char        path[64];  /* UNIX: file path; INET: "ip:port"   */
} SockAddr;

/* ─────────────────────────────────────────────────────────────
 * Socket state
 * ───────────────────────────────────────────────────────────── */
typedef enum {
    SS_UNCONNECTED = 0,
    SS_BOUND,
    SS_LISTENING,
    SS_CONNECTING,
    SS_CONNECTED,
    SS_DISCONNECTED
} SockState;

/* ─────────────────────────────────────────────────────────────
 * Internal socket descriptor
 *
 * Three logical layers:
 *   socket  layer  — this structure (system-call interface)
 *   protocol layer — proto field (TCP/UDP behaviour)
 *   device  layer  — send/recv buffers represent the Eth driver
 * ───────────────────────────────────────────────────────────── */
#define SOCK_BUF_SIZE 4096

typedef struct Socket {
    int          sd;
    SockDomain   domain;
    SockType     type;
    SockProto    proto;
    SockState    state;
    SockAddr     local_addr;
    SockAddr     peer_addr;
    bool         active;

    /* ── Protocol layer (TCP/UDP) ─────────────────────────── */
    bool         can_send;
    bool         can_recv;

    /* ── Device layer (Eth driver / ring buffers) ─────────── */
    uint8_t      send_buf[SOCK_BUF_SIZE];
    size_t       send_len;
    uint8_t      recv_buf[SOCK_BUF_SIZE];
    size_t       recv_len;
    int          recv_head;

    /* ── Server listen backlog ────────────────────────────── */
    int          pending[IPC_MAX_PENDING]; /* pending conn SDs  */
    int          pending_count;
    int          backlog;

    /* ── Out-of-band data ─────────────────────────────────── */
    uint8_t      oob_byte;
    bool         oob_present;

    /* ── Paired socket (for accept()) ────────────────────── */
    int          peer_sd;

    int          owner_pid;
} Socket;

/* ─────────────────────────────────────────────────────────────
 * Socket layer API
 * ───────────────────────────────────────────────────────────── */
void socket_subsystem_init(void);

/* Create a socket endpoint */
int  sys_socket(SockDomain domain, SockType type, SockProto proto);

/* Associate a name/address with a socket */
int  sys_bind(int sd, const SockAddr *addr);

/* Client: request connection to server */
int  sys_connect(int sd, const SockAddr *addr);

/* Server: declare willingness to accept, set backlog */
int  sys_listen(int sd, int backlog);

/* Server: accept one pending connection; returns new sd */
int  sys_accept(int sd, SockAddr *client_addr);

/* Send data; flag MSG_OOB for out-of-band */
int  sys_send(int sd, const void *msg, size_t length, int flags);

/* Receive data; flag MSG_PEEK to inspect without consuming */
int  sys_recv(int sd, void *buf, size_t length, int flags);

/* Datagram variants */
int  sys_sendto(int sd, const void *msg, size_t len, int flags,
                const SockAddr *dest);
int  sys_recvfrom(int sd, void *buf, size_t len, int flags,
                  SockAddr *src);

/* Partial or full shutdown */
int  sys_shutdown(int sd, int how);

/* Close and free the socket descriptor */
int  sys_close_socket(int sd);

/* Query / option management */
int  sys_getsockname(int sd, SockAddr *name);
int  sys_getsockopt(int sd, int optname, void *optval, size_t *optlen);
int  sys_setsockopt(int sd, int optname, const void *optval, size_t optlen);

#endif /* UIOX_SOCKET_H */
