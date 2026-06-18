/**
 * @file    uiox_hdmi_device.h
 * @brief   UIOX HDMI top-level application-facing device API.
 * @date    2026-05-28
 */
//Layer 5 — Device API
 #ifndef UIOX_HDMI_DEVICE_H
 #define UIOX_HDMI_DEVICE_H
 
 #include "uiox_hdmi_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_hdmi_hw_t             *hw;
     const uiox_hdmi_hw_ops_t   *hw_ops;
     uiox_hdmi_colorspace_t      cs;
     uiox_hdmi_bpc_t             bpc;
     uint16_t                    pref_w, pref_h;
     uint8_t                     pref_hz;
     uint32_t                    dpms_timeout_ms;
     uiox_hdmi_evt_cb_t          evt_cb;
     void                       *evt_ctx;
 } uiox_hdmi_open_params_t;
 
 typedef struct {
     uiox_hdmi_subsys_t  subsys;
     uiox_hdmi_hw_t     *hw;
     bool                open;
 } uiox_hdmi_device_t;
 
 int  uiox_hdmi_open          (uiox_hdmi_device_t            *dev,
                                const uiox_hdmi_open_params_t *p);
 int  uiox_hdmi_start         (uiox_hdmi_device_t *dev);
 void uiox_hdmi_stop          (uiox_hdmi_device_t *dev);
 void uiox_hdmi_close         (uiox_hdmi_device_t *dev);
 void uiox_hdmi_tick          (uiox_hdmi_device_t *dev, uint32_t now_ms);
 void uiox_hdmi_activity      (uiox_hdmi_device_t *dev, uint32_t now_ms);
 
 uiox_hdmi_fb_t *uiox_hdmi_acquire(uiox_hdmi_device_t *dev);
 int             uiox_hdmi_present (uiox_hdmi_device_t *dev,
                                    uiox_hdmi_fb_t     *fb);
 
 int  uiox_hdmi_set_hdr       (uiox_hdmi_device_t       *dev,
                                const uiox_hdmi_hdr_t    *hdr);
 int  uiox_hdmi_set_audio     (uiox_hdmi_device_t          *dev,
                                const uiox_hdmi_audio_cfg_t *a);
 int  uiox_hdmi_audio_write   (uiox_hdmi_device_t *dev,
                                const uint8_t *samples, uint32_t bytes);
 int  uiox_hdmi_cec_send      (uiox_hdmi_device_t *dev,
                                uint8_t dst_la, uint8_t opcode,
                                const uint8_t *params, uint8_t plen);
 
 bool uiox_hdmi_connected     (const uiox_hdmi_device_t *dev);
 void uiox_hdmi_get_resolution(const uiox_hdmi_device_t *dev,
                                uint16_t *w, uint16_t *h);
 void uiox_hdmi_print_info    (const uiox_hdmi_device_t *dev);
 void uiox_hdmi_print_stats   (const uiox_hdmi_device_t *dev);
 const char *uiox_hdmi_evt_name(uiox_hdmi_evt_t evt);
 const char *uiox_hdmi_state_name(uiox_hdmi_subsys_state_t state);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_HDMI_DEVICE_H */
 