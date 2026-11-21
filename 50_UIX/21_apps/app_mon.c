/*
 * 50_UIX/21_apps/app_mon.c
 *
 * Monitor application — EL0.  Control only: state, mode, DPMS, fps,
 * activity.
 *
 *   21_apps/app_mon.c → dev/monitor/uiox_mon_device.c
 *     → dev/uiox_dev_stream.c → 40_SCIX → 30_KIX
 *
 * No 30_KIX header.
 *
 * v1.0.0  @date 2026-11-21
 */

 #include "monitor/uiox_mon_device.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 #include "uix_string.h"
 
 int app_mon_main(void)
 {
     uiox_mon_device_t      dev;
     uiox_mon_open_params_t params;
     uiox_dev_err_t         rc;
 
     uix_printf("mon: opening /dev/mon0\n");
 
     uix_memset(&params, 0, sizeof(params));
     params.pref_w          = 1920u;
     params.pref_h          = 1080u;
     params.pref_hz         = 60u;
     params.dpms_timeout_ms = 300000u;   /* dim after 5 min idle */
     params.target_fps      = 60u;
 
     rc = uiox_mon_open(&dev, &params, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("mon: open failed (%d) — no /dev/mon0.\n", (int)rc);
         uix_printf("      Is the node created, and did uiox_mon_probe() run?\n");
         return 1;
     }
 
     uix_printf("mon: %ux%u @ %u Hz  fps cap %u\n",
                (unsigned)params.pref_w, (unsigned)params.pref_h,
                (unsigned)params.pref_hz, (unsigned)params.target_fps);
     uix_printf("mon: state %s\n",
                uiox_mon_state_name(uiox_mon_state(&dev)));
 
     uix_printf("\n--- DPMS / pacing ---\n");
     if (uiox_mon_set_dpms(&dev, 600000u) == UIOX_DEV_OK)
         uix_printf("mon: DPMS timeout 600000 ms\n");
 
     if (uiox_mon_set_fps(&dev, 30u) == UIOX_DEV_OK)
         uix_printf("mon: frame rate capped at 30\n");
 
     if (uiox_mon_activity(&dev, 1000u) == UIOX_DEV_OK)
         uix_printf("mon: activity signalled\n");
 
     uix_printf("\n--- Frames ---\n");
     uix_printf("mon: acquire/present are kernel-side — a frame is a\n");
     uix_printf("     pointer into the pool and must survive between two\n");
     uix_printf("     calls.  No row copies one to EL0.\n");
 
     uiox_mon_close(&dev);
     uix_printf("mon: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_mon_main();
 }
 