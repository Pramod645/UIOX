/**
 * @file    uiox_mon_subsys.c
 * @brief   UIOX Monitor subsystem implementation.
 * @date    2026-05-27
 */

 #include "uiox_mon_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 /* =========================================================================
  * Init
  * ====================================================================== */
 
 int uiox_mon_subsys_init(uiox_mon_subsys_t        *sys,
                           uiox_mon_hw_t            *hw,
                           uiox_mon_if_type_t        if_type,
                           const uiox_mon_dsp_cfg_t *dsp_cfg,
                           uint16_t pref_w, uint16_t pref_h,
                           uint8_t  pref_hz)
 {
     if (!sys || !hw || !dsp_cfg) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
 
     /* 1. Probe panel — read EDID, select mode */
     int rc = uiox_mon_panel_probe(&sys->panel, hw);
     if (rc < 0) return rc;
 
     rc = uiox_mon_panel_select_mode(&sys->panel, pref_w, pref_h, pref_hz);
     if (rc < 0) return rc;
 
     /* 2. Configure interface with selected timing */
     const uiox_mon_timing_t *t = uiox_mon_panel_timing(&sys->panel);
     rc = uiox_mon_if_config(&sys->mif, hw, if_type, t, dsp_cfg->scale_mode
                              == UIOX_MON_SCALE_NEAREST ?
                              UIOX_MON_FMT_XRGB8888 :
                              UIOX_MON_FMT_XRGB8888);
     if (rc < 0) return rc;
 
     /* 3. Initialise DSP context */
     rc = uiox_mon_dsp_init(&sys->dsp, dsp_cfg);
     if (rc < 0) return rc;
 
     sys->state       = UIOX_MON_SUBSYS_STOPPED;
     sys->target_fps  = 60u;
     sys->dpms_state  = UIOX_MON_DPMS_OFF;
     sys->frame_id    = 0u;
     return 0;
 }
 
 /* =========================================================================
  * Start / Stop
  * ====================================================================== */
 
 int uiox_mon_subsys_start(uiox_mon_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
 
     /* Power on panel */
     int rc = uiox_mon_panel_power_on(&sys->panel, sys->mif.hw);
     if (rc < 0) return rc;
 
     /* Enable interface */
     rc = uiox_mon_if_enable(&sys->mif);
     if (rc < 0) return rc;
 
     /* Set backlight */
     uiox_mon_hw_set_backlight(sys->mif.hw, 200u);
 
     sys->state      = UIOX_MON_SUBSYS_RUNNING;
     sys->dpms_state = UIOX_MON_DPMS_ON;
 
     /* Allocate triple buffers */
     sys->fb_render  = uiox_mon_buf_alloc();
     sys->fb_display = NULL;
     sys->fb_pending = NULL;
 
     return 0;
 }
 
 void uiox_mon_subsys_stop(uiox_mon_subsys_t *sys)
 {
     if (!sys) return;
 
     /* Release framebuffers */
     if (sys->fb_render)  { uiox_mon_buf_free(sys->fb_render);  sys->fb_render  = NULL; }
     if (sys->fb_pending) { uiox_mon_buf_free(sys->fb_pending); sys->fb_pending = NULL; }
     if (sys->fb_display) { uiox_mon_buf_free(sys->fb_display); sys->fb_display = NULL; }
 
     uiox_mon_if_disable(&sys->mif);
     uiox_mon_panel_power_off(&sys->panel, sys->mif.hw);
     uiox_mon_dsp_deinit(&sys->dsp);
     sys->state = UIOX_MON_SUBSYS_STOPPED;
 }
 
 /* =========================================================================
  * Framebuffer acquire / present
  * ====================================================================== */
 
 uiox_mon_fb_t *uiox_mon_subsys_acquire(uiox_mon_subsys_t *sys)
 {
     if (!sys || sys->state != UIOX_MON_SUBSYS_RUNNING) return NULL;
 
     /* Return pre-allocated render buffer if available */
     if (sys->fb_render) return sys->fb_render;
 
     /* Try to allocate a fresh one */
     sys->fb_render = uiox_mon_buf_alloc();
     return sys->fb_render;
 }
 
 int uiox_mon_subsys_present(uiox_mon_subsys_t *sys, uiox_mon_fb_t *fb)
 {
     if (!sys || !fb) return -EINVAL;
     if (sys->state != UIOX_MON_SUBSYS_RUNNING) return -ENETDOWN;
 
     /* Tag frame */
     fb->frame_id = ++sys->frame_id;
 
     /* Run DSP pipeline */
     uiox_mon_dsp_process(&sys->dsp, fb);
 
     /* If a pending flip is already queued, drop old pending */
     if (sys->fb_pending) {
         sys->dropped_frames++;
         uiox_mon_buf_free(sys->fb_pending);
     }
 
     sys->fb_pending = fb;
     sys->fb_render  = NULL;
 
     /* Submit flip to hardware */
     int rc = uiox_mon_if_flip(&sys->mif, fb);
     if (rc < 0) return rc;
 
     /* Wait for VBlank */
     rc = uiox_mon_if_vsync(&sys->mif, 50u);
     if (rc == 0) {
         /* Flip complete — rotate buffers */
         if (sys->fb_display) uiox_mon_buf_free(sys->fb_display);
         sys->fb_display       = sys->fb_pending;
         sys->fb_display->state = UIOX_MON_BUF_DISPLAYED;
         sys->fb_pending       = NULL;
         sys->total_frames++;
 
         if (sys->vblank_cb)
             sys->vblank_cb(sys->frame_id, sys->vblank_ctx);
         sys->total_vblanks++;
     }
 
     /* Pre-allocate next render buffer */
     sys->fb_render = uiox_mon_buf_alloc();
     return rc;
 }
 
 /* =========================================================================
  * Periodic tick
  * ====================================================================== */
 
 void uiox_mon_subsys_tick(uiox_mon_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys) return;
 
     /* Hotplug detection */
     bool connected = uiox_mon_hw_connected(sys->mif.hw);
     if (connected != sys->last_connected) {
         sys->last_connected = connected;
         if (sys->hotplug_cb)
             sys->hotplug_cb(connected, sys->hotplug_ctx);
         if (connected && sys->state == UIOX_MON_SUBSYS_STOPPED) {
             /* Re-probe panel on hotplug */
             uiox_mon_panel_probe(&sys->panel, sys->mif.hw);
         }
     }
 
     /* DPMS auto-blank */
     if (sys->dpms_timeout_ms &&
         sys->state == UIOX_MON_SUBSYS_RUNNING &&
         (now_ms - sys->last_activity_ms) >= sys->dpms_timeout_ms) {
         uiox_mon_hw_set_dpms(sys->mif.hw, UIOX_MON_DPMS_STANDBY);
         uiox_mon_hw_set_backlight(sys->mif.hw, 0u);
         sys->dpms_state = UIOX_MON_DPMS_STANDBY;
         sys->state      = UIOX_MON_SUBSYS_DPMS_SAVING;
     }
 }
 
 /* =========================================================================
  * Configuration helpers
  * ====================================================================== */
 
 void uiox_mon_subsys_set_hotplug_cb(uiox_mon_subsys_t    *sys,
                                      uiox_mon_hotplug_cb_t cb,
                                      void                 *ctx)
 {
     if (!sys) return;
     sys->hotplug_cb  = cb;
     sys->hotplug_ctx = ctx;
 }
 
 void uiox_mon_subsys_set_vblank_cb(uiox_mon_subsys_t   *sys,
                                     uiox_mon_vblank_cb_t cb,
                                     void                *ctx)
 {
     if (!sys) return;
     sys->vblank_cb  = cb;
     sys->vblank_ctx = ctx;
 }
 
 void uiox_mon_subsys_set_dpms_timeout(uiox_mon_subsys_t *sys,
                                        uint32_t           timeout_ms)
 {
     if (sys) sys->dpms_timeout_ms = timeout_ms;
 }
 
 void uiox_mon_subsys_set_fps(uiox_mon_subsys_t *sys, uint32_t fps)
 {
     if (sys) sys->target_fps = fps;
 }
 
 void uiox_mon_subsys_activity(uiox_mon_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys) return;
     sys->last_activity_ms = now_ms;
 
     /* Wake display if DPMS saving */
     if (sys->state == UIOX_MON_SUBSYS_DPMS_SAVING) {
         uiox_mon_hw_set_dpms(sys->mif.hw, UIOX_MON_DPMS_ON);
         uiox_mon_hw_set_backlight(sys->mif.hw, 200u);
         sys->dpms_state = UIOX_MON_DPMS_ON;
         sys->state      = UIOX_MON_SUBSYS_RUNNING;
     }
 }
 