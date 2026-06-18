/**
 * @file    uiox_wifi_demo.c
 * @brief   UIOX WiFi stack end-to-end demonstration.
 *
 * Demonstrates the complete flow:
 *   HAL init → scan → connect (WPA2-PSK) → 4-way handshake →
 *   data TX/RX → quality metrics → auto-reconnect → teardown.
 *
 * Uses stub HAL ops — replace with real SDIO/SPI/PCIe driver.
 * @date    2026-05-28
 */
//Demo Application
 #include "uiox_wifi_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int stub_init(uiox_wifi_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] init  MAC=%02X:%02X:%02X:%02X:%02X:%02X\n",
            hw->mac_addr[0], hw->mac_addr[1], hw->mac_addr[2],
            hw->mac_addr[3], hw->mac_addr[4], hw->mac_addr[5]);
     return 0;
 }
 
 static void stub_deinit (uiox_wifi_hw_t *hw) { (void)hw; printf("  [hal] deinit\n"); }
 static int  stub_start  (uiox_wifi_hw_t *hw) { (void)hw; printf("  [hal] start\n"); return 0; }
 static void stub_stop   (uiox_wifi_hw_t *hw) { (void)hw; printf("  [hal] stop\n"); }
 
 static int stub_set_mode(uiox_wifi_hw_t *hw, uiox_wifi_mode_t mode)
 {
     (void)hw;
     static const char *names[] = {"STA","AP","MONITOR","P2P_GO","P2P_CLIENT"};
     printf("  [hal] mode → %s\n", names[mode]);
     return 0;
 }
 
 static int stub_set_channel(uiox_wifi_hw_t *hw,
                              const uiox_wifi_channel_t *ch)
 {
     (void)hw;
     printf("  [hal] channel %u  %u MHz  BW=%u MHz\n",
            ch->channel, ch->freq_mhz,
            ch->bw == UIOX_WIFI_BW_20MHZ  ? 20  :
            ch->bw == UIOX_WIFI_BW_40MHZ  ? 40  :
            ch->bw == UIOX_WIFI_BW_80MHZ  ? 80  : 160);
     return 0;
 }
 
 static int stub_set_mac(uiox_wifi_hw_t *hw, const uiox_wifi_mac_t mac)
 { (void)hw; (void)mac; return 0; }
 
 static int stub_set_tx_power(uiox_wifi_hw_t *hw, int8_t dbm)
 { (void)hw; printf("  [hal] TX power %d dBm\n", dbm); return 0; }
 
 /* Simulate beacon from a WPA2 AP */
 static uint8_t s_sim_beacon[64] = {
     /* FC = beacon */
     0x80, 0x00,
     /* Duration */ 0x00, 0x00,
     /* DA broadcast */ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
     /* SA (BSSID) */ 0xAA,0xBB,0xCC,0x11,0x22,0x33,
     /* BSSID */      0xAA,0xBB,0xCC,0x11,0x22,0x33,
     /* Seq */ 0x00, 0x00,
     /* TSF (8 bytes) */ 0,0,0,0,0,0,0,0,
     /* Beacon interval = 100 TU */ 0x64, 0x00,
     /* Capability: ESS + Privacy */ 0x31, 0x04,
     /* SSID IE */ UIOX_IE_SSID, 8, 'U','I','O','X','_','L','A','B',
     /* Channel IE */ UIOX_IE_CHANNEL, 1, 6,
     /* RSN IE (simplified) */
     UIOX_IE_RSN, 12,
     0x01,0x00,                   /* RSN version */
     0x00,0x0F,0xAC,0x04,         /* Group cipher: CCMP */
     0x01,0x00,                   /* Pairwise cipher count */
     0x00,0x0F,0xAC,0x04          /* Pairwise: CCMP */
 };
 
 static uint32_t s_rx_call = 0;
 
 static int stub_tx_submit(uiox_wifi_hw_t *hw,
                            uintptr_t phys, uint32_t length,
                            uint8_t rate_idx, bool ampdu)
 {
     (void)hw; (void)phys;
     printf("  [hal] TX submit  len=%u  rate_idx=%u  ampdu=%d\n",
            length, rate_idx, (int)ampdu);
     return 0;
 }
 
 static int stub_tx_reclaim(uiox_wifi_hw_t *hw) { (void)hw; return 0; }
 
 static int stub_rx_poll(uiox_wifi_hw_t *hw,
                          uintptr_t *phys_out,
                          uint32_t  *len_out,
                          int8_t    *rssi_out)
 {
     (void)hw;
     s_rx_call++;
     /* Deliver one simulated beacon on first call */
     if (s_rx_call == 1) {
         *phys_out = (uintptr_t)s_sim_beacon;
         *len_out  = sizeof(s_sim_beacon);
         *rssi_out = -62;
         return (int)sizeof(s_sim_beacon);
     }
     /* Simulate auth response on 2nd call */
     if (s_rx_call == 2) {
         static uint8_t auth_rsp[30] = {
             0xB0,0x00, 0,0,
             0x01,0x02,0x03,0x04,0x05,0x06, /* DA = own */
             0xAA,0xBB,0xCC,0x11,0x22,0x33, /* SA = AP */
             0xAA,0xBB,0xCC,0x11,0x22,0x33, /* BSSID */
             0x00,0x00,
             0x00,0x00, /* Open System */
             0x02,0x00, /* Seq 2 */
             0x00,0x00  /* Status 0 */
         };
         *phys_out = (uintptr_t)auth_rsp;
         *len_out  = sizeof(auth_rsp);
         *rssi_out = -62;
         return (int)sizeof(auth_rsp);
     }
     /* Simulate assoc response on 3rd call */
     if (s_rx_call == 3) {
         static uint8_t assoc_rsp[30] = {
             0x10,0x00, 0,0,
             0x01,0x02,0x03,0x04,0x05,0x06,
             0xAA,0xBB,0xCC,0x11,0x22,0x33,
             0xAA,0xBB,0xCC,0x11,0x22,0x33,
             0x00,0x00,
             0x31,0x04, /* caps */
             0x00,0x00, /* status 0 */
             0x01,0x00  /* AID = 1 */
         };
         *phys_out = (uintptr_t)assoc_rsp;
         *len_out  = sizeof(assoc_rsp);
         *rssi_out = -62;
         return (int)sizeof(assoc_rsp);
     }
     /* Simulate EAPOL msg 1 on 4th call */
     if (s_rx_call == 4) {
         static uint8_t eapol_msg1[99];
         memset(eapol_msg1, 0, sizeof(eapol_msg1));
         eapol_msg1[0] = 0x02u;   /* EAPOL-Key */
         eapol_msg1[5] = 0x00u;   /* No MIC (msg 1) */
         /* ANonce at offset 17 */
         for (int i = 0; i < 32; i++) eapol_msg1[17 + i] = (uint8_t)(i + 1);
         *phys_out = (uintptr_t)eapol_msg1;
         *len_out  = sizeof(eapol_msg1);
         *rssi_out = -62;
         return (int)sizeof(eapol_msg1);
     }
     return 0;
 }
 
 static int stub_hw_key_install(uiox_wifi_hw_t *hw, uint8_t key_idx,
                                 const uint8_t *key, uint8_t key_len,
                                 const uiox_wifi_mac_t peer,
                                 bool is_group, uint8_t cipher)
 {
     (void)hw; (void)key; (void)peer;
     printf("  [hal] key install  idx=%u  len=%u  group=%d  cipher=0x%02X\n",
            key_idx, key_len, (int)is_group, cipher);
     return 0;
 }
 
 static void stub_hw_key_delete(uiox_wifi_hw_t *hw, uint8_t key_idx)
 { (void)hw; printf("  [hal] key delete  idx=%u\n", key_idx); }
 
 static int stub_get_rssi(uiox_wifi_hw_t *hw, int8_t *rssi_dbm)
 { (void)hw; *rssi_dbm = -65; return 0; }
 
 static int stub_get_noise(uiox_wifi_hw_t *hw, int8_t *noise_dbm)
 { (void)hw; *noise_dbm = -95; return 0; }
 
 static void stub_isr_tx (uiox_wifi_hw_t *hw) { (void)hw; }
 static void stub_isr_rx (uiox_wifi_hw_t *hw) { (void)hw; }
 static void stub_isr_bcn(uiox_wifi_hw_t *hw) { (void)hw; }
 static void stub_isr_err(uiox_wifi_hw_t *hw) { (void)hw; }
 
 static int stub_bus_read(uiox_wifi_hw_t *hw, uint32_t addr,
                           void *buf, uint16_t len)
 { (void)hw; (void)addr; memset(buf, 0, len); return 0; }
 
 static int stub_bus_write(uiox_wifi_hw_t *hw, uint32_t addr,
                            const void *buf, uint16_t len)
 { (void)hw; (void)addr; (void)buf; (void)len; return 0; }
 
 static const uiox_wifi_hw_ops_t stub_ops = {
     .init           = stub_init,
     .deinit         = stub_deinit,
     .start          = stub_start,
     .stop           = stub_stop,
     .set_mode       = stub_set_mode,
     .set_channel    = stub_set_channel,
     .set_mac        = stub_set_mac,
     .set_tx_power   = stub_set_tx_power,
     .tx_submit      = stub_tx_submit,
     .tx_reclaim     = stub_tx_reclaim,
     .rx_poll        = stub_rx_poll,
     .hw_key_install = stub_hw_key_install,
     .hw_key_delete  = stub_hw_key_delete,
     .get_rssi       = stub_get_rssi,
     .get_noise      = stub_get_noise,
     .isr_tx         = stub_isr_tx,
     .isr_rx         = stub_isr_rx,
     .isr_bcn        = stub_isr_bcn,
     .isr_err        = stub_isr_err,
     .bus_read       = stub_bus_read,
     .bus_write      = stub_bus_write,
 };
 
 /* =========================================================================
  * Hardware device instance
  * ====================================================================== */
 
 static uiox_wifi_hw_t s_hw = {
     .base_addr   = 0xA0000000uL,
     .irq         = 45,
     .caps        = UIOX_WIFI_CAP_2G4       |
                    UIOX_WIFI_CAP_5G        |
                    UIOX_WIFI_CAP_11N       |
                    UIOX_WIFI_CAP_11AC      |
                    UIOX_WIFI_CAP_HW_CRYPT  |
                    UIOX_WIFI_CAP_AMPDU_TX  |
                    UIOX_WIFI_CAP_AMPDU_RX  |
                    UIOX_WIFI_CAP_WMM       |
                    UIOX_WIFI_CAP_MIMO_2T2R |
                    UIOX_WIFI_CAP_WOWLAN,
     .bus         = UIOX_WIFI_BUS_SDIO,
     .mode        = UIOX_WIFI_MODE_STA,
     .mac_addr    = { 0x02, 0x11, 0x22, 0x33, 0x44, 0x55 },
     .clk_hz      = 40000000u,
     .tx_ring_sz  = 32,
     .rx_ring_sz  = 64,
 };
 
 /* =========================================================================
  * Event callback
  * ====================================================================== */
 
 static void on_wifi_event(uiox_wifi_evt_t evt, void *ctx)
 {
     (void)ctx;
     printf("  [event] %s\n", uiox_wifi_evt_name(evt));
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX WiFi Stack Demo ===\n\n");
 
     /* ------------------------------------------------------------------ */
     /* 1. Open device                                                      */
     /* ------------------------------------------------------------------ */
 
     printf("--- Open ---\n");
     uiox_wifi_device_t dev;
     uiox_wifi_open_params_t p;
     memset(&p, 0, sizeof(p));
 
     p.hw      = &s_hw;
     p.hw_ops  = &stub_ops;
     p.evt_cb  = on_wifi_event;
 
     /* Subsystem config */
     strncpy(p.subsys_cfg.ssid,       "UIOX_LAB",  UIOX_WIFI_SSID_MAX);
     strncpy(p.subsys_cfg.passphrase, "SecurePass123!", 63);
     p.subsys_cfg.auto_reconnect     = true;
     p.subsys_cfg.reconnect_delay_ms = 5000u;
     p.subsys_cfg.scan_interval_ms   = 60000u;
     p.subsys_cfg.ps_mode            = UIOX_WIFI_PS_MIN;
 
     int rc = uiox_wifi_open(&dev, &p);
     if (rc < 0) {
         printf("[error] uiox_wifi_open failed: %d\n", rc);
         return 1;
     }
 
     uiox_wifi_mac_t mac;
     uiox_wifi_get_mac(&dev, mac);
     printf("  MAC address : %02X:%02X:%02X:%02X:%02X:%02X\n",
            mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
     printf("  Capabilities: 0x%08X\n", uiox_wifi_caps(&s_hw));
 
     /* ------------------------------------------------------------------ */
     /* 2. Start driver                                                     */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Start ---\n");
     rc = uiox_wifi_start(&dev);
     if (rc < 0) {
         printf("[error] uiox_wifi_start failed: %d\n", rc);
         uiox_wifi_close(&dev);
         return 1;
     }
 
     /* ------------------------------------------------------------------ */
     /* 3. Scan                                                             */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Scan (500 ms) ---\n");
     rc = uiox_wifi_scan(&dev, 500u);
     printf("  Networks found : %d\n", rc > 0 ? rc : 0);
 
     uint8_t bss_count = 0;
     const uiox_wifi_bss_t *bss_list = uiox_wifi_bss_list(&dev, &bss_count);
     for (uint8_t i = 0; i < bss_count; i++) {
         const uiox_wifi_bss_t *b = &bss_list[i];
         if (!b->valid) continue;
         printf("  [%u] SSID=%-24s  BSSID=%02X:%02X:%02X:%02X:%02X:%02X"
                "  ch=%2u  RSSI=%d dBm  cipher=0x%02X  akm=0x%02X\n",
                i, b->ssid,
                b->bssid[0],b->bssid[1],b->bssid[2],
                b->bssid[3],b->bssid[4],b->bssid[5],
                b->channel, b->rssi_dbm, b->cipher, b->akm);
     }
 
     /* ------------------------------------------------------------------ */
     /* 4. Connect                                                          */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Connect to 'UIOX_LAB' ---\n");
     rc = uiox_wifi_connect(&dev, "UIOX_LAB", "SecurePass123!", 5000u);
     printf("  Connect rc=%d  state=%s\n",
            rc, uiox_wifi_state_name(dev.subsys.state));
 
     /* ------------------------------------------------------------------ */
     /* 5. Data TX                                                          */
     /* ------------------------------------------------------------------ */
 
     if (uiox_wifi_connected(&dev)) {
         printf("\n--- Data TX ---\n");
 
         /* ARP packet (ethertype 0x0806) */
         static const uiox_wifi_mac_t broadcast_mac =
             {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
         uint8_t arp_payload[28] = {
             0x00,0x01, 0x08,0x00, 0x06, 0x04, 0x00,0x01,
             0x02,0x11,0x22,0x33,0x44,0x55,  /* sender MAC */
             0xC0,0xA8,0x01,0x64,            /* sender IP 192.168.1.100 */
             0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  /* target MAC (broadcast) */
             0xC0,0xA8,0x01,0x01             /* target IP 192.168.1.1 */
         };
         rc = uiox_wifi_tx(&dev, broadcast_mac, 0x0806u,
                            arp_payload, sizeof(arp_payload));
         printf("  ARP TX  rc=%d\n", rc);
 
         /* IPv4 UDP probe (ethertype 0x0800) */
         static const uiox_wifi_mac_t gw_mac =
             {0xAA,0xBB,0xCC,0x11,0x22,0x33};
         uint8_t udp_payload[20] = {
             0x45,0x00,0x00,0x14, 0,0,0,0, 0x40,0x11,
             0x00,0x00,
             0xC0,0xA8,0x01,0x64,  /* src 192.168.1.100 */
             0xC0,0xA8,0x01,0x01   /* dst 192.168.1.1   */
         };
         rc = uiox_wifi_tx(&dev, gw_mac, 0x0800u,
                            udp_payload, sizeof(udp_payload));
         printf("  IPv4 TX  rc=%d\n", rc);
 
         /* Flush TX queues */
         uiox_wifi_if_tx_flush(&dev.subsys.wif);
     }
 
     /* ------------------------------------------------------------------ */
     /* 6. Periodic tick loop                                               */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Tick loop (10 × 100 ms) ---\n");
     for (uint32_t t = 100u; t <= 1000u; t += 100u) {
         uiox_wifi_tick(&dev, t);
         if (t % 500u == 0u)
             printf("  tick t=%u ms  state=%s  RSSI=%d dBm\n",
                    t,
                    uiox_wifi_state_name(dev.subsys.state),
                    dev.subsys.quality.rssi_dbm);
     }
 
     /* ------------------------------------------------------------------ */
     /* 7. Quality metrics                                                  */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Quality metrics ---\n");
     uiox_wifi_quality_t q;
     uiox_wifi_get_quality(&dev, &q);
     printf("  RSSI           : %d dBm\n",  q.rssi_dbm);
     printf("  Link quality   : %u %%\n",   q.link_quality);
     printf("  TX rate index  : %u\n",       q.tx_rate_idx);
     printf("  TX frames      : %u\n",       q.tx_frames);
     printf("  RX frames      : %u\n",       q.rx_frames);
     printf("  TX retries     : %u\n",       q.tx_retries);
     printf("  Reconnects     : %u\n",       q.reconnect_count);
 
     /* ------------------------------------------------------------------ */
     /* 8. Statistics                                                       */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Statistics ---\n");
     uiox_wifi_print_stats(&dev);
 
     /* ------------------------------------------------------------------ */
     /* 9. Disconnect and close                                             */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Disconnect ---\n");
     uiox_wifi_disconnect(&dev);
     printf("  State : %s\n", uiox_wifi_state_name(dev.subsys.state));
 
     printf("\n--- Close ---\n");
     uiox_wifi_close(&dev);
     printf("  Device : CLOSED\n");
 
     printf("\n=== UIOX WiFi Demo complete ===\n");
     return 0;
 }
 