/**
 * @file    uiox_proto.c
 * @brief   UIOX protocol stack — IPv4, ARP, UDP, TCP, ICMP, routing.
 * @date    2026-05-25
 */

 #include "uiox_proto.h"
 #include "uiox_socket.h"
 #include <string.h>
 #include <errno.h>
 
 /* =========================================================================
  * Routing table
  * ====================================================================== */
 
 static uiox_route_t s_route_table[UIOX_ROUTE_TABLE_SIZE];
 
 void uiox_proto_init(void)
 {
     memset(s_route_table, 0, sizeof(s_route_table));
 }
 
 int uiox_route_add(uint32_t network, uint32_t mask,
                    uint32_t gateway, uiox_netif_t *netif, uint8_t metric)
 {
     for (int i = 0; i < UIOX_ROUTE_TABLE_SIZE; i++) {
         if (!s_route_table[i].valid) {
             s_route_table[i].network = network;
             s_route_table[i].mask    = mask;
             s_route_table[i].gateway = gateway;
             s_route_table[i].netif   = netif;
             s_route_table[i].metric  = metric;
             s_route_table[i].valid   = true;
             return 0;
         }
     }
     return -ENOMEM;
 }
 
 void uiox_route_del(uint32_t network, uint32_t mask)
 {
     for (int i = 0; i < UIOX_ROUTE_TABLE_SIZE; i++) {
         if (s_route_table[i].valid &&
             s_route_table[i].network == network &&
             s_route_table[i].mask    == mask) {
             s_route_table[i].valid = false;
             return;
         }
     }
 }
 
 const uiox_route_t *uiox_route_lookup(uint32_t dst_ip)
 {
     const uiox_route_t *best   = NULL;
     uint8_t             best_m = 0;
 
     for (int i = 0; i < UIOX_ROUTE_TABLE_SIZE; i++) {
         const uiox_route_t *r = &s_route_table[i];
         if (!r->valid) continue;
         if ((dst_ip & r->mask) != r->network) continue;
 
         /* Longest-prefix match */
         uint8_t bits = __builtin_popcount(r->mask);
         if (!best || bits > best_m ||
             (bits == best_m && r->metric < best->metric)) {
             best   = r;
             best_m = bits;
         }
     }
     return best;
 }
 
 /* =========================================================================
  * Checksum utilities
  * ====================================================================== */
 
 uint16_t uiox_inet_checksum(const void *data, uint16_t len)
 {
     const uint8_t *p   = (const uint8_t *)data;
     uint32_t       sum = 0;
 
     while (len > 1) {
         sum += (uint32_t)((p[0] << 8) | p[1]);
         p   += 2;
         len -= 2;
     }
     if (len) sum += (uint32_t)(p[0] << 8);
 
     while (sum >> 16)
         sum = (sum & 0xFFFF) + (sum >> 16);
 
     return (uint16_t)(~sum);
 }
 
 uint16_t uiox_proto_checksum4(uint32_t src, uint32_t dst,
                                uint8_t proto,
                                const void *data, uint16_t len)
 {
     /* RFC 793 pseudo-header */
     struct __attribute__((packed)) {
         uint32_t src;
         uint32_t dst;
         uint8_t  zero;
         uint8_t  proto;
         uint16_t len;
     } ph;
 
     ph.src   = __builtin_bswap32(src);
     ph.dst   = __builtin_bswap32(dst);
     ph.zero  = 0;
     ph.proto = proto;
     ph.len   = __builtin_bswap16(len);
 
     uint32_t sum = 0;
     const uint8_t *p;
     uint16_t n;
 
     /* Accumulate pseudo-header */
     p = (const uint8_t *)&ph;
     n = (uint16_t)sizeof(ph);
     while (n > 1) { sum += (uint32_t)((p[0] << 8) | p[1]); p += 2; n -= 2; }
 
     /* Accumulate payload */
     p = (const uint8_t *)data;
     n = len;
     while (n > 1) { sum += (uint32_t)((p[0] << 8) | p[1]); p += 2; n -= 2; }
     if (n) sum += (uint32_t)(p[0] << 8);
 
     while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
     return (uint16_t)(~sum);
 }
 
 /* =========================================================================
  * ARP input / output
  * ====================================================================== */
 
 /* Pending ARP reply notification — simple semaphore slot */
 static struct {
     uint32_t ip4;
     uint8_t  mac[UIOX_HW_MAC_ADDR_LEN];
     bool     ready;
 } s_arp_pending;
 
 void uiox_arp_input(uiox_netif_t *netif, uiox_netbuf_t *buf)
 {
     if (buf->len < sizeof(uiox_arp_hdr_t)) {
         uiox_netbuf_free(buf);
         return;
     }
 
     uiox_arp_hdr_t *arp = (uiox_arp_hdr_t *)buf->data;
     uint16_t op = (uint16_t)((arp->operation >> 8) | (arp->operation << 8));
 
     uint32_t sender_ip = __builtin_bswap32(arp->sender_ip);
     uint32_t target_ip = __builtin_bswap32(arp->target_ip);
 
     /* Always update ARP cache with sender info */
     uiox_arp_set(netif, sender_ip, arp->sender_mac);
 
     if (op == 1 && target_ip == netif->ip4_addr) {
         /* ARP Request for us — send reply */
         uiox_netbuf_t *reply = uiox_netbuf_alloc();
         if (!reply) { uiox_netbuf_free(buf); return; }
 
         uiox_arp_hdr_t *r = (uiox_arp_hdr_t *)uiox_netbuf_put(reply,
                                                 sizeof(uiox_arp_hdr_t));
         r->hw_type    = arp->hw_type;
         r->proto_type = arp->proto_type;
         r->hw_len     = 6;
         r->proto_len  = 4;
         r->operation  = __builtin_bswap16(2);   /* reply */
         memcpy(r->sender_mac, netif->mac, UIOX_HW_MAC_ADDR_LEN);
         r->sender_ip  = __builtin_bswap32(netif->ip4_addr);
         memcpy(r->target_mac, arp->sender_mac, UIOX_HW_MAC_ADDR_LEN);
         r->target_ip  = arp->sender_ip;
 
         reply->proto = UIOX_ETHERTYPE_ARP;
         if (netif->output)
             netif->output(netif, reply, sender_ip);
 
     } else if (op == 2) {
         /* ARP Reply — wake pending resolver */
         if (s_arp_pending.ip4 == sender_ip) {
             memcpy(s_arp_pending.mac, arp->sender_mac, UIOX_HW_MAC_ADDR_LEN);
             s_arp_pending.ready = true;
         }
     }
 
     uiox_netbuf_free(buf);
 }
 
 int uiox_proto_arp_request(uiox_netif_t *netif, uint32_t ip4,
                             uint8_t mac_out[UIOX_HW_MAC_ADDR_LEN],
                             uint32_t timeout_ms)
 {
     uiox_netbuf_t *buf = uiox_netbuf_alloc();
     if (!buf) return -ENOMEM;
 
     uiox_arp_hdr_t *arp = (uiox_arp_hdr_t *)uiox_netbuf_put(buf,
                                               sizeof(uiox_arp_hdr_t));
     arp->hw_type    = __builtin_bswap16(1);
     arp->proto_type = __builtin_bswap16(UIOX_ETHERTYPE_IP4);
     arp->hw_len     = 6;
     arp->proto_len  = 4;
     arp->operation  = __builtin_bswap16(1);   /* request */
     memcpy(arp->sender_mac, netif->mac, UIOX_HW_MAC_ADDR_LEN);
     arp->sender_ip  = __builtin_bswap32(netif->ip4_addr);
     memset(arp->target_mac, 0xFF, UIOX_HW_MAC_ADDR_LEN);
     arp->target_ip  = __builtin_bswap32(ip4);
 
     s_arp_pending.ip4   = ip4;
     s_arp_pending.ready = false;
 
     buf->proto = UIOX_ETHERTYPE_ARP;
     if (netif->output)
         netif->output(netif, buf, 0xFFFFFFFFu);  /* broadcast */
 
     /* Spin-wait — replace with OS semaphore/event on RTOS */
     uint32_t waited = 0;
     while (!s_arp_pending.ready && waited < timeout_ms) {
         /* uiox_os_sleep_ms(1); */
         waited++;
     }
 
     if (!s_arp_pending.ready) return -ETIMEDOUT;
 
     memcpy(mac_out, s_arp_pending.mac, UIOX_HW_MAC_ADDR_LEN);
     uiox_arp_set(netif, ip4, mac_out);
     return 0;
 }
 
 /* =========================================================================
  * IPv4 receive path
  * ====================================================================== */
 
 void uiox_proto_ip4_input(uiox_netif_t *netif, uiox_netbuf_t *buf)
 {
     if (buf->len < sizeof(uiox_ip4_hdr_t)) {
         netif->stats.rx_errors++;
         uiox_netbuf_free(buf);
         return;
     }
 
     uiox_ip4_hdr_t *ip = (uiox_ip4_hdr_t *)buf->data;
     uint8_t  ihl   = (ip->ver_ihl & 0x0F) << 2;
     uint16_t total = __builtin_bswap16(ip->total_len);
 
     /* Basic sanity */
     if ((ip->ver_ihl >> 4) != 4 || ihl < 20 ||
         total > buf->len || uiox_inet_checksum(ip, ihl) != 0) {
         netif->stats.rx_errors++;
         uiox_netbuf_free(buf);
         return;
     }
 
     /* Trim buffer to IP total_len */
     uiox_netbuf_trim(buf, buf->len - total);
 
     /* Strip IP header */
     uiox_netbuf_pull(buf, ihl);
 
     uint32_t src_ip = __builtin_bswap32(ip->src);
     uint32_t dst_ip = __builtin_bswap32(ip->dst);
 
     /* Dispatch to transport layer */
     switch (ip->proto) {
     case UIOX_IPPROTO_UDP:
         uiox_proto_udp_input(netif, buf, src_ip, dst_ip);
         break;
     case UIOX_IPPROTO_TCP:
         uiox_proto_tcp_input(netif, buf, src_ip, dst_ip);
         break;
     case UIOX_IPPROTO_ICMP:
         uiox_proto_icmp_input(netif, buf, src_ip, dst_ip);
         break;
     default:
         uiox_netbuf_free(buf);
         break;
     }
 }
 
 /* Stub — IPv6 receive deferred to full dual-stack extension */
 void uiox_proto_ip6_input(uiox_netif_t *netif, uiox_netbuf_t *buf)
 {
     (void)netif;
     uiox_netbuf_free(buf);   /* Drop until IPv6 stack is implemented */
 }
 
 /* =========================================================================
  * ICMP
  * ====================================================================== */
 
 static void uiox_proto_icmp_input(uiox_netif_t *netif, uiox_netbuf_t *buf,
                                    uint32_t src_ip, uint32_t dst_ip)
 {
     (void)dst_ip;
     if (buf->len < sizeof(uiox_icmp_hdr_t)) {
         uiox_netbuf_free(buf); return;
     }
 
     uiox_icmp_hdr_t *icmp = (uiox_icmp_hdr_t *)buf->data;
 
     if (icmp->type == UIOX_ICMP_ECHO_REQUEST) {
         /* Reuse buffer as echo reply */
         icmp->type     = UIOX_ICMP_ECHO_REPLY;
         icmp->checksum = 0;
         icmp->checksum = uiox_inet_checksum(icmp, buf->len);
 
         /* Send back to requester */
         uiox_proto_ip4_send(netif, netif->ip4_addr, src_ip,
                             UIOX_IPPROTO_ICMP, buf);
     } else {
         uiox_netbuf_free(buf);
     }
 }
 
 int uiox_proto_icmp_ping(uint32_t dst_ip, uint16_t id,
                           uint16_t seq,    uint32_t timeout_ms)
 {
     const uiox_route_t *route = uiox_route_lookup(dst_ip);
     if (!route) return -ENETUNREACH;
 
     uiox_netbuf_t *buf = uiox_netbuf_alloc();
     if (!buf) return -ENOMEM;
 
     uiox_icmp_hdr_t *icmp = (uiox_icmp_hdr_t *)uiox_netbuf_put(buf,
                                                  sizeof(uiox_icmp_hdr_t));
     icmp->type     = UIOX_ICMP_ECHO_REQUEST;
     icmp->code     = 0;
     icmp->checksum = 0;
     icmp->rest     = __builtin_bswap32(((uint32_t)id << 16) | seq);
     icmp->checksum = uiox_inet_checksum(icmp, buf->len);
 
     return uiox_proto_ip4_send(route->netif, route->netif->ip4_addr,
                                dst_ip, UIOX_IPPROTO_ICMP, buf);
 }
 
 /* =========================================================================
  * IPv4 send path (internal)
  * ====================================================================== */
 
 static int uiox_proto_ip4_send(uiox_netif_t *netif,
                                 uint32_t src_ip, uint32_t dst_ip,
                                 uint8_t proto,   uiox_netbuf_t *buf)
 {
     uiox_ip4_hdr_t *ip = (uiox_ip4_hdr_t *)uiox_netbuf_push(buf,
                                              sizeof(uiox_ip4_hdr_t));
     if (!ip) { uiox_netbuf_free(buf); return -ENOBUFS; }
 
     static uint16_t s_ip_id = 0;
 
     ip->ver_ihl   = UIOX_IP4_VER_IHL_DEFAULT;
     ip->tos       = 0;
     ip->total_len = __builtin_bswap16((uint16_t)buf->len);
     ip->id        = __builtin_bswap16(s_ip_id++);
     ip->frag_off  = 0;
     ip->ttl       = UIOX_IP4_TTL_DEFAULT;
     ip->proto     = proto;
     ip->checksum  = 0;
     ip->src       = __builtin_bswap32(src_ip);
     ip->dst       = __builtin_bswap32(dst_ip);
     ip->checksum  = uiox_inet_checksum(ip, sizeof(uiox_ip4_hdr_t));
 
     if (!netif->output) { uiox_netbuf_free(buf); return -ENETDOWN; }
     return netif->output(netif, buf, dst_ip);
 }
 
 /* =========================================================================
  * UDP
  * ====================================================================== */
 
 static void uiox_proto_udp_input(uiox_netif_t *netif, uiox_netbuf_t *buf,
                                   uint32_t src_ip, uint32_t dst_ip)
 {
     if (buf->len < sizeof(uiox_udp_hdr_t)) {
         uiox_netbuf_free(buf); return;
     }
     uiox_udp_hdr_t *udp      = (uiox_udp_hdr_t *)buf->data;
     uint16_t        src_port = __builtin_bswap16(udp->src_port);
     uint16_t        dst_port = __builtin_bswap16(udp->dst_port);
 
     uiox_netbuf_pull(buf, sizeof(uiox_udp_hdr_t));
 
     /* Deliver to matching socket */
     uiox_socket_deliver(UIOX_IPPROTO_UDP, src_ip, src_port,
                         dst_ip, dst_port, buf);
 }
 
 int uiox_proto_udp_send(uint32_t src_ip,  uint32_t dst_ip,
                          uint16_t src_port, uint16_t dst_port,
                          uiox_netbuf_t *buf)
 {
     uiox_udp_hdr_t *udp = (uiox_udp_hdr_t *)uiox_netbuf_push(buf,
                                               sizeof(uiox_udp_hdr_t));
     if (!udp) { uiox_netbuf_free(buf); return -ENOBUFS; }
 
     udp->src_port = __builtin_bswap16(src_port);
     udp->dst_port = __builtin_bswap16(dst_port);
     udp->len      = __builtin_bswap16((uint16_t)buf->len);
     udp->checksum = 0;
     udp->checksum = uiox_proto_checksum4(src_ip, dst_ip,
                                           UIOX_IPPROTO_UDP,
                                           udp, (uint16_t)buf->len);
 
     const uiox_route_t *route = uiox_route_lookup(dst_ip);
     if (!route) { uiox_netbuf_free(buf); return -ENETUNREACH; }
 
     uint32_t actual_src = src_ip ? src_ip : route->netif->ip4_addr;
     return uiox_proto_ip4_send(route->netif, actual_src,
                                dst_ip, UIOX_IPPROTO_UDP, buf);
 }
 
 /* =========================================================================
  * TCP (minimal — SYN/ACK/RST/FIN state machine)
  * ====================================================================== */
 
 static void uiox_proto_tcp_input(uiox_netif_t *netif, uiox_netbuf_t *buf,
                                   uint32_t src_ip, uint32_t dst_ip)
 {
     if (buf->len < sizeof(uiox_tcp_hdr_t)) {
         uiox_netbuf_free(buf); return;
     }
     uiox_tcp_hdr_t *tcp      = (uiox_tcp_hdr_t *)buf->data;
     uint16_t        src_port = __builtin_bswap16(tcp->src_port);
     uint16_t        dst_port = __builtin_bswap16(tcp->dst_port);
     uint8_t         hdr_len  = (tcp->data_off >> 4) << 2;
 
     uiox_netbuf_pull(buf, hdr_len);
 
     uiox_socket_deliver(UIOX_IPPROTO_TCP, src_ip, src_port,
                         dst_ip, dst_port, buf);
 }
 
 int uiox_proto_tcp_send(uint32_t src_ip,  uint32_t dst_ip,
                          uint16_t src_port, uint16_t dst_port,
                          uint32_t seq,      uint32_t ack,
                          uint8_t  flags,    uiox_netbuf_t *buf)
 {
     uiox_tcp_hdr_t *tcp = (uiox_tcp_hdr_t *)uiox_netbuf_push(buf,
                                               sizeof(uiox_tcp_hdr_t));
     if (!tcp) { uiox_netbuf_free(buf); return -ENOBUFS; }
 
     tcp->src_port = __builtin_bswap16(src_port);
     tcp->dst_port = __builtin_bswap16(dst_port);
     tcp->seq      = __builtin_bswap32(seq);
     tcp->ack      = __builtin_bswap32(ack);
     tcp->data_off = (uint8_t)((sizeof(uiox_tcp_hdr_t) / 4) << 4);
     tcp->flags    = flags;
     tcp->window   = __builtin_bswap16(4096);
     tcp->checksum = 0;
     tcp->urg_ptr  = 0;
     tcp->checksum = uiox_proto_checksum4(src_ip, dst_ip,
                                           UIOX_IPPROTO_TCP,
                                           tcp, (uint16_t)buf->len);
 
     const uiox_route_t *route = uiox_route_lookup(dst_ip);
     if (!route) { uiox_netbuf_free(buf); return -ENETUNREACH; }
 
     uint32_t actual_src = src_ip ? src_ip : route->netif->ip4_addr;
     return uiox_proto_ip4_send(route->netif, actual_src,
                                dst_ip, UIOX_IPPROTO_TCP, buf);
 }
 