/**
 * @file    uiox_socket.c
 * @brief   UIOX BSD-compatible socket implementation.
 * @date    2026-05-25
 */

 #include "uiox_socket.h"
 #include "uiox_proto.h"
 #include "uiox_netbuf.h"
 #include <string.h>
 #include <errno.h>
 
 /* =========================================================================
  * Socket table
  * ====================================================================== */
 
 static uiox_socket_t    s_sockets[UIOX_SOCKET_MAX];
 static uiox_rxq_entry_t s_rxq_pool[UIOX_SOCKET_MAX * UIOX_SOCKET_RX_QMAX];
 static uiox_rxq_entry_t *s_rxq_free = NULL;
 
 /* -------------------------------------------------------------------------
  * Subsystem init
  * ---------------------------------------------------------------------- */
 
 void uiox_socket_subsystem_init(void)
 {
     memset(s_sockets, 0, sizeof(s_sockets));
     s_rxq_free = NULL;
 
     /* Build RX queue entry free list */
     for (int i = 0; i < UIOX_SOCKET_MAX * UIOX_SOCKET_RX_QMAX; i++) {
         s_rxq_pool[i].next = s_rxq_free;
         s_rxq_free         = &s_rxq_pool[i];
     }
 
     for (int i = 0; i < UIOX_SOCKET_MAX; i++) {
         s_sockets[i].fd     = (int8_t)i;
         s_sockets[i].in_use = false;
     }
 }
 
 /* -------------------------------------------------------------------------
  * Internal helpers
  * ---------------------------------------------------------------------- */
 
 static uiox_socket_t *sock_get(int fd)
 {
     if (fd < 0 || fd >= UIOX_SOCKET_MAX) return NULL;
     if (!s_sockets[fd].in_use)           return NULL;
     return &s_sockets[fd];
 }
 
 static uiox_rxq_entry_t *rxq_alloc(void)
 {
     if (!s_rxq_free) return NULL;
     uiox_rxq_entry_t *e = s_rxq_free;
     s_rxq_free = e->next;
     e->next    = NULL;
     return e;
 }
 
 static void rxq_free(uiox_rxq_entry_t *e)
 {
     e->next    = s_rxq_free;
     s_rxq_free = e;
 }
 
 static void rxq_push(uiox_socket_t *s, uiox_rxq_entry_t *e)
 {
     e->next = NULL;
     if (!s->rx_head) s->rx_head = e;
     else             s->rx_tail->next = e;
     s->rx_tail = e;
     s->rx_count++;
 }
 
 static uiox_rxq_entry_t *rxq_pop(uiox_socket_t *s)
 {
     if (!s->rx_head) return NULL;
     uiox_rxq_entry_t *e = s->rx_head;
     s->rx_head = e->next;
     if (!s->rx_head) s->rx_tail = NULL;
     s->rx_count--;
     return e;
 }
 
 /* -------------------------------------------------------------------------
  * socket()
  * ---------------------------------------------------------------------- */
 
 int uiox_socket(int domain, int type, int protocol)
 {
     if (domain != UIOX_AF_INET) return -EAFNOSUPPORT;
     if (type != UIOX_SOCK_STREAM && type != UIOX_SOCK_DGRAM)
         return -EPROTONOSUPPORT;
 
     for (int i = 0; i < UIOX_SOCKET_MAX; i++) {
         if (!s_sockets[i].in_use) {
             uiox_socket_t *s = &s_sockets[i];
             memset(s, 0, sizeof(*s));
             s->fd         = (int8_t)i;
             s->in_use     = true;
             s->domain     = (uint8_t)domain;
             s->type       = (uint8_t)type;
             s->proto      = (uint8_t)(protocol ? protocol :
                             (type == UIOX_SOCK_STREAM ?
                              UIOX_IPPROTO_TCP : UIOX_IPPROTO_UDP));
             s->tcp_state  = UIOX_TCP_CLOSED;
             s->tx_window  = 4096;
             s->rcv_timeout_ms = 0;   /* 0 = block forever */
             s->snd_timeout_ms = 0;
             return i;
         }
     }
     return -EMFILE;
 }
 
 /* -------------------------------------------------------------------------
  * bind()
  * ---------------------------------------------------------------------- */
 
 int uiox_bind(int fd, const uiox_sockaddr_in_t *addr)
 {
     uiox_socket_t *s = sock_get(fd);
     if (!s || !addr) return -EINVAL;
     if (addr->family != UIOX_AF_INET) return -EAFNOSUPPORT;
 
     /* Check for port conflict if REUSEADDR not set */
     if (!s->reuse_addr) {
         for (int i = 0; i < UIOX_SOCKET_MAX; i++) {
             if (!s_sockets[i].in_use || i == fd) continue;
             if (s_sockets[i].local_port == addr->port &&
                 s_sockets[i].proto      == s->proto)
                 return -EADDRINUSE;
         }
     }
 
     s->local_ip   = addr->addr;
     s->local_port = addr->port;
     return 0;
 }
 
 /* -------------------------------------------------------------------------
  * listen()
  * ---------------------------------------------------------------------- */
 
 int uiox_listen(int fd, int backlog)
 {
     uiox_socket_t *s = sock_get(fd);
     if (!s) return -EBADF;
     if (s->type != UIOX_SOCK_STREAM) return -EOPNOTSUPP;
     if (!s->local_port) return -EINVAL;
 
     (void)backlog;   /* backlog clamped to UIOX_SOCKET_BACKLOG_MAX */
     s->tcp_state     = UIOX_TCP_LISTEN;
     s->backlog_count = 0;
     return 0;
 }
 
 /* -------------------------------------------------------------------------
  * accept()
  * ---------------------------------------------------------------------- */
 
 int uiox_accept(int fd, uiox_sockaddr_in_t *addr_out)
 {
     uiox_socket_t *s = sock_get(fd);
     if (!s) return -EBADF;
     if (s->tcp_state != UIOX_TCP_LISTEN) return -EINVAL;
 
     /* Spin-wait for a backlog entry — replace with OS semaphore on RTOS */
     uint32_t waited = 0;
     while (s->backlog_count == 0) {
         if (s->nonblocking) return -EAGAIN;
         /* uiox_os_sleep_ms(1); */
         if (++waited > (s->rcv_timeout_ms ? s->rcv_timeout_ms : 30000))
             return -ETIMEDOUT;
     }
 
     int new_fd = s->backlog[--s->backlog_count];
     uiox_socket_t *ns = sock_get(new_fd);
     if (!ns) return -EBADF;
 
     if (addr_out) {
         addr_out->family = UIOX_AF_INET;
         addr_out->addr   = ns->remote_ip;
         addr_out->port   = ns->remote_port;
     }
     return new_fd;
 }
 
 /* -------------------------------------------------------------------------
  * connect()
  * ---------------------------------------------------------------------- */
 
 int uiox_connect(int fd, const uiox_sockaddr_in_t *addr)
 {
     uiox_socket_t *s = sock_get(fd);
     if (!s || !addr) return -EINVAL;
 
     s->remote_ip   = addr->addr;
     s->remote_port = addr->port;
 
     if (s->type == UIOX_SOCK_DGRAM) return 0;  /* UDP: just record peer */
 
     /* TCP: send SYN */
     s->tx_seq    = 0x12345678u;   /* ISN — use random on production */
     s->tcp_state = UIOX_TCP_SYN_SENT;
 
     uiox_netbuf_t *buf = uiox_netbuf_alloc();
     if (!buf) return -ENOMEM;
 
     int rc = uiox_proto_tcp_send(s->local_ip,  s->remote_ip,
                                   s->local_port, s->remote_port,
                                   s->tx_seq, 0,
                                   UIOX_TCP_SYN, buf);
     if (rc < 0) { s->tcp_state = UIOX_TCP_CLOSED; return rc; }
 
     /* Wait for SYN-ACK */
     uint32_t waited = 0;
     while (s->tcp_state != UIOX_TCP_ESTABLISHED) {
         if (s->nonblocking) return -EINPROGRESS;
         if (++waited > (s->snd_timeout_ms ? s->snd_timeout_ms : 5000))
             return -ETIMEDOUT;
         if (s->tcp_state == UIOX_TCP_CLOSED) return -ECONNREFUSED;
     }
     return 0;
 }
 
 /* -------------------------------------------------------------------------
  * send() / sendto()
  * ---------------------------------------------------------------------- */
 
 ssize_t uiox_send(int fd, const void *data, size_t len, int flags)
 {
     (void)flags;
     uiox_socket_t *s = sock_get(fd);
     if (!s || !data || !len) return -EINVAL;
 
     uiox_netbuf_t *buf = uiox_netbuf_alloc();
     if (!buf) return -ENOMEM;
 
     uint16_t send_len = (uint16_t)(len > UIOX_NETBUF_DATA_SIZE ?
                                    UIOX_NETBUF_DATA_SIZE : len);
     void *p = uiox_netbuf_put(buf, send_len);
     if (!p) { uiox_netbuf_free(buf); return -ENOBUFS; }
     memcpy(p, data, send_len);
 
     int rc;
     if (s->type == UIOX_SOCK_DGRAM) {
         rc = uiox_proto_udp_send(s->local_ip,  s->remote_ip,
                                   s->local_port, s->remote_port, buf);
     } else {
         rc = uiox_proto_tcp_send(s->local_ip,  s->remote_ip,
                                   s->local_port, s->remote_port,
                                   s->tx_seq, s->rx_seq,
                                   UIOX_TCP_ACK | UIOX_TCP_PSH, buf);
         if (rc == 0) s->tx_seq += send_len;
     }
     return rc < 0 ? rc : (ssize_t)send_len;
 }
 
 ssize_t uiox_sendto(int fd, const void *data, size_t len, int flags,
                     const uiox_sockaddr_in_t *dst)
 {
     uiox_socket_t *s = sock_get(fd);
     if (!s || !dst) return -EINVAL;
 
     /* Temporarily bind remote for send */
     uint32_t saved_ip   = s->remote_ip;
     uint16_t saved_port = s->remote_port;
     s->remote_ip   = dst->addr;
     s->remote_port = dst->port;
 
     ssize_t rc = uiox_send(fd, data, len, flags);
 
     s->remote_ip   = saved_ip;
     s->remote_port = saved_port;
     return rc;
 }
 
 /* -------------------------------------------------------------------------
  * recv() / recvfrom()
  * ---------------------------------------------------------------------- */
 
 ssize_t uiox_recv(int fd, void *buf, size_t len, int flags)
 {
     (void)flags;
     uiox_socket_t *s = sock_get(fd);
     if (!s || !buf || !len) return -EINVAL;
 
     uint32_t waited = 0;
     uint32_t timeout = s->rcv_timeout_ms ? s->rcv_timeout_ms : 0xFFFFFFFFu;
 
     while (!s->rx_head) {
         if (s->nonblocking) return -EAGAIN;
         if (waited++ >= timeout) return -ETIMEDOUT;
         /* uiox_os_sleep_ms(1); */
     }
 
     uiox_rxq_entry_t *e = rxq_pop(s);
     if (!e) return -EAGAIN;
 
     uint16_t copy_len = (uint16_t)(e->buf->len < len ? e->buf->len : len);
     memcpy(buf, e->buf->data, copy_len);
     uiox_netbuf_free(e->buf);
     rxq_free(e);
     return (ssize_t)copy_len;
 }
 
 ssize_t uiox_recvfrom(int fd, void *buf, size_t len, int flags,
                       uiox_sockaddr_in_t *src_out)
 {
     uiox_socket_t *s = sock_get(fd);
     if (!s) return -EBADF;
 
     /* Peek at the head entry for source address before popping */
     uint32_t waited = 0;
     uint32_t timeout = s->rcv_timeout_ms ? s->rcv_timeout_ms : 0xFFFFFFFFu;
     while (!s->rx_head) {
         if (s->nonblocking) return -EAGAIN;
         if (waited++ >= timeout) return -ETIMEDOUT;
     }
 
     if (src_out && s->rx_head) {
         src_out->family = UIOX_AF_INET;
         src_out->addr   = s->rx_head->src_ip;
         src_out->port   = s->rx_head->src_port;
     }
     return uiox_recv(fd, buf, len, flags);
 }
 
 /* -------------------------------------------------------------------------
  * setsockopt() / getsockopt()
  * ---------------------------------------------------------------------- */
 
 int uiox_setsockopt(int fd, int level, int optname,
                     const void *optval, uint32_t optlen)
 {
     uiox_socket_t *s = sock_get(fd);
     if (!s || !optval) return -EINVAL;
     (void)level; (void)optlen;
 
     switch (optname) {
     case UIOX_SO_REUSEADDR:
         s->reuse_addr = *(const int *)optval != 0; break;
     case UIOX_SO_KEEPALIVE:
         s->keep_alive = *(const int *)optval != 0; break;
     case UIOX_SO_RCVTIMEO:
         s->rcv_timeout_ms = *(const uint32_t *)optval; break;
     case UIOX_SO_SNDTIMEO:
         s->snd_timeout_ms = *(const uint32_t *)optval; break;
     default:
         return -ENOPROTOOPT;
     }
     return 0;
 }
 
 int uiox_getsockopt(int fd, int level, int optname,
                     void *optval, uint32_t *optlen)
 {
     uiox_socket_t *s = sock_get(fd);
     if (!s || !optval) return -EINVAL;
     (void)level; (void)optlen;
 
     switch (optname) {
     case UIOX_SO_REUSEADDR:
         *(int *)optval = s->reuse_addr ? 1 : 0; break;
     case UIOX_SO_KEEPALIVE:
         *(int *)optval = s->keep_alive ? 1 : 0; break;
     case UIOX_SO_RCVTIMEO:
         *(uint32_t *)optval = s->rcv_timeout_ms; break;
     case UIOX_SO_SNDTIMEO:
         *(uint32_t *)optval = s->snd_timeout_ms; break;
     default:
         return -ENOPROTOOPT;
     }
     return 0;
 }
 
 /* -------------------------------------------------------------------------
  * shutdown() / close()
  * ---------------------------------------------------------------------- */
 
 int uiox_shutdown(int fd, int how)
 {
     uiox_socket_t *s = sock_get(fd);
     if (!s) return -EBADF;
 
     if (s->type == UIOX_SOCK_STREAM &&
         s->tcp_state == UIOX_TCP_ESTABLISHED) {
 
         if (how == 1 || how == 2) {
             /* Send FIN */
             uiox_netbuf_t *buf = uiox_netbuf_alloc();
             if (buf) {
                 uiox_proto_tcp_send(s->local_ip,  s->remote_ip,
                                     s->local_port, s->remote_port,
                                     s->tx_seq, s->rx_seq,
                                     UIOX_TCP_FIN | UIOX_TCP_ACK, buf);
                 s->tx_seq++;
             }
             s->tcp_state = UIOX_TCP_FIN_WAIT_1;
         }
     }
     return 0;
 }
 
 int uiox_close(int fd)
 {
     uiox_socket_t *s = sock_get(fd);
     if (!s) return -EBADF;
 
     uiox_shutdown(fd, 2);
 
     /* Drain RX queue */
     uiox_rxq_entry_t *e;
     while ((e = rxq_pop(s)) != NULL) {
         uiox_netbuf_free(e->buf);
         rxq_free(e);
     }
 
     s->tcp_state = UIOX_TCP_CLOSED;
     s->in_use    = false;
     return 0;
 }
 
 /* -------------------------------------------------------------------------
  * Delivery hook — called from protocol stack
  * ---------------------------------------------------------------------- */
 
 void uiox_socket_deliver(uint8_t proto,
                           uint32_t src_ip,  uint16_t src_port,
                           uint32_t dst_ip,  uint16_t dst_port,
                           uiox_netbuf_t *buf)
 {
     for (int i = 0; i < UIOX_SOCKET_MAX; i++) {
         uiox_socket_t *s = &s_sockets[i];
         if (!s->in_use)  continue;
         if (s->proto     != proto)    continue;
         if (s->local_port != dst_port) continue;
         if (s->local_ip  != 0 && s->local_ip != dst_ip) continue;
 
         if (s->rx_count >= UIOX_SOCKET_RX_QMAX) {
             uiox_netbuf_free(buf);
             return;
         }
 
         uiox_rxq_entry_t *e = rxq_alloc();
         if (!e) { uiox_netbuf_free(buf); return; }
 
         e->buf      = buf;
         e->src_ip   = src_ip;
         e->src_port = src_port;
         rxq_push(s, e);
 
         /* TCP: handle SYN → SYN-ACK handshake */
         if (proto == UIOX_IPPROTO_TCP) {
             if (s->tcp_state == UIOX_TCP_SYN_SENT) {
                 s->rx_seq    = 0;   /* extracted from SYN-ACK in full impl */
                 s->tcp_state = UIOX_TCP_ESTABLISHED;
             } else if (s->tcp_state == UIOX_TCP_LISTEN) {
                 /* Clone a new socket for the incoming connection */
                 int new_fd = uiox_socket(UIOX_AF_INET,
                                          UIOX_SOCK_STREAM,
                                          UIOX_IPPROTO_TCP);
                 if (new_fd >= 0 &&
                     s->backlog_count < UIOX_SOCKET_BACKLOG_MAX) {
                     uiox_socket_t *ns  = &s_sockets[new_fd];
                     ns->local_ip       = dst_ip;
                     ns->local_port     = dst_port;
                     ns->remote_ip      = src_ip;
                     ns->remote_port    = src_port;
                     ns->tcp_state      = UIOX_TCP_ESTABLISHED;
                     s->backlog[s->backlog_count++] = new_fd;
                 }
             }
         }
         return;
     }
     /* No matching socket */
     uiox_netbuf_free(buf);
 }
 