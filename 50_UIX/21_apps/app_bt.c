/*
 * 50_UIX/21_apps/app_bt.c
 *
 * Bluetooth application — EL0.  Mirrors app_wifi.c.
 *
 *   21_apps/app_bt.c → dev/bt/uiox_bt_device.c
 *     → dev/uiox_dev_stream.c → 40_SCIX → 30_KIX
 *
 * No 30_KIX header.
 *
 * v1.0.0  @date 2026-11-21
 */

 #include "bt/uiox_bt_device.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 
 int app_bt_main(void)
 {
     uiox_bt_device_t dev;
     uiox_dev_err_t   rc;
 
     uix_printf("bt: opening /dev/bt0\n");
 
     rc = uiox_bt_open(&dev, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("bt: open failed (%d) — no /dev/bt0.\n", (int)rc);
         uix_printf("      Is the node created, and did uiox_bt_probe() run?\n");
         return 1;
     }
 
     uix_printf("bt: state %s\n", uiox_bt_state_name(uiox_bt_state(&dev)));
 
     uix_printf("bt: scan (8 x 1.28 s)\n");
     if (uiox_bt_scan(&dev, 8u) == UIOX_DEV_OK)
         uix_printf("bt: scan started, state %s\n",
                    uiox_bt_state_name(uiox_bt_state(&dev)));
     else
         uix_printf("bt: scan failed\n");
 
     uix_printf("bt: scan stop\n");
     uiox_bt_scan_stop(&dev);
     uix_printf("bt: state %s\n", uiox_bt_state_name(uiox_bt_state(&dev)));
 
     uiox_bt_disconnect(&dev, 0u);
     uix_printf("bt: disconnect requested for handle 0\n");
 
     uiox_bt_close(&dev);
     uix_printf("bt: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_bt_main();
 }
 