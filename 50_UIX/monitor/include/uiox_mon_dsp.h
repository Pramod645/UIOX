/**
 * @file    uiox_mon_dsp.h
 * @brief   UIOX Monitor DSP layer — scaling, colour, gamma, OSD.
 *
 * Software display signal processing applied to framebuffers before
 * or after they reach the display controller:
 *
 *   - Bilinear / nearest-neighbour scaling
 *   - Colour temperature adjustment (warm/cool/neutral)
 *   - Brightness / contrast / saturation
 *   - Software gamma correction (LUT-based)
 *   - On-Screen Display (OSD) text/rectangle compositor
 *   - Blue-light filter (reduce short-wave blue component)
 *
 * @date    2026-05-27
 */
//Layer 3 — Display Signal Processing
 #ifndef UIOX_MON_DSP_H
 #define UIOX_MON_DSP_H
 
 #include "uiox_mon_buf.h"
 #include <stdint.h>
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * DSP configuration
  * ====================================================================== */
 
 typedef enum {
     UIOX_MON_SCALE_NEAREST = 0,
     UIOX_MON_SCALE_BILINEAR,
 } uiox_mon_scale_mode_t;
 
 typedef enum {
     UIOX_MON_COLOUR_NEUTRAL = 0,
     UIOX_MON_COLOUR_WARM,          /**< Reduce blue (evening mode)        */
     UIOX_MON_COLOUR_COOL,          /**< Boost blue (daylight mode)        */
     UIOX_MON_COLOUR_CUSTOM,
 } uiox_mon_colour_mode_t;
 
 typedef struct {
     /* Scaling */
     uiox_mon_scale_mode_t  scale_mode;
     uint16_t               src_w, src_h;  /**< Source resolution           */
     uint16_t               dst_w, dst_h;  /**< Target resolution           */
 
     /* Colour adjustment (-100..+100, 0=neutral) */
     int8_t    brightness;    /**< −100 = black, +100 = max bright          */
     int8_t    contrast;      /**< −100 = flat,  +100 = high contrast       */
     int8_t    saturation;    /**< −100 = grey,  +100 = vivid               */
 
     /* Colour temperature */
     uiox_mon_colour_mode_t colour_mode;
     uint8_t   blue_filter_pct;  /**< Blue channel reduce % (0=off, 50=max)*/
 
     /* Gamma (100 = 1.0, 220 = 2.2) */
     uint16_t  gamma_x100;
 
     /* OSD */
     bool      osd_enabled;
 } uiox_mon_dsp_cfg_t;
 
 /* =========================================================================
  * OSD element
  * ====================================================================== */
 
 #define UIOX_MON_OSD_MAX_ELEMENTS   16
 #define UIOX_MON_OSD_TEXT_MAX       64
 
 typedef enum {
     UIOX_MON_OSD_RECT = 0,
     UIOX_MON_OSD_TEXT,
 } uiox_mon_osd_type_t;
 
 typedef struct {
     uiox_mon_osd_type_t type;
     uint16_t  x, y, w, h;       /**< Position and size (pixels)          */
     uint32_t  fg_colour;         /**< Foreground XRGB8888                 */
     uint32_t  bg_colour;         /**< Background XRGB8888                 */
     uint8_t   alpha;             /**< Opacity 0=transparent, 255=opaque   */
     char      text[UIOX_MON_OSD_TEXT_MAX];
     bool      enabled;
 } uiox_mon_osd_elem_t;
 
 /* =========================================================================
  * DSP context
  * ====================================================================== */
 
 typedef struct {
     uiox_mon_dsp_cfg_t    cfg;
     uint8_t               gamma_r[256];  /**< Red gamma LUT                */
     uint8_t               gamma_g[256];  /**< Green gamma LUT              */
     uint8_t               gamma_b[256];  /**< Blue gamma LUT               */
     uiox_mon_osd_elem_t   osd[UIOX_MON_OSD_MAX_ELEMENTS];
     uint8_t               osd_count;
 } uiox_mon_dsp_t;
 
 /* =========================================================================
  * DSP API
  * ====================================================================== */
 
 int  uiox_mon_dsp_init     (uiox_mon_dsp_t *dsp,
                              const uiox_mon_dsp_cfg_t *cfg);
 void uiox_mon_dsp_deinit   (uiox_mon_dsp_t *dsp);
 
 /** Rebuild gamma LUT from current cfg.gamma_x100. */
 void uiox_mon_dsp_build_gamma(uiox_mon_dsp_t *dsp);
 
 /**
  * @brief  Apply full DSP pipeline to a framebuffer in-place.
  *
  * Chain: gamma → colour adjust → blue filter → OSD composite
  *
  * @param  dsp  Initialised DSP context.
  * @param  fb   Framebuffer to process (XRGB8888 or RGB888).
  */
 int  uiox_mon_dsp_process  (uiox_mon_dsp_t *dsp, uiox_mon_fb_t *fb);
 
 /** Scale src into dst using configured scale mode. */
 int  uiox_mon_dsp_scale    (uiox_mon_dsp_t *dsp,
                              const uiox_mon_fb_t *src,
                              uiox_mon_fb_t       *dst);
 
 /** Add an OSD element. Returns element index or negative errno. */
 int  uiox_mon_dsp_osd_add  (uiox_mon_dsp_t          *dsp,
                              const uiox_mon_osd_elem_t *elem);
 
 /** Remove an OSD element by index. */
 void uiox_mon_dsp_osd_remove(uiox_mon_dsp_t *dsp, uint8_t idx);
 
 /** Composite all enabled OSD elements onto fb. */
 void uiox_mon_dsp_osd_render(uiox_mon_dsp_t *dsp, uiox_mon_fb_t *fb);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MON_DSP_H */
 