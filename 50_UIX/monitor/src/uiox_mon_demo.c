/**
 * @file    uiox_mon_demo.c
 * @brief   UIOX Monitor stack end-to-end demonstration.
 * @date    2026-05-27
 */
//Demo Application
 #include "uiox_mon_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <math.h>
 
 /* =========================================================================
  * Stub EDID block — simulates a 1920×1080@60Hz FHD panel
  * ====================================================================== */
 
 static const uint8_t s_fake_edid[128] = {
     0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00, /* Header          */
     0x4C,0x2D,0x05,0x07,0x01,0x00,0x00,0x00, /* Mfr: SAM 0x0705 */
     0x01,0x1D,                                /* Mfr week/year   */
     0x01,0x04,                                /* EDID ver 1.4    */
     0xB5,0x3C,0x22,0x78,0x2A,                /* Video params    */
     0xEE,0x91,0xA3,0x54,0x4C,0x99,0x26,0x0F,0x50,0x54,
     0xBF,0xEF,0x80,                           /* Established timings */
     0x71,0x4F,0x81,0x00,0x81,0x40,0x81,0x80, /* Standard timings */
     0x95,0x00,0xA9,0xC0,0xB3,0x00,
     /* Detailed timing descriptor 0: 1920×1080@60Hz */
     0x02,0x3A, /* pixel clock = 0x3A02 × 10 = 148340 kHz ≈ 148.5 MHz */
     0x80,      /* h_active lo = 0x80 */
     0x18,      /* h_blank  lo = 0x18 = 24 */
     0x71,      /* h_active hi[7:4]=0x7=1920-wait, h_blank hi = 0x1 */
     0x38,      /* v_active lo = 0x38 = 56... (simplified) */
     0x2D,      /* v_blank lo  */
     0x40,      /* v hi nibbles */
     0x58,      /* h front porch */
     0x2C,      /* h sync width */
     0x45,0x00, /* v front/sync */
     0x00,0x00,0x00, /* size mm */
     0x1E,           /* border */
     0x18,           /* flags: +hsync +vsync */
     /* Descriptor 1: monitor name */
     0x00,0x00,0x00,0xFC,0x00,
     'U','I','O','X',' ','F','H','D',0x0A,0x20,0x20,0x20,
     /* Descriptor 2: range limits */
     0x00,0x00,0x00,0xFD,0x00,0x32,0x4B,0x1E,0x51,0x11,0x00,
     0x0A,0x20,0x20,0x20,0x20,0x20,0x20,
     /* Descriptor 3: serial */
     0x00,0x00,0x00,0xFF,0x00,
     'U','I','O','X','0','0','0','1',0x0A,0x20,0x20,0x20,
     0x01,  /* extension blocks */
     0x00   /* checksum placeholder */
 };
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int stub_init(uiox_mon_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] init  HDMI controller  base=0x%08lX\n",
            (unsigned long)hw->base_addr);
     hw->connected = true;
     return 0;
 }
 
 static void stub_deinit  (uiox_mon_hw_t *hw) { (void)hw; printf("  [hal] deinit\n"); }
 static int  stub_enable  (uiox_mon_hw_t *hw) { (void)hw; printf("  [hal] enable\n");  return 0; }
 static void stub_disable (uiox_mon_hw_t *hw) { (void)hw; printf("  [hal] disable\n"); }
 
 static int stub_set_timing(uiox_mon_hw_t *hw, const uiox_mon_timing_t *t)
 {
     (void)hw;
     printf("  [hal] timing  %ux%u@%uHz  pclk=%u kHz\n",
            t->h_active, t->v_active, t->refresh_hz, t->pixel_clk_khz);
     return 0;
 }
 
 static int stub_set_pixfmt(uiox_mon_hw_t *hw, uiox_mon_pixfmt_t fmt)
 {
     (void)hw;
     static const char *names[] = {"RGB565","RGB888","XRGB8888","ARGB8888",
                                    "YUV420","YUV422","YUV444"};
     printf("  [hal] pixfmt  %s\n", names[fmt]);
     return 0;
 }
 
 static uint32_t s_flip_count = 0;
 static int stub_flip(uiox_mon_hw_t *hw, uintptr_t phys, uint32_t stride)
 {
     (void)hw;
     printf("  [hal] flip  phys=0x%08lX  stride=%u  (#%u)\n",
            (unsigned long)phys, stride, ++s_flip_count);
     return 0;
 }
 
 static int stub_wait_vblank(uiox_mon_hw_t *hw, uint32_t timeout_ms)
 {
     (void)hw; (void)timeout_ms;
     hw->vblank_count++;
     return 0;
 }
 
 static int stub_read_edid(uiox_mon_hw_t *hw, uint8_t *buf)
 {
     (void)hw;
     /* Fix EDID checksum */
     memcpy(buf, s_fake_edid, 128);
     uint8_t sum = 0;
     for (int i = 0; i < 127; i++) sum += buf[i];
     buf[127] = (uint8_t)(256u - sum);
     printf("  [hal] read_edid  (128 bytes, checksum fixed)\n");
     return 0;
 }
 
 static int stub_set_dpms(uiox_mon_hw_t *hw, uiox_mon_dpms_t dpms)
 {
     (void)hw;
     static const char *names[] = {"ON","STANDBY","SUSPEND","OFF"};
     printf("  [hal] DPMS → %s\n", names[dpms]);
     return 0;
 }
 
 static int stub_set_backlight(uiox_mon_hw_t *hw, uint8_t level)
 {
     (void)hw;
     printf("  [hal] backlight = %u / 255  (%.1f%%)\n",
            level, (float)level * 100.0f / 255.0f);
     return 0;
 }
 
 static int stub_set_gamma(uiox_mon_hw_t *hw,
                            const uint16_t *r,
                            const uint16_t *g,
                            const uint16_t *b)
 {
     (void)hw;
     printf("  [hal] gamma LUT loaded  r[128]=%u  g[128]=%u  b[128]=%u\n",
            r[128], g[128], b[128]);
     return 0;
 }
 
 static bool stub_hotplug_state(uiox_mon_hw_t *hw)
 {
     (void)hw;
     return true;   /* always connected in simulation */
 }
 
 static void stub_isr_vblank (uiox_mon_hw_t *hw) { (void)hw; }
 static void stub_isr_hotplug(uiox_mon_hw_t *hw) { (void)hw; }
 
 static const uiox_mon_hw_ops_t stub_ops = {
     .init          = stub_init,
     .deinit        = stub_deinit,
     .enable        = stub_enable,
     .disable       = stub_disable,
     .set_timing    = stub_set_timing,
     .set_pixfmt    = stub_set_pixfmt,
     .flip          = stub_flip,
     .wait_vblank   = stub_wait_vblank,
     .read_edid     = stub_read_edid,
     .set_dpms      = stub_set_dpms,
     .set_backlight = stub_set_backlight,
     .set_gamma     = stub_set_gamma,
     .hotplug_state = stub_hotplug_state,
     .isr_vblank    = stub_isr_vblank,
     .isr_hotplug   = stub_isr_hotplug,
 };
 
 /* =========================================================================
  * Hardware device instance — HDMI FHD display controller
  * ====================================================================== */
 
 static uiox_mon_hw_t s_hw = {
     .base_addr    = 0x90000000uL,   /* Display controller MMIO base        */
     .irq_vblank   = 50,
     .irq_hotplug  = 51,
     .caps         = UIOX_MON_CAP_HDMI        |
                     UIOX_MON_CAP_DMA_FLIP    |
                     UIOX_MON_CAP_GAMMA       |
                     UIOX_MON_CAP_SCALING     |
                     UIOX_MON_CAP_CURSOR      |
                     UIOX_MON_CAP_DDC         |
                     UIOX_MON_CAP_BACKLIGHT_PWM |
                     UIOX_MON_CAP_HOTPLUG_IRQ |
                     UIOX_MON_CAP_VBLANK_IRQ  |
                     UIOX_MON_CAP_AUDIO,
     .if_type      = UIOX_MON_IF_HDMI,
     .pll_ref_hz   = 27000000u,      /* 27 MHz reference                    */
     .mipi_lanes   = 0u,
     .lvds_links   = 0u,
 };
 
 /* =========================================================================
  * Event callbacks
  * ====================================================================== */
 
 static void on_hotplug(bool connected, void *ctx)
 {
     (void)ctx;
     printf("  [hotplug] display %s\n",
            connected ? "CONNECTED" : "DISCONNECTED");
 }
 
 static void on_vblank(uint32_t frame_id, void *ctx)
 {
     (void)ctx;
     if (frame_id % 10u == 0u)
         printf("  [vblank] frame %u\n", frame_id);
 }
 
 /* =========================================================================
  * Simple test pattern generators
  * ====================================================================== */
 
 /** Solid colour fill */
 static void draw_solid(uiox_mon_fb_t *fb, uint32_t colour)
 {
     uiox_mon_buf_clear(fb, colour);
 }
 
 /** Horizontal gradient: black → white */
 static void draw_gradient(uiox_mon_fb_t *fb)
 {
     uint32_t *px = (uint32_t *)fb->vaddr;
     for (uint16_t y = 0; y < fb->height; y++) {
         for (uint16_t x = 0; x < fb->width; x++) {
             uint8_t v = (uint8_t)((uint32_t)x * 255u / fb->width);
             px[y * fb->width + x] =
                 0xFF000000u | ((uint32_t)v << 16u) |
                 ((uint32_t)v << 8u) | v;
         }
     }
 }
 
 /** Colour bars: 8 vertical bars (SMPTE-style) */
 static void draw_colour_bars(uiox_mon_fb_t *fb)
 {
     static const uint32_t bars[8] = {
         0xFFFFFFFF, /* White      */
         0xFFFFFF00, /* Yellow     */
         0xFF00FFFF, /* Cyan       */
         0xFF00FF00, /* Green      */
         0xFFFF00FF, /* Magenta    */
         0xFFFF0000, /* Red        */
         0xFF0000FF, /* Blue       */
         0xFF000000, /* Black      */
     };
     uint32_t *px = (uint32_t *)fb->vaddr;
     uint16_t bar_w = fb->width / 8u;
     for (uint16_t y = 0; y < fb->height; y++) {
         for (uint16_t x = 0; x < fb->width; x++) {
             uint8_t bar = (uint8_t)(x / bar_w);
             if (bar > 7u) bar = 7u;
             px[y * fb->width + x] = bars[bar];
         }
     }
 }
 
 /** Grid pattern for geometry test */
 static void draw_grid(uiox_mon_fb_t *fb, uint16_t step, uint32_t colour)
 {
     uint32_t *px = (uint32_t *)fb->vaddr;
     uint32_t bg = 0xFF101010u;
     for (uint16_t y = 0; y < fb->height; y++) {
         for (uint16_t x = 0; x < fb->width; x++) {
             bool on_grid = (x % step == 0) || (y % step == 0);
             px[y * fb->width + x] = on_grid ? colour : bg;
         }
     }
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX Monitor Stack Demo ===\n\n");
 
     /* ------------------------------------------------------------------ */
     /* 1. Build open parameters                                            */
     /* ------------------------------------------------------------------ */
 
     uiox_mon_open_params_t p;
     memset(&p, 0, sizeof(p));
 
     p.hw       = &s_hw;
     p.hw_ops   = &stub_ops;
     p.if_type  = UIOX_MON_IF_HDMI;
     p.pref_w   = 1920u;
     p.pref_h   = 1080u;
     p.pref_hz  = 60u;
 
     /* DSP config */
     p.dsp.scale_mode      = UIOX_MON_SCALE_BILINEAR;
     p.dsp.src_w           = 1920u;
     p.dsp.src_h           = 1080u;
     p.dsp.dst_w           = 1920u;
     p.dsp.dst_h           = 1080u;
     p.dsp.brightness      = 0;
     p.dsp.contrast        = 0;
     p.dsp.saturation      = 0;
     p.dsp.colour_mode     = UIOX_MON_COLOUR_NEUTRAL;
     p.dsp.blue_filter_pct = 0u;
     p.dsp.gamma_x100      = 220u;   /* sRGB gamma 2.2 */
     p.dsp.osd_enabled     = true;
 
     /* Power management */
     p.dpms_timeout_ms = 300000u;  /* 5 minutes */
     p.target_fps      = 60u;
 
     /* Callbacks */
     p.hotplug_cb  = on_hotplug;
     p.vblank_cb   = on_vblank;
 
     /* ------------------------------------------------------------------ */
     /* 2. Open device                                                      */
     /* ------------------------------------------------------------------ */
 
     printf("--- Open ---\n");
     uiox_mon_device_t dev;
     int rc = uiox_mon_open(&dev, &p);
     if (rc < 0) {
         printf("[error] uiox_mon_open failed: %d\n", rc);
         return 1;
     }
 
     /* ------------------------------------------------------------------ */
     /* 3. Print panel info                                                 */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Panel info ---\n");
     uiox_mon_print_info(&dev);
 
     uint16_t res_w = 0, res_h = 0;
     uiox_mon_get_resolution(&dev, &res_w, &res_h);
     printf("  Resolution   : %u × %u\n", res_w, res_h);
     printf("  Connected    : %s\n", uiox_mon_connected(&dev) ? "YES" : "NO");
 
     /* ------------------------------------------------------------------ */
     /* 4. Start display                                                    */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Start ---\n");
     rc = uiox_mon_start(&dev);
     if (rc < 0) {
         printf("[error] uiox_mon_start failed: %d\n", rc);
         uiox_mon_close(&dev);
         return 1;
     }
     printf("  Display: ACTIVE\n");
 
     /* ------------------------------------------------------------------ */
     /* 5. OSD overlay                                                      */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- OSD setup ---\n");
 
     /* Status bar rectangle at top */
     uiox_mon_osd_elem_t status_bar = {
         .type       = UIOX_MON_OSD_RECT,
         .x          = 0,   .y    = 0,
         .w          = 1920, .h   = 40,
         .fg_colour  = 0x80204080u,
         .alpha      = 128u,
         .enabled    = true,
     };
     int bar_idx = uiox_mon_osd_add(&dev, &status_bar);
     printf("  Status bar OSD  idx=%d\n", bar_idx);
 
     /* Text label */
     uiox_mon_osd_elem_t label = {
         .type       = UIOX_MON_OSD_TEXT,
         .x          = 16,  .y  = 12,
         .fg_colour  = 0xFFFFFFFFu,
         .bg_colour  = 0x00000000u,
         .alpha      = 255u,
         .enabled    = true,
     };
     snprintf(label.text, sizeof(label.text), "UIOX Monitor v1.0  1920x1080@60Hz");
     int lbl_idx = uiox_mon_osd_add(&dev, &label);
     printf("  Label OSD  idx=%d  text='%s'\n", lbl_idx, label.text);
 
     /* ------------------------------------------------------------------ */
     /* 6. Render and present test patterns                                 */
     /* ------------------------------------------------------------------ */
 
     typedef struct { const char *name; void (*draw)(uiox_mon_fb_t*); } pattern_t;
 
     /* Colour bars need extra args — wrap in lambdas via local helpers */
     static uiox_mon_fb_t *g_fb;
 
     printf("\n--- Test pattern rendering (%d frames) ---\n", 8);
 
     uint32_t now_ms = 0u;
 
     for (int frame = 0; frame < 8; frame++) {
         now_ms += 17u;  /* ~60 fps tick */
         uiox_mon_tick(&dev, now_ms);
         uiox_mon_activity(&dev, now_ms);
 
         uiox_mon_fb_t *fb = uiox_mon_acquire(&dev);
         if (!fb) {
             printf("  [frame %d] acquire failed — skip\n", frame);
             continue;
         }
 
         /* Cycle through test patterns */
         switch (frame % 4) {
         case 0:
             draw_solid(fb, 0xFF1A1A2Eu);
             printf("  [frame %d] solid fill  0x1A1A2E (dark blue)\n", frame);
             break;
         case 1:
             draw_gradient(fb);
             printf("  [frame %d] horizontal gradient\n", frame);
             break;
         case 2:
             draw_colour_bars(fb);
             printf("  [frame %d] SMPTE colour bars\n", frame);
             break;
         case 3:
             draw_grid(fb, 120u, 0xFF00FF88u);
             printf("  [frame %d] geometry grid (step=120px)\n", frame);
             break;
         }
 
         rc = uiox_mon_present(&dev, fb);
         printf("  [frame %d] present rc=%d  flip#=%u\n",
                frame, rc, s_flip_count);
     }
 
     /* ------------------------------------------------------------------ */
     /* 7. Mid-stream colour adjustments                                    */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Colour adjustments ---\n");
 
     uiox_mon_set_brightness(&dev, 20);
     printf("  Brightness +20\n");
 
     uiox_mon_set_contrast(&dev, 15);
     printf("  Contrast   +15\n");
 
     uiox_mon_set_colour_mode(&dev, UIOX_MON_COLOUR_WARM);
     printf("  Colour mode: WARM (evening)\n");
 
     uiox_mon_set_gamma(&dev, 180u);
     printf("  Gamma: 1.80 (brighter midtones)\n");
 
     /* One more frame with adjusted settings */
     uiox_mon_fb_t *adj_fb = uiox_mon_acquire(&dev);
     if (adj_fb) {
         draw_colour_bars(adj_fb);
         uiox_mon_present(&dev, adj_fb);
         printf("  Colour-adjusted frame presented\n");
     }
 
     /* ------------------------------------------------------------------ */
     /* 8. Backlight control                                                */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Backlight ---\n");
     for (uint8_t lvl = 255u; lvl >= 128u; lvl -= 32u) {
         uiox_mon_set_backlight(&dev, lvl);
         now_ms += 100u;
         uiox_mon_tick(&dev, now_ms);
     }
     uiox_mon_set_backlight(&dev, 200u);  /* restore */
 
     /* ------------------------------------------------------------------ */
     /* 9. OSD removal                                                      */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- OSD removal ---\n");
     uiox_mon_osd_remove(&dev, (uint8_t)lbl_idx);
     printf("  Label OSD removed (idx=%d)\n", lbl_idx);
 
     /* ------------------------------------------------------------------ */
     /* 10. DPMS test                                                       */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- DPMS test ---\n");
     uiox_mon_set_dpms(&dev, UIOX_MON_DPMS_STANDBY);
     printf("  DPMS → STANDBY\n");
     now_ms += 500u;
     uiox_mon_tick(&dev, now_ms);
 
     uiox_mon_activity(&dev, now_ms);  /* simulate user activity = wake up */
     printf("  User activity → DPMS wake\n");
 
     /* ------------------------------------------------------------------ */
     /* 11. Statistics                                                      */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Statistics ---\n");
     uiox_mon_print_stats(&dev);
 
     uiox_mon_if_stats_t if_stats;
     uiox_mon_if_stats_get(&dev.subsys.mif, &if_stats);
     printf("  IF frames      : %llu\n", (unsigned long long)if_stats.frame_count);
     printf("  IF VBlanks     : %llu\n", (unsigned long long)if_stats.vblank_count);
     printf("  IF underruns   : %llu\n", (unsigned long long)if_stats.underrun_count);
     printf("  IF bytes TX    : %llu\n", (unsigned long long)if_stats.bytes_transferred);
     printf("  Buf free       : %u / %u\n",
            uiox_mon_buf_free_count(), UIOX_MON_BUF_POOL_SIZE);
 
     /* ------------------------------------------------------------------ */
     /* 12. Stop and close                                                  */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Stop and close ---\n");
     uiox_mon_stop(&dev);
     printf("  Display : STOPPED\n");
     uiox_mon_close(&dev);
     printf("  Device  : CLOSED\n");
 
     printf("\n=== UIOX Monitor Demo complete ===\n");
     return 0;
 } 
 