/*
 * 50_UIX/21_apps/app_cam.c
 *
 * Camera application — EL0.  Mirrors app_spk.c, with values in place of
 * buffers: camera is configurable from EL0 and not readable from it.
 *
 *   21_apps/app_cam.c → dev/cam/uiox_cam_device.c
 *     → dev/uiox_dev_stream.c → 40_SCIX → 30_KIX
 *
 * No 30_KIX header.
 *
 * ── WHY NO FRAMES ───────────────────────────────────────────────────
 * uiox_cam_capture_try() returns uiox_cam_frame_t *, and the demo
 * prints f->paddr from it — a PHYSICAL address only something able to
 * program DMA can consume.  Nothing to copy into a caller buffer, so
 * the kernel has no row for it and this app cannot ask.  HDMI's FLIP
 * and TB4's get_router hit the same wall.
 *
 * v1.0.0  @date 2026-11-21
 */

 #include "cam/uiox_cam_device.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 #include "uix_string.h"          /* uix_memset */
 
 int app_cam_main(void)
 {
     uiox_cam_device_t      dev;
     uiox_cam_open_params_t params;
     uiox_dev_err_t         rc;
 
     uix_printf("cam: opening /dev/cam0\n");
 
     /* ── the mode is the application's choice ─────────────────────── */
     uix_memset(&params, 0, sizeof(params));
     params.width  = 1280u;
     params.height = 720u;
     params.fps    = 30u;
     params.pixfmt = UIOX_CAM_PIX_FMT_RAW10;
     params.stride = 1280u * 2u;      /* bytes per line, RAW10 packed */
 
     rc = uiox_cam_open(&dev, &params, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("cam: open failed (%d) — no /dev/cam0.\n", (int)rc);
         uix_printf("      Is the node created, and did uiox_cam_probe() run?\n");
         return 1;
     }
 
     uix_printf("cam: %ux%u @ %u fps  %s\n",
                (unsigned)params.width, (unsigned)params.height,
                (unsigned)params.fps,
                uiox_cam_pixfmt_name((uiox_cam_pixfmt_t)params.pixfmt));
     uix_printf("cam: state %s\n",
                uiox_cam_state_name(uiox_cam_state(&dev)));
 
     /* ── controls, before streaming ───────────────────────────────── */
     uix_printf("\n--- Controls ---\n");
 
     if (uiox_cam_set_exposure(&dev, 5000u) == UIOX_DEV_OK)
         uix_printf("cam: exposure 5000 us\n");
     else
         uix_printf("cam: exposure rejected\n");
 
     if (uiox_cam_set_gain(&dev, 150u) == UIOX_DEV_OK)
         uix_printf("cam: gain 1.50x\n");
     else
         uix_printf("cam: gain rejected\n");
 
     /* ── stream ───────────────────────────────────────────────────── */
     uix_printf("\n--- Start streaming ---\n");
     if (uiox_cam_stream(&dev, UIX_TRUE) == UIOX_DEV_OK)
         uix_printf("cam: state %s\n",
                    uiox_cam_state_name(uiox_cam_state(&dev)));
     else
         uix_printf("cam: start failed\n");
 
     /* ── AE change mid-stream ─────────────────────────────────────── */
     uix_printf("\n--- AE update mid-stream ---\n");
     uiox_cam_set_exposure(&dev, 8000u);
     uiox_cam_set_gain(&dev, 200u);
     uix_printf("cam: exposure 8000 us, gain 2.00x\n");
 
     uix_printf("\n--- Frames ---\n");
     uix_printf("cam: capture is kernel-side — a frame's payload is a\n");
     uix_printf("     physical address (f->paddr), so there is no row\n");
     uix_printf("     that copies one to EL0.\n");
 
     /* ── stop ─────────────────────────────────────────────────────── */
     uix_printf("\n--- Stop streaming ---\n");
     uiox_cam_stream(&dev, UIX_FALSE);
     uix_printf("cam: state %s\n",
                uiox_cam_state_name(uiox_cam_state(&dev)));
 
     uiox_cam_close(&dev);
     uix_printf("cam: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_cam_main();
 }
 