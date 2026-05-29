/**
 * @file    uiox_mon_device.c
 * @brief   UIOX Monitor device API implementation.
 * @date    2026-05-27
 */

 #include "uiox_mon_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_mon_open(uiox_mon_device_t           *dev,
                    const uiox_mon_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
 
     int rc = uiox_mon_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
 
     rc = uiox_mon_subsys_init(&dev->subsys, p->hw, p->if_type,
                                &p->dsp,
                                p->pref_w, p->pref_h, p->pref_hz);
     if (rc < 0) return rc;
 
     uiox_mon_subsys_set_dpms_timeout(&dev->subsys, p->dpms_timeout_ms);
     uiox_mon_subsys_set_fps(&dev->subsys, p->target_fps);
 
     if (p->hotplug_cb)
         uiox_mon_subsys_set_hotplug_cb(&dev->subsys,
                                         p->hotplug_cb, p->hotplug_ctx);
     if (p->vblank_cb)
         uiox_mon_subsys_set_vblank_cb(&dev->subsys,
                                        p->vblank_cb, p->vblank_ctx);
 
     dev->open = true;
     return 0;
 }
 
 int uiox_mon_start(uiox_mon_device_t *dev)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_mon_subsys_start(&dev->subsys);
 }
 
 void uiox_mon_stop(uiox_mon_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_mon_subsys_stop(&dev->subsys);
 }
 
 void uiox_mon_close(uiox_mon_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_mon_stop(dev);
     uiox_mon_hw_deinit(dev->hw);
     dev->open = false;
 }
 
 uiox_mon_fb_t *uiox_mon_acquire(uiox_mon_device_t *dev)
 {
     if (!dev || !dev->open) return NULL;
     return uiox_mon_subsys_acquire(&dev->subsys);
 }
 
 int uiox_mon_present(uiox_mon_device_t *dev, uiox_mon_fb_t *fb)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_mon_subsys_present(&dev->subsys, fb);
 }
 
 void uiox_mon_tick(uiox_mon_device_t *dev, uint32_t now_ms)
 {
     if (!dev || !dev->open) return;
     uiox_mon_subsys_tick(&dev->subsys, now_ms);
 }
 
 void uiox_mon_activity(uiox_mon_device_t *dev, uint32_t now_ms)
 {
     if (!dev || !dev->open) return;
     uiox_mon_subsys_activity(&dev->subsys, now_ms);
 }
 
 int uiox_mon_set_backlight(uiox_mon_device_t *dev, uint8_t level)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_mon_hw_set_backlight(dev->hw, level);
 }
 
 int uiox_mon_set_dpms(uiox_mon_device_t *dev, uiox_mon_dpms_t dpms)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_mon_hw_set_dpms(dev->hw, dpms);
 }
 
 int uiox_mon_set_gamma(uiox_mon_device_t *dev, uint16_t gamma_x100)
 {
     if (!dev || !dev->open) return -EINVAL;
     dev->subsys.dsp.cfg.gamma_x100 = gamma_x100;
     uiox_mon_dsp_build_gamma(&dev->subsys.dsp);
     return 0;
 }
 
 int uiox_mon_set_brightness(uiox_mon_device_t *dev, int8_t brightness)
 {
     if (!dev || !dev->open) return -EINVAL;
     dev->subsys.dsp.cfg.brightness = brightness;
     return 0;
 }
 
 int uiox_mon_set_contrast(uiox_mon_device_t *dev, int8_t contrast)
 {
     if (!dev || !dev->open) return -EINVAL;
     dev->subsys.dsp.cfg.contrast = contrast;
     return 0;
 }
 
 int uiox_mon_set_colour_mode(uiox_mon_device_t      *dev,
                                uiox_mon_colour_mode_t  mode)
 {
     if (!dev || !dev->open) return -EINVAL;
     dev->subsys.dsp.cfg.colour_mode = mode;
     uiox_mon_dsp_build_gamma(&dev->subsys.dsp);
     return 0;
 }
 
 int uiox_mon_osd_add(uiox_mon_device_t         *dev,
                       const uiox_mon_osd_elem_t *elem)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_mon_dsp_osd_add(&dev->subsys.dsp, elem);
 }
 
 void uiox_mon_osd_remove(uiox_mon_device_t *dev, uint8_t idx)
 {
     if (!dev || !dev->open) return;
     uiox_mon_dsp_osd_remove(&dev->subsys.dsp, idx);
 }
 
 bool uiox_mon_connected(uiox_mon_device_t *dev)
 {
     if (!dev || !dev->open) return false;
     return uiox_mon_hw_connected(dev->hw);
 }
 
 void uiox_mon_get_resolution(const uiox_mon_device_t *dev,
                               uint16_t *w, uint16_t *h)
 {
     if (!dev || !w || !h) return;
     *w = dev->subsys.panel.current_mode.timing.h_active;
     *h = dev->subsys.panel.current_mode.timing.v_active;
 }
 
 void uiox_mon_print_info(const uiox_mon_device_t *dev)
 {
     if (!dev) return;
     uiox_mon_panel_print(&dev->subsys.panel);
 }
 
 void uiox_mon_print_stats(const uiox_mon_device_t *dev)
 {
     if (!dev) return;
     const uiox_mon_subsys_t *s = &dev->subsys;
     printf("  Total frames   : %llu\n", (unsigned long long)s->total_frames);
     printf("  Total VBlanks  : %llu\n", (unsigned long long)s->total_vblanks);
     printf("  Dropped frames : %llu\n", (unsigned long long)s->dropped_frames);
     printf("  Underruns      : %llu\n", (unsigned long long)s->underruns);
     printf("  Frame ID       : %u\n",   s->frame_id);
     printf("  DPMS state     : %s\n",   uiox_mon_dpms_name(s->dpms_state));
     printf("  Buffers free   : %u / %u\n",
            uiox_mon_buf_free_count(), UIOX_MON_BUF_POOL_SIZE);
 }
 
 const char *uiox_mon_dpms_name(uiox_mon_dpms_t dpms)
 {
     switch (dpms) {
     case UIOX_MON_DPMS_ON:       return "ON";
     case UIOX_MON_DPMS_STANDBY:  return "STANDBY";
     case UIOX_MON_DPMS_SUSPEND:  return "SUSPEND";
     case UIOX_MON_DPMS_OFF:      return "OFF";
     default:                      return "UNKNOWN";
     }
 }
 