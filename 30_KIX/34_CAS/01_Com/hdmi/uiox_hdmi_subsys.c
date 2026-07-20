/**
 * @file    uiox_hdmi_subsys.c
 * @brief   UIOX HDMI subsystem implementation.
 * @date    2026-05-28
 */

 #include "uiox_hdmi_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 static void fire(uiox_hdmi_subsys_t *sys, uiox_hdmi_evt_t evt)
 {
     if (sys->evt_cb) sys->evt_cb(evt, sys->evt_ctx);
 }
 
 int uiox_hdmi_subsys_init(uiox_hdmi_subsys_t     *sys,
                            uiox_hdmi_hw_t         *hw,
                            uiox_hdmi_colorspace_t  cs,
                            uiox_hdmi_bpc_t         bpc,
                            uint16_t pref_w, uint16_t pref_h,
                            uint8_t  pref_hz)
 {
     if (!sys || !hw) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
 
     /* Probe sink / EDID */
     int rc = uiox_hdmi_sink_probe(&sys->sink, hw);
     if (rc < 0) return rc;
 
     rc = uiox_hdmi_sink_select_mode(&sys->sink, pref_w, pref_h, pref_hz);
     if (rc < 0) return rc;
 
     /* Configure interface */
     const uiox_hdmi_timing_t *t = uiox_hdmi_sink_timing(&sys->sink);
     rc = uiox_hdmi_if_config(&sys->hif, hw, t, cs, bpc);
     if (rc < 0) return rc;
 
     /* Init protocol */
     rc = uiox_hdmi_proto_init(&sys->proto, &sys->hif, &sys->sink);
     if (rc < 0) return rc;
 
     /* Default AVI: BT.709, 16:9 */
     sys->proto.avi.colorimetry    = 2u;
     sys->proto.avi.picture_aspect = 2u;
     sys->proto.avi.active_aspect  = 8u;
     sys->proto.avi.rgb_quant      = (cs == UIOX_HDMI_CS_RGB) ? 2u : 0u;
     sys->proto.avi.ycc_quant      = (cs != UIOX_HDMI_CS_RGB) ? 1u : 0u;
 
     sys->state              = UIOX_HDMI_SUBSYS_STOPPED;
     sys->last_connected     = false;
     sys->dpms_timeout_ms    = 0u;
     return 0;
 }
 
 int uiox_hdmi_subsys_start(uiox_hdmi_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     int rc = uiox_hdmi_if_enable(&sys->hif);
     if (rc < 0) return rc;
 
     /* Set AVMUTE during startup, then clear */
     uiox_hdmi_proto_send_gcp(&sys->proto, true);
     uiox_hdmi_hw_wait_vblank(sys->hif.hw, 20u);
 
     /* Send initial infoframes */
     uiox_hdmi_proto_send_spd(&sys->proto, "UIOX", "HDMI Device");
     uiox_hdmi_proto_send_avi(&sys->proto);
     uiox_hdmi_proto_send_audio_if(&sys->proto);
     uiox_hdmi_proto_send_gcp(&sys->proto, false);
 
     /* HDCP if sink supports it */
     if (sys->sink.edid.hdcp14 || sys->sink.edid.hdcp23)
         uiox_hdmi_sink_hdcp_start(&sys->sink, sys->hif.hw);
 
     /* Allocate triple buffers */
     sys->fb_render  = uiox_hdmi_buf_alloc_fb();
     sys->fb_display = NULL;
     sys->fb_pending = NULL;
 
     sys->state = UIOX_HDMI_SUBSYS_RUNNING;
     fire(sys, UIOX_HDMI_EVT_CONNECT);
     return 0;
 }
 
 void uiox_hdmi_subsys_stop(uiox_hdmi_subsys_t *sys)
 {
     if (!sys) return;
     uiox_hdmi_proto_send_gcp(&sys->proto, true);  /* mute before stop */
     uiox_hdmi_sink_hdcp_stop(&sys->sink, sys->hif.hw);
     if (sys->fb_render)  { uiox_hdmi_buf_free_fb(sys->fb_render);  sys->fb_render  = NULL; }
     if (sys->fb_pending) { uiox_hdmi_buf_free_fb(sys->fb_pending); sys->fb_pending = NULL; }
     if (sys->fb_display) { uiox_hdmi_buf_free_fb(sys->fb_display); sys->fb_display = NULL; }
     uiox_hdmi_if_disable(&sys->hif);
     sys->state = UIOX_HDMI_SUBSYS_STOPPED;
 }
 
 uiox_hdmi_fb_t *uiox_hdmi_subsys_acquire(uiox_hdmi_subsys_t *sys)
 {
     if (!sys || sys->state != UIOX_HDMI_SUBSYS_RUNNING) return NULL;
     if (sys->fb_render) return sys->fb_render;
     sys->fb_render = uiox_hdmi_buf_alloc_fb();
     return sys->fb_render;
 }
 
 int uiox_hdmi_subsys_present(uiox_hdmi_subsys_t *sys, uiox_hdmi_fb_t *fb)
 {
     if (!sys || !fb) return -EINVAL;
     if (sys->state != UIOX_HDMI_SUBSYS_RUNNING) return -ENETDOWN;
 
     fb->frame_id = ++sys->frame_id;
 
     if (sys->fb_pending) {
         sys->dropped_frames++;
         uiox_hdmi_buf_free_fb(sys->fb_pending);
     }
     sys->fb_pending = fb;
     sys->fb_render  = NULL;
 
     int rc = uiox_hdmi_if_flip(&sys->hif, fb);
     if (rc < 0) return rc;
 
     rc = uiox_hdmi_if_vsync(&sys->hif, 50u);
     if (rc == 0) {
         if (sys->fb_display) uiox_hdmi_buf_free_fb(sys->fb_display);
         sys->fb_display        = sys->fb_pending;
         sys->fb_display->state = UIOX_HDMI_FB_DISPLAYED;
         sys->fb_pending        = NULL;
         sys->total_frames++;
         sys->total_vblanks++;
         fire(sys, UIOX_HDMI_EVT_VBLANK);
     }
     sys->fb_render = uiox_hdmi_buf_alloc_fb();
     return rc;
 }
 
 void uiox_hdmi_subsys_tick(uiox_hdmi_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys) return;
 
     /* Hotplug */
     bool connected = uiox_hdmi_hw_connected(sys->hif.hw);
     if (connected != sys->last_connected) {
         sys->last_connected = connected;
         if (connected) {
             uiox_hdmi_sink_probe(&sys->sink, sys->hif.hw);
             fire(sys, UIOX_HDMI_EVT_CONNECT);
         } else {
             uiox_hdmi_subsys_stop(sys);
             fire(sys, UIOX_HDMI_EVT_DISCONNECT);
         }
     }
 
     if (sys->state != UIOX_HDMI_SUBSYS_RUNNING) return;
 
     /* HDCP status poll */
     const uiox_hdmi_hw_ops_t *ops =
         (const uiox_hdmi_hw_ops_t *)sys->hif.hw->priv;
     if (ops && ops->hdcp_status) {
         uiox_hdcp_state_t hs;
         if (ops->hdcp_status(sys->hif.hw, &hs) == 0) {
             if (hs == UIOX_HDCP_AUTHENTICATED &&
                 sys->sink.hdcp_state != UIOX_HDCP_AUTHENTICATED) {
                 sys->sink.hdcp_state = hs;
                 fire(sys, UIOX_HDMI_EVT_HDCP_AUTH);
             } else if (hs == UIOX_HDCP_FAILED) {
                 fire(sys, UIOX_HDMI_EVT_HDCP_FAIL);
             }
         }
     }
 
     /* Infoframe refresh */
     uiox_hdmi_proto_tick(&sys->proto, now_ms);
 
     /* DPMS auto-blank */
     if (sys->dpms_timeout_ms &&
         (now_ms - sys->last_activity_ms) >= sys->dpms_timeout_ms) {
         const uiox_hdmi_hw_ops_t *o =
             (const uiox_hdmi_hw_ops_t *)sys->hif.hw->priv;
         if (o && o->phy_power) o->phy_power(sys->hif.hw, false);
         sys->state = UIOX_HDMI_SUBSYS_SUSPENDED;
     }
 }
 
 void uiox_hdmi_subsys_activity(uiox_hdmi_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys) return;
     sys->last_activity_ms = now_ms;
     if (sys->state == UIOX_HDMI_SUBSYS_SUSPENDED) {
         const uiox_hdmi_hw_ops_t *ops =
             (const uiox_hdmi_hw_ops_t *)sys->hif.hw->priv;
         if (ops && ops->phy_power) ops->phy_power(sys->hif.hw, true);
         sys->state = UIOX_HDMI_SUBSYS_RUNNING;
     }
 }
 
 void uiox_hdmi_subsys_set_evt_cb(uiox_hdmi_subsys_t *sys,
                                   uiox_hdmi_evt_cb_t cb, void *ctx)
 {
     if (!sys) return;
     sys->evt_cb  = cb;
     sys->evt_ctx = ctx;
 }
 
 void uiox_hdmi_subsys_set_dpms(uiox_hdmi_subsys_t *sys, uint32_t timeout_ms)
 {
     if (sys) sys->dpms_timeout_ms = timeout_ms;
 }
 