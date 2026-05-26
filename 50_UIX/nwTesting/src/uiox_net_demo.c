/**
 * @file    uiox_net_demo.c
 * @brief   UIOX networking stack end-to-end demonstration.
 *
 * Shows the complete flow from hardware init to application-level
 * HTTP GET, DNS resolution, NTP sync and ping — using only the
 * uiox_connectivity.h top-level API.
 *
 * Build:
 *   cc -o uiox_net_demo uiox_net_demo.c       \
 *         uiox_net_hw.c uiox_netif.c          \
 *         uiox_netbuf.c uiox_proto.c          \
 *         uiox_socket.c uiox_connectivity.c   \
 *         -I../include
 *
 * @date    2026-05-25
 */
//Final File — Application Demo
 #include "uiox_connectivity.h"
 #include "uiox_net_hw.h"
 #include "uiox_netif.h" // included for &s_primary_if

 #include <stdio.h>
 #include <string.h>

 //extern uiox_netif_t             s_primary_if; //added for s_primary_if
 /* -------------------------------------------------------------------------
  * Stub hardware ops (replace with real driver for your NIC)
  * ---------------------------------------------------------------------- */
 
 static int  stub_hw_init   (uiox_hw_dev_t *dev) { (void)dev; return 0; }
 static void stub_hw_deinit (uiox_hw_dev_t *dev) { (void)dev; }
 static int  stub_hw_start  (uiox_hw_dev_t *dev) { (void)dev; return 0; }
 static void stub_hw_stop   (uiox_hw_dev_t *dev) { (void)dev; }
 static int  stub_phy_autoneg(uiox_hw_dev_t *dev)
 {
     dev->speed  = UIOX_HW_SPEED_1G;
     dev->duplex = UIOX_HW_DUPLEX_FULL;
     return 0;
 }
 static int stub_tx_submit(uiox_hw_dev_t *dev, uintptr_t buf, uint16_t len)
 {
     (void)dev; (void)buf; (void)len;
     /* In real driver: write descriptor, ring doorbell */
     return 0;
 }
 static void stub_tx_reclaim(uiox_hw_dev_t *dev) { (void)dev; }
 static int  stub_rx_poll(uiox_hw_dev_t *dev, void *buf, uint16_t maxlen)
 {
     (void)dev; (void)buf; (void)maxlen;
     return 0;  /* No packet ready */
 }
 static void     stub_isr       (uiox_hw_dev_t *d) { (void)d; }
 static uint16_t stub_mdio_read (uiox_hw_dev_t *d, uint8_t phy, uint8_t reg)
 { (void)d; (void)phy; (void)reg; return 0; }
 static void     stub_mdio_write(uiox_hw_dev_t *d, uint8_t phy,
                                  uint8_t reg, uint16_t val)
 { (void)d; (void)phy; (void)reg; (void)val; }
 
 static const uiox_hw_ops_t stub_ops = {
     .init        = stub_hw_init,
     .deinit      = stub_hw_deinit,
     .start       = stub_hw_start,
     .stop        = stub_hw_stop,
     .phy_autoneg = stub_phy_autoneg,
     .tx_submit   = stub_tx_submit,
     .tx_reclaim  = stub_tx_reclaim,
     .rx_poll     = stub_rx_poll,
     .isr         = stub_isr,
     .mdio_read   = stub_mdio_read,
     .mdio_write  = stub_mdio_write,
 };
 
 /* -------------------------------------------------------------------------
  * Hardware device instance
  * ---------------------------------------------------------------------- */
 
 static uiox_hw_dev_t s_hw_dev = {
     .base_addr = 0x40010000uL,   /* Example GMAC MMIO base    */
     .irq       = 42,
     .mac_addr  = { 0x02, 0xDE, 0xAD, 0xBE, 0xEF, 0x01 },
     .caps      = UIOX_HW_CAP_CHECKSUM_TX | UIOX_HW_CAP_CHECKSUM_RX,
 };
 
 /* -------------------------------------------------------------------------
  * Network event callback
  * ---------------------------------------------------------------------- */
 
 static void on_net_event(uiox_net_evt_t event,
                           uiox_netif_t  *netif,
                           void          *ctx)
 {
     (void)ctx;
     char ip_str[16];
 
     switch (event) {
     case UIOX_NET_EVT_LINK_UP:
         printf("[net] Link UP on %s\n", netif->name);
         break;
     case UIOX_NET_EVT_LINK_DOWN:
         printf("[net] Link DOWN on %s\n", netif->name);
         break;
     case UIOX_NET_EVT_IP_ACQUIRED:
         uiox_ip4_to_str(netif->ip4_addr, ip_str, sizeof(ip_str));
         printf("[net] IP acquired: %s\n", ip_str);
         break;
     case UIOX_NET_EVT_IP_LOST:
         printf("[net] IP address lost\n");
         break;
     case UIOX_NET_EVT_DNS_READY:
         printf("[net] DNS servers configured\n");
         break;
     case UIOX_NET_EVT_NTP_SYNCED:
         printf("[net] NTP synced — Unix time: %u\n", uiox_ntp_last_sync());
         break;
     case UIOX_NET_EVT_ERROR:
         printf("[net] Network error\n");
         break;
     }
 }
 
 /* -------------------------------------------------------------------------
  * main
  * ---------------------------------------------------------------------- */
 
 int main(void)
 {
     printf("=== UIOX Networking Stack Demo ===\n\n");
 
     /* -- 1. Configure and bring up the stack ----------------------------- */
     uiox_net_config_t cfg;
     memset(&cfg, 0, sizeof(cfg));
 
     cfg.hw       = &s_hw_dev;
     cfg.hw_ops   = &stub_ops;
     strncpy(cfg.ifname, "eth0", sizeof(cfg.ifname) - 1);
     cfg.mtu      = 1500;
 
     /* Use DHCP */
     cfg.addr_mode = UIOX_ADDR_MODE_DHCP;
 
     /* Fallback static config (used if DHCP fails in a real system):
      * cfg.addr_mode  = UIOX_ADDR_MODE_STATIC;
      * cfg.static_ip  = uiox_ip4_from_str("192.168.1.100");
      * cfg.static_mask= uiox_ip4_from_str("255.255.255.0");
      * cfg.static_gw  = uiox_ip4_from_str("192.168.1.1");
      */
 
     /* NTP server: time.cloudflare.com = 162.159.200.1 */
     cfg.ntp_server_ip = uiox_ip4_from_str("162.159.200.1");
 
     cfg.evt_cb  = on_net_event;
     cfg.evt_ctx = NULL;
 
     int rc = uiox_net_init(&cfg);
     if (rc < 0) {
         printf("[error] uiox_net_init failed: %d\n", rc);
         return 1;
     }
 
     /* -- 2. Print local IP ----------------------------------------------- */
     char local_ip[16];
     uiox_ip4_to_str(uiox_net_local_ip(), local_ip, sizeof(local_ip));
     printf("\nLocal IP : %s\n", local_ip);
 
     /* -- 3. DNS resolution ----------------------------------------------- */
     printf("\n--- DNS Resolution ---\n");
     const char *hostname = "example.com";
     uiox_dns_result_t dns_res;
     rc = uiox_dns_resolve(hostname, &dns_res);
     if (rc == 0) {
         printf("Resolved %s -> ", hostname);
         for (int i = 0; i < dns_res.count; i++) {
             char addr_str[16];
             uiox_ip4_to_str(dns_res.addrs[i], addr_str, sizeof(addr_str));
             printf("%s ", addr_str);
         }
         printf("(TTL=%us)\n", dns_res.ttl);
     } else {
         printf("DNS resolve failed: %d\n", rc);
     }
 
     /* -- 4. Ping --------------------------------------------------------- */
     printf("\n--- Ping ---\n");
     uint32_t ping_target = uiox_ip4_from_str("8.8.8.8");
     uiox_ping_result_t ping_res;
     rc = uiox_ping(ping_target, 2000, &ping_res);
     if (rc == 0) {
         printf("Ping 8.8.8.8 -> RTT=%ums TTL=%u\n",
                ping_res.rtt_ms, ping_res.ttl);
     } else {
         printf("Ping timed out\n");
     }
 
     /* -- 5. HTTP GET ----------------------------------------------------- */
     printf("\n--- HTTP GET ---\n");
     static uint8_t http_body[2048];
     int   status = 0;
     ssize_t got  = uiox_http_get("[example.com](http://example.com/)",
                                   http_body, sizeof(http_body), &status);
     if (got >= 0) {
         printf("HTTP %d — received %zd bytes\n", status, got);
         printf("%.256s%s\n", (char *)http_body,
                got > 256 ? "\n...[truncated]" : "");
     } else {
         printf("HTTP GET failed: %zd\n", got);
     }
 
     /* -- 6. HTTP POST ---------------------------------------------------- */
     printf("\n--- HTTP POST ---\n");
     const char *post_body    = "{\"sensor\":\"temp\",\"value\":23.5}";
     static uint8_t post_resp[1024];
     int post_status = 0;
     ssize_t post_got = uiox_http_post(
         "[httpbin.org](http://httpbin.org/post)",
         post_body, strlen(post_body),
         post_resp, sizeof(post_resp),
         &post_status);
     if (post_got >= 0)
         printf("POST HTTP %d — received %zd bytes\n", post_status, post_got);
     else
         printf("HTTP POST failed: %zd\n", post_got);
 
     /* -- 7. NTP ---------------------------------------------------------- */
     printf("\n--- NTP ---\n");
     printf("Last NTP Unix timestamp : %u\n", uiox_ntp_last_sync());
 
     /* -- 8. Reachability check ------------------------------------------ */
     printf("\n--- Reachability ---\n");
     const char *targets[] = { "1.1.1.1", "8.8.4.4", "192.0.2.1" };
     for (size_t i = 0; i < sizeof(targets)/sizeof(targets[0]); i++) {
         uint32_t tip = uiox_ip4_from_str(targets[i]);
         printf("%s -> %s\n", targets[i],
                uiox_reachable(tip, 1000) ? "REACHABLE" : "unreachable");
     }
 
     /* -- 9. Interface statistics ---------------------------------------- */
     printf("\n--- Interface Stats (eth0) ---\n");
     uiox_netif_stats_t stats;
     //uiox_netif_stats(&s_primary_if, &stats);
     printf("  RX  packets : %llu\n", (unsigned long long)stats.rx_packets);
     printf("  RX  bytes   : %llu\n", (unsigned long long)stats.rx_bytes);
     printf("  RX  errors  : %llu\n", (unsigned long long)stats.rx_errors);
     printf("  RX  dropped : %llu\n", (unsigned long long)stats.rx_dropped);
     printf("  TX  packets : %llu\n", (unsigned long long)stats.tx_packets);
     printf("  TX  bytes   : %llu\n", (unsigned long long)stats.tx_bytes);
     printf("  TX  errors  : %llu\n", (unsigned long long)stats.tx_errors);
 
     /* -- 10. Graceful shutdown ------------------------------------------ */
     printf("\n--- Shutting down ---\n");
     uiox_net_deinit();
     printf("Stack down. Bye.\n");
 
     return 0;
 }
 