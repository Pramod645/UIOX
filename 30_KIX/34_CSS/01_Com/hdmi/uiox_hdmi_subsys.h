/**
 * @file    uiox_hdmi_subsys.h
 * @brief   UIOX HDMI subsystem — hotplug, pipeline, HDCP, DPMS.
 *
 * Top subsystem layer. Manages:
 *   - Hotplug detection and automatic EDID re-probe
 *   - Full pipeline: acquire FB → render → DSP → flip → vsync
 *   - HDCP authentication lifecycle
 *   - DPMS / power state management
 *   - Infoframe periodic re-transmission
 *   - Event callbacks (connect, disconnect, HDCP, VBlank)
 *   - Frame statistics
 *
 * @date    2026-05-28
 */
//Layer 4 — Subsystem
 #ifndef UIOX_HDMI_SUBSYS_H
 #define UIOX_HDMI_SUBSYS_H
 
 #include "uiox_hdmi_proto.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * HDMI events
  * ====================================================================== */
 
 typedef enum {
     UIOX_HDMI_EVT_CONNECT = 0,
     UIOX_HDMI_EVT_DISCONNECT,
     UIOX_HDMI_EVT_HDCP_AUTH,
     UIOX_HDMI_EVT_HDCP_FAIL,
     UIOX_HDMI_EVT_VBLANK,
     UIOX_HDMI_EVT_AUDIO_UNDERRUN,
     UIOX_HDMI_EVT_ERROR,
 } uiox_hdmi_evt_t;
 
 typedef void (*uiox_hdmi_evt_cb_t)(uiox_hdmi_evt_t evt, void *ctx);
 
 /* =========================================================================
  * Subsystem state
  * ====================================================================== */
 
 typedef enum {
     UIOX_HDMI_SUBSYS_STOPPED = 0,
     UIOX_HDMI_SUBSYS_PROBING,
     UIOX_HDMI_SUBSYS_RUNNING,
     UIOX_HDMI_SUBSYS_SUSPENDED,
 } uiox_hdmi_subsys_state_t;
 
 /* =========================================================================
  * Subsystem descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_hdmi_if_t            hif;
     uiox_hdmi_sink_t          sink;
     uiox_hdmi_proto_t         proto;
     uiox_hdmi_subsys_state_t  state;
 
     /* Triple-buffer */
     uiox_hdmi_fb_t           *fb_display;
     uiox_hdmi_fb_t           *fb_render;
     uiox_hdmi_fb_t           *fb_pending;
     uint32_t                  frame_id;
 
     /* Events */
     uiox_hdmi_evt_cb_t        evt_cb;
     void                     *evt_ctx;
     bool                      last_connected;
 
     /* DPMS */
     uint32_t                  last_activity_ms;
     uint32_t                  dpms_timeout_ms;
 
     /* Statistics */
     uint64_t                  total_frames;
     uint64_t                  dropped_frames;
     uint64_t                  total_vblanks;
 } uiox_hdmi_subsys_t;
 
 /* =========================================================================
  * Subsystem API
  * ====================================================================== */
 
 int  uiox_hdmi_subsys_init    (uiox_hdmi_subsys_t         *sys,
                                 uiox_hdmi_hw_t             *hw,
                                 uiox_hdmi_colorspace_t      cs,
                                 uiox_hdmi_bpc_t             bpc,
                                 uint16_t pref_w, uint16_t pref_h,
                                 uint8_t  pref_hz);
 
 int  uiox_hdmi_subsys_start   (uiox_hdmi_subsys_t *sys);
 void uiox_hdmi_subsys_stop    (uiox_hdmi_subsys_t *sys);
 
 uiox_hdmi_fb_t *uiox_hdmi_subsys_acquire (uiox_hdmi_subsys_t *sys);
 int             uiox_hdmi_subsys_present  (uiox_hdmi_subsys_t *sys,
                                            uiox_hdmi_fb_t     *fb);
 void            uiox_hdmi_subsys_tick     (uiox_hdmi_subsys_t *sys,
                                            uint32_t now_ms);
 
 void uiox_hdmi_subsys_set_evt_cb (uiox_hdmi_subsys_t *sys,
                                    uiox_hdmi_evt_cb_t  cb, void *ctx);
 void uiox_hdmi_subsys_activity   (uiox_hdmi_subsys_t *sys, uint32_t now_ms);
 void uiox_hdmi_subsys_set_dpms   (uiox_hdmi_subsys_t *sys,
                                    uint32_t timeout_ms);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_HDMI_SUBSYS_H */
 