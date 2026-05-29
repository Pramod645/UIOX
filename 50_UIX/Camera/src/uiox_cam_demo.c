/**
 * @file    uiox_cam_demo.c
 * @brief   UIOX camera stack end-to-end demonstration.
 *
 * Shows the complete flow from HAL init → sensor detection →
 * pipeline configuration → streaming → frame capture → teardown,
 * using only the uiox_cam_device.h top-level API.
 *
 * Build:
 *   See Makefile.
 *
 * @date    2026-05-26
 */

 #include "uiox_cam_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 
 /* =========================================================================
  * Stub I2C bus ops (replace with platform I2C driver)
  * ====================================================================== */
 
 static int stub_i2c_write8(uint8_t dev_addr, uint16_t reg, uint8_t val)
 {
     printf("  [i2c] W  addr=0x%02X  reg=0x%04X  val=0x%02X\n",
            dev_addr, reg, val);
     return 0;
 }
 
 static int stub_i2c_read8(uint8_t dev_addr, uint16_t reg, uint8_t *val)
 {
     /* Simulate chip-ID read succeeding */
     (void)dev_addr; (void)reg;
     *val = 0x40;   /* pretend chip-ID low byte = 0x40 */
     return 0;
 }
 
 static int stub_delay_ms(uint32_t ms)
 {
     (void)ms;
     return 0;
 }
 
 /* =========================================================================
  * Stub HAL ops (replace with concrete CSI/ISP driver)
  * ====================================================================== */
 
 static int  stub_cam_init    (uiox_cam_hw_t *hw) { (void)hw; printf("  [hal] init\n");  return 0; }
 static void stub_cam_deinit  (uiox_cam_hw_t *hw) { (void)hw; printf("  [hal] deinit\n"); }
 static int  stub_cam_start   (uiox_cam_hw_t *hw) { (void)hw; printf("  [hal] start\n"); return 0; }
 static void stub_cam_stop    (uiox_cam_hw_t *hw) { (void)hw; printf("  [hal] stop\n"); }
 
 static int stub_cam_set_csi(uiox_cam_hw_t *hw,
                              uint8_t lanes, uiox_csi_dt_t dt, uint8_t vc)
 {
     (void)hw;
     printf("  [hal] CSI-2  lanes=%u  DT=0x%02X  VC=%u\n", lanes, dt, vc);
     return 0;
 }
 
 static int stub_cam_set_format(uiox_cam_hw_t *hw,
                                uint16_t w, uint16_t h,
                                uiox_cam_pixfmt_t fmt)
 {
     (void)hw;
     printf("  [hal] format  %ux%u  fmt=%d\n", w, h, (int)fmt);
     return 0;
 }
 
 static int stub_cam_dma_queue(uiox_cam_hw_t *hw, uintptr_t phys, uint32_t length)
 {
     (void)hw;
     printf("  [hal] DMA queue  phys=0x%08lX  len=%u\n",
            (unsigned long)phys, length);
     return 0;
 }
 
 static int stub_cam_dma_complete(uiox_cam_hw_t *hw,
                                   uintptr_t *phys_out, uint32_t *bytes_out)
 {
     (void)hw;
     /* Simulate a completed frame of 1280*720*2 bytes */
     static uintptr_t fake_phys = 0x20000000uL;
     *phys_out  = fake_phys;
     *bytes_out = 1280u * 720u * 2u;
     fake_phys += *bytes_out;
     return (int)*bytes_out;
 }
 
 static void     stub_cam_isr       (uiox_cam_hw_t *hw) { (void)hw; }
 static uint16_t stub_cam_mdio_read (uiox_cam_hw_t *hw, uint8_t phy, uint8_t reg)
                 { (void)hw; (void)phy; (void)reg; return 0; }
 static void     stub_cam_mdio_write(uiox_cam_hw_t *hw, uint8_t phy,
                                      uint8_t reg, uint16_t val)
                 { (void)hw; (void)phy; (void)reg; (void)val; }
 
 static const uiox_cam_hw_ops_t stub_cam_ops = {
     .init         = stub_cam_init,
     .deinit       = stub_cam_deinit,
     .start        = stub_cam_start,
     .stop         = stub_cam_stop,
     .set_csi      = stub_cam_set_csi,
     .set_format   = stub_cam_set_format,
     .dma_queue    = stub_cam_dma_queue,
     .dma_complete = stub_cam_dma_complete,
     .isr          = stub_cam_isr,
 };
 
 /* =========================================================================
  * Hardware device instance (example: ARM64 MIPI CSI-2 controller)
  * ====================================================================== */
 
 static uiox_cam_hw_t s_cam_hw = {
     .base_addr = 0x50000000uL,   /* CSI-2 controller MMIO base   */
     .irq       = 60,
     .caps      = UIOX_CAM_CAP_CSI2 |
                  UIOX_CAM_CAP_DMA_CONTIG |
                  UIOX_CAM_CAP_EMBD_DATA,
     .max_lanes = 4,
     .priv      = NULL,
 };
 
 /* =========================================================================
  * Pixel format label helper
  * ====================================================================== */
 
 static const char *pixfmt_name(uiox_cam_pixfmt_t f)
 {
     switch (f) {
     case UIOX_CAM_PIX_FMT_RAW8:   return "RAW8";
     case UIOX_CAM_PIX_FMT_RAW10:  return "RAW10";
     case UIOX_CAM_PIX_FMT_RAW12:  return "RAW12";
     case UIOX_CAM_PIX_FMT_YUV420: return "YUV420";
     case UIOX_CAM_PIX_FMT_YUV422: return "YUV422";
     case UIOX_CAM_PIX_FMT_RGB888: return "RGB888";
     default:                       return "UNKNOWN";
     }
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX Camera Stack Demo ===\n\n");
 
     /* ------------------------------------------------------------------ */
     /* 1. Open the camera device                                           */
     /* ------------------------------------------------------------------ */
 
     uiox_cam_device_t dev;
     uiox_cam_open_params_t params;
     memset(&params, 0, sizeof(params));
 
     params.hw       = &s_cam_hw;
     params.hw_ops   = &stub_cam_ops;
 
     /* I2C bus operations */
     params.bus.i2c_write8 = stub_i2c_write8;
     params.bus.i2c_read8  = stub_i2c_read8;
     params.bus.delay_ms   = stub_delay_ms;
 
     /* Sensor identity */
     params.sensor_name  = "imx219";
     params.i2c_addr     = 0x10;       /* 7-bit address */
     params.chip_id_reg  = 0x0000;     /* whoami reg    */
     params.chip_id_val  = 0x0219;     /* expected ID   */
 
     /* CSI-2 interface */
     params.lanes    = 2;
     params.virt_chan= 0;
     params.csi_dt   = UIOX_CSI_DT_RAW10;
 
     /* Capture format */
     params.pixfmt      = UIOX_CAM_PIX_FMT_RAW10;
     params.width       = 1280;
     params.height      = 720;
     params.stride      = 1280 * 2;    /* bytes per line for RAW10 packed */
     params.fps         = 30;
     params.queue_count = 4;           /* prime 4 DMA buffers */
 
     printf("--- Open ---\n");
     int rc = uiox_cam_open(&dev, &params);
     if (rc < 0) {
         printf("[error] uiox_cam_open failed: %d\n", rc);
         return 1;
     }
     printf("  Sensor   : %s  (I2C 0x%02X)\n",
            params.sensor_name, params.i2c_addr);
     printf("  Format   : %ux%u @ %u fps  %s\n",
            params.width, params.height, params.fps,
            pixfmt_name(params.pixfmt));
     printf("  Lanes    : %u  VC=%u  DT=0x%02X\n",
            params.lanes, params.virt_chan, (unsigned)params.csi_dt);
     printf("  Buffers  : %u queued\n\n", params.queue_count);
 
     /* ------------------------------------------------------------------ */
     /* 2. Set exposure and gain before streaming                           */
     /* ------------------------------------------------------------------ */
 
     printf("--- Controls ---\n");
     rc = uiox_cam_set_exposure(&dev, 5000);   /* 5 ms */
     printf("  Exposure : 5000 us  rc=%d\n", rc);
 
     rc = uiox_cam_set_gain(&dev, 150);         /* 1.50x */
     printf("  Gain     : 1.50x   rc=%d\n\n", rc);
 
     /* ------------------------------------------------------------------ */
     /* 3. Start streaming                                                  */
     /* ------------------------------------------------------------------ */
 
     printf("--- Start streaming ---\n");
     rc = uiox_cam_start(&dev);
     if (rc < 0) {
         printf("[error] uiox_cam_start failed: %d\n", rc);
         return 1;
     }
     printf("  Streaming: ACTIVE\n\n");
 
     /* ------------------------------------------------------------------ */
     /* 4. Capture frames                                                   */
     /* ------------------------------------------------------------------ */
 
     printf("--- Capture frames ---\n");
     const int NUM_FRAMES = 5;
     int captured = 0;
 
     while (captured < NUM_FRAMES) {
         uiox_cam_frame_t *f = uiox_cam_capture_try(&dev);
         if (!f) {
             /* No frame yet — in a real system sleep and retry */
             /* uiox_os_sleep_ms(1); */
             continue;
         }
 
         printf("  Frame %d:  phys=0x%08lX  size=%ux%u  stride=%u"
                "  bytes=%u  fmt=%s\n",
                captured + 1,
                (unsigned long)f->paddr,
                f->width, f->height, f->stride,
                f->length,
                pixfmt_name((uiox_cam_pixfmt_t)f->fmt));
 
         /* Application processes frame here (e.g. ISP, encode, display) */
         /* ... */
 
         /* Return buffer to pool */
         uiox_cam_buf_free(f);
 
         /* Re-prime one buffer back into DMA ring */
         uiox_cam_frame_t *fresh = uiox_cam_buf_alloc();
         if (fresh) {
             const uiox_cam_hw_ops_t *ops =
                 (const uiox_cam_hw_ops_t *)dev.hw->priv;
             if (ops && ops->dma_queue)
                 ops->dma_queue(dev.hw, fresh->paddr, fresh->length);
         }
 
         captured++;
     }
 
     /* ------------------------------------------------------------------ */
     /* 5. Change exposure mid-stream (AE simulation)                      */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- AE update mid-stream ---\n");
     uiox_cam_set_exposure(&dev, 8000);  /* 8 ms */
     uiox_cam_set_gain(&dev, 200);       /* 2.00x */
     printf("  Exposure : 8000 us\n");
     printf("  Gain     : 2.00x\n");
 
     /* ------------------------------------------------------------------ */
     /* 6. Buffer pool status                                               */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Buffer pool ---\n");
     printf("  Free frames : %u / %u\n",
            uiox_cam_buf_free_count(), UIOX_CAM_POOL_FRAMES);
 
     /* ------------------------------------------------------------------ */
     /* 7. Stop streaming and close                                         */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Stop streaming ---\n");
     uiox_cam_stop(&dev);
     printf("  Streaming: STOPPED\n");
 
     printf("\n=== Demo complete ===\n");
     return 0;
 }
 