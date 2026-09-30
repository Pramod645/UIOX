/**
 * @file    uiox_mon_dsp.c
 * @brief   UIOX Monitor DSP implementation.
 * @date    2026-05-27
 */

 #include "uiox_mon_dsp.h"
 #include <string.h>
 #include <math.h>
 #include <errno.h>
 
 #ifndef M_PI
 #define M_PI 3.14159265358979323846
 #endif
 
 /* =========================================================================
  * Gamma LUT build  (power-law: out = (in/255)^(gamma) × 255)
  * ====================================================================== */
 
 void uiox_mon_dsp_build_gamma(uiox_mon_dsp_t *dsp)
 {
     if (!dsp) return;
     float g = (float)dsp->cfg.gamma_x100 / 100.0f;
     if (g < 0.1f) g = 0.1f;
     if (g > 5.0f) g = 5.0f;
 
     for (int i = 0; i < 256; i++) {
         float normalised = (float)i / 255.0f;
         float corrected  = powf(normalised, g);
         uint8_t out      = (uint8_t)(corrected * 255.0f + 0.5f);
         dsp->gamma_r[i]  = out;
         dsp->gamma_g[i]  = out;
         dsp->gamma_b[i]  = out;
     }
 
     /* Apply blue filter to blue channel */
     if (dsp->cfg.blue_filter_pct > 0u) {
         float reduce = 1.0f - (float)dsp->cfg.blue_filter_pct / 100.0f;
         for (int i = 0; i < 256; i++) {
             float v = (float)dsp->gamma_b[i] * reduce;
             dsp->gamma_b[i] = (uint8_t)(v + 0.5f);
         }
     }
 
     /* Colour temperature preset */
     switch (dsp->cfg.colour_mode) {
     case UIOX_MON_COLOUR_WARM:
         /* Warm: reduce blue by 20%, boost red by 10% */
         for (int i = 0; i < 256; i++) {
             uint16_t r = (uint16_t)(dsp->gamma_r[i] * 110u / 100u);
             dsp->gamma_r[i] = r > 255u ? 255u : (uint8_t)r;
             dsp->gamma_b[i] = (uint8_t)(dsp->gamma_b[i] * 80u / 100u);
         }
         break;
     case UIOX_MON_COLOUR_COOL:
         /* Cool: boost blue by 15%, reduce red by 10% */
         for (int i = 0; i < 256; i++) {
             uint16_t b = (uint16_t)(dsp->gamma_b[i] * 115u / 100u);
             dsp->gamma_b[i] = b > 255u ? 255u : (uint8_t)b;
             dsp->gamma_r[i] = (uint8_t)(dsp->gamma_r[i] * 90u / 100u);
         }
         break;
     default:
         break;
     }
 }
 
 /* =========================================================================
  * Init / deinit
  * ====================================================================== */
 
 int uiox_mon_dsp_init(uiox_mon_dsp_t *dsp, const uiox_mon_dsp_cfg_t *cfg)
 {
     if (!dsp || !cfg) return -EINVAL;
     memset(dsp, 0, sizeof(*dsp));
     memcpy(&dsp->cfg, cfg, sizeof(*cfg));
     uiox_mon_dsp_build_gamma(dsp);
     return 0;
 }
 
 void uiox_mon_dsp_deinit(uiox_mon_dsp_t *dsp)
 {
     if (dsp) memset(dsp, 0, sizeof(*dsp));
 }
 
 /* =========================================================================
  * Per-pixel colour adjustment  (brightness / contrast / saturation)
  * All operate on XRGB8888
  * ====================================================================== */
 
 static inline uint8_t clamp_u8(int32_t v)
 {
     return (v < 0) ? 0u : (v > 255) ? 255u : (uint8_t)v;
 }
 
 static void pixel_adjust(uint8_t *r, uint8_t *g, uint8_t *b,
                           int8_t brightness, int8_t contrast,
                           int8_t saturation)
 {
     /* Brightness: simple offset */
     int32_t ri = (int32_t)*r + brightness;
     int32_t gi = (int32_t)*g + brightness;
     int32_t bi = (int32_t)*b + brightness;
 
     /* Contrast: scale around 128 */
     float cf = 1.0f + (float)contrast / 100.0f;
     ri = (int32_t)((float)(ri - 128) * cf + 128.0f);
     gi = (int32_t)((float)(gi - 128) * cf + 128.0f);
     bi = (int32_t)((float)(bi - 128) * cf + 128.0f);
 
     /* Saturation: mix towards luminance */
     float lum = 0.299f * (float)ri +
                 0.587f * (float)gi +
                 0.114f * (float)bi;
     float sf = 1.0f + (float)saturation / 100.0f;
     ri = (int32_t)(lum + sf * ((float)ri - lum));
     gi = (int32_t)(lum + sf * ((float)gi - lum));
     bi = (int32_t)(lum + sf * ((float)bi - lum));
 
     *r = clamp_u8(ri);
     *g = clamp_u8(gi);
     *b = clamp_u8(bi);
 }
 
 /* =========================================================================
  * Full DSP process pass (in-place on XRGB8888 framebuffer)
  * ====================================================================== */
 
 int uiox_mon_dsp_process(uiox_mon_dsp_t *dsp, uiox_mon_fb_t *fb)
 {
     if (!dsp || !fb || !fb->vaddr) return -EINVAL;
     if (fb->fmt != UIOX_MON_FMT_XRGB8888 &&
         fb->fmt != UIOX_MON_FMT_ARGB8888) return -ENOTSUP;
 
     uint32_t *px = (uint32_t *)fb->vaddr;
     uint32_t  n  = (uint32_t)fb->width * fb->height;
 
     bool do_gamma   = (dsp->cfg.gamma_x100 != 100u);
     bool do_colour  = (dsp->cfg.brightness || dsp->cfg.contrast ||
                        dsp->cfg.saturation);
     bool do_blue    = (dsp->cfg.blue_filter_pct > 0u);
     bool do_colour_t = (dsp->cfg.colour_mode != UIOX_MON_COLOUR_NEUTRAL);
 
     if (!do_gamma && !do_colour && !do_blue && !do_colour_t) {
         /* Nothing to do — skip to OSD */
         if (dsp->cfg.osd_enabled)
             uiox_mon_dsp_osd_render(dsp, fb);
         return 0;
     }
 
     for (uint32_t i = 0; i < n; i++) {
         uint32_t p = px[i];
         uint8_t  a = (uint8_t)(p >> 24u);
         uint8_t  r = (uint8_t)(p >> 16u);
         uint8_t  g = (uint8_t)(p >>  8u);
         uint8_t  b = (uint8_t)(p >>  0u);
 
         /* Gamma */
         if (do_gamma || do_blue || do_colour_t) {
             r = dsp->gamma_r[r];
             g = dsp->gamma_g[g];
             b = dsp->gamma_b[b];
         }
 
         /* Brightness / contrast / saturation */
         if (do_colour)
             pixel_adjust(&r, &g, &b,
                          dsp->cfg.brightness,
                          dsp->cfg.contrast,
                          dsp->cfg.saturation);
 
         px[i] = ((uint32_t)a << 24u) |
                 ((uint32_t)r << 16u) |
                 ((uint32_t)g <<  8u) |
                  (uint32_t)b;
     }
 
     /* OSD composite */
     if (dsp->cfg.osd_enabled)
         uiox_mon_dsp_osd_render(dsp, fb);
 
     return 0;
 }
 
 /* =========================================================================
  * Nearest-neighbour and bilinear scaling
  * ====================================================================== */
 
 int uiox_mon_dsp_scale(uiox_mon_dsp_t      *dsp,
                         const uiox_mon_fb_t *src,
                         uiox_mon_fb_t       *dst)
 {
     if (!dsp || !src || !dst) return -EINVAL;
     if (!src->vaddr || !dst->vaddr) return -EINVAL;
     if (src->fmt != UIOX_MON_FMT_XRGB8888 &&
         src->fmt != UIOX_MON_FMT_ARGB8888) return -ENOTSUP;
 
     const uint32_t *sp = (const uint32_t *)src->vaddr;
     uint32_t       *dp = (uint32_t *)dst->vaddr;
     uint16_t sw = src->width,  sh = src->height;
     uint16_t dw = dst->width,  dh = dst->height;
 
     if (dsp->cfg.scale_mode == UIOX_MON_SCALE_NEAREST) {
         for (uint16_t dy = 0; dy < dh; dy++) {
             uint16_t sy = (uint16_t)((uint32_t)dy * sh / dh);
             for (uint16_t dx = 0; dx < dw; dx++) {
                 uint16_t sx = (uint16_t)((uint32_t)dx * sw / dw);
                 dp[dy * dw + dx] = sp[sy * sw + sx];
             }
         }
     } else {
         /* Bilinear */
         for (uint16_t dy = 0; dy < dh; dy++) {
             float fy  = (float)dy * (float)(sh - 1u) / (float)(dh - 1u);
             uint16_t y0 = (uint16_t)fy;
             uint16_t y1 = (y0 + 1u < sh) ? y0 + 1u : y0;
             float    wy = fy - (float)y0;
 
             for (uint16_t dx = 0; dx < dw; dx++) {
                 float    fx  = (float)dx * (float)(sw - 1u) / (float)(dw - 1u);
                 uint16_t x0  = (uint16_t)fx;
                 uint16_t x1  = (x0 + 1u < sw) ? x0 + 1u : x0;
                 float    wx  = fx - (float)x0;
 
                 uint32_t p00 = sp[y0 * sw + x0];
                 uint32_t p10 = sp[y0 * sw + x1];
                 uint32_t p01 = sp[y1 * sw + x0];
                 uint32_t p11 = sp[y1 * sw + x1];
 
                 /* Interpolate each channel */
                 for (int ch = 0; ch < 3; ch++) {
                     uint8_t shift = (uint8_t)(ch * 8u);
                     float   c00 = (float)((p00 >> shift) & 0xFFu);
                     float   c10 = (float)((p10 >> shift) & 0xFFu);
                     float   c01 = (float)((p01 >> shift) & 0xFFu);
                     float   c11 = (float)((p11 >> shift) & 0xFFu);
                     float   out = c00 * (1.f-wx) * (1.f-wy)
                                 + c10 * wx       * (1.f-wy)
                                 + c01 * (1.f-wx) * wy
                                 + c11 * wx       * wy;
                     uint32_t ov = (uint32_t)(out + 0.5f) & 0xFFu;
                     dp[dy*dw+dx] = (dp[dy*dw+dx] & ~(0xFFu << shift))
                                  | (ov << shift);
                 }
             }
         }
     }
     return 0;
 }
 
 /* =========================================================================
  * OSD compositor
  * ====================================================================== */
 
 int uiox_mon_dsp_osd_add(uiox_mon_dsp_t          *dsp,
                           const uiox_mon_osd_elem_t *elem)
 {
     if (!dsp || !elem) return -EINVAL;
     if (dsp->osd_count >= UIOX_MON_OSD_MAX_ELEMENTS) return -ENOSPC;
     memcpy(&dsp->osd[dsp->osd_count], elem, sizeof(*elem));
     dsp->osd[dsp->osd_count].enabled = true;
     return (int)dsp->osd_count++;
 }
 
 void uiox_mon_dsp_osd_remove(uiox_mon_dsp_t *dsp, uint8_t idx)
 {
     if (!dsp || idx >= dsp->osd_count) return;
     dsp->osd[idx].enabled = false;
 }
 
 void uiox_mon_dsp_osd_render(uiox_mon_dsp_t *dsp, uiox_mon_fb_t *fb)
 {
     if (!dsp || !fb || !fb->vaddr) return;
     uint32_t *px = (uint32_t *)fb->vaddr;
 
     for (uint8_t ei = 0; ei < dsp->osd_count; ei++) {
         const uiox_mon_osd_elem_t *e = &dsp->osd[ei];
         if (!e->enabled) continue;
 
         if (e->type == UIOX_MON_OSD_RECT) {
             /* Fill rectangle with alpha blend */
             float alpha = (float)e->alpha / 255.0f;
             uint8_t fr = (uint8_t)(e->fg_colour >> 16u);
             uint8_t fg = (uint8_t)(e->fg_colour >>  8u);
             uint8_t fb_ = (uint8_t)(e->fg_colour);
 
             for (uint16_t y = e->y;
                  y < e->y + e->h && y < fb->height; y++) {
                 for (uint16_t x = e->x;
                      x < e->x + e->w && x < fb->width; x++) {
                     uint32_t  dst = px[y * fb->width + x];
                     uint8_t   dr  = (uint8_t)(dst >> 16u);
                     uint8_t   dg  = (uint8_t)(dst >>  8u);
                     uint8_t   db  = (uint8_t)(dst);
                     uint8_t   or_ = clamp_u8((int32_t)(fr * alpha +
                                               dr * (1.0f - alpha)));
                     uint8_t   og  = clamp_u8((int32_t)(fg * alpha +
                                               dg * (1.0f - alpha)));
                     uint8_t   ob  = clamp_u8((int32_t)(fb_ * alpha +
                                               db * (1.0f - alpha)));
                     px[y * fb->width + x] =
                         0xFF000000u |
                         ((uint32_t)or_ << 16u) |
                         ((uint32_t)og  <<  8u) |
                          (uint32_t)ob;
                 }
             }
         } else if (e->type == UIOX_MON_OSD_TEXT) {
             /* Text rendering stub — in production use a bitmap font */
             /* For now, draw a thin indicator bar at text position   */
             for (uint16_t x = e->x;
                  x < e->x + (uint16_t)(strlen(e->text) * 8u) &&
                  x < fb->width; x++) {
                 if (e->y < fb->height)
                     px[e->y * fb->width + x] = e->fg_colour;
             }
         }
     }
 }
 