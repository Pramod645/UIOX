/**
 * @file    uiox_hdmi_device.c
 * @brief   UIOX HDMI device API implementation.
 * @date    2026-05-28
 */

 #include "uiox_hdmi_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_hdmi_open(uiox_hdmi_device_t *dev, const uiox_hdmi_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
 
     int rc = uiox_hdmi_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
 
     rc = uiox_hdmi_subsys_init(&dev->subsys, p->hw,
                                 p->cs, p->bpc,
                                 p->pref_w, p->pref_h, p->pref_hz);
     if (rc < 0) return rc;
 
     uiox_hdmi_subsys_set_dpms(&dev->subsys, p->dpms_timeout_ms);
     if (p->evt_cb)
         uiox_hdmi_subsys_set_evt_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
 
     dev->open = true;
     return 0;
 }
 
 int uiox_hdmi_start(uiox_hdmi_device_t *dev)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_hdmi_subsys_start(&dev->subsys);
 }
 
 void uiox_hdmi_stop(uiox_hdmi_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_hdmi_subsys_stop(&dev->subsys);
 }
 
 void uiox_hdmi_close(uiox_hdmi_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_hdmi_stop(dev);
     uiox_hdmi_hw_deinit(dev->hw);
     dev->open = false;
 }
 
 void uiox_hdmi_tick(uiox_hdmi_device_t *dev, uint32_t now_ms)
 {
     if (!dev || !dev->open) return;
     uiox_hdmi_subsys_tick(&dev->subsys, now_ms);
 }
 
 void uiox_hdmi_activity(uiox_hdmi_device_t *dev, uint32_t now_ms)
 {
     if (!dev || !dev->open) return;
     uiox_hdmi_subsys_activity(&dev->subsys, now_ms);
 }
 
 uiox_hdmi_fb_t *uiox_hdmi_acquire(uiox_hdmi_device_t *dev)
 {
     if (!dev || !dev->open) return NULL;
     return uiox_hdmi_subsys_acquire(&dev->subsys);
 }
 
 int uiox_hdmi_present(uiox_hdmi_device_t *dev, uiox_hdmi_fb_t *fb)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_hdmi_subsys_present(&dev->subsys, fb);
 }
 
 int uiox_hdmi_set_hdr(uiox_hdmi_device_t *dev, const uiox_hdmi_hdr_t *hdr)
 {
     if (!dev || !dev->open || !hdr) return -EINVAL;
     return uiox_hdmi_proto_send_hdr(&dev->subsys.proto, hdr);
 }
 
 int uiox_hdmi_set_audio(uiox_hdmi_device_t *dev,
                          const uiox_hdmi_audio_cfg_t *a)
 {
     if (!dev || !dev->open || !a) return -EINVAL;
     return uiox_hdmi_if_set_audio(&dev->subsys.hif, a);
 }
 
 int uiox_hdmi_audio_write(uiox_hdmi_device_t *dev,
                            const uint8_t *samples, uint32_t bytes)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_hdmi_if_audio_write(&dev->subsys.hif, samples, bytes);
 }
 
 int uiox_hdmi_cec_send(uiox_hdmi_device_t *dev,
                         uint8_t dst_la, uint8_t opcode,
                         const uint8_t *params, uint8_t plen)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_hdmi_proto_cec_send(&dev->subsys.proto,
                                      dst_la, opcode, params, plen);
 }
 
 bool uiox_hdmi_connected(const uiox_hdmi_device_t *dev)
 {
     if (!dev || !dev->open) return false;
     return dev->subsys.last_connected;
 }
 
 void uiox_hdmi_get_resolution(const uiox_hdmi_device_t *dev,
                                uint16_t *w, uint16_t *h)
 {
     if (!dev || !w || !h) return;
     *w = dev->subsys.sink.current_mode.h_active;
     *h = dev->subsys.sink.current_mode.v_active;
 }
 
 void uiox_hdmi_print_info(const uiox_hdmi_device_t *dev)
 {
     if (!dev) return;
     uiox_hdmi_sink_print(&dev->subsys.sink);
     const uiox_hdmi_acr_t *acr = &dev->subsys.hif.acr;
     printf("  ACR N        : %u\n", acr->N);
     printf("  ACR CTS      : %u\n", acr->CTS);
     printf("  Scrambling   : %s\n",
            dev->subsys.hif.scrambling ? "enabled" : "disabled");
     printf("  Link type    : %s\n",
            dev->subsys.hif.link == UIOX_HDMI_LINK_FRL ? "FRL" : "TMDS");
     printf("  HDCP state   : %s\n",
            dev->subsys.sink.hdcp_state == UIOX_HDCP_AUTHENTICATED ?
            "authenticated" :
            dev->subsys.sink.hdcp_state == UIOX_HDCP_AUTHENTICATING ?
            "authenticating" :
            dev->subsys.sink.hdcp_state == UIOX_HDCP_FAILED ?
            "failed" : "disabled");
 }
 
 void uiox_hdmi_print_stats(const uiox_hdmi_device_t *dev)
 {
     if (!dev) return;
     const uiox_hdmi_subsys_t *s = &dev->subsys;
     printf("  State          : %s\n", uiox_hdmi_state_name(s->state));
     printf("  Total frames   : %llu\n", (unsigned long long)s->total_frames);
     printf("  Total VBlanks  : %llu\n", (unsigned long long)s->total_vblanks);
     printf("  Dropped frames : %llu\n", (unsigned long long)s->dropped_frames);
     printf("  Frame ID       : %u\n",   s->frame_id);
     printf("  FB pool free   : %u / %u\n",
            uiox_hdmi_buf_fb_free(), UIOX_HDMI_FB_POOL_SIZE);
     printf("  PKT pool free  : %u / %u\n",
            uiox_hdmi_buf_pkt_free(), UIOX_HDMI_PKT_POOL_SIZE);
     uiox_hdmi_if_stats_t is;
     uiox_hdmi_if_stats_get(&dev->subsys.hif, &is);
     printf("  Frame count    : %llu\n", (unsigned long long)is.frame_count);
     printf("  VBlank count   : %llu\n", (unsigned long long)is.vblank_count);
     printf("  Audio underruns: %llu\n", (unsigned long long)is.audio_underrun);
     printf("  Parity errors  : %llu\n", (unsigned long long)is.parity_errors);
     printf("  HPD events     : %llu\n", (unsigned long long)is.hotplug_events);
     printf("  HDCP auths     : %llu\n", (unsigned long long)is.hdcp_auth_count);
 }
 
 const char *uiox_hdmi_evt_name(uiox_hdmi_evt_t evt)
 {
     switch (evt) {
     case UIOX_HDMI_EVT_CONNECT:       return "CONNECT";
     case UIOX_HDMI_EVT_DISCONNECT:    return "DISCONNECT";
     case UIOX_HDMI_EVT_HDCP_AUTH:     return "HDCP_AUTH";
     case UIOX_HDMI_EVT_HDCP_FAIL:     return "HDCP_FAIL";
     case UIOX_HDMI_EVT_VBLANK:        return "VBLANK";
     case UIOX_HDMI_EVT_AUDIO_UNDERRUN:return "AUDIO_UNDERRUN";
     case UIOX_HDMI_EVT_ERROR:         return "ERROR";
     default:                           return "UNKNOWN";
     }
 }
 
 const char *uiox_hdmi_state_name(uiox_hdmi_subsys_state_t s)
 {
     switch (s) {
     case UIOX_HDMI_SUBSYS_STOPPED:   return "STOPPED";
     case UIOX_HDMI_SUBSYS_PROBING:   return "PROBING";
     case UIOX_HDMI_SUBSYS_RUNNING:   return "RUNNING";
     case UIOX_HDMI_SUBSYS_SUSPENDED: return "SUSPENDED";
     default:                          return "UNKNOWN";
     }
 }
 