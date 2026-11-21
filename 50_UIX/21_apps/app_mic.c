/*
 * 50_UIX/21_apps/app_mic.c
 *
 * Microphone application — EL0.  Mirrors app_wifi.c and app_bt.c.
 *
 *   21_apps/app_mic.c → dev/mic/uiox_mic_device.c
 *     → dev/uiox_dev_stream.c → 40_SCIX → 30_KIX
 *
 * No 30_KIX header.
 *
 * v1.0.0  @date 2026-11-21
 */

 #include "mic/uiox_mic_device.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 
 int app_mic_main(void)
 {
     uiox_mic_device_t dev;
     uix_int16_t       pcm[UIOX_MIC_FRAME_SAMPLES];
     uiox_dev_err_t    rc;
     uix_ssize_t       got;
     uix_uint32_t      t;
 
     uix_printf("mic: opening /dev/mic0\n");
 
     rc = uiox_mic_open(&dev, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("mic: open failed (%d) — no /dev/mic0.\n", (int)rc);
         uix_printf("      Is the node created, and did uiox_mic_probe() run?\n");
         return 1;
     }
 
     uix_printf("mic: state %s\n",
                uiox_mic_state_name(uiox_mic_state(&dev)));
 
     uix_printf("\n--- Set gain 25 dB ---\n");
     if (uiox_mic_set_gain(&dev, 25u) == UIOX_DEV_OK)
         uix_printf("mic: gain set\n");
     else
         uix_printf("mic: gain rejected\n");
 
     uix_printf("\n--- Capture (4 reads of %u samples) ---\n",
                (unsigned)UIOX_MIC_FRAME_SAMPLES);
 
     for (t = 0u; t < 4u; t++) {
         got = uiox_mic_read(&dev, pcm, UIOX_MIC_FRAME_SAMPLES);
         if (got > 0)
             uix_printf("mic: read %d samples\n", (int)got);
         else
             uix_printf("mic: read failed (%d)\n", (int)got);
     }
 
     uix_printf("\n--- Mute / unmute ---\n");
     uiox_mic_set_mute(&dev, UIX_TRUE);
     uix_printf("mic: state %s\n",
                uiox_mic_state_name(uiox_mic_state(&dev)));
     uiox_mic_set_mute(&dev, UIX_FALSE);
     uix_printf("mic: state %s\n",
                uiox_mic_state_name(uiox_mic_state(&dev)));
 
     uiox_mic_close(&dev);
     uix_printf("mic: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_mic_main();
 }
 