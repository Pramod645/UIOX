/*
 * 50_UIX/21_apps/app_wifi.c
 *
 * WiFi application — EL0.  Mirrors app_eth.c: body as app_wifi_main(),
 * main() calling it, uix_printf throughout.
 *
 *   21_apps/app_wifi.c → dev/WiFi/uiox_wifi_device.c
 *     → dev/uiox_dev_stream.c → 40_SCIX → 30_KIX
 *
 * No 30_KIX header.
 *
 * v1.0.0  @date 2026-11-21
 */

 #include "WiFi/uiox_wifi_device.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 #include "uix_string.h"
 
 int app_wifi_main(void)
 {
     uiox_wifi_device_t  w;
     uiox_wifi_quality_t q;
     uiox_dev_err_t      rc;
 
     uix_printf("wifi: opening /dev/wlan0\n");
 
     rc = uiox_wifi_open(&w, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("wifi: open failed (%d) — no /dev/wlan0.\n", (int)rc);
         uix_printf("      Is the node created, and did uiox_wifi_probe() run?\n");
         return 1;
     }
 
     uix_printf("wifi: state %s\n", uiox_wifi_state_name(uiox_wifi_state(&w)));
 
     uix_printf("wifi: scan (5000 ms)\n");
     if (uiox_wifi_scan(&w, 5000u) == UIOX_DEV_OK)
         uix_printf("wifi: scan returned\n");
     else
         uix_printf("wifi: scan failed\n");
 
     if (uiox_wifi_quality(&w, &q) == UIOX_DEV_OK) {
         uix_printf("wifi: rssi %d dBm  link %u %%  rate idx %u\n",
                    (int)q.rssi_dbm, (unsigned)q.link_quality,
                    (unsigned)q.tx_rate_idx);
         uix_printf("wifi: tx %u  rx %u  retries %u  reconnects %u\n",
                    (unsigned)q.tx_frames, (unsigned)q.rx_frames,
                    (unsigned)q.tx_retries, (unsigned)q.reconnect_count);
     } else {
         uix_printf("wifi: quality unavailable\n");
     }
 
     uiox_wifi_disconnect(&w);
     uix_printf("wifi: disconnect requested\n");
 
     uiox_wifi_close(&w);
     uix_printf("wifi: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_wifi_main();
 }
 