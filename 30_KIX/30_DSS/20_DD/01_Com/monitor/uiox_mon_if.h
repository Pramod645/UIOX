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
 #include "uiox_devclass.h"
 
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
/* ═══════════════════════════ INSERTED — registry layer ══════════════
 *  §6 head only — §9 is uiox_devclass.h's last section, so CMD has no
 *  family pair.  The display surface travels through ioctl on the head;
 *  mmap belongs to 33_PCS. */
//#include "uiox_devclass.h"

#define UIOX_MON_IOC_ENABLE      0x01u
#define UIOX_MON_IOC_DISABLE     0x02u
#define UIOX_MON_IOC_FLIP        0x03u
#define UIOX_MON_IOC_VSYNC       0x04u
#define UIOX_MON_IOC_GET_INFO    0x05u
#define UIOX_MON_IOC_GET_STATS   0x06u

typedef struct { uiox_mon_fb_t *fb; } uiox_mon_flip_t;

typedef struct {
    uintptr_t            base_addr;
    uint32_t             irq_vblank;
    uint32_t             irq_hotplug;
    uiox_mon_if_type_t   type;
    uiox_dp_link_state_t dp_link;
    uint8_t              dp_lanes;
    uint8_t              dsi_lanes;
} uiox_mon_info_t;

typedef struct {
    uiox_dev_ops_t head;
} uiox_mon_ops_t;

extern const uiox_mon_ops_t uiox_mon_class_ops;

int uiox_mon_probe(void);
/* ═══════════════════════════ END INSERT ═════════════════════════ */
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MON_IF_H */
 