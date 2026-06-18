/**
 * @file    uiox_connectivity.c
 * @brief   UIOX high-level connectivity feature implementation.
 *
 * Implements stack lifecycle, DHCP, DNS, NTP, HTTP, and ping on top
 * of the UIOX socket and protocol layers.
 *
 * @date    2026-05-25
 */

 #include "uiox_connectivity.h"
 #include "uiox_proto.h"
 #include "uiox_socket.h"
 #include "uiox_netbuf.h"
 #include <string.h>
 #include <stdio.h>
 #include <errno.h>
 
 /* =========================================================================
  * Module-private state
  * ====================================================================== */
 
 static const uiox_net_config_t *s_cfg        = NULL;
 static uiox_netif_t             s_primary_if;
 static uiox_dhcp_lease_t        s_dhcp_lease;
 static bool                     s_dhcp_active = false;
 static uint32_t                 s_ntp_last    = 0;
 
 static uint32_t  s_dns_servers[UIOX_DNS_MAX_SERVERS];
 static uint8_t   s_dns_count   = 0;
 
 /* =========================================================================
  * Internal helpers
  * ====================================================================== */
 
 static void fire_event(uiox_net_evt_t evt)
 {
     if (s_cfg && s_cfg->evt_cb)
         s_cfg->evt_cb(evt, &s_primary_if, s_cfg->evt_ctx);
 }
 
 static void link_state_cb(uiox_netif_t *netif, bool up)
 {
     (void)netif;
     fire_event(up ? UIOX_NET_EVT_LINK_UP : UIOX_NET_EVT_LINK_DOWN);
 }
 
 /* =========================================================================
  * Stack lifecycle
  * ====================================================================== */
 
 int uiox_net_init(const uiox_net_config_t *cfg)
 {
     if (!cfg || !cfg->hw || !cfg->hw_ops) return -EINVAL;
     s_cfg = cfg;
 
     /* 1. Initialise subsystems */
     uiox_netbuf_pool_init();
     uiox_netif_subsystem_init();
     uiox_socket_subsystem_init();
     uiox_proto_init();
 
     /* 2. Prepare netif */
     memset(&s_primary_if, 0, sizeof(s_primary_if));
     strncpy(s_primary_if.name, cfg->ifname, UIOX_NETIF_NAME_LEN - 1);
     s_primary_if.mtu     = cfg->mtu ? cfg->mtu : UIOX_HW_MTU_ETHERNET;
     s_primary_if.hw      = cfg->hw;
     s_primary_if.link_cb = link_state_cb;
 
     /* Use provided MAC or read from hardware */
     bool mac_zero = true;
     for (int i = 0; i < UIOX_HW_MAC_ADDR_LEN; i++)
         if (cfg->mac[i]) { mac_zero = false; break; }
 
     if (!mac_zero)
         memcpy(s_primary_if.mac, cfg->mac, UIOX_HW_MAC_ADDR_LEN);
     else
         memcpy(s_primary_if.mac, cfg->hw->mac_addr, UIOX_HW_MAC_ADDR_LEN);
 
     /* 3. Hardware init */
     int rc = uiox_hw_init(cfg->hw, cfg->hw_ops);
     if (rc < 0) return rc;
 
     /* 4. Register and bring interface up */
     rc = uiox_netif_register(&s_primary_if);
     if (rc < 0) return rc;
 
     rc = uiox_netif_up(&s_primary_if);
     if (rc < 0) return rc;
 
     /* 5. IP addressing */
     if (cfg->addr_mode == UIOX_ADDR_MODE_DHCP) {
         rc = uiox_dhcp_request(&s_primary_if, &s_dhcp_lease);
         if (rc < 0) {
             fire_event(UIOX_NET_EVT_ERROR);
             return rc;
         }
         s_dhcp_active = true;
 
         /* Apply lease */
         uiox_netif_set_ip4(&s_primary_if,
                             s_dhcp_lease.ip,
                             s_dhcp_lease.mask,
                             s_dhcp_lease.gateway);
 
         /* Install default route */
         uiox_route_add(0, 0, s_dhcp_lease.gateway, &s_primary_if, 100);
 
         /* DNS from DHCP */
         uiox_dns_set_servers(s_dhcp_lease.dns, s_dhcp_lease.dns_count);
 
     } else {
         uiox_netif_set_ip4(&s_primary_if,
                             cfg->static_ip,
                             cfg->static_mask,
                             cfg->static_gw);
 
         uiox_route_add(cfg->static_ip & cfg->static_mask,
                        cfg->static_mask, 0, &s_primary_if, 10);
         uiox_route_add(0, 0, cfg->static_gw, &s_primary_if, 100);
         uiox_dns_set_servers(cfg->dns_servers, cfg->dns_server_count);
     }
 
     fire_event(UIOX_NET_EVT_IP_ACQUIRED);
 
     /* 6. DNS servers configured */
     if (s_dns_count > 0)
         fire_event(UIOX_NET_EVT_DNS_READY);
 
     /* 7. Optional NTP sync */
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
         uiox_dhcp_release(&s_primary_if);
         s_dhcp_active = false;
     }
     uiox_netif_down(&s_primary_if);
     uiox_netif_unregister(&s_primary_if);
     uiox_hw_down(s_primary_if.hw);
     s_cfg = NULL;
 }
 
 uiox_netif_t *uiox_net_primary_if(void)
 {
     return &s_primary_if;
 }
 
 uint32_t uiox_net_local_ip(void)
 {
     return s_primary_if.ip4_addr;
 }
 
 void uiox_net_tick(uint32_t now_ms)
 {
     static uint32_t last_arp_gc_ms  = 0;
     static uint32_t last_dhcp_ms    = 0;
 
     /* ARP GC every 60 seconds */
     if (now_ms - last_arp_gc_ms >= 60000) {
         uiox_arp_gc(&s_primary_if, now_ms / 1000);
         last_arp_gc_ms = now_ms;
     }
 
     /* DHCP renew at T1 */
     if (s_dhcp_active &&
         s_dhcp_lease.renew_time_s > 0 &&
         (now_ms - last_dhcp_ms) >= (s_dhcp_lease.renew_time_s * 1000u)) {
         uiox_dhcp_renew(&s_primary_if, &s_dhcp_lease);
         last_dhcp_ms = now_ms;
     }
 }
 
 /* =========================================================================
  * DHCP client
  *
  * Implements a minimal DHCP DORA exchange over UDP port 67/68.
  * Message format: RFC 2131.
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
 
 /* DHCP option tags */
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
     uint8_t  op, htype, hlen, hops;
     uint32_t xid;
     uint16_t secs, flags;
     uint32_t ciaddr, yiaddr, siaddr, giaddr;
     uint8_t  chaddr[16];
     uint8_t  sname[64];
     uint8_t  file[128];
     uint32_t magic;
     uint8_t  options[308];
 } uiox_dhcp_msg_t;
 
 static uint8_t *dhcp_opt_find(uint8_t *opts, uint16_t opt_len, uint8_t tag)
 {
     uint16_t i = 0;
     while (i < opt_len) {
         if (opts[i] == DHCP_OPT_END) break;
         if (opts[i] == 0) { i++; continue; }
         if (opts[i] == tag) return &opts[i];
         i += 2 + opts[i + 1];
     }
     return NULL;
 }
 
 static int dhcp_transact(uiox_netif_t *netif, uint8_t msg_type,
                           uint32_t server_ip, uint32_t xid,
                           uiox_dhcp_lease_t *out)
 {
     int fd = uiox_socket(UIOX_AF_INET, UIOX_SOCK_DGRAM, 0);
     if (fd < 0) return fd;
 
     int one = 1;
     uiox_setsockopt(fd, UIOX_SOL_SOCKET, UIOX_SO_REUSEADDR, &one, sizeof(one));
 
     uint32_t to = UIOX_DHCP_TIMEOUT_MS;
     uiox_setsockopt(fd, UIOX_SOL_SOCKET, UIOX_SO_RCVTIMEO, &to, sizeof(to));
 
     uiox_sockaddr_in_t local = {
        .family = UIOX_AF_INET,
        .addr   = 0,
        .port   = DHCP_CLIENT_PORT
    };
    uiox_bind(fd, &local);

    /* Build DHCP message */
    uiox_dhcp_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.op    = DHCP_OP_REQUEST;
    msg.htype = 1;                  /* Ethernet */
    msg.hlen  = UIOX_HW_MAC_ADDR_LEN;
    msg.xid   = __builtin_bswap32(xid);
    msg.flags = __builtin_bswap16(0x8000); /* Broadcast flag */
    msg.magic = __builtin_bswap32(DHCP_MAGIC_COOKIE);
    memcpy(msg.chaddr, netif->mac, UIOX_HW_MAC_ADDR_LEN);

    /* Build options */
    uint8_t *opt = msg.options;
    *opt++ = DHCP_OPT_MSG_TYPE; *opt++ = 1; *opt++ = msg_type;

    if (msg_type == DHCP_MSG_REQUEST && server_ip) {
        *opt++ = DHCP_OPT_SERVER_ID; *opt++ = 4;
        uint32_t sip = __builtin_bswap32(server_ip);
        memcpy(opt, &sip, 4); opt += 4;
    }
    *opt++ = DHCP_OPT_END;

    /* Send to broadcast */
    uiox_sockaddr_in_t dst = {
        .family = UIOX_AF_INET,
        .addr   = 0xFFFFFFFFu,
        .port   = DHCP_SERVER_PORT
    };
    uiox_sendto(fd, &msg, sizeof(msg), 0, &dst);

    /* Receive reply */
    uiox_dhcp_msg_t reply;
    memset(&reply, 0, sizeof(reply));
    int rc = (int)uiox_recv(fd, &reply, sizeof(reply), 0);
    uiox_close(fd);

    if (rc < 0) return rc;
    if (__builtin_bswap32(reply.xid) != xid) return -EAGAIN;

    /* Parse options into lease */
    if (out) {
        memset(out, 0, sizeof(*out));
        out->ip         = __builtin_bswap32(reply.yiaddr);
        out->server_ip  = server_ip;

        uint16_t opt_len = (uint16_t)(rc -
                            (int)offsetof(uiox_dhcp_msg_t, options));
        uint8_t *o;

        o = dhcp_opt_find(reply.options, opt_len, DHCP_OPT_SUBNET);
        if (o && o[1] == 4)
            memcpy(&out->mask, o + 2, 4),
            out->mask = __builtin_bswap32(out->mask);

        o = dhcp_opt_find(reply.options, opt_len, DHCP_OPT_ROUTER);
        if (o && o[1] >= 4)
            memcpy(&out->gateway, o + 2, 4),
            out->gateway = __builtin_bswap32(out->gateway);

        o = dhcp_opt_find(reply.options, opt_len, DHCP_OPT_DNS);
        if (o) {
            out->dns_count = (uint8_t)(o[1] / 4);
            if (out->dns_count > UIOX_DNS_MAX_SERVERS)
                out->dns_count = UIOX_DNS_MAX_SERVERS;
            for (int i = 0; i < out->dns_count; i++) {
                memcpy(&out->dns[i], o + 2 + i * 4, 4);
                out->dns[i] = __builtin_bswap32(out->dns[i]);
            }
        }

        o = dhcp_opt_find(reply.options, opt_len, DHCP_OPT_LEASE);
        if (o && o[1] == 4) {
            memcpy(&out->lease_time_s, o + 2, 4);
            out->lease_time_s = __builtin_bswap32(out->lease_time_s);
        }

        o = dhcp_opt_find(reply.options, opt_len, DHCP_OPT_RENEW);
        if (o && o[1] == 4) {
            memcpy(&out->renew_time_s, o + 2, 4);
            out->renew_time_s = __builtin_bswap32(out->renew_time_s);
        }

        o = dhcp_opt_find(reply.options, opt_len, DHCP_OPT_REBIND);
        if (o && o[1] == 4) {
            memcpy(&out->rebind_time_s, o + 2, 4);
            out->rebind_time_s = __builtin_bswap32(out->rebind_time_s);
        }

        o = dhcp_opt_find(reply.options, opt_len, DHCP_OPT_SERVER_ID);
        if (o && o[1] == 4) {
            memcpy(&out->server_ip, o + 2, 4);
            out->server_ip = __builtin_bswap32(out->server_ip);
        }
    }
    return 0;
}

int uiox_dhcp_request(uiox_netif_t *netif, uiox_dhcp_lease_t *lease_out)
{
    if (!netif) return -EINVAL;

    static uint32_t s_xid = 0xABCD1234u;
    uint32_t xid = s_xid++;

    uiox_dhcp_lease_t offer;

    /* DISCOVER */
    for (int attempt = 0; attempt < UIOX_DHCP_RETRY_COUNT; attempt++) {
        int rc = dhcp_transact(netif, DHCP_MSG_DISCOVER, 0, xid, &offer);
        if (rc == 0) goto got_offer;
    }
    return -ETIMEDOUT;

got_offer:
    /* REQUEST */
    for (int attempt = 0; attempt < UIOX_DHCP_RETRY_COUNT; attempt++) {
        int rc = dhcp_transact(netif, DHCP_MSG_REQUEST,
                               offer.server_ip, xid, lease_out);
        if (rc == 0) return 0;
    }
    return -ETIMEDOUT;
}

void uiox_dhcp_release(uiox_netif_t *netif)
{
    if (!netif || !s_dhcp_lease.server_ip) return;
    dhcp_transact(netif, DHCP_MSG_RELEASE,
                  s_dhcp_lease.server_ip, 0xDEADBEEFu, NULL);
    memset(&s_dhcp_lease, 0, sizeof(s_dhcp_lease));
    uiox_netif_set_ip4(netif, 0, 0, 0);
    fire_event(UIOX_NET_EVT_IP_LOST);
}

int uiox_dhcp_renew(uiox_netif_t *netif, uiox_dhcp_lease_t *lease_out)
{
    if (!netif) return -EINVAL;
    static uint32_t s_xid = 0xCAFE0000u;
    int rc = dhcp_transact(netif, DHCP_MSG_REQUEST,
                           s_dhcp_lease.server_ip, s_xid++, lease_out);
    if (rc == 0 && lease_out)
        memcpy(&s_dhcp_lease, lease_out, sizeof(s_dhcp_lease));
    return rc;
}

bool uiox_dhcp_lease(uiox_netif_t *netif, uiox_dhcp_lease_t *out)
{
    (void)netif;
    if (!s_dhcp_active || !out) return false;
    memcpy(out, &s_dhcp_lease, sizeof(*out));
    return true;
}

/* =========================================================================
 * DNS resolver
 *
 * Implements a minimal RFC 1035 DNS client over UDP port 53.
 * Supports A record queries only (IPv4).
 * ====================================================================== */

#define DNS_PORT        53
#define DNS_HDR_LEN     12
#define DNS_MAX_MSG     512

/* DNS header flags */
#define DNS_FLAG_QR     0x8000u  /* Response bit          */
#define DNS_FLAG_RD     0x0100u  /* Recursion desired     */
#define DNS_FLAG_RA     0x0080u  /* Recursion available   */
#define DNS_QTYPE_A     1        /* IPv4 address record   */
#define DNS_QCLASS_IN   1        /* Internet class        */

void uiox_dns_set_servers(const uint32_t *servers, uint8_t count)
{
    s_dns_count = count < UIOX_DNS_MAX_SERVERS ? count : UIOX_DNS_MAX_SERVERS;
    for (int i = 0; i < s_dns_count; i++)
        s_dns_servers[i] = servers[i];
}

/* Encode hostname into DNS wire format labels */
static uint16_t dns_encode_name(const char *hostname,
                                 uint8_t *buf, uint16_t buf_len)
{
    uint16_t pos = 0;
    const char *p = hostname;

    while (*p) {
        const char *dot = p;
        while (*dot && *dot != '.') dot++;
        uint8_t label_len = (uint8_t)(dot - p);
        if (pos + 1 + label_len >= buf_len) return 0;
        buf[pos++] = label_len;
        memcpy(&buf[pos], p, label_len);
        pos += label_len;
        p = *dot ? dot + 1 : dot;
    }
    if (pos + 1 >= buf_len) return 0;
    buf[pos++] = 0;   /* Root label */
    return pos;
}

/* Decode a DNS name from wire format (handles compression pointers) */
static uint16_t dns_decode_name(const uint8_t *msg,  uint16_t msg_len,
                                 uint16_t offset,
                                 char *out,           uint16_t out_len)
{
    uint16_t pos   = offset;
    uint16_t wrote = 0;
    bool     first = true;
    uint16_t jumped_to = 0;
    bool     jumped    = false;

    while (pos < msg_len) {
        uint8_t len = msg[pos];

        if ((len & 0xC0) == 0xC0) {
            /* Compression pointer */
            if (pos + 1 >= msg_len) break;
            uint16_t ptr = (uint16_t)(((len & 0x3F) << 8) | msg[pos + 1]);
            if (!jumped) jumped_to = pos + 2;
            jumped = true;
            pos    = ptr;
            continue;
        }

        if (len == 0) {
            if (!jumped) jumped_to = pos + 1;
            break;
        }

        pos++;
        if (!first && wrote + 1 < out_len) out[wrote++] = '.';
        first = false;
        uint16_t copy = (uint16_t)(len < (out_len - wrote - 1) ?
                                   len : (out_len - wrote - 1));
        memcpy(&out[wrote], &msg[pos], copy);
        wrote += copy;
        pos   += len;
    }
    if (wrote < out_len) out[wrote] = '\0';
    return jumped ? jumped_to : pos + 1;
}

int uiox_dns_resolve(const char *hostname, uiox_dns_result_t *result)
{
    if (!hostname || !result || s_dns_count == 0) return -EINVAL;
    memset(result, 0, sizeof(*result));

    uint8_t  pkt[DNS_MAX_MSG];
    uint8_t  resp[DNS_MAX_MSG];
    static uint16_t s_dns_id = 1;
    uint16_t qid = s_dns_id++;

    /* Build query packet */
    memset(pkt, 0, DNS_HDR_LEN);
    pkt[0] = (uint8_t)(qid >> 8);
    pkt[1] = (uint8_t)(qid & 0xFF);
    pkt[2] = (uint8_t)(DNS_FLAG_RD >> 8);   /* Flags hi: RD set  */
    pkt[3] = 0;                               /* Flags lo          */
    pkt[4] = 0; pkt[5] = 1;                  /* QDCOUNT = 1       */
    pkt[6] = 0; pkt[7] = 0;                  /* ANCOUNT = 0       */
    pkt[8] = 0; pkt[9] = 0;                  /* NSCOUNT = 0       */
    pkt[10]= 0; pkt[11]= 0;                  /* ARCOUNT = 0       */

    uint16_t pos = DNS_HDR_LEN;
    pos += dns_encode_name(hostname, pkt + pos, DNS_MAX_MSG - pos);
    pkt[pos++] = 0; pkt[pos++] = DNS_QTYPE_A;    /* QTYPE  = A  */
    pkt[pos++] = 0; pkt[pos++] = DNS_QCLASS_IN;  /* QCLASS = IN */

    /* Try each configured DNS server */
    for (int srv = 0; srv < s_dns_count; srv++) {
        for (int retry = 0; retry < UIOX_DNS_RETRIES; retry++) {

            int fd = uiox_socket(UIOX_AF_INET, UIOX_SOCK_DGRAM, 0);
            if (fd < 0) return fd;

            uint32_t to = UIOX_DNS_TIMEOUT_MS;
            uiox_setsockopt(fd, UIOX_SOL_SOCKET, UIOX_SO_RCVTIMEO,
                            &to, sizeof(to));

            uiox_sockaddr_in_t dst = {
                .family = UIOX_AF_INET,
                .addr   = s_dns_servers[srv],
                .port   = DNS_PORT
            };
            uiox_sendto(fd, pkt, pos, 0, &dst);

            int rlen = (int)uiox_recv(fd, resp, DNS_MAX_MSG, 0);
            uiox_close(fd);

            if (rlen < DNS_HDR_LEN) continue;

            /* Validate response ID */
            uint16_t rid = (uint16_t)((resp[0] << 8) | resp[1]);
            if (rid != qid) continue;

            /* Check QR + RCODE */
            uint16_t flags = (uint16_t)((resp[2] << 8) | resp[3]);
            if (!(flags & DNS_FLAG_QR)) continue;
            if ((flags & 0x000F) != 0) { return -ENOENT; } /* NXDOMAIN etc */

            uint16_t ancount = (uint16_t)((resp[6] << 8) | resp[7]);
            if (!ancount) return -ENOENT;

            /* Skip question section */
            uint16_t rpos = DNS_HDR_LEN;
            char tmpname[UIOX_DNS_NAME_MAX];
            rpos = dns_decode_name(resp, (uint16_t)rlen,
                                   rpos, tmpname, sizeof(tmpname));
            rpos += 4;   /* skip QTYPE + QCLASS */

            /* Parse answer records */
            for (int an = 0; an < ancount && rpos + 10 < rlen; an++) {
                rpos = dns_decode_name(resp, (uint16_t)rlen,
                                       rpos, tmpname, sizeof(tmpname));
                uint16_t rtype  = (uint16_t)((resp[rpos]<<8)|resp[rpos+1]);
                uint16_t rdlen  = (uint16_t)((resp[rpos+8]<<8)|resp[rpos+9]);
                uint32_t rttl   = ((uint32_t)resp[rpos+4]<<24) |
                                  ((uint32_t)resp[rpos+5]<<16) |
                                  ((uint32_t)resp[rpos+6]<<8)  |
                                   (uint32_t)resp[rpos+7];
                rpos += 10;

                if (rtype == DNS_QTYPE_A && rdlen == 4 &&
                    result->count < 4) {
                    uint32_t ip;
                    memcpy(&ip, &resp[rpos], 4);
                    result->addrs[result->count++] = __builtin_bswap32(ip);
                    result->ttl = rttl;
                }
                rpos += rdlen;
            }
            return result->count ? 0 : -ENOENT;
        }
    }
    return -ETIMEDOUT;
}

uint32_t uiox_dns_resolve_first(const char *hostname)
{
    uiox_dns_result_t r;
    if (uiox_dns_resolve(hostname, &r) == 0 && r.count > 0)
        return r.addrs[0];
    return 0;
}

void uiox_dns_cache_flush(void)
{
    /* DNS cache not implemented in this minimal build —
     * extend with a hash-table keyed on hostname for production. */
}

/* =========================================================================
 * NTP client (RFC 4330 SNTPv4)
 * ====================================================================== */

#define NTP_EPOCH_DELTA  2208988800u   /* Seconds between 1900 and 1970  */

typedef struct __attribute__((packed)) {
    uint8_t  li_vn_mode;    /* LI(2) | VN(3) | Mode(3)            */
    uint8_t  stratum;
    uint8_t  poll;
    int8_t   precision;
    uint32_t root_delay;
    uint32_t root_disp;
    uint32_t ref_id;
    uint32_t ref_ts_sec;
    uint32_t ref_ts_frac;
    uint32_t orig_ts_sec;
    uint32_t orig_ts_frac;
    uint32_t rx_ts_sec;
    uint32_t rx_ts_frac;
    uint32_t tx_ts_sec;
    uint32_t tx_ts_frac;
} uiox_ntp_pkt_t;

int uiox_ntp_sync(uint32_t server_ip, uiox_ntp_result_t *result)
{
    if (!server_ip) return -EINVAL;

    int fd = uiox_socket(UIOX_AF_INET, UIOX_SOCK_DGRAM, 0);
    if (fd < 0) return fd;

    uint32_t to = UIOX_NTP_TIMEOUT_MS;
    uiox_setsockopt(fd, UIOX_SOL_SOCKET, UIOX_SO_RCVTIMEO, &to, sizeof(to));

    uiox_ntp_pkt_t req;
    memset(&req, 0, sizeof(req));
    req.li_vn_mode = (0 << 6) | (4 << 3) | 3;  /* LI=0, VN=4, Mode=3(client) */

    uiox_sockaddr_in_t dst = {
        .family = UIOX_AF_INET,
        .addr   = server_ip,
        .port   = UIOX_NTP_PORT
    };
    uiox_sendto(fd, &req, sizeof(req), 0, &dst);

    uiox_ntp_pkt_t resp;
    int rc = (int)uiox_recv(fd, &resp, sizeof(resp), 0);
    uiox_close(fd);

    if (rc < (int)sizeof(resp)) return -EIO;

    uint32_t tx_sec  = __builtin_bswap32(resp.tx_ts_sec);
    uint32_t rx_sec  = __builtin_bswap32(resp.rx_ts_sec);
    uint32_t unix_ts = tx_sec > NTP_EPOCH_DELTA ?
                       tx_sec - NTP_EPOCH_DELTA : 0;

    if (result) {
        result->unix_time     = unix_ts;
        result->round_trip_ms = (tx_sec - rx_sec) * 1000u;
        result->offset_ms     = 0;   /* Extend with T1/T2/T3/T4 for full SNTP */
    }

    s_ntp_last = unix_ts;
    return 0;
}

uint32_t uiox_ntp_last_sync(void)
{
    return s_ntp_last;
}

/* =========================================================================
 * HTTP client
 *
 * Minimal HTTP/1.1 client.  Resolves hostname via DNS, opens TCP
 * connection, sends request, reads response headers + body.
 * ====================================================================== */

#define HTTP_DEFAULT_PORT   80
#define HTTP_DEFAULT_TIMEOUT_MS  10000

/* Parse "[hostname](http://hostname)[:port]/path" */
static int http_parse_url(const char *url,
                           char *host_out,  uint16_t host_len,
                           uint16_t *port_out,
                           char *path_out,  uint16_t path_len)
{
    const char *p = url;
    if (strncmp(p, "http://", 7) == 0) p += 7;
    else if (strncmp(p, "https://", 8) == 0) {
        /* HTTPS not supported in this build */
        return -EPROTONOSUPPORT;
    }

    /* Extract host */
    const char *host_start = p;
    while (*p && *p != '/' && *p != ':') p++;

    uint16_t hlen = (uint16_t)(p - host_start);
    if (hlen == 0 || hlen >= host_len) return -EINVAL;
    memcpy(host_out, host_start, hlen);
    host_out[hlen] = '\0';

    /* Optional port */
    *port_out = HTTP_DEFAULT_PORT;
    if (*p == ':') {
        p++;
        *port_out = 0;
        while (*p >= '0' && *p <= '9')
            *port_out = (uint16_t)(*port_out * 10 + (*p++ - '0'));
    }

    /* Path */
    if (*p == '/') {
        strncpy(path_out, p, path_len - 1);
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
    if (!req || !resp) return -EINVAL;

    char     host[128], path[UIOX_HTTP_MAX_URL_LEN];
    uint16_t port;

    int rc = http_parse_url(req->url, host, sizeof(host), &port,
                            path, sizeof(path));
    if (rc < 0) return rc;

    /* Resolve hostname */
    uint32_t server_ip = uiox_dns_resolve_first(host);
    if (!server_ip) return -ENOENT;

    /* Open TCP connection */
    int fd = uiox_socket(UIOX_AF_INET, UIOX_SOCK_STREAM, 0);
    if (fd < 0) return fd;

    uint32_t timeout = req->timeout_ms ? req->timeout_ms : HTTP_DEFAULT_TIMEOUT_MS;
    uiox_setsockopt(fd, UIOX_SOL_SOCKET, UIOX_SO_RCVTIMEO, &timeout, sizeof(timeout));
    uiox_setsockopt(fd, UIOX_SOL_SOCKET, UIOX_SO_SNDTIMEO, &timeout, sizeof(timeout));

    uiox_sockaddr_in_t dst = {
        .family = UIOX_AF_INET,
        .addr   = server_ip,
        .port   = port
    };
    rc = uiox_connect(fd, &dst);
    if (rc < 0) { uiox_close(fd); return rc; }

    /* Build and send request line + headers */
    char txbuf[1024];
    int  txlen = snprintf(txbuf, sizeof(txbuf),
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "Content-Length: %zu\r\n",
        http_method_str(req->method), path, host,
        req->body ? req->body_len : 0);

    /* Append custom headers */
    for (int i = 0; i < req->header_count && txlen < (int)sizeof(txbuf) - 4; i++) {
        txlen += snprintf(txbuf + txlen, sizeof(txbuf) - (size_t)txlen,
                          "%s: %s\r\n",
                          req->headers[i].key,
                          req->headers[i].value);
    }
    txlen += snprintf(txbuf + txlen, sizeof(txbuf) - (size_t)txlen, "\r\n");

    uiox_send(fd, txbuf, (size_t)txlen, 0);

    /* Send optional body */
    if (req->body && req->body_len)
        uiox_send(fd, req->body, req->body_len, 0);

    /* Read response */
    char rxbuf[2048];
    memset(rxbuf, 0, sizeof(rxbuf));
    ssize_t rlen = uiox_recv(fd, rxbuf, sizeof(rxbuf) - 1, 0);
    uiox_close(fd);

    if (rlen < 0) return (int)rlen;

    /* Parse status line: "HTTP/1.x NNN ..." */
    resp->status_code  = 0;
    resp->header_count = 0;
    resp->body_len     = 0;

    char *line = rxbuf;
    char *crlf = strstr(line, "\r\n");
    if (!crlf) return -EIO;

    sscanf(line, "HTTP/%*d.%*d %d", &resp->status_code);
    line = crlf + 2;

    /* Parse response headers */
    while ((crlf = strstr(line, "\r\n")) != NULL) {
        if (crlf == line) { line += 2; break; }   /* End of headers */
        if (resp->header_count < UIOX_HTTP_MAX_HEADERS) {
            size_t hlen = (size_t)(crlf - line);
            char *colon = memchr(line, ':', hlen);
            if (colon) {
                size_t klen = (size_t)(colon - line);
                size_t vlen = hlen - klen - 1;
                const char *vstart = colon + 1;
                while (*vstart == ' ') { vstart++; vlen--; }
                uiox_http_header_t *h =
                    &resp->headers[resp->header_count++];
                snprintf(h->key,   sizeof(h->key),   "%.*s",
                         (int)klen, line);
                snprintf(h->value, sizeof(h->value),  "%.*s",
                         (int)vlen, vstart);
            }
        }
        line = crlf + 2;
    }

    /* Copy body into caller buffer */
    /* Copy body into caller buffer */
    if (resp->body && resp->body_capacity > 0) {
        size_t body_avail = (size_t)((rxbuf + rlen) - line);
        resp->body_len = body_avail < resp->body_capacity ?
                         body_avail : resp->body_capacity - 1;
        memcpy(resp->body, line, resp->body_len);
        ((uint8_t *)resp->body)[resp->body_len] = '\0';
    }

    return 0;
}

ssize_t uiox_http_get(const char *url, void *buf, size_t buf_len,
                       int *status_out)
{
    if (!url || !buf || !buf_len) return -EINVAL;

    uiox_http_request_t req;
    memset(&req, 0, sizeof(req));
    req.method      = UIOX_HTTP_GET;
    req.timeout_ms  = HTTP_DEFAULT_TIMEOUT_MS;
    strncpy(req.url, url, UIOX_HTTP_MAX_URL_LEN - 1);

    uiox_http_response_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.body          = (uint8_t *)buf;
    resp.body_capacity = buf_len;

    int rc = uiox_http_request(&req, &resp);
    if (rc < 0) return rc;

    if (status_out) *status_out = resp.status_code;
    return (ssize_t)resp.body_len;
}

ssize_t uiox_http_post(const char *url,
                        const void *body,     size_t body_len,
                        void       *resp_buf, size_t resp_buf_len,
                        int        *status_out)
{
    if (!url || !resp_buf || !resp_buf_len) return -EINVAL;

    uiox_http_request_t req;
    memset(&req, 0, sizeof(req));
    req.method      = UIOX_HTTP_POST;
    req.timeout_ms  = HTTP_DEFAULT_TIMEOUT_MS;
    req.body        = body;
    req.body_len    = body_len;
    strncpy(req.url, url, UIOX_HTTP_MAX_URL_LEN - 1);

    /* Add Content-Type header */
    strncpy(req.headers[0].key,   "Content-Type",       UIOX_HTTP_MAX_HDR_LEN - 1);
    strncpy(req.headers[0].value, "application/json",   UIOX_HTTP_MAX_HDR_LEN - 1);
    req.header_count = 1;

    uiox_http_response_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.body          = (uint8_t *)resp_buf;
    resp.body_capacity = resp_buf_len;

    int rc = uiox_http_request(&req, &resp);
    if (rc < 0) return rc;

    if (status_out) *status_out = resp.status_code;
    return (ssize_t)resp.body_len;
}

/* =========================================================================
 * Ping / reachability
 * ====================================================================== */

/* Shared ping reply notification slot */
static struct {
    uint16_t id;
    uint16_t seq;
    uint32_t rtt_ms;
    uint8_t  ttl;
    bool     ready;
} s_ping_reply;

/* Called by ICMP input when an echo reply arrives */
void uiox_connectivity_icmp_notify(uint16_t id, uint16_t seq,
                                    uint32_t rtt_ms, uint8_t ttl)
{
    if (s_ping_reply.id == id && s_ping_reply.seq == seq) {
        s_ping_reply.rtt_ms = rtt_ms;
        s_ping_reply.ttl    = ttl;
        s_ping_reply.ready  = true;
    }
}

int uiox_ping(uint32_t target_ip, uint32_t timeout_ms,
               uiox_ping_result_t *result)
{
    if (!target_ip) return -EINVAL;

    static uint16_t s_ping_id  = 0x5555;
    static uint16_t s_ping_seq = 0;

    uint16_t id  = s_ping_id;
    uint16_t seq = s_ping_seq++;

    s_ping_reply.id    = id;
    s_ping_reply.seq   = seq;
    s_ping_reply.ready = false;

    /* Send ICMP echo request */
    int rc = uiox_proto_icmp_ping(target_ip, id, seq, timeout_ms);
    if (rc < 0) return rc;

    /* Wait for reply notification */
    uint32_t waited = 0;
    while (!s_ping_reply.ready && waited < timeout_ms) {
        /* uiox_os_sleep_ms(1); */
        waited++;
    }

    if (result) {
        result->target_ip  = target_ip;
        result->reachable  = s_ping_reply.ready;
        result->rtt_ms     = s_ping_reply.ready ? s_ping_reply.rtt_ms : 0;
        result->ttl        = s_ping_reply.ready ? s_ping_reply.ttl    : 0;
    }

    return s_ping_reply.ready ? 0 : -ETIMEDOUT;
}

bool uiox_reachable(uint32_t target_ip, uint32_t timeout_ms)
{
    return uiox_ping(target_ip, timeout_ms, NULL) == 0;
}

/* =========================================================================
 * IP address utilities
 * ====================================================================== */

uint32_t uiox_ip4_from_str(const char *str)
{
    if (!str) return 0;

    uint32_t result = 0;
    uint8_t  octet  = 0;
    uint8_t  dots   = 0;
    bool     digits = false;

    for (const char *p = str; ; p++) {
        if (*p >= '0' && *p <= '9') {
            octet  = (uint8_t)(octet * 10 + (*p - '0'));
            digits = true;
        } else if (*p == '.' || *p == '\0') {
            if (!digits) return 0;
            result = (result << 8) | octet;
            octet  = 0;
            digits = false;
            if (*p == '.') {
                if (++dots > 3) return 0;
            } else {
                break;
            }
        } else {
            return 0;   /* Invalid character */
        }
    }

    return dots == 3 ? result : 0;
}

void uiox_ip4_to_str(uint32_t ip, char *buf, size_t buf_len)
{
    if (!buf || buf_len < 16) return;
    snprintf(buf, buf_len, "%u.%u.%u.%u",
             (ip >> 24) & 0xFF,
             (ip >> 16) & 0xFF,
             (ip >>  8) & 0xFF,
              ip        & 0xFF);
}


 