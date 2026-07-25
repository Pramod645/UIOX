/**
 * @file    uiox_netif.h
 * @brief   UIOX Network Interface (netif) driver abstraction.
 *
 * Sits between the hardware HAL and the protocol stack. Each logical
 * network interface (eth0, wlan0, lo …) is represented by one
 * uiox_netif_t. The protocol stack never touches hardware directly;
 * it calls uiox_netif_* functions here.
 *
 * This layer handles:
 *   - Interface registration / enumeration
 *   - MAC framing (Ethernet II)
 *   - ARP table management
 *   - IP address / netmask / gateway assignment
 *   - Interface statistics
 *
 * @date    2026-05-25
 */
//Layer 2 — Network Interface (IF) Driver
 #ifndef UIOX_NETIF_H
 #define UIOX_NETIF_H
 
 #include "uiox_net_hw.h"
 #include "uiox_netbuf.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Constants
  * ====================================================================== */
 
 #define UIOX_NETIF_NAME_LEN     16      /**< Max interface name length       */
 #define UIOX_NETIF_MAX          8       /**< Max registered interfaces       */
 #define UIOX_ARP_CACHE_SIZE     64      /**< ARP cache entries               */
 #define UIOX_ARP_TIMEOUT_S      300     /**< ARP entry TTL (seconds)         */
 
 /* =========================================================================
  * Interface flags
  * ====================================================================== */
 
 #define UIOX_IFF_UP             (1u << 0)   /**< Interface is administratively up */
 #define UIOX_IFF_RUNNING        (1u << 1)   /**< Link is physically up            */
 #define UIOX_IFF_BROADCAST      (1u << 2)   /**< Interface supports broadcast     */
 #define UIOX_IFF_MULTICAST      (1u << 3)   /**< Interface supports multicast     */
 #define UIOX_IFF_LOOPBACK       (1u << 4)   /**< Loopback interface               */
 #define UIOX_IFF_PROMISC        (1u << 5)   /**< Promiscuous mode                 */
 #define UIOX_IFF_NOARP          (1u << 6)   /**< No ARP on this interface         */
 
 /* =========================================================================
  * Ethernet frame header (IEEE 802.3 / Ethernet II)
  * ====================================================================== */
 
 #define UIOX_ETHERTYPE_IP4      0x0800
 #define UIOX_ETHERTYPE_IP6      0x86DD
 #define UIOX_ETHERTYPE_ARP      0x0806
 #define UIOX_ETHERTYPE_VLAN     0x8100
 
 typedef struct __attribute__((packed)) {
     uint8_t  dst[UIOX_HW_MAC_ADDR_LEN];
     uint8_t  src[UIOX_HW_MAC_ADDR_LEN];
     uint16_t ethertype;             /**< Big-endian EtherType / length        */
 } uiox_eth_hdr_t;
 
 /* =========================================================================
  * ARP cache entry
  * ====================================================================== */
 
 typedef struct {
     uint32_t ip4;                           /**< IPv4 address (host order)    */
     uint8_t  mac[UIOX_HW_MAC_ADDR_LEN];    /**< Resolved MAC                 */
     uint32_t timestamp;                     /**< Creation time (seconds)      */
     bool     valid;
 } uiox_arp_entry_t;
 
 /* =========================================================================
  * Interface statistics
  * ====================================================================== */
 
 typedef struct {
     uint64_t rx_packets;
     uint64_t rx_bytes;
     uint64_t rx_errors;
     uint64_t rx_dropped;
     uint64_t tx_packets;
     uint64_t tx_bytes;
     uint64_t tx_errors;
     uint64_t tx_dropped;
     uint64_t collisions;
 } uiox_netif_stats_t;
 
 /* =========================================================================
  * Network interface descriptor
  * ====================================================================== */
 
 typedef struct uiox_netif {
     char                name[UIOX_NETIF_NAME_LEN]; /**< e.g. "eth0"          */
     uint32_t            flags;          /**< UIOX_IFF_* bitmask               */
     uint32_t            ip4_addr;       /**< IPv4 address (host byte order)   */
     uint32_t            ip4_mask;       /**< Subnet mask (host byte order)    */
     uint32_t            ip4_gw;         /**< Default gateway (host byte order)*/
     uint8_t             mac[UIOX_HW_MAC_ADDR_LEN];
     uint16_t            mtu;
     uiox_hw_dev_t      *hw;             /**< Underlying hardware device       */
     uiox_netif_stats_t  stats;
     uiox_arp_entry_t    arp_cache[UIOX_ARP_CACHE_SIZE];
 
     /**
      * Output hook — called by the protocol stack to send a buffer.
      * The netif takes ownership and frees the buffer when done.
      */
     int (*output)(struct uiox_netif *netif, uiox_netbuf_t *buf,
                   uint32_t dst_ip4);
 
     /** Link-state change callback (set by upper layer). */
     void (*link_cb)(struct uiox_netif *netif, bool up);
 
     struct uiox_netif  *next;           /**< Intrusive linked list            */
 } uiox_netif_t;
 
 /* =========================================================================
  * Public netif API
  * ====================================================================== */
 
 /** Initialise the netif subsystem (call once at boot). */
 void uiox_netif_subsystem_init(void);
 
 /**
  * @brief  Register a network interface.
  * @return 0 on success, -ENOMEM if table full.
  */
 int  uiox_netif_register  (uiox_netif_t *netif);
 
 /** Unregister a previously registered interface. */
 void uiox_netif_unregister(uiox_netif_t *netif);
 
 /** Find interface by name (e.g. "eth0"). Returns NULL if not found. */
 uiox_netif_t *uiox_netif_find(const char *name);
 
 /** Return the head of the interface linked list. */
 uiox_netif_t *uiox_netif_list(void);
 
 /** Bring interface up (sets UIOX_IFF_UP, calls uiox_hw_up). */
 int  uiox_netif_up  (uiox_netif_t *netif);
 
 /** Bring interface down. */
 void uiox_netif_down(uiox_netif_t *netif);
 
 /** Assign IPv4 address, mask and gateway. */
 int  uiox_netif_set_ip4(uiox_netif_t *netif,
                          uint32_t addr, uint32_t mask, uint32_t gw);
 
 /**
  * @brief  Called from the RX path (ISR bottom-half / DPC) to hand
  *         a freshly received buffer to the protocol stack.
  */
 void uiox_netif_input(uiox_netif_t *netif, uiox_netbuf_t *buf);
 
 /** ARP: resolve IPv4 → MAC.  Blocks up to timeout_ms. */
 int  uiox_arp_resolve(uiox_netif_t *netif, uint32_t ip4,
                       uint8_t mac_out[UIOX_HW_MAC_ADDR_LEN],
                       uint32_t timeout_ms);
 
 /** ARP: insert or refresh a static entry. */
 void uiox_arp_set(uiox_netif_t *netif, uint32_t ip4,
                   const uint8_t mac[UIOX_HW_MAC_ADDR_LEN]);
 
 /** ARP: flush all expired entries (call from a periodic timer). */
 void uiox_arp_gc(uiox_netif_t *netif, uint32_t now_s);
 
 /** Snapshot interface statistics. */
 void uiox_netif_stats(const uiox_netif_t *netif, uiox_netif_stats_t *out);
 
 /** Reset interface statistics counters. */
 void uiox_netif_stats_reset(uiox_netif_t *netif);

 /* =========================================================================
 * Protocol-stack callbacks — implemented by the network protocol layer.
 * Forward-declared here so the netif driver compiles without a dependency
 * on the (not-yet-generated) protocol stack headers.
 * ====================================================================== */

void uiox_proto_ip4_input(uiox_netif_t *netif, uiox_netbuf_t *buf);
void uiox_proto_ip6_input(uiox_netif_t *netif, uiox_netbuf_t *buf);
void uiox_arp_input      (uiox_netif_t *netif, uiox_netbuf_t *buf);
int  uiox_proto_arp_request(uiox_netif_t *netif, uint32_t ip4,
                             uint8_t mac_out[UIOX_HW_MAC_ADDR_LEN],
                             uint32_t timeout_ms);

 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* UIOX_NETIF_H */
 