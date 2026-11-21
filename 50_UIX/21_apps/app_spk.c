/*
 * 50_UIX/21_apps/app_spk.c
 *
 * Speaker application — EL0.  Mirrors app_mic.c, with write in place of
 * read and stereo pairs in place of mono samples.
 *
 *   21_apps/app_spk.c → dev/spk/uiox_spk_device.c
 *     → dev/uiox_dev_stream.c → 40_SCIX → 30_KIX
 *
 * No 30_KIX header.
 *
 * v1.0.0  @date 2026-11-21
 */

 #include "spk/uiox_spk_device.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 
 #define APP_SPK_FRAME_PAIRS  480u   /* 480 stereo pairs per write */
 
 int app_spk_main(void)
 {
     static uix_int16_t tone[APP_SPK_FRAME_PAIRS * 2];
     uiox_spk_device_t  dev;
     uiox_dev_err_t     rc;
     uix_ssize_t        sent;
     uix_uint32_t       i, t;
 
     uix_printf("spk: opening /dev/spk0\n");
 
     rc = uiox_spk_open(&dev, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("spk: open failed (%d) — no /dev/spk0.\n", (int)rc);
         uix_printf("      Is the node created, and did uiox_spk_probe() run?\n");
         return 1;
     }
 
     uix_printf("spk: state %s\n",
                uiox_spk_state_name(uiox_spk_state(&dev)));
 
     /* ── a simple 440 Hz stereo tone, so there is something to send ─── */
     for (i = 0u; i < APP_SPK_FRAME_PAIRS; i++) {
         uix_int16_t s = (i % 64u) < 32u ? 8000 : -8000;   /* square, cheap */
         tone[i * 2]     = s;
         tone[i * 2 + 1] = s;
     }
 
     uix_printf("\n--- Volume 60 %% ---\n");
     if (uiox_spk_set_volume(&dev, 60u) == UIOX_DEV_OK)
         uix_printf("spk: volume set\n");
     else
         uix_printf("spk: volume rejected\n");
 
     uix_printf("\n--- Play (4 writes of %u stereo pairs) ---\n",
                (unsigned)APP_SPK_FRAME_PAIRS);
 
     for (t = 0u; t < 4u; t++) {
         sent = uiox_spk_write(&dev, tone, APP_SPK_FRAME_PAIRS);
         if (sent > 0)
             uix_printf("spk: wrote %d stereo pairs\n", (int)sent);
         else
             uix_printf("spk: write failed (%d)\n", (int)sent);
     }
 
     uix_printf("\n--- Mute / unmute ---\n");
     uiox_spk_set_mute(&dev, UIX_TRUE);
     uix_printf("spk: muted\n");
     uiox_spk_set_mute(&dev, UIX_FALSE);
     uix_printf("spk: unmuted\n");
 
     uiox_spk_close(&dev);
     uix_printf("spk: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_spk_main();
 }
 