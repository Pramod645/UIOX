/**
 * @file    uiox_socket.h
 * @brief   UIOX BSD-compatible socket API.
 *
 * Provides a POSIX-like socket interface over the UIOX protocol stack.
 * Supports SOCK_STREAM (TCP) and SOCK_DGRAM (UDP) over AF_INET (IPv4).
 *
 * Socket descriptors are indices into a fixed table — no dynamic
 * allocation. Maximum open sockets is UIOX_SOCKET_MAX.
 *
 * @date    2026-05-25
 */
//Layer 4 — Socket API
 #ifndef UIOX_SOCKET_H
 #define UIOX_SOCKET_H
 
 #include "uiox_netbuf.h"
 #include <stdint.h>
 #include <stddef.h>
 #include <stdbool.h>
 #include <sys/types.h> // included for ssize_t

 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Constants
  * ====================================================================== */
 
 #define UIOX_SOCKET_MAX         32      /**< Max simultaneously open sockets  */
 #define UIOX_SOCKET_BACKLOG_MAX 8       /**< Max listen backlog               */
 #define UIOX_SOCKET_RX_QMAX    16       /**< Max RX buffers queued per socket  */
 #define UIOX_SOCKET_INVALID    -1       /**< Invalid socket descriptor         */
 
 /* =========================================================================
  * Address families / socket types  (mirrors POSIX)
  * ====================================================================== */
 
 #define UIOX_AF_INET            2
 #define UIOX_AF_INET6           10
 
 #define UIOX_SOCK_STREAM        1       /**< TCP                               */
 #define UIOX_SOCK_DGRAM         2       /**< UDP                               */
 #define UIOX_SOCK_RAW           3       /**< Raw IP                            */
 
 #define UIOX_IPPROTO_TCP_SOCK   6
 #define UIOX_IPPROTO_UDP_SOCK   17
 
 /* =========================================================================
  * Socket option levels / names
  * ====================================================================== */
 
 #define UIOX_SOL_SOCKET         1
 #define UIOX_SO_REUSEADDR       2
 #define UIOX_SO_KEEPALIVE       9
 #define UIOX_SO_RCVTIMEO        20
 #define UIOX_SO_SNDTIMEO        21
 #define UIOX_SO_SNDBUF          7
 #define UIOX_SO_RCVBUF          8
 
 /* =========================================================================
  * Socket address structures
  * ====================================================================== */
 
 typedef struct {
     uint16_t    family;         /**< UIOX_AF_INET                              */
     uint16_t    port;           /**< Port number (host byte order)             */
     uint32_t    addr;           /**< IPv4 address (host byte order)            */
     uint8_t     pad[8];
 } uiox_sockaddr_in_t;
 
 typedef struct {
     uint16_t    family;
     uint8_t     data[26];
 } uiox_sockaddr_t;
 
 /* =========================================================================
  * TCP connection state machine
  * ====================================================================== */
 
 typedef enum {
     UIOX_TCP_CLOSED = 0,
     UIOX_TCP_LISTEN,
     UIOX_TCP_SYN_SENT,
     UIOX_TCP_SYN_RECEIVED,
     UIOX_TCP_ESTABLISHED,
     UIOX_TCP_FIN_WAIT_1,
     UIOX_TCP_FIN_WAIT_2,
     UIOX_TCP_CLOSE_WAIT,
     UIOX_TCP_CLOSING,
     UIOX_TCP_LAST_ACK,
     UIOX_TCP_TIME_WAIT,
 } uiox_tcp_state_t;
 
 /* =========================================================================
  * Socket receive queue entry
  * ====================================================================== */
 
 typedef struct uiox_rxq_entry {
     uiox_netbuf_t      *buf;
     uint32_t            src_ip;
     uint16_t            src_port;
     struct uiox_rxq_entry *next;
 } uiox_rxq_entry_t;
 
 /* =========================================================================
  * Socket control block
  * ====================================================================== */
 
 typedef struct {
     int8_t              fd;             /**< Socket descriptor (index)        */
     uint8_t             domain;         /**< UIOX_AF_INET / UIOX_AF_INET6     */
     uint8_t             type;           /**< UIOX_SOCK_STREAM / DGRAM / RAW   */
     uint8_t             proto;          /**< UIOX_IPPROTO_TCP/UDP             */
     bool                in_use;
     bool                nonblocking;
 
     /* Addressing */
     uint32_t            local_ip;
     uint16_t            local_port;
     uint32_t            remote_ip;
     uint16_t            remote_port;
 
     /* TCP state */
     uiox_tcp_state_t    tcp_state;
     uint32_t            tx_seq;
     uint32_t            rx_seq;
     uint16_t            tx_window;
 
     /* Options */
     uint32_t            rcv_timeout_ms;
     uint32_t            snd_timeout_ms;
     bool                reuse_addr;
     bool                keep_alive;
 
     /* RX queue — singly linked list of uiox_rxq_entry_t */
     uiox_rxq_entry_t   *rx_head;
     uiox_rxq_entry_t   *rx_tail;
     uint8_t             rx_count;
 
     /* Listen backlog (TCP servers) */
     int                 backlog[UIOX_SOCKET_BACKLOG_MAX];
     uint8_t             backlog_count;
 } uiox_socket_t;
 
 /* =========================================================================
  * Public socket API  (POSIX-compatible signatures)
  * ====================================================================== */
 
 /** Initialise the socket subsystem — call once at boot. */
 void uiox_socket_subsystem_init(void);
 
 /**
  * @brief  Create a socket.
  * @return Socket descriptor ≥ 0 on success, negative errno on failure.
  */
 int  uiox_socket (int domain, int type, int protocol);
 
 /**
  * @brief  Bind socket to a local address and port.
  */
 int  uiox_bind   (int fd, const uiox_sockaddr_in_t *addr);
 
 /**
  * @brief  Mark a TCP socket as passive (listening).
  * @param  backlog  Max pending connections.
  */
 int  uiox_listen (int fd, int backlog);
 
 /**
  * @brief  Accept a connection on a listening TCP socket.
  * @param  addr_out  Filled with remote address on success (may be NULL).
  * @return New socket descriptor, or negative errno.
  */
 int  uiox_accept (int fd, uiox_sockaddr_in_t *addr_out);
 
 /**
  * @brief  Initiate a TCP connection to a remote address.
  */
 int  uiox_connect(int fd, const uiox_sockaddr_in_t *addr);
 
 /**
  * @brief  Send data on a connected socket.
  * @return Bytes sent, or negative errno.
  */
 ssize_t uiox_send   (int fd, const void *buf, size_t len, int flags);
 
 /**
  * @brief  Send a datagram to a specific address (UDP).
  * @return Bytes sent, or negative errno.
  */
 ssize_t uiox_sendto (int fd, const void *buf, size_t len, int flags,
                      const uiox_sockaddr_in_t *dst);
 
 /**
  * @brief  Receive data from a connected socket.
  * @return Bytes received, 0 on EOF/close, negative errno on error.
  */
 ssize_t uiox_recv   (int fd, void *buf, size_t len, int flags);
 
 /**
  * @brief  Receive a datagram and capture source address (UDP).
  */
 ssize_t uiox_recvfrom(int fd, void *buf, size_t len, int flags,
                       uiox_sockaddr_in_t *src_out);
 
 /**
  * @brief  Set a socket option.
  */
 int  uiox_setsockopt(int fd, int level, int optname,
                      const void *optval, uint32_t optlen);
 
 /**
  * @brief  Get a socket option.
  */
 int  uiox_getsockopt(int fd, int level, int optname,
                      void *optval, uint32_t *optlen);
 
 /**
  * @brief  Initiate graceful TCP shutdown.
  * @param  how  0=stop recv, 1=stop send, 2=both.
  */
 int  uiox_shutdown(int fd, int how);
 
 /**
  * @brief  Close and release a socket descriptor.
  */
 int  uiox_close   (int fd);
 
 /* =========================================================================
  * Internal delivery hook — called by protocol stack
  * ====================================================================== */
 
 /**
  * @brief  Deliver a received buffer to the matching bound socket.
  *         Called from uiox_proto_udp_input / uiox_proto_tcp_input.
  */
 void uiox_socket_deliver(uint8_t proto,
                           uint32_t src_ip,  uint16_t src_port,
                           uint32_t dst_ip,  uint16_t dst_port,
                           uiox_netbuf_t *buf);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* UIOX_SOCKET_H */
 