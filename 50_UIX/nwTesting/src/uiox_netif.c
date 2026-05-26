/**
 * @file    uiox_netif.c
 * @brief   UIOX network interface driver — registration, ARP, I/O dispatch.
 * @date    2026-05-25
 */

 #include "uiox_netif.h"
 #include "uiox_netbuf.h"
 #include "uiox_proto.h"
 #include <string.h>
 #include <errno.h>
 
 /* -------------------------------------------------------------------------
  * Global interface list
  * ---------------------------------------------------------------------- */
 
 static uiox_netif_t *s_netif_list = NULL;
 static uint8_t       s_netif_count = 0;
 
 /* -------------------------------------------------------------------------
  * Subsystem init
  * ---------------------------------------------------------------------- */
 
 void uiox_netif_subsystem_init(void)
 {
     s_netif_list  = NULL;
     s_netif_count = 0;
 }
 
 /* -------------------------------------------------------------------------
  * Registration
  * ---------------------------------------------------------------------- */
 
 int uiox_netif_register(uiox_netif_t *netif)
 {
     if (!netif) return -EINVAL;
     if (s_netif_count >= UIOX_NETIF_MAX) return -ENOMEM;
 
     netif->next   = s_netif_list;
     s_netif_list  = netif;
     s_netif_count++;
     return 0;
 }
 
 void uiox_netif_unregister(uiox_netif_t *netif)
 {
     uiox_netif_t **pp = &s_netif_list;
     while (*pp) {
         if (*pp == netif) {
             *pp = netif->next;
             s_netif_count--;
             return;
         }
         pp = &(*pp)->next;
     }
 }
 
 uiox_netif_t *uiox_netif_find(const char *name)
 {
     for (uiox_netif_t *n = s_netif_list; n; n = n->next)
         if (strncmp(n->name, name, UIOX_NETIF_NAME_LEN) == 0)
             return n;
     return NULL;
 }
 
 uiox_netif_t *uiox_netif_list(void)
 {
     return s_netif_list;
 }
 
 /* -------------------------------------------------------------------------
  * Link management
  * ---------------------------------------------------------------------- */
 
 int uiox_netif_up(uiox_netif_t *netif)
 {
     if (!netif) return -EINVAL;
 
     int rc = uiox_hw_up(netif->hw);
     if (rc < 0) return rc;
 
     netif->flags |= UIOX_IFF_UP | UIOX_IFF_RUNNING;
     if (netif->link_cb)
         netif->link_cb(netif, true);
     return 0;
 }
 
 void uiox_netif_down(uiox_netif_t *netif)
 {
     if (!netif) return;
     uiox_hw_down(netif->hw);
     netif->flags &= ~(UIOX_IFF_UP | UIOX_IFF_RUNNING);
     if (netif->link_cb)
         netif->link_cb(netif, false);
 }
 

int uiox_netif_set_ip4(uiox_netif_t *netif,
                            uint32_t addr, uint32_t mask, uint32_t gw)
    {
        if (!netif) return -EINVAL;
        netif->ip4_addr = addr;
        netif->ip4_mask = mask;
        netif->ip4_gw   = gw;
        return 0;
    }
    
    /* -------------------------------------------------------------------------
     * RX input path — called from ISR bottom-half / DPC
     * ---------------------------------------------------------------------- */
    
    void uiox_netif_input(uiox_netif_t *netif, uiox_netbuf_t *buf)
    {
        if (!netif || !buf) return;
    
        if (buf->len < sizeof(uiox_eth_hdr_t)) {
            netif->stats.rx_errors++;
            uiox_netbuf_free(buf);
            return;
        }
    
        uiox_eth_hdr_t *eth = (uiox_eth_hdr_t *)buf->data;
        uint16_t etype = (uint16_t)((eth->ethertype >> 8) |
                                    (eth->ethertype << 8)); /* be16 → host */
    
        netif->stats.rx_packets++;
        netif->stats.rx_bytes += buf->len;
    
        /* Strip Ethernet header — advance data pointer */
        uiox_netbuf_pull(buf, sizeof(uiox_eth_hdr_t));
    
        switch (etype) {
        case UIOX_ETHERTYPE_IP4:
            uiox_proto_ip4_input(netif, buf);
            break;
        case UIOX_ETHERTYPE_IP6:
            uiox_proto_ip6_input(netif, buf);
            break;
        case UIOX_ETHERTYPE_ARP:
            uiox_arp_input(netif, buf);
            break;
        default:
            netif->stats.rx_dropped++;
            uiox_netbuf_free(buf);
            break;
        }
    }
    
    /* -------------------------------------------------------------------------
     * ARP implementation
     * ---------------------------------------------------------------------- */
    
    static uiox_arp_entry_t *arp_find(uiox_netif_t *netif, uint32_t ip4)
    {
        for (int i = 0; i < UIOX_ARP_CACHE_SIZE; i++)
            if (netif->arp_cache[i].valid && netif->arp_cache[i].ip4 == ip4)
                return &netif->arp_cache[i];
        return NULL;
    }
    
    static uiox_arp_entry_t *arp_alloc(uiox_netif_t *netif)
    {
        /* First try an empty slot */
        for (int i = 0; i < UIOX_ARP_CACHE_SIZE; i++)
            if (!netif->arp_cache[i].valid)
                return &netif->arp_cache[i];
    
        /* Evict oldest */
        uiox_arp_entry_t *oldest = &netif->arp_cache[0];
        for (int i = 1; i < UIOX_ARP_CACHE_SIZE; i++)
            if (netif->arp_cache[i].timestamp < oldest->timestamp)
                oldest = &netif->arp_cache[i];
        return oldest;
    }
    
    void uiox_arp_set(uiox_netif_t *netif, uint32_t ip4,
                      const uint8_t mac[UIOX_HW_MAC_ADDR_LEN])
    {
        if (!netif) return;
        uiox_arp_entry_t *e = arp_find(netif, ip4);
        if (!e) e = arp_alloc(netif);
        e->ip4   = ip4;
        e->valid = true;
        memcpy(e->mac, mac, UIOX_HW_MAC_ADDR_LEN);
        /* timestamp set by caller via uiox_arp_gc / protocol stack */
    }
    
    void uiox_arp_gc(uiox_netif_t *netif, uint32_t now_s)
    {
        if (!netif) return;
        for (int i = 0; i < UIOX_ARP_CACHE_SIZE; i++) {
            uiox_arp_entry_t *e = &netif->arp_cache[i];
            if (e->valid && (now_s - e->timestamp) > UIOX_ARP_TIMEOUT_S)
                e->valid = false;
        }
    }
    
    int uiox_arp_resolve(uiox_netif_t *netif, uint32_t ip4,
                         uint8_t mac_out[UIOX_HW_MAC_ADDR_LEN],
                         uint32_t timeout_ms)
    {
        if (!netif || !mac_out) return -EINVAL;
    
        /* Cache hit */
        uiox_arp_entry_t *e = arp_find(netif, ip4);
        if (e) {
            memcpy(mac_out, e->mac, UIOX_HW_MAC_ADDR_LEN);
            return 0;
        }
    
        /* Send ARP request and wait — delegated to protocol layer */
        return uiox_proto_arp_request(netif, ip4, mac_out, timeout_ms);
    }
    
    /* -------------------------------------------------------------------------
     * Statistics
     * ---------------------------------------------------------------------- */
    
    void uiox_netif_stats(const uiox_netif_t *netif, uiox_netif_stats_t *out)
    {
        if (!netif || !out) return;
        memcpy(out, &netif->stats, sizeof(*out));
    }
    
    void uiox_netif_stats_reset(uiox_netif_t *netif)
    {
        if (!netif) return;
        memset(&netif->stats, 0, sizeof(netif->stats));
    }
    