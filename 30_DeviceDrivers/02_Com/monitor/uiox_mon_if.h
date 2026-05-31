/**
 * @file    uiox_mon_if.h
 * @brief   UIOX Monitor interface driver (HDMI/DP/MIPI/LVDS/RGB).
 *
 * Sits between HAL and panel abstraction. Manages:
 *   - Physical interface negotiation (link training for DP, TMDS for HDMI)
 *   - MIPI DSI command/video mode setup
 *   - LVDS serialiser configuration
 *   - Pixel clock PLL lock
 *   - Audio infoframe (HDMI/DP)
 *   - Interface statistics (underruns, sync errors)
 *
 * @date    2026-05-27
 */
//Layer 2 — Interface Driver
 #ifndef UIOX_MON_IF_H
 #define UIOX_MON_IF_H
 
 #include "uiox_mon_hw.h"
 #include "uiox_mon_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Interface statistics
  * ====================================================================== */
 
 typedef struct {
     uint64_t  vblank_count;
     uint64_t  frame_count;
     uint64_t  underrun_count;
     uint64_t  sync_error_count;
     uint64_t  hotplug_events;
     uint64_t  bytes_transferred;
 } uiox_mon_if_stats_t;
 
 /* =========================================================================
  * Link training state (DisplayPort)
  * ====================================================================== */
 
 typedef enum {
     UIOX_DP_LINK_IDLE = 0,
     UIOX_DP_LINK_TRAINING_CR,    /**< Clock recovery                      */
     UIOX_DP_LINK_TRAINING_EQ,    /**< Channel equalisation                */
     UIOX_DP_LINK_TRAINED,
     UIOX_DP_LINK_FAILED,
 } uiox_dp_link_state_t;
 
 /* =========================================================================
  * Interface descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_mon_hw_t        *hw;
     uiox_mon_if_type_t    type;
     uiox_mon_timing_t     timing;
     uiox_mon_pixfmt_t     pixfmt;
     bool                  enabled;
     bool                  audio_enabled;
 
     /* DP-specific */
     uiox_dp_link_state_t  dp_link;
     uint8_t               dp_lanes;       /**< 1, 2, or 4 lanes           */
     uint32_t              dp_link_rate_hz;/**< e.g. 2700000000 = HBR1     */
 
     /* MIPI DSI-specific */
     uint8_t               dsi_lanes;
     bool                  dsi_video_mode; /**< false = command mode        */
 
     /* Statistics */
     uiox_mon_if_stats_t   stats;
 } uiox_mon_if_t;
 
 /* =========================================================================
  * Interface API
  * ====================================================================== */
 
 int  uiox_mon_if_config  (uiox_mon_if_t        *mif,
                            uiox_mon_hw_t        *hw,
                            uiox_mon_if_type_t    type,
                            const uiox_mon_timing_t *timing,
                            uiox_mon_pixfmt_t     pixfmt);
 
 int  uiox_mon_if_enable  (uiox_mon_if_t *mif);
 void uiox_mon_if_disable (uiox_mon_if_t *mif);
 
 /** Submit framebuffer for display on next VBlank. */
 int  uiox_mon_if_flip    (uiox_mon_if_t *mif, uiox_mon_fb_t *fb);
 
 /** Wait for VBlank and confirm flip completion. */
 int  uiox_mon_if_vsync   (uiox_mon_if_t *mif, uint32_t timeout_ms);
 
 void uiox_mon_if_stats_get  (const uiox_mon_if_t *mif,
                               uiox_mon_if_stats_t *out);
 void uiox_mon_if_stats_reset(uiox_mon_if_t *mif);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MON_IF_H */
 