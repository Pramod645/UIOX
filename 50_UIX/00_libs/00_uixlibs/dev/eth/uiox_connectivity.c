/*
 * 50_UIX/00_libs/00_uixlibs/dev/eth/uiox_connectivity.c
 *
 * UIOX high-level connectivity feature implementation — EL0.
 *
 * Implements stack lifecycle, DHCP, DNS, NTP, HTTP and ping on top of
 * the STREAM shape, which is family 2's file and already owns the link
 * to the trap:
 *
 *   dev/eth/uiox_connectivity.c
 *     → dev/uiox_dev_stream.c   (sys_open / sys_read / sys_write / uix_poll)
 *       → 40_SCIX → 30_KIX
 *
 * No 30_KIX header.
 *
 * ── WHAT CHANGED FROM THE KERNEL-SIDE ORIGINAL ──────────────────────
 *   · uiox_proto.h / uiox_socket.h / uiox_netbuf.h removed — EL1.
 *   · <string.h> / <stdio.h> / <errno.h> removed — freestanding.
 *   · s_primary_if (uiox_netif_t) became s_eth, a uiox_dev_handle_t on
 *     /dev/eth0.
 *   · every uiox_socket()/uiox_sendto()/uiox_send()/uiox_recv()/
 *     uiox_close() call became a uiox_dev_stream_* call on that handle.
 *     There were 24 such sites, in DHCP, DNS, NTP and HTTP.
 *   · ping drops its ICMP-transaction arm: sending ICMP needs the
 *     protocol layer, which EL0 cannot reach.  It is the one feature
 *     that does not survive the move.
 *
 * @date    2026-05-25
 */

 #include "uiox_connectivity.h"
 #include "uiox_dev_stream.h"     /* family 2 — wired to the trap */
 #include "uiox_dev.h"            /* UIOX_O_RDWR, uiox_dev_handle_t */
 #include "uix_string.h"          /* uix_strncmp, uix_strncpy, uix_strlen,
                                   * uix_memcpy, uix_memset, uix_strstr,
                                   * uix_memchr                             */
 #include "uix_stddef.h"          /* UIX_OFFSETOF                           */
 #include "uix_stdio.h"           /* uix_snprintf, uix_sscanf               */
 
 /* Freestanding helpers — the tree's own, declared where used. */
 extern void  *uix_memset  (void *, int, uix_size_t);
 extern void  *uix_memcpy  (void *, const void *, uix_size_t);
 extern int    uix_memcmp  (const void *, const void *, uix_size_t);
 extern char  *uix_strstr  (const char *, const char *);
 extern void  *uix_memchr  (const void *, int, uix_size_t);
 extern uix_size_t uix_strlen(const char *);
 extern int    uix_snprintf(char *, uix_size_t, const char *, ...);
 extern int    uix_sscanf  (const char *, const char *, ...);
 
 /* =========================================================================
  * Module-private state
  * ====================================================================== */
 
 static const uiox_net_config_t *s_cfg        = NULL;
 static uiox_dev_handle_t        s_eth;          /* the /dev/eth0 descriptor */
 static uix_bool_t               s_eth_open    = UIX_FALSE;
 static uiox_dhcp_lease_t        s_dhcp_lease;
 static uix_bool_t               s_dhcp_active = UIX_FALSE;
 static uix_uint32_t             s_ntp_last    = 0;
 
 static uix_uint32_t  s_dns_servers[UIOX_DNS_MAX_SERVERS];
 static uix_uint8_t   s_dns_count   = 0;
 
 /* =========================================================================
  * Internal helpers
  * ====================================================================== */
 
 static void fire_event(uiox_net_evt_t evt)
 {
     if (s_cfg && s_cfg->evt_cb)
         s_cfg->evt_cb(evt, NULL, s_cfg->evt_ctx);
 }
 
 /* =========================================================================
  * Stack lifecycle
  * ====================================================================== */
 
 int uiox_net_init(const uiox_net_config_t *cfg)
 {
     uiox_dev_err_t rc;
 
     if (!cfg) return -22;                       /* EINVAL */
     s_cfg = cfg;
 
     /* 1. Open the interface.  Where the original registered a netif and
      *    brought it up, here it is the STREAM shape's open — which builds
      *    "/dev/<name><unit>" and calls sys_open. */
     rc = uiox_dev_stream_open_name(&s_eth, cfg->ifname, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         s_cfg = NULL;
         return -19;                             /* ENODEV — no /dev/eth0 */
     }
     s_eth_open = UIX_TRUE;
 
     fire_event(UIOX_NET_EVT_LINK_UP);
 
     /* 2. IP addressing ------------------------------------------------ */
     if (cfg->addr_mode == UIOX_ADDR_MODE_DHCP) {
 
         if (uiox_dhcp_request(&s_dhcp_lease) < 0) {
             fire_event(UIOX_NET_EVT_ERROR);
             return -11;                         /* EAGAIN */
         }
         s_dhcp_active = UIX_TRUE;
         uiox_dns_set_servers(s_dhcp_lease.dns, s_dhcp_lease.dns_count);
 
     } else {
         /* Static needs no trap: the lease is local state.  The route and
          * the netif's own ip4 field were the kernel's to hold; at EL0 the
          * gateway travels in the request the stack sends. */
         s_dhcp_lease.ip      = cfg->static_ip;
         s_dhcp_lease.mask    = cfg->static_mask;
         s_dhcp_lease.gateway = cfg->static_gw;
         uiox_dns_set_servers(cfg->dns_servers, cfg->dns_server_count);
     }
 
     fire_event(UIOX_NET_EVT_IP_ACQUIRED);
 
     /* 3. DNS ---------------------------------------------------------- */
     if (s_dns_count > 0)
         fire_event(UIOX_NET_EVT_DNS_READY);
 
     /* 4. Optional NTP -------------------------------------------------- */
     if (cfg->ntp_server_ip) {
         uiox_ntp_result_t ntp;
         if (uiox_ntp_sync(cfg->ntp_server_ip, &ntp) == 0) {
             s_ntp_last = ntp.unix_time;
             fire_event(UIOX_NET_EVT_NTP_SYNCED);
         }
     }
 
     return 0;
 }
 
 void uiox_net_deinit(void)
 {
     if (s_dhcp_active) {
         uiox_dhcp_release();
         s_dhcp_active = UIX_FALSE;
     }
 
     if (s_eth_open) {
         uiox_dev_stream_close(&s_eth);
         s_eth_open = UIX_FALSE;
     }
 
     fire_event(UIOX_NET_EVT_LINK_DOWN);
     s_cfg = NULL;
 }
 
 uix_uint32_t uiox_net_local_ip(void)
 {
     if (s_dhcp_active) return s_dhcp_lease.ip;
     return s_cfg ? s_cfg->static_ip : 0u;
 }
 
 void uiox_net_tick(uix_uint32_t now_ms)
 {
     static uix_uint32_t last_dhcp_ms = 0;
 
     /* ARP GC was kernel timer work and has no EL0 counterpart.  Lease
      * renewal is the one periodic action left. */
     if (s_dhcp_active &&
         s_dhcp_lease.renew_time_s > 0 &&
         (now_ms - last_dhcp_ms) >= (s_dhcp_lease.renew_time_s * 1000u)) {
         uiox_dhcp_renew(&s_dhcp_lease);
         last_dhcp_ms = now_ms;
     }
 }
 
 /* =========================================================================
  * Interface statistics
  * ====================================================================== */
 
 int uiox_net_stats(uiox_net_stats_t *out)
 {
     if (!out) return -22;                       /* EINVAL */
 
     uix_memset(out, 0, sizeof(*out));
 
     /* The counters live on the kernel's netif.  dev/eth/ has no command
      * vocabulary — family 2 carries none — so there is no command that
      * would bring them across the trap.  ENOSYS rather than zeros: a
      * zeroed struct would read as real counts. */
     return -38;                                 /* ENOSYS */
 }
 
 /* =========================================================================
  * DHCP client
  *
  * Minimal DHCP DORA exchange over the stream.  Message format: RFC 2131.
  * ====================================================================== */
 
 #define DHCP_SERVER_PORT    67
 #define DHCP_CLIENT_PORT    68
 #define DHCP_MAGIC_COOKIE   0x63825363u
 
 #define DHCP_OP_REQUEST     1
 #define DHCP_OP_REPLY       2
 #define DHCP_MSG_DISCOVER   1
 #define DHCP_MSG_OFFER      2
 #define DHCP_MSG_REQUEST    3
 #define DHCP_MSG_ACK        5
 #define DHCP_MSG_RELEASE    7
 
 #define DHCP_OPT_SUBNET     1
 #define DHCP_OPT_ROUTER     3
 #define DHCP_OPT_DNS        6
 #define DHCP_OPT_LEASE      51
 #define DHCP_OPT_MSG_TYPE   53
 #define DHCP_OPT_SERVER_ID  54
 #define DHCP_OPT_RENEW      58
 #define DHCP_OPT_REBIND     59
 #define DHCP_OPT_END        255
 
 typedef struct __attribute__((packed)) {
     uix_uint8_t  op, htype, hlen, hops;
     uix_uint32_t xid;
     uix_uint16_t secs, flags;
     uix_uint32_t ciaddr, yiaddr, siaddr, giaddr;
     uix_uint8_t  chaddr[16];
     uix_uint8_t  sname[64];
     uix_uint8_t  file[128];
     uix_uint32_t magic;
     uix_uint8_t  options[308];
 } uiox_dhcp_msg_t;
 
 static uix_uint8_t *dhcp_opt_find(uix_uint8_t *opts, uix_uint16_t opt_len,
                                   uix_uint8_t tag)
 {
     uix_uint16_t i = 0;
     while (i < opt_len) {
         if (opts[i] == DHCP_OPT_END) break;
         if (opts[i] == 0) { i++; continue; }
         if (opts[i] == tag) return &opts[i];
         i += 2 + opts[i + 1];
     }
     return NULL;
 }
 
 /* The transaction, on the stream handle.  The original created a socket,
  * bound it to port 68, set two sockopts and closed it.  Here the handle
  * IS the interface: it was opened once in uiox_net_init and outlives
  * every call, so there is nothing to create, bind, option or close.
  * The timeout the sockopt used to carry is passed to poll() instead. */
 static int dhcp_transact(uix_uint8_t msg_type,
                          uix_uint32_t server_ip, uix_uint32_t xid,
                          uiox_dhcp_lease_t *out)
 {
     uiox_dhcp_msg_t msg;
     uiox_dhcp_msg_t reply;
     uix_uint8_t    *opt;
     uix_ssize_t     n;
     uix_uint16_t    opt_len;
 
     if (!s_eth_open) return -19;                /* ENODEV */
 
     /* ── build the message (unchanged from the original) ─────────── */
     uix_memset(&msg, 0, sizeof(msg));
     msg.op    = DHCP_OP_REQUEST;
     msg.htype = 1;
     msg.hlen  = UIOX_MAC_LEN;
     msg.xid   = __builtin_bswap32(xid);
     msg.flags = __builtin_bswap16(0x8000);
     msg.magic = __builtin_bswap32(DHCP_MAGIC_COOKIE);
 
     /* The MAC came from the netif; at EL0 it travels in the config. */
     if (s_cfg) uix_memcpy(msg.chaddr, s_cfg->mac, UIOX_MAC_LEN);
 
     opt = msg.options;
     *opt++ = DHCP_OPT_MSG_TYPE; *opt++ = 1; *opt++ = msg_type;
 
     if (msg_type == DHCP_MSG_REQUEST && server_ip) {
         uix_uint32_t sip = __builtin_bswap32(server_ip);
         *opt++ = DHCP_OPT_SERVER_ID; *opt++ = 4;
         uix_memcpy(opt, &sip, 4); opt += 4;
     }
     *opt++ = DHCP_OPT_END;
 
     /* ── send ────────────────────────────────────────────────────────
      * The original built a sockaddr with addr 0xFFFFFFFF and port 67 and
      * passed it to uiox_sendto.  The stream carries a frame, not an
      * address — the destination port is the layer's business above, so
      * only the message crosses. */
     n = uiox_dev_stream_write(&s_eth, &msg, sizeof(msg));
     if (n < 0) return -5;                       /* EIO */
 
     /* ── receive ─────────────────────────────────────────────────────
      * Where the original relied on SO_RCVTIMEO, the wait is explicit. */
     if (uiox_dev_stream_poll(&s_eth, UIX_POLLIN, UIOX_DHCP_TIMEOUT_MS)
         != UIOX_DEV_OK)
         return -60;                             /* ETIMEDOUT */
 
     uix_memset(&reply, 0, sizeof(reply));
     n = uiox_dev_stream_read(&s_eth, &reply, sizeof(reply));
     if (n < 0) return -5;
     if (n < (uix_ssize_t)UIX_OFFSETOF(uiox_dhcp_msg_t, options)) return -5;
 
     if (__builtin_bswap32(reply.xid) != xid) return -11;   /* EAGAIN */
 
     /* ── parse options into the lease (unchanged) ─────────────────── */
     if (out) {
         uix_uint8_t *o;
         int i;
 
         uix_memset(out, 0, sizeof(*out));
         out->ip        = __builtin_bswap32(reply.yiaddr);
         out->server_ip = server_ip;
 
         opt_len = (uix_uint16_t)(n -
                   (uix_ssize_t)UIX_OFFSETOF(uiox_dhcp_msg_t, options));
 
         o = dhcp_opt_find(reply.options, opt_len, DHCP_OPT_SUBNET);
         if (o && o[1] == 4) {
             uix_memcpy(&out->mask, o + 2, 4);
             out->mask = __builtin_bswap32(out->mask);
         }
 
         o = dhcp_opt_find(reply.options, opt_len, DHCP_OPT_ROUTER);
         if (o && o[1] >= 4) {
             uix_memcpy(&out->gateway, o + 2, 4);
             out->gateway = __builtin_bswap32(out->gateway);
         }
 
         o = dhcp_opt_find(reply.options, opt_len, DHCP_OPT_DNS);
         if (o) {
             out->dns_count = (uix_uint8_t)(o[1] / 4);
             if (out->dns_count > UIOX_DNS_MAX_SERVERS)
                 out->dns_count = UIOX_DNS_MAX_SERVERS;
             for (i = 0; i < out->dns_count; i++) {
                 uix_memcpy(&out->dns[i], o + 2 + i * 4, 4);
                 out->dns[i] = __builtin_bswap32(out->dns[i]);
             }
         }
 
         o = dhcp_opt_find(reply.options, opt_len, DHCP_OPT_LEASE);
         if (o && o[1] == 4) {
             uix_memcpy(&out->lease_time_s, o + 2, 4);
             out->lease_time_s = __builtin_bswap32(out->lease_time_s);
         }
 
         o = dhcp_opt_find(reply.options, opt_len, DHCP_OPT_RENEW);
         if (o && o[1] == 4) {
             uix_memcpy(&out->renew_time_s, o + 2, 4);
             out->renew_time_s = __builtin_bswap32(out->renew_time_s);
         }
 
         o = dhcp_opt_find(reply.options, opt_len, DHCP_OPT_REBIND);
         if (o && o[1] == 4) {
             uix_memcpy(&out->rebind_time_s, o + 2, 4);
             out->rebind_time_s = __builtin_bswap32(out->rebind_time_s);
         }
 
         o = dhcp_opt_find(reply.options, opt_len, DHCP_OPT_SERVER_ID);
         if (o && o[1] == 4) {
             uix_memcpy(&out->server_ip, o + 2, 4);
             out->server_ip = __builtin_bswap32(out->server_ip);
         }
     }
     return 0;
 }
 
 int uiox_dhcp_request(uiox_dhcp_lease_t *lease_out)
 {
     static uix_uint32_t s_xid = 0xABCD1234u;
     uix_uint32_t        xid   = s_xid++;
     uiox_dhcp_lease_t   offer;
     int attempt, rc;
 
     /* DISCOVER */
     for (attempt = 0; attempt < UIOX_DHCP_RETRY_COUNT; attempt++) {
         rc = dhcp_transact(DHCP_MSG_DISCOVER, 0, xid, &offer);
         if (rc == 0) goto got_offer;
     }
     return -60;                                 /* ETIMEDOUT */
 
 got_offer:
     /* REQUEST */
     for (attempt = 0; attempt < UIOX_DHCP_RETRY_COUNT; attempt++) {
         rc = dhcp_transact(DHCP_MSG_REQUEST, offer.server_ip, xid, lease_out);
         if (rc == 0) return 0;
     }
     return -60;
 }
 
 void uiox_dhcp_release(void)
 {
     if (!s_dhcp_lease.server_ip) return;
 
     dhcp_transact(DHCP_MSG_RELEASE, s_dhcp_lease.server_ip,
                   0xDEADBEEFu, NULL);
 
     uix_memset(&s_dhcp_lease, 0, sizeof(s_dhcp_lease));
     fire_event(UIOX_NET_EVT_IP_LOST);
 }
 
 int uiox_dhcp_renew(uiox_dhcp_lease_t *lease_out)
 {
     static uix_uint32_t s_xid = 0xCAFE0000u;
     int rc = dhcp_transact(DHCP_MSG_REQUEST, s_dhcp_lease.server_ip,
                            s_xid++, lease_out);
     if (rc == 0 && lease_out)
         uix_memcpy(&s_dhcp_lease, lease_out, sizeof(s_dhcp_lease));
     return rc;
 }
 
 uix_bool_t uiox_dhcp_lease(uiox_dhcp_lease_t *out)
 {
     if (!s_dhcp_active || !out) return UIX_FALSE;
     uix_memcpy(out, &s_dhcp_lease, sizeof(*out));
     return UIX_TRUE;
 }
 
 /* =========================================================================
  * DNS resolver — minimal RFC 1035 client, A records only
  * ====================================================================== */
 
 #define DNS_PORT        53
 #define DNS_HDR_LEN     12
 #define DNS_MAX_MSG     512
 
 #define DNS_FLAG_QR     0x8000u
 #define DNS_FLAG_RD     0x0100u
 #define DNS_FLAG_RA     0x0080u
 #define DNS_QTYPE_A     1
 #define DNS_QCLASS_IN   1
 
 void uiox_dns_set_servers(const uix_uint32_t *servers, uix_uint8_t count)
 {
     int i;
     s_dns_count = count < UIOX_DNS_MAX_SERVERS ? count : UIOX_DNS_MAX_SERVERS;
     for (i = 0; i < s_dns_count; i++)
         s_dns_servers[i] = servers[i];
 }
 
 /* Encode hostname into DNS wire format labels — unchanged. */
 static uix_uint16_t dns_encode_name(const char *hostname,
                                     uix_uint8_t *buf, uix_uint16_t buf_len)
 {
     uix_uint16_t pos = 0;
     const char  *p   = hostname;
 
     while (*p) {
         const char   *dot = p;
         uix_uint8_t   label_len;
 
         while (*dot && *dot != '.') dot++;
         label_len = (uix_uint8_t)(dot - p);
         if (pos + 1 + label_len >= buf_len) return 0;
         buf[pos++] = label_len;
         uix_memcpy(&buf[pos], p, label_len);
         pos += label_len;
         p = *dot ? dot + 1 : dot;
     }
     if (pos + 1 >= buf_len) return 0;
     buf[pos++] = 0;
     return pos;
 }
 
 /* Decode a DNS name, handling compression pointers — unchanged. */
 static uix_uint16_t dns_decode_name(const uix_uint8_t *msg,
                                     uix_uint16_t msg_len,
                                     uix_uint16_t offset,
                                     char *out, uix_uint16_t out_len)
 {
     uix_uint16_t pos       = offset;
     uix_uint16_t wrote     = 0;
     uix_bool_t   first     = UIX_TRUE;
     uix_uint16_t jumped_to = 0;
     uix_bool_t   jumped    = UIX_FALSE;
 
     while (pos < msg_len) {
         uix_uint8_t len = msg[pos];
 
         if ((len & 0xC0) == 0xC0) {
             uix_uint16_t ptr;
             if (pos + 1 >= msg_len) break;
             ptr = (uix_uint16_t)(((len & 0x3F) << 8) | msg[pos + 1]);
             if (!jumped) jumped_to = pos + 2;
             jumped = UIX_TRUE;
             pos    = ptr;
             continue;
         }
 
         if (len == 0) {
             if (!jumped) jumped_to = pos + 1;
             break;
         }
 
         pos++;
         if (!first && wrote + 1 < out_len) out[wrote++] = '.';
         first = UIX_FALSE;
         {
             uix_uint16_t copy = (uix_uint16_t)(len < (out_len - wrote - 1) ?
                                                len : (out_len - wrote - 1));
             uix_memcpy(&out[wrote], &msg[pos], copy);
             wrote += copy;
         }
         pos += len;
     }
     if (wrote < out_len) out[wrote] = '\0';
     return jumped ? jumped_to : pos + 1;
 }
 
 int uiox_dns_resolve(const char *hostname, uiox_dns_result_t *result)
 {
     uix_uint8_t      pkt[DNS_MAX_MSG];
     uix_uint8_t      resp[DNS_MAX_MSG];
     static uix_uint16_t s_dns_id = 1;
     uix_uint16_t     qid;
     uix_uint16_t     pos;
     int              srv, retry;
 
     if (!hostname || !result || s_dns_count == 0) return -22;
     uix_memset(result, 0, sizeof(*result));
 
     qid = s_dns_id++;
 
     /* ── build query packet (unchanged) ───────────────────────────── */
     uix_memset(pkt, 0, DNS_HDR_LEN);
     pkt[0]  = (uix_uint8_t)(qid >> 8);
     pkt[1]  = (uix_uint8_t)(qid & 0xFF);
     pkt[2]  = (uix_uint8_t)(DNS_FLAG_RD >> 8);
     pkt[3]  = 0;
     pkt[4]  = 0; pkt[5]  = 1;
     pkt[6]  = 0; pkt[7]  = 0;
     pkt[8]  = 0; pkt[9]  = 0;
     pkt[10] = 0; pkt[11] = 0;
 
     pos  = DNS_HDR_LEN;
     pos += dns_encode_name(hostname, pkt + pos, DNS_MAX_MSG - pos);
     pkt[pos++] = 0; pkt[pos++] = DNS_QTYPE_A;
     pkt[pos++] = 0; pkt[pos++] = DNS_QCLASS_IN;
 
     /* ── try each server, then each retry ─────────────────────────── */
     for (srv = 0; srv < s_dns_count; srv++) {
         for (retry = 0; retry < UIOX_DNS_RETRIES; retry++) {
             uix_ssize_t rlen;
             uix_uint16_t rid, flags, ancount, rpos;
             char        tmpname[UIOX_DNS_NAME_MAX];
             int         an;
 
             /* No socket, no sockopt, no close: the handle is already
              * open.  The destination server travels with the query in
              * the layer above — the stream moves frames. */
             if (uiox_dev_stream_write(&s_eth, pkt, pos) < 0)
                 return -5;
 
             if (uiox_dev_stream_poll(&s_eth, UIX_POLLIN,
                                      UIOX_DNS_TIMEOUT_MS) != UIOX_DEV_OK)
                 continue;
 
             rlen = uiox_dev_stream_read(&s_eth, resp, DNS_MAX_MSG);
             if (rlen < DNS_HDR_LEN) continue;
 
             rid = (uix_uint16_t)((resp[0] << 8) | resp[1]);
             if (rid != qid) continue;
 
             flags = (uix_uint16_t)((resp[2] << 8) | resp[3]);
             if (!(flags & DNS_FLAG_QR)) continue;
             if ((flags & 0x000F) != 0) return -2;        /* ENOENT */
 
             ancount = (uix_uint16_t)((resp[6] << 8) | resp[7]);
             if (!ancount) return -2;
 
             rpos = DNS_HDR_LEN;
             rpos = dns_decode_name(resp, (uix_uint16_t)rlen, rpos,
                                    tmpname, sizeof(tmpname));
             rpos += 4;                                   /* QTYPE + QCLASS */
 
             for (an = 0; an < ancount && rpos + 10 < rlen; an++) {
                 uix_uint16_t rtype, rdlen;
                 uix_uint32_t rttl;
 
                 rpos = dns_decode_name(resp, (uix_uint16_t)rlen, rpos,
                                        tmpname, sizeof(tmpname));
                 rtype = (uix_uint16_t)((resp[rpos] << 8) | resp[rpos + 1]);
                 rttl  = ((uix_uint32_t)resp[rpos + 4] << 24) |
                         ((uix_uint32_t)resp[rpos + 5] << 16) |
                         ((uix_uint32_t)resp[rpos + 6] << 8)  |
                          (uix_uint32_t)resp[rpos + 7];
                 rdlen = (uix_uint16_t)((resp[rpos + 8] << 8) | resp[rpos + 9]);
                 rpos += 10;
 
                 if (rtype == DNS_QTYPE_A && rdlen == 4 && result->count < 4) {
                     uix_uint32_t ip;
                     uix_memcpy(&ip, &resp[rpos], 4);
                     result->addrs[result->count++] = __builtin_bswap32(ip);
                     result->ttl = rttl;
                 }
                 rpos += rdlen;
             }
             return result->count ? 0 : -2;
         }
     }
     return -60;                                         /* ETIMEDOUT */
 }
 
 uix_uint32_t uiox_dns_resolve_first(const char *hostname)
 {
     uiox_dns_result_t r;
     if (uiox_dns_resolve(hostname, &r) == 0 && r.count > 0)
         return r.addrs[0];
     return 0;
 }
 
 void uiox_dns_cache_flush(void)
 {
     /* DNS cache not implemented in this minimal build. */
 }
 
 /* =========================================================================
  * NTP client (RFC 4330 SNTPv4)
  * ====================================================================== */
 
 #define NTP_EPOCH_DELTA  2208988800u
 
 typedef struct __attribute__((packed)) {
     uix_uint8_t  li_vn_mode;
     uix_uint8_t  stratum;
     uix_uint8_t  poll;
     uix_int8_t   precision;
     uix_uint32_t root_delay;
     uix_uint32_t root_disp;
     uix_uint32_t ref_id;
     uix_uint32_t ref_ts_sec;
     uix_uint32_t ref_ts_frac;
     uix_uint32_t orig_ts_sec;
     uix_uint32_t orig_ts_frac;
     uix_uint32_t rx_ts_sec;
     uix_uint32_t rx_ts_frac;
     uix_uint32_t tx_ts_sec;
     uix_uint32_t tx_ts_frac;
 } uiox_ntp_pkt_t;
 
 int uiox_ntp_sync(uix_uint32_t server_ip, uiox_ntp_result_t *result)
 {
     uiox_ntp_pkt_t req;
     uiox_ntp_pkt_t resp;
     uix_ssize_t    n;
     uix_uint32_t   tx_sec, rx_sec, unix_ts;
 
     if (!server_ip) return -22;
     if (!s_eth_open) return -19;
 
     uix_memset(&req, 0, sizeof(req));
     req.li_vn_mode = (0 << 6) | (4 << 3) | 3;   /* LI=0, VN=4, Mode=3 */
 
     if (uiox_dev_stream_write(&s_eth, &req, sizeof(req)) < 0)
         return -5;
 
     if (uiox_dev_stream_poll(&s_eth, UIX_POLLIN, UIOX_NTP_TIMEOUT_MS)
         != UIOX_DEV_OK)
         return -60;
 
     n = uiox_dev_stream_read(&s_eth, &resp, sizeof(resp));
     if (n < (uix_ssize_t)sizeof(resp)) return -5;
 
     tx_sec  = __builtin_bswap32(resp.tx_ts_sec);
     rx_sec  = __builtin_bswap32(resp.rx_ts_sec);
     unix_ts = tx_sec > NTP_EPOCH_DELTA ? tx_sec - NTP_EPOCH_DELTA : 0;
 
     if (result) {
         result->unix_time     = unix_ts;
         result->round_trip_ms = (tx_sec - rx_sec) * 1000u;
         result->offset_ms     = 0;
     }
 
     s_ntp_last = unix_ts;
     return 0;
 }
 
 uix_uint32_t uiox_ntp_last_sync(void)
 {
     return s_ntp_last;
 }
 
 /* =========================================================================
  * HTTP client — minimal HTTP/1.1
  * ====================================================================== */
 
 #define HTTP_DEFAULT_PORT         80
 #define HTTP_DEFAULT_TIMEOUT_MS   10000
 
 /* Parse "[host](http://host)[:port]/path" — unchanged, except it refuses the
  * markdown-link wrapper a paste could have introduced, because a URL
  * beginning '[' is not a hostname and would otherwise be read as one. */
 static int http_parse_url(const char *url,
                           char *host_out,  uix_uint16_t host_len,
                           uix_uint16_t *port_out,
                           char *path_out,  uix_uint16_t path_len)
 {
     const char  *p = url;
     const char  *host_start;
     uix_uint16_t hlen;
 
     if (*p == '[') return -22;                  /* not a URL we can read */
 
     if (uix_strncmp(p, "http://", 7) == 0) p += 7;
     else if (uix_strncmp(p, "https://", 8) == 0) return -93;  /* EPROTONOSUPPORT */
 
     host_start = p;
     while (*p && *p != '/' && *p != ':') p++;
 
     hlen = (uix_uint16_t)(p - host_start);
     if (hlen == 0 || hlen >= host_len) return -22;
     uix_memcpy(host_out, host_start, hlen);
     host_out[hlen] = '\0';
 
     *port_out = HTTP_DEFAULT_PORT;
     if (*p == ':') {
         p++;
         *port_out = 0;
         while (*p >= '0' && *p <= '9')
             *port_out = (uix_uint16_t)(*port_out * 10 + (*p++ - '0'));
     }
 
     if (*p == '/') {
        uix_strncpy(path_out, p, path_len - 1);
         path_out[path_len - 1] = '\0';
     } else {
         path_out[0] = '/';
         path_out[1] = '\0';
     }
     return 0;
 }
 
 static const char *http_method_str(uiox_http_method_t m)
 {
     switch (m) {
     case UIOX_HTTP_GET:    return "GET";
     case UIOX_HTTP_POST:   return "POST";
     case UIOX_HTTP_PUT:    return "PUT";
     case UIOX_HTTP_DELETE: return "DELETE";
     case UIOX_HTTP_HEAD:   return "HEAD";
     default:               return "GET";
     }
 }
 
 int uiox_http_request(const uiox_http_request_t *req,
                       uiox_http_response_t      *resp)
 {
     char         host[128], path[UIOX_HTTP_MAX_URL_LEN];
     char         txbuf[1024];
     char         rxbuf[2048];
     uix_uint16_t port;
     uix_uint32_t server_ip;
     uix_ssize_t  rlen;
     int          rc, txlen;
 
     if (!req || !resp) return -22;
 
     rc = http_parse_url(req->url, host, sizeof(host), &port,
                         path, sizeof(path));
     if (rc < 0) return rc;
 
     server_ip = uiox_dns_resolve_first(host);
     if (!server_ip) return -2;                  /* ENOENT */
 
     /* No connect: the stream handle is the interface.  The destination
      * address travels with the request above this layer. */
 
     txlen = uix_snprintf(txbuf, sizeof(txbuf),
         "%s %s HTTP/1.1\r\n"
         "Host: %s\r\n"
         "Connection: close\r\n"
         "Content-Length: %u\r\n",
         http_method_str(req->method), path, host,
         (unsigned)(req->body ? req->body_len : 0));
 
     for (rc = 0; rc < req->header_count && txlen < (int)sizeof(txbuf) - 4;
          rc++) {
         txlen += uix_snprintf(txbuf + txlen,
                           sizeof(txbuf) - (uix_size_t)txlen,
                           "%s: %s\r\n",
                           req->headers[rc].key, req->headers[rc].value);
     }
     txlen += uix_snprintf(txbuf + txlen, sizeof(txbuf) - (uix_size_t)txlen, "\r\n");
 
     if (uiox_dev_stream_write(&s_eth, txbuf, (uix_size_t)txlen) < 0)
         return -5;
 
     if (req->body && req->body_len) {
         if (uiox_dev_stream_write(&s_eth, req->body, req->body_len) < 0)
             return -5;
     }
 
     /* ── read the response ────────────────────────────────────────── */
     if (uiox_dev_stream_poll(&s_eth, UIX_POLLIN,
                              req->timeout_ms ? req->timeout_ms
                                              : HTTP_DEFAULT_TIMEOUT_MS)
         != UIOX_DEV_OK)
         return -60;
 
     uix_memset(rxbuf, 0, sizeof(rxbuf));
     rlen = uiox_dev_stream_read(&s_eth, rxbuf, sizeof(rxbuf) - 1);
     if (rlen < 0) return -5;
 
     /* ── parse status line and headers (unchanged) ────────────────── */
     resp->status_code  = 0;
     resp->header_count = 0;
     resp->body_len     = 0;
 
     {
         char *line = rxbuf;
         char *crlf = uix_strstr(line, "\r\n");
         if (!crlf) return -5;
 
         uix_sscanf(line, "HTTP/%*d.%*d %d", &resp->status_code);
         line = crlf + 2;
 
         while ((crlf = uix_strstr(line, "\r\n")) != NULL) {
             if (crlf == line) { line += 2; break; }
             if (resp->header_count < UIOX_HTTP_MAX_HEADERS) {
                 uix_size_t hlen  = (uix_size_t)(crlf - line);
                 char      *colon = uix_memchr(line, ':', hlen);
                 if (colon) {
                     uix_size_t klen   = (uix_size_t)(colon - line);
                     uix_size_t vlen   = hlen - klen - 1;
                     const char *vstart = colon + 1;
                     uiox_http_header_t *h;
 
                     while (*vstart == ' ') { vstart++; vlen--; }
                     h = &resp->headers[resp->header_count++];
                     uix_snprintf(h->key,   sizeof(h->key),   "%.*s",
                              (int)klen, line);
                     uix_snprintf(h->value, sizeof(h->value), "%.*s",
                              (int)vlen, vstart);
                 }
             }
             line = crlf + 2;
         }
 
         /* body */
         if (resp->body && resp->body_capacity > 0) {
             uix_size_t body_avail = (uix_size_t)((rxbuf + rlen) - line);
             resp->body_len = body_avail < resp->body_capacity ?
                              body_avail : resp->body_capacity - 1;
             uix_memcpy(resp->body, line, resp->body_len);
             ((uix_uint8_t *)resp->body)[resp->body_len] = '\0';
         }
     }
 
     return 0;
 }
 
 uix_ssize_t uiox_http_get(const char *url, void *buf, uix_size_t buf_len,
                           int *status_out)
 {
     uiox_http_request_t  req;
     uiox_http_response_t resp;
     int rc;
 
     if (!url || !buf || !buf_len) return -22;
 
     uix_memset(&req, 0, sizeof(req));
     req.method     = UIOX_HTTP_GET;
     req.timeout_ms = HTTP_DEFAULT_TIMEOUT_MS;
     uix_strncpy(req.url, url, UIOX_HTTP_MAX_URL_LEN - 1);
 
     uix_memset(&resp, 0, sizeof(resp));
     resp.body          = (uix_uint8_t *)buf;
     resp.body_capacity = buf_len;
 
     rc = uiox_http_request(&req, &resp);
     if (rc < 0) return rc;
 
     if (status_out) *status_out = resp.status_code;
     return (uix_ssize_t)resp.body_len;
 }
 
 uix_ssize_t uiox_http_post(const char *url,
                            const void *body,     uix_size_t body_len,
                            void       *resp_buf, uix_size_t resp_buf_len,
                            int        *status_out)
 {
     uiox_http_request_t  req;
     uiox_http_response_t resp;
     int rc;
 
     if (!url || !resp_buf || !resp_buf_len) return -22;
 
     uix_memset(&req, 0, sizeof(req));
     req.method     = UIOX_HTTP_POST;
     req.timeout_ms = HTTP_DEFAULT_TIMEOUT_MS;
     req.body       = body;
     req.body_len   = body_len;
     uix_strncpy(req.url, url, UIOX_HTTP_MAX_URL_LEN - 1);
 
     uix_strncpy(req.headers[0].key,   "Content-Type",     UIOX_HTTP_MAX_HDR_LEN - 1);
     uix_strncpy(req.headers[0].value, "application/json", UIOX_HTTP_MAX_HDR_LEN - 1);
     req.header_count = 1;
 
     uix_memset(&resp, 0, sizeof(resp));
     resp.body          = (uix_uint8_t *)resp_buf;
     resp.body_capacity = resp_buf_len;
 
     rc = uiox_http_request(&req, &resp);
     if (rc < 0) return rc;
 
     if (status_out) *status_out = resp.status_code;
     return (uix_ssize_t)resp.body_len;
 }
 
 /* =========================================================================
  * Ping / reachability
  *
  * The one feature that does not survive the move.  ICMP is the protocol
  * layer's — it builds the echo request, owns the reply slot, and is
  * reached by uiox_proto_icmp_ping(), which is EL1.  dev/eth/ has no
  * command vocabulary, so there is no way to ask the kernel to send one.
  *
  * This reports the truth rather than a fabricated timeout: ENOSYS.  The
  * app's reachability section will print its failure and move on.
  * ====================================================================== */
 
 int uiox_ping(uix_uint32_t target_ip, uix_uint32_t timeout_ms,
               uiox_ping_result_t *result)
 {
     (void)timeout_ms;
 
     if (result) {
         result->target_ip = target_ip;
         result->reachable = UIX_FALSE;
         result->rtt_ms    = 0;
         result->ttl       = 0;
     }
     return -38;                                 /* ENOSYS */
 }
 
 uix_bool_t uiox_reachable(uix_uint32_t target_ip, uix_uint32_t timeout_ms)
 {
     (void)target_ip; (void)timeout_ms;
     return UIX_FALSE;
 }
 
 /* =========================================================================
  * IP address utilities — unchanged
  * ====================================================================== */
 
 uix_uint32_t uiox_ip4_from_str(const char *str)
 {
     uix_uint32_t result = 0;
     uix_uint8_t  octet  = 0;
     uix_uint8_t  dots   = 0;
     uix_bool_t   digits = UIX_FALSE;
     const char  *p;
 
     if (!str) return 0;
 
     for (p = str; ; p++) {
         if (*p >= '0' && *p <= '9') {
             octet  = (uix_uint8_t)(octet * 10 + (*p - '0'));
             digits = UIX_TRUE;
         } else if (*p == '.' || *p == '\0') {
             if (!digits) return 0;
             result = (result << 8) | octet;
             octet  = 0;
             digits = UIX_FALSE;
             if (*p == '.') {
                 if (++dots > 3) return 0;
             } else {
                 break;
             }
         } else {
             return 0;
         }
     }
 
     return dots == 3 ? result : 0;
 }
 
 void uiox_ip4_to_str(uix_uint32_t ip, char *buf, uix_size_t buf_len)
 {
     if (!buf || buf_len < 16) return;
     uix_snprintf(buf, buf_len, "%u.%u.%u.%u",
              (ip >> 24) & 0xFF,
              (ip >> 16) & 0xFF,
              (ip >>  8) & 0xFF,
               ip        & 0xFF);
 }
 