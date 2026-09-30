/**
 * @file    uiox_mon_subsys.h
 * @brief   UIOX Monitor subsystem — pipeline, hotplug, DPMS, multi-head.
 *
 * Top processing layer. Manages:
 *   - Full display pipeline: render → DSP → flip → vsync
 *   - Hotplug detection and automatic re-probe
 *   - DPMS (power management) state machine
 *   - Backlight auto-dim on inactivity
 *   - Frame pacing (target FPS limiter)
 *   - Per-frame statistics and timing
 *
 * @date    2026-05-27
 */
//Layer 4 — Monitor Subsystem
 #ifndef UIOX_MON_SUBSYS_H
 #define UIOX_MON_SUBSYS_H
 
 #include "uiox_mon_if.h"
 #include "uiox_mon_panel.h"
 #include "uiox_mon_dsp.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_MON_SUBSYS_STOPPED = 0,
     UIOX_MON_SUBSYS_RUNNING,
     UIOX_MON_SUBSYS_DPMS_SAVING,
 } uiox_mon_subsys_state_t;
 
 /** Hotplug event callback. */
 typedef void (*uiox_mon_hotplug_cb_t)(bool connected, void *ctx);
 
 /** VBlank event callback. */
 typedef void (*uiox_mon_vblank_cb_t)(uint32_t frame_id, void *ctx);
 
 typedef struct {
     uiox_mon_if_t            mif;
     uiox_mon_panel_t         panel;
     uiox_mon_dsp_t           dsp;
     uiox_mon_subsys_state_t  state;
 
     /* Triple-buffer state */
     uiox_mon_fb_t           *fb_display;  /**< Currently on screen         */
     uiox_mon_fb_t           *fb_render;   /**< CPU drawing into this       */
     uiox_mon_fb_t           *fb_pending;  /**< Waiting for VBlank flip     */
     uint32_t                 frame_id;
 
     /* Hotplug */
     uiox_mon_hotplug_cb_t    hotplug_cb;
     void                    *hotplug_ctx;
     bool                     last_connected;
 
     /* VBlank callback */
     uiox_mon_vblank_cb_t     vblank_cb;
     void                    *vblank_ctx;
 
     /* DPMS auto-dim */
     uint32_t                 last_activity_ms;
     uint32_t                 dpms_timeout_ms;  /**< 0 = disabled           */
     uiox_mon_dpms_t          dpms_state;
 
     /* Frame pacing */
     uint32_t                 target_fps;        /**< 0 = unlimited          */
     uint32_t                 last_frame_ms;
 
     /* Statistics */
     uint64_t                 total_frames;
     uint64_t                 total_vblanks;
     uint64_t                 dropped_frames;
     uint64_t                 underruns;
 } uiox_mon_subsys_t;
 
 /* =========================================================================
  * Subsystem API
  * ====================================================================== */
 
  int  uiox_mon_subsys_init   (uiox_mon_subsys_t          *sys,
    uiox_mon_hw_t              *hw,
    uiox_mon_if_type_t          if_type,
    const uiox_mon_dsp_cfg_t   *dsp_cfg,
    uint16_t pref_w, uint16_t pref_h,
    uint8_t  pref_hz);

int  uiox_mon_subsys_start  (uiox_mon_subsys_t *sys);
void uiox_mon_subsys_stop   (uiox_mon_subsys_t *sys);

/**
* @brief  Acquire the next render framebuffer.
*
* Returns a framebuffer the CPU can draw into. NULL if all buffers
* are occupied (previous flip not yet consumed).
*/
uiox_mon_fb_t *uiox_mon_subsys_acquire(uiox_mon_subsys_t *sys);

/**
* @brief  Submit rendered framebuffer for display.
*
* Runs DSP pipeline on fb, then schedules a page flip on next VBlank.
*/
int  uiox_mon_subsys_present(uiox_mon_subsys_t *sys,
    uiox_mon_fb_t     *fb);

/**
* @brief  Periodic tick — drives hotplug, DPMS, frame pacing, VBlank.
* @param  now_ms  Monotonic time (ms).
*/
void uiox_mon_subsys_tick   (uiox_mon_subsys_t *sys, uint32_t now_ms);

/** Register hotplug event callback. */
void uiox_mon_subsys_set_hotplug_cb(uiox_mon_subsys_t    *sys,
           uiox_mon_hotplug_cb_t cb,
           void                 *ctx);

/** Register VBlank event callback. */
void uiox_mon_subsys_set_vblank_cb (uiox_mon_subsys_t   *sys,
           uiox_mon_vblank_cb_t cb,
           void                *ctx);

/** Configure DPMS auto-blank timeout. */
void uiox_mon_subsys_set_dpms_timeout(uiox_mon_subsys_t *sys,
             uint32_t           timeout_ms);

/** Configure target frame rate limiter (0 = unlimited). */
void uiox_mon_subsys_set_fps(uiox_mon_subsys_t *sys, uint32_t fps);

/** Signal user activity — resets DPMS idle timer. */
void uiox_mon_subsys_activity(uiox_mon_subsys_t *sys, uint32_t now_ms);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_MON_SUBSYS_H */
 