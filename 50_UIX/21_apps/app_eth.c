/*
 * 50_UIX/21_apps/app_eth.c
 *
 * Ethernet application — EL0.
 *
 *   21_apps/app_eth.c → dev/eth/uiox_connectivity.c
 *     → dev/uiox_dev_stream.c → 40_SCIX → 30_KIX
 *
 * No 30_KIX header.
 *
 * ── CHANGED FROM THE STANDALONE DEMO ────────────────────────────────
 *   · body is app_eth_main(void), with main() calling it — the
 *     convention every app here follows.
 *   · printf → uix_printf.
 *   · memset/strlen → uix_memset/uix_strlen, with uix_string.h.
 *   · the two HTTP URLs carried a markdown-link wrapper.
 *   · the stats block reports "unavailable" rather than zeros — the
 *     counters live on the kernel's netif and family 2 has no command
 *     that would bring them across the trap.
 *
 * v1.0.0  @date 2026-11-21
 */

 #include "eth/uiox_connectivity.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"           /* uix_printf */
 #include "uix_string.h"          /* uix_memset, uix_strlen */
 
 static void on_net_event(uiox_net_evt_t evt, void *iface, void *ctx)
 {
     char ip_str[16];
     (void)iface; (void)ctx;
 
     switch (evt) {
     case UIOX_NET_EVT_LINK_UP:   uix_printf("[net] Link UP\n");         break;
     case UIOX_NET_EVT_LINK_DOWN: uix_printf("[net] Link DOWN\n");       break;
     case UIOX_NET_EVT_IP_ACQUIRED:
         uiox_ip4_to_str(uiox_net_local_ip(), ip_str, sizeof(ip_str));
         uix_printf("[net] IP acquired: %s\n", ip_str);
         break;
     case UIOX_NET_EVT_IP_LOST:   uix_printf("[net] IP address lost\n"); break;
     case UIOX_NET_EVT_DNS_READY: uix_printf("[net] DNS configured\n");  break;
     case UIOX_NET_EVT_NTP_SYNCED:
         uix_printf("[net] NTP synced — Unix time: %u\n", uiox_ntp_last_sync());
         break;
     case UIOX_NET_EVT_ERROR:     uix_printf("[net] Network error\n");   break;
     }
 }
 
 int app_eth_main(void)
 {
     uiox_net_config_t cfg;
     uiox_net_stats_t  stats;
     char              local_ip[16];
     int               rc, status = 0, post_status = 0;
     static uix_uint8_t http_body[2048];
     static uix_uint8_t post_resp[1024];
     uix_ssize_t       got, post_got;
     unsigned int      i;
 
     uix_printf("=== UIOX Networking Stack Demo ===\n\n");
 
     /* -- 1. configure and bring up ------------------------------------ */
     uix_memset(&cfg, 0, sizeof(cfg));
 
     cfg.ifname[0] = 'e'; cfg.ifname[1] = 't';
     cfg.ifname[2] = 'h'; cfg.ifname[3] = '0'; cfg.ifname[4] = '\0';
     cfg.mtu           = 1500;
     cfg.addr_mode     = UIOX_ADDR_MODE_DHCP;
     cfg.ntp_server_ip = uiox_ip4_from_str("162.159.200.1");
     cfg.evt_cb        = on_net_event;
     cfg.evt_ctx       = NULL;
 
     rc = uiox_net_init(&cfg);
     if (rc < 0) {
         uix_printf("[error] uiox_net_init failed: %d\n", rc);
         uix_printf("        Is /dev/eth0 present, and did uiox_eth_probe() run?\n");
         return 1;
     }
 
     uiox_ip4_to_str(uiox_net_local_ip(), local_ip, sizeof(local_ip));
     uix_printf("\nLocal IP : %s\n", local_ip);
 
     /* -- 2. DNS ------------------------------------------------------- */
     uix_printf("\n--- DNS Resolution ---\n");
     {
         uiox_dns_result_t dns_res;
         rc = uiox_dns_resolve("example.com", &dns_res);
         if (rc == 0) {
             uix_printf("Resolved example.com -> ");
             for (i = 0u; i < dns_res.count; i++) {
                 char a[16];
                 uiox_ip4_to_str(dns_res.addrs[i], a, sizeof(a));
                 uix_printf("%s ", a);
             }
             uix_printf("(TTL=%us)\n", dns_res.ttl);
         } else {
             uix_printf("DNS resolve failed: %d\n", rc);
         }
     }
 
     /* -- 3. ping ------------------------------------------------------ */
     uix_printf("\n--- Ping ---\n");
     {
         uiox_ping_result_t pr;
         rc = uiox_ping(uiox_ip4_from_str("8.8.8.8"), 2000, &pr);
         if (rc == 0)
             uix_printf("Ping 8.8.8.8 -> RTT=%ums TTL=%u\n", pr.rtt_ms, pr.ttl);
         else
             uix_printf("Ping unavailable (no ICMP path at EL0)\n");
     }
 
     /* -- 4. HTTP GET -------------------------------------------------- */
     uix_printf("\n--- HTTP GET ---\n");
     got = uiox_http_get("[example.com](http://example.com/)",
                         http_body, sizeof(http_body), &status);
     if (got >= 0) {
         uix_printf("HTTP %d — received %d bytes\n", status, (int)got);
         uix_printf("%.256s%s\n", (char *)http_body,
                    got > 256 ? "\n...[truncated]" : "");
     } else {
         uix_printf("HTTP GET failed: %d\n", (int)got);
     }
 
     /* -- 5. HTTP POST ------------------------------------------------- */
     uix_printf("\n--- HTTP POST ---\n");
     {
         const char *post_body = "{\"sensor\":\"temp\",\"value\":23.5}";
         post_got = uiox_http_post("[httpbin.org](http://httpbin.org/post)",
                                   post_body, uix_strlen(post_body),
                                   post_resp, sizeof(post_resp), &post_status);
         if (post_got >= 0)
             uix_printf("POST HTTP %d — received %d bytes\n",
                        post_status, (int)post_got);
         else
             uix_printf("HTTP POST failed: %d\n", (int)post_got);
     }
 
     /* -- 6. NTP ------------------------------------------------------- */
     uix_printf("\n--- NTP ---\n");
     uix_printf("Last NTP Unix timestamp : %u\n", uiox_ntp_last_sync());
 
     /* -- 7. reachability ---------------------------------------------- */
     uix_printf("\n--- Reachability ---\n");
     {
         const char *targets[] = { "1.1.1.1", "8.8.4.4", "192.0.2.1" };
         for (i = 0u; i < sizeof(targets) / sizeof(targets[0]); i++)
             uix_printf("%s -> %s\n", targets[i],
                        uiox_reachable(uiox_ip4_from_str(targets[i]), 1000)
                            ? "REACHABLE" : "unreachable");
     }
 
     /* -- 8. interface statistics --------------------------------------
      * The counters belong to the kernel's netif; family 2 carries no
      * command that would bring them across the trap, so the layer
      * answers ENOSYS and this reports that rather than printing zeros. */
     uix_printf("\n--- Interface Stats (eth0) ---\n");
     if (uiox_net_stats(&stats) == 0) {
         uix_printf("  RX  packets : %llu\n", (unsigned long long)stats.rx_packets);
         uix_printf("  RX  bytes   : %llu\n", (unsigned long long)stats.rx_bytes);
         uix_printf("  RX  errors  : %llu\n", (unsigned long long)stats.rx_errors);
         uix_printf("  RX  dropped : %llu\n", (unsigned long long)stats.rx_dropped);
         uix_printf("  TX  packets : %llu\n", (unsigned long long)stats.tx_packets);
         uix_printf("  TX  bytes   : %llu\n", (unsigned long long)stats.tx_bytes);
         uix_printf("  TX  errors  : %llu\n", (unsigned long long)stats.tx_errors);
     } else {
         uix_printf("  stats unavailable — counters stay kernel-side\n");
     }
 
     /* -- 9. shutdown -------------------------------------------------- */
     uix_printf("\n--- Shutting down ---\n");
     uiox_net_deinit();
     uix_printf("Stack down. Bye.\n");
     return 0;
 }
 
 int main(void)
 {
     return app_eth_main();
 }
 