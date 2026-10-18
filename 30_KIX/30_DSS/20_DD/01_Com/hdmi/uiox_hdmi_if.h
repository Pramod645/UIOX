/**
 * @file    uiox_hdmi_if.h
 * @brief   UIOX HDMI interface driver (TMDS/FRL lanes, PHY, scrambling).
 *
 * Manages:
 *   - TMDS character rate and pixel clock programming
 *   - FRL lane training and rate negotiation (HDMI 2.1)
 *   - PHY power-up sequence and link verification
 *   - HDMI 2.0 scrambling (LFSR)
 *   - YCbCr 4:2:0 downsampling path
 *   - Audio clock regeneration (N/CTS values)
 *   - Interface statistics (underruns, parity errors)
 *
 * @date    2026-05-28
 */
//Layer 2 — Interface Driver
 #ifndef UIOX_HDMI_IF_H
 #define UIOX_HDMI_IF_H
 
 #include "uiox_hdmi_hw.h"
 #include "uiox_hdmi_buf.h"
 #include "uiox_devclass.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Audio clock regeneration (N/CTS pair for IEC 60958)
  * ====================================================================== */
 
 typedef struct {
     uint32_t N;    /**< ACR N value (typically 6144 for 48kHz @ 148.5MHz) */
     uint32_t CTS;  /**< Cycle Time Stamp (derived from pixel clock)        */
 } uiox_hdmi_acr_t;
 
 /* =========================================================================
  * Interface statistics
  * ====================================================================== */
 
 typedef struct {
     uint64_t  vblank_count;
     uint64_t  frame_count;
     uint64_t  audio_underrun;
     uint64_t  parity_errors;
     uint64_t  hotplug_events;
     uint64_t  hdcp_auth_count;
     uint64_t  frl_retrain_count;
 } uiox_hdmi_if_stats_t;
 
 /* =========================================================================
  * Interface descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_hdmi_hw_t        *hw;
     uiox_hdmi_link_t       link;
     uiox_hdmi_frl_rate_t   frl_rate;
     uiox_hdmi_timing_t     timing;
     uiox_hdmi_colorspace_t cs;
     uiox_hdmi_bpc_t        bpc;
     uiox_hdmi_audio_cfg_t  audio;
     uiox_hdmi_acr_t        acr;
     bool                   scrambling;  /**< HDMI 2.0+ scrambling enabled  */
     bool                   enabled;
     uiox_hdmi_if_stats_t   stats;
 } uiox_hdmi_if_t;
 
 /* =========================================================================
  * Interface API
  * ====================================================================== */
 
 int  uiox_hdmi_if_config  (uiox_hdmi_if_t        *hif,
                             uiox_hdmi_hw_t        *hw,
                             const uiox_hdmi_timing_t *timing,
                             uiox_hdmi_colorspace_t cs,
                             uiox_hdmi_bpc_t        bpc);
 
 int  uiox_hdmi_if_enable  (uiox_hdmi_if_t *hif);
 void uiox_hdmi_if_disable (uiox_hdmi_if_t *hif);
 
 int  uiox_hdmi_if_flip    (uiox_hdmi_if_t *hif, uiox_hdmi_fb_t *fb);
 int  uiox_hdmi_if_vsync   (uiox_hdmi_if_t *hif, uint32_t timeout_ms);
 
 int  uiox_hdmi_if_set_audio(uiox_hdmi_if_t             *hif,
                              const uiox_hdmi_audio_cfg_t *a);
 int  uiox_hdmi_if_audio_write(uiox_hdmi_if_t *hif,
                                const uint8_t *samples, uint32_t bytes);
 
                                void uiox_hdmi_if_stats_get  (const uiox_hdmi_if_t *hif,
                                    uiox_hdmi_if_stats_t *out);
     void uiox_hdmi_if_stats_reset(uiox_hdmi_if_t *hif);
     
     /* Compute N/CTS for audio clock regeneration */
     uiox_hdmi_acr_t uiox_hdmi_if_compute_acr(uint32_t pixel_clk_khz,
                                                uint32_t sample_rate_hz);
     
     /* ═══════════════════════════ INSERTED — registry layer ══════════════
 *  §6 head only — §9 is uiox_devclass.h's last section, so CMD has no
 *  family pair.  The display control surface travels through ioctl on
 *  the head; mmap belongs to 33_PCS. */

#define UIOX_HDMI_IOC_ENABLE     0x01u  /* in:  NULL                     */
#define UIOX_HDMI_IOC_DISABLE    0x02u  /* in:  NULL                     */
#define UIOX_HDMI_IOC_FLIP       0x03u  /* in:  uiox_hdmi_flip_t         */
#define UIOX_HDMI_IOC_VSYNC      0x04u  /* in:  uint32_t timeout_ms      */
#define UIOX_HDMI_IOC_SET_AUDIO  0x05u  /* in:  uiox_hdmi_audio_cfg_t    */
#define UIOX_HDMI_IOC_GET_STATS  0x06u  /* out: uiox_hdmi_if_stats_t     */
#define UIOX_HDMI_IOC_GET_ACR    0x07u  /* in/out: uiox_hdmi_acr_req_t   */

/* The caller's framebuffer descriptor — what _if_flip() needs.  The
 * backing memory comes from the 00_drvbuff hdmi pool
 * (uiox_hdmi_buf_alloc_fb), so the pointer is the pool's, not ours. */
typedef struct { uiox_hdmi_fb_t *fb; } uiox_hdmi_flip_t;
typedef struct { uint32_t pixel_clk_khz, sample_rate_hz; } uiox_hdmi_acr_req_t;

typedef struct {
    uiox_dev_ops_t head;   /* name, probe, init, deinit, ioctl */
} uiox_hdmi_ops_t;

extern const uiox_hdmi_ops_t uiox_hdmi_class_ops;

int uiox_hdmi_probe(void);
/* ═══════════════════════════ END INSERT ═════════════════════════ */                                           

     #ifdef __cplusplus
     }
     #endif
     #endif /* UIOX_HDMI_IF_H */     
 