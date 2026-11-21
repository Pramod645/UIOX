/*
 * 50_UIX/21_apps/app_hdmi.c
 *
 * HDMI application — EL0.  Control only: state, mode, DPMS, activity.
 *
 *   21_apps/app_hdmi.c → dev/hdmi/uiox_hdmi_device.c
 *     → dev/uiox_dev_stream.c → 40_SCIX → 30_KIX
 *
 * No 30_KIX header.
 *
 * v1.0.0  @date 2026-11-21
 */

 #include "hdmi/uiox_hdmi_device.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 #include "uix_string.h"
 
 int app_hdmi_main(void)
 {
     uiox_hdmi_device_t      dev;
     uiox_hdmi_open_params_t params;
     uiox_dev_err_t          rc;
 
     uix_printf("hdmi: opening /dev/hdmi0\n");
 
     uix_memset(&params, 0, sizeof(params));
     params.pref_w         = 1920u;
     params.pref_h         = 1080u;
     params.pref_hz        = 60u;
     params.dpms_timeout_ms = 300000u;   /* blank after 5 min idle */
 
     rc = uiox_hdmi_open(&dev, &params, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("hdmi: open failed (%d) — no /dev/hdmi0.\n", (int)rc);
         uix_printf("      Is the node created, and did uiox_hdmi_probe() run?\n");
         return 1;
     }
 
     uix_printf("hdmi: %ux%u @ %u Hz\n",
                (unsigned)params.pref_w, (unsigned)params.pref_h,
                (unsigned)params.pref_hz);
     uix_printf("hdmi: state %s\n",
                uiox_hdmi_state_name(uiox_hdmi_state(&dev)));
 
     uix_printf("\n--- Activity / DPMS ---\n");
     if (uiox_hdmi_activity(&dev, 1000u) == UIOX_DEV_OK)
         uix_printf("hdmi: activity signalled\n");
 
     if (uiox_hdmi_set_dpms(&dev, 600000u) == UIOX_DEV_OK)
         uix_printf("hdmi: DPMS timeout 600000 ms\n");
 
     uix_printf("\n--- Frames ---\n");
     uix_printf("hdmi: acquire/present are kernel-side — a frame is a\n");
     uix_printf("      pointer into the pool, and it must survive between\n");
     uix_printf("      two calls.  No row copies one to EL0.\n");
 
     uiox_hdmi_close(&dev);
     uix_printf("hdmi: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_hdmi_main();
 }
 