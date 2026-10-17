/**
 * @file    uiox_netif.c
 * @brief   UIOX network interface driver — registration, ARP, I/O dispatch.
 * @date    2026-05-25
 */

 #include "uiox_netif.h"
 #include "uiox_netbuf.h"
 #include "uiox_klibc.h"
 
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
 
     int rc = uiox_net_hw_up(netif->hw);
     if (rc < 0) return rc;
 
     netif->flags |= UIOX_IFF_UP | UIOX_IFF_RUNNING;
     if (netif->link_cb)
         netif->link_cb(netif, true);
     return 0;
 }
 
 void uiox_netif_down(uiox_netif_t *netif)
 {
     if (!netif) return;
     uiox_net_hw_down(netif->hw);
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
    /**
 * @brief  Hand a raw received frame to the protocol stack.
 *
 * The conversion point dss.md:139 names.  Three layers want three
 * different things from the same packet:
 *
 *   uiox_fw_eth's ISR   calls rx_cb(NULL, 0, priv) — a SIGNAL, no data
 *   uiox_hw_ops_t       offers rx_poll(dev, buf, maxlen) — a PULL
 *   uiox_netif_input()  wants a uiox_netbuf_t * — an owned buffer
 *
 * This is the one place those meet: pull the bytes into a buffer from
 * the pool, then dispatch.  One conversion point, in the kernel where
 * dss.md says it belongs.
 *
 * THE FRAME IS COMPLETE — Ethernet header included.  uiox_netif_input()
 * strips that header itself (uiox_netbuf_pull of sizeof(uiox_eth_hdr_t))
 * and dispatches on the EtherType, so this function must not strip it.
 *
 * @param frame  The full Ethernet frame, header first.
 * @param len    Its length in bytes, including the 14-byte header.
 * @return       0 on success, negative errno.  -ENOMEM when the pool is
 *               exhausted — the caller drops the packet, which is the
 *               correct behaviour for a full RX ring.
 */
int uiox_netif_rx_frame(uiox_netif_t *netif, const uint8_t *frame,
    uint32_t len)
{
if (!netif || !frame)     return -EINVAL;
if (len < sizeof(uiox_eth_hdr_t)) return -EINVAL;

/* A frame larger than the pool's payload cannot be carried.  The
* pool is UIOX_NETBUF_DATA_SIZE bytes (1600), and the pool's own
* header calls that "per-buffer payload capacity" — so a longer
* frame is a hardware configuration error, not a transient drop. */
if (len > UIOX_NETBUF_DATA_SIZE) {
netif->stats.rx_dropped++;
return -EMSGSIZE;
}

uiox_netbuf_t *buf = uiox_netbuf_alloc();
if (!buf) {
netif->stats.rx_dropped++;
return -ENOMEM;
}

/* Reserve the payload, then copy into it.  put() advances the tail
* and hands back the write position, so this is one copy and no
* reallocation — the headroom stays intact for the TX path that may
* reuse the buffer later. */
uint8_t *dst = (uint8_t *)uiox_netbuf_put(buf, (uint16_t)len);
if (!dst) {
uiox_netbuf_free(buf);
netif->stats.rx_errors++;
return -ENOSPC;         /* no tailroom left */
}

memcpy(dst, frame, len);

/* Populate what uiox_netif_input will otherwise keep local.  This is
* dss.md:624's point: the field exists (uiox_netbuf.h:52, "Detected
* EtherType (filled by netif)") and a caller building a buffer by
* another route would leave it zero.  Setting it here satisfies the
* contract at the one entry point raw frames arrive through. */
{
const uiox_eth_hdr_t *eth = (const uiox_eth_hdr_t *)dst;
buf->proto = (uint16_t)((eth->ethertype >> 8) |
            (eth->ethertype << 8));   /* be16 → host */
}

/* Hand it up.  input() strips the header and dispatches; it owns the
* buffer from here and frees it on every path — including the
* rx_errors path, as its body at :120 shows. */
uiox_netif_input(netif, buf);
return 0;
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
    