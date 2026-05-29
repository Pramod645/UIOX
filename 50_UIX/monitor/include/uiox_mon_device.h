/**
 * @file    uiox_mon_device.h
 * @brief   UIOX Monitor top-level application-facing device API.
 * @date    2026-05-27
 */
//Layer 5 — Device API
 #ifndef UIOX_MON_DEVICE_H
 #define UIOX_MON_DEVICE_H
 
 #include "uiox_mon_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_mon_hw_t             *hw;
     const uiox_mon_hw_ops_t   *hw_ops;
     uiox_mon_if_type_t         if_type;
     uiox_mon_dsp_cfg_t         dsp;
     uint16_t                   pref_w, pref_h;
     uint8_t                    pref_hz;
     uint32_t                   dpms_timeout_ms;
     uint32_t                   target_fps;
     uiox_mon_hotplug_cb_t      hotplug_cb;
     void                      *hotplug_ctx;
     uiox_mon_vblank_cb_t       vblank_cb;
     void                      *vblank_ctx;
 } uiox_mon_open_params_t;
 
 typedef struct {
     uiox_mon_subsys_t  subsys;
     uiox_mon_hw_t     *hw;
     bool               open;
 } uiox_mon_device_t;
 
 /* =========================================================================
  * Application API
  * ====================================================================== */
 
 int  uiox_mon_open          (uiox_mon_device_t           *dev,
                               const uiox_mon_open_params_t *p);
 int  uiox_mon_start         (uiox_mon_device_t *dev);
 void uiox_mon_stop          (uiox_mon_device_t *dev);
 void uiox_mon_close         (uiox_mon_device_t *dev);
 
 /** Acquire a framebuffer to render into. */
 uiox_mon_fb_t *uiox_mon_acquire(uiox_mon_device_t *dev);
 
 /** Submit rendered framebuffer (DSP + page flip + vsync). */
 int  uiox_mon_present       (uiox_mon_device_t *dev, uiox_mon_fb_t *fb);
 
 /** Periodic tick — hotplug, DPMS, frame pacing. */
 void uiox_mon_tick          (uiox_mon_device_t *dev, uint32_t now_ms);
 
 /** Signal user activity to reset DPMS idle timer. */
 void uiox_mon_activity      (uiox_mon_device_t *dev, uint32_t now_ms);
 
 int  uiox_mon_set_backlight (uiox_mon_device_t *dev, uint8_t level);
 int  uiox_mon_set_dpms      (uiox_mon_device_t *dev, uiox_mon_dpms_t dpms);
 int  uiox_mon_set_gamma     (uiox_mon_device_t *dev, uint16_t gamma_x100);
 int  uiox_mon_set_brightness(uiox_mon_device_t *dev, int8_t brightness);
 int  uiox_mon_set_contrast  (uiox_mon_device_t *dev, int8_t contrast);
 int  uiox_mon_set_colour_mode(uiox_mon_device_t    *dev,
                                uiox_mon_colour_mode_t mode);
 int  uiox_mon_osd_add       (uiox_mon_device_t         *dev,
                               const uiox_mon_osd_elem_t *elem);
 void uiox_mon_osd_remove    (uiox_mon_device_t *dev, uint8_t idx);
 
 bool uiox_mon_connected     (uiox_mon_device_t *dev);
 void uiox_mon_get_resolution(const uiox_mon_device_t *dev,
                               uint16_t *w, uint16_t *h);
 void uiox_mon_print_info    (const uiox_mon_device_t *dev);
 void uiox_mon_print_stats   (const uiox_mon_device_t *dev);
 
 const char *uiox_mon_dpms_name(uiox_mon_dpms_t dpms);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MON_DEVICE_H */
 