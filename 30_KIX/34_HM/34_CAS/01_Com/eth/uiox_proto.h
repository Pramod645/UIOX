/**
 * @file    uiox_proto.h
 * @brief   UIOX protocol stack — IPv4, IPv6, ICMP, UDP, TCP, ARP.
 *
 * This layer implements the core IETF protocols. It sits above the
 * netif/buffer layers and below the socket API.
 *
 * Each receive function is called from uiox_netif_input() with the
 * Ethernet header already stripped from the buffer.
 *
 * @date    2026-05-25
 */
//Layer 3/4 — Protocol Stack
 #ifndef UIOX_PROTO_H
 #define UIOX_PROTO_H
 
 #include "uiox_netif.h"
 #include "uiox_netbuf.h"
 #include <stdint.h>
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Well-known IP protocol numbers
  * ====================================================================== */
 
 #define UIOX_IPPROTO_ICMP       1
 #define UIOX_IPPROTO_TCP        6
 #define UIOX_IPPROTO_UDP        17
 #define UIOX_IPPROTO_ICMPv6     58
 
 /* =========================================================================
  * ICMP type codes
  * ====================================================================== */
 
 #define UIOX_ICMP_ECHO_REPLY    0
 #define UIOX_ICMP_ECHO_REQUEST  8
 #define UIOX_ICMP_DEST_UNREACH  3
 #define UIOX_ICMP_TIME_EXCEEDED 11
 
 /* =========================================================================
  * IPv4 header
  * ====================================================================== */
 
 typedef struct __attribute__((packed)) {
     uint8_t     ver_ihl;        /**< Version (4) + IHL in 32-bit words        */
     uint8_t     tos;            /**< Type of service / DSCP+ECN               */
     uint16_t    total_len;      /**< Total length including header (big-endian)*/
     uint16_t    id;             /**< Identification (fragmentation)            */
     uint16_t    frag_off;       /**< Flags + fragment offset (big-endian)      */
     uint8_t     ttl;            /**< Time to live                              */
     uint8_t     proto;          /**< Protocol (UIOX_IPPROTO_*)                */
     uint16_t    checksum;       /**< Header checksum                           */
     uint32_t    src;            /**< Source address (big-endian)               */
     uint32_t    dst;            /**< Destination address (big-endian)          */
 } uiox_ip4_hdr_t;
 
 #define UIOX_IP4_VER_IHL_DEFAULT    0x45    /* version=4, IHL=5 (no options) */
 #define UIOX_IP4_TTL_DEFAULT        64
 
 /* =========================================================================
  * IPv6 header
  * ====================================================================== */
 
 typedef struct __attribute__((packed)) {
     uint32_t    ver_tc_fl;      /**< Version(4)+TrafficClass(8)+FlowLabel(20)  */
     uint16_t    payload_len;    /**< Payload length (big-endian)               */
     uint8_t     next_hdr;       /**< Next header protocol number               */
     uint8_t     hop_limit;      /**< Hop limit (analogous to TTL)              */
     uint8_t     src[16];        /**< Source IPv6 address                       */
     uint8_t     dst[16];        /**< Destination IPv6 address                  */
 } uiox_ip6_hdr_t;
 
 /* =========================================================================
  * UDP header
  * ====================================================================== */
 
 typedef struct __attribute__((packed)) {
     uint16_t    src_port;
     uint16_t    dst_port;
     uint16_t    len;            /**< Header + payload length (big-endian)      */
     uint16_t    checksum;
 } uiox_udp_hdr_t;
 
 /* =========================================================================
  * TCP header
  * ====================================================================== */
 
 typedef struct __attribute__((packed)) {
     uint16_t    src_port;
     uint16_t    dst_port;
     uint32_t    seq;
     uint32_t    ack;
     uint8_t     data_off;       /**< Data offset (header length in 32-bit wds) */
     uint8_t     flags;          /**< TCP control flags                          */
     uint16_t    window;
     uint16_t    checksum;
     uint16_t    urg_ptr;
 } uiox_tcp_hdr_t;
 
 /* TCP flag bits */
 #define UIOX_TCP_FIN    (1u << 0)
 #define UIOX_TCP_SYN    (1u << 1)
 #define UIOX_TCP_RST    (1u << 2)
 #define UIOX_TCP_PSH    (1u << 3)
 #define UIOX_TCP_ACK    (1u << 4)
 #define UIOX_TCP_URG    (1u << 5)
 
 /* =========================================================================
  * ICMP header
  * ====================================================================== */
 
 typedef struct __attribute__((packed)) {
     uint8_t     type;
     uint8_t     code;
     uint16_t    checksum;
     uint32_t    rest;           /**< Type-specific data (id+seq for echo)      */
 } uiox_icmp_hdr_t;
 
 /* =========================================================================
  * ARP header (Ethernet/IPv4)
  * ====================================================================== */
 
 typedef struct __attribute__((packed)) {
     uint16_t    hw_type;        /**< 0x0001 = Ethernet                        */
     uint16_t    proto_type;     /**< 0x0800 = IPv4                            */
     uint8_t     hw_len;         /**< 6 (MAC address length)                   */
     uint8_t     proto_len;      /**< 4 (IPv4 address length)                  */
     uint16_t    operation;      /**< 1=request, 2=reply                       */
     uint8_t     sender_mac[UIOX_HW_MAC_ADDR_LEN];
     uint32_t    sender_ip;
     uint8_t     target_mac[UIOX_HW_MAC_ADDR_LEN];
     uint32_t    target_ip;
 } uiox_arp_hdr_t;
 
 /* =========================================================================
  * Routing table entry
  * ====================================================================== */
 
 #define UIOX_ROUTE_TABLE_SIZE   16
 
 typedef struct {
     uint32_t        network;    /**< Destination network (host order)         */
     uint32_t        mask;       /**< Subnet mask (host order)                 */
     uint32_t        gateway;    /**< Next-hop gateway (0 = directly attached) */
     uiox_netif_t   *netif;      /**< Egress interface                         */
     uint8_t         metric;     /**< Route metric (lower = preferred)         */
     bool            valid;
 } uiox_route_t;
 
 /* =========================================================================
  * Protocol subsystem API
  * ====================================================================== */
 
 /** Initialise the protocol stack. Call once after netif subsystem init. */
 void uiox_proto_init(void);
 
 /* --- Receive path (called by uiox_netif_input) ------------------------- */
 void uiox_proto_ip4_input (uiox_netif_t *netif, uiox_netbuf_t *buf);
 void uiox_proto_ip6_input (uiox_netif_t *netif, uiox_netbuf_t *buf);
 void uiox_arp_input        (uiox_netif_t *netif, uiox_netbuf_t *buf);
 
 /* --- Transmit path (called by socket layer) ---------------------------- */
 
 /**
  * @brief  Send a UDP datagram.
  * @param  src_ip   Source IPv4 (0 = auto-select from route).
  * @param  dst_ip   Destination IPv4.
  * @param  src_port Source UDP port (host order).
  * @param  dst_port Destination UDP port (host order).
  * @param  buf      Payload buffer (proto stack prepends headers).
  */
 int uiox_proto_udp_send(uint32_t src_ip,  uint32_t dst_ip,
                          uint16_t src_port, uint16_t dst_port,
                          uiox_netbuf_t *buf);
 
 /**
  * @brief  Send a TCP segment.
  */
 int uiox_proto_tcp_send(uint32_t src_ip,  uint32_t dst_ip,
                          uint16_t src_port, uint16_t dst_port,
                          uint32_t seq,      uint32_t ack,
                          uint8_t  flags,    uiox_netbuf_t *buf);
 
 /**
  * @brief  Send an ICMPv4 echo request (ping).
  */
 int uiox_proto_icmp_ping(uint32_t dst_ip, uint16_t id,
                           uint16_t seq,    uint32_t timeout_ms);
 
 /* --- ARP helper (called by uiox_arp_resolve) -------------------------- */
 int uiox_proto_arp_request(uiox_netif_t *netif, uint32_t ip4,
                             uint8_t mac_out[UIOX_HW_MAC_ADDR_LEN],
                             uint32_t timeout_ms);
 
 /* --- Routing ----------------------------------------------------------- */
 
 /** Add a route entry to the routing table. */
 int  uiox_route_add(uint32_t network, uint32_t mask,
                     uint32_t gateway, uiox_netif_t *netif, uint8_t metric);
 
 /** Remove matching route entry. */
 void uiox_route_del(uint32_t network, uint32_t mask);
 
 /**
  * @brief  Look up egress interface and next-hop for a destination IP.
  * @return Matched route or NULL if no route found.
  */
 const uiox_route_t *uiox_route_lookup(uint32_t dst_ip);
 
 /* --- Checksum utility -------------------------------------------------- */
 
 /** Compute Internet checksum (RFC 1071) over `len` bytes at `data`. */
 uint16_t uiox_inet_checksum(const void *data, uint16_t len);
 
 /** Compute TCP/UDP pseudo-header + payload checksum. */
 uint16_t uiox_proto_checksum4(uint32_t src, uint32_t dst,
                                uint8_t proto, const void *data, uint16_t len);
 

static void uiox_proto_udp_input(uiox_netif_t *netif, uiox_netbuf_t *buf,
                                    uint32_t src_ip, uint32_t dst_ip); 
                                    
static void uiox_proto_udp_input(uiox_netif_t *netif, uiox_netbuf_t *buf,
                                        uint32_t src_ip, uint32_t dst_ip);

static void uiox_proto_icmp_input(uiox_netif_t *netif, uiox_netbuf_t *buf,
                                            uint32_t src_ip, uint32_t dst_ip);

static void uiox_proto_tcp_input(uiox_netif_t *netif, uiox_netbuf_t *buf,
                                                uint32_t src_ip, uint32_t dst_ip);
                                            
static int uiox_proto_ip4_send(uiox_netif_t *netif, uint32_t src_ip, uint32_t dst_ip,
                                                    uint8_t proto,   uiox_netbuf_t *buf);
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* UIOX_PROTO_H */
 