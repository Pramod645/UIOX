/**
 * @file    uiox_hdmi_hw.h
 * @brief   UIOX HDMI Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to HDMI TX hardware. Owns:
 *   - MMIO register access to HDMI controller / display engine
 *   - TMDS (Transition-Minimised Differential Signalling) PHY
 *   - FRL (Fixed Rate Link) PHY for HDMI 2.1
 *   - Pixel clock PLL programming
 *   - DDC (Display Data Channel) I2C for EDID and HDCP
 *   - CEC (Consumer Electronics Control) GPIO/UART
 *   - IRQ handling: hotplug, HDCP, VBlank, audio FIFO
 *   - HDCP 1.4 / 2.3 engine registers
 *   - Audio sample injection (IEC 60958 / SPDIF)
 *
 * Supports:
 *   - HDMI 1.4  (up to 4K@30, 3D, Audio Return Channel)
 *   - HDMI 2.0  (up to 4K@60, HDR, 32-ch audio)
 *   - HDMI 2.1  (up to 10K, FRL, eARC, VRR, ALLM)
 *
 * @version 1.0.0
 * @date    2026-05-28
 */
//Layer 1 — Hardware Abstraction
 #ifndef UIOX_HDMI_HW_H
 #define UIOX_HDMI_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_HDMI_CAP_HDMI14        (1u << 0)  /**< HDMI 1.4 (4K@30)     */
 #define UIOX_HDMI_CAP_HDMI20        (1u << 1)  /**< HDMI 2.0 (4K@60)     */
 #define UIOX_HDMI_CAP_HDMI21        (1u << 2)  /**< HDMI 2.1 (FRL/10K)   */
 #define UIOX_HDMI_CAP_HDCP14        (1u << 3)  /**< HDCP 1.4              */
 #define UIOX_HDMI_CAP_HDCP23        (1u << 4)  /**< HDCP 2.3              */
 #define UIOX_HDMI_CAP_CEC           (1u << 5)  /**< Consumer Electronics  */
 #define UIOX_HDMI_CAP_ARC           (1u << 6)  /**< Audio Return Channel  */
 #define UIOX_HDMI_CAP_EARC          (1u << 7)  /**< Enhanced ARC          */
 #define UIOX_HDMI_CAP_HDR10         (1u << 8)  /**< HDR10 static metadata */
 #define UIOX_HDMI_CAP_HLGDOLBY     (1u << 9)  /**< HLG / Dolby Vision    */
 #define UIOX_HDMI_CAP_VRR           (1u << 10) /**< Variable Refresh Rate */
 #define UIOX_HDMI_CAP_ALLM          (1u << 11) /**< Auto Low Latency Mode */
 #define UIOX_HDMI_CAP_DSC           (1u << 12) /**< Display Stream Compr. */
 #define UIOX_HDMI_CAP_FRL           (1u << 13) /**< Fixed Rate Link lanes */
 #define UIOX_HDMI_CAP_AUDIO_HBR     (1u << 14) /**< High Bit Rate audio   */
 #define UIOX_HDMI_CAP_3D            (1u << 15) /**< 3D frame packing      */
 
 /* =========================================================================
  * HDMI version
  * ====================================================================== */
 
 typedef enum {
     UIOX_HDMI_VER_14 = 0,
     UIOX_HDMI_VER_20,
     UIOX_HDMI_VER_21,
 } uiox_hdmi_ver_t;
 
 /* =========================================================================
  * Link type
  * ====================================================================== */
 
 typedef enum {
     UIOX_HDMI_LINK_TMDS = 0,  /**< Classic TMDS (HDMI 1.4 / 2.0)        */
     UIOX_HDMI_LINK_FRL,        /**< Fixed Rate Link (HDMI 2.1)            */
 } uiox_hdmi_link_t;
 
 /* =========================================================================
  * FRL rate (Gbps per lane × lanes)
  * ====================================================================== */
 
 typedef enum {
     UIOX_HDMI_FRL_3G3L  = 0,  /**< 3 Gbps × 3 lanes =  9 Gbps           */
     UIOX_HDMI_FRL_6G3L,        /**< 6 Gbps × 3 lanes = 18 Gbps           */
     UIOX_HDMI_FRL_6G4L,        /**< 6 Gbps × 4 lanes = 24 Gbps           */
     UIOX_HDMI_FRL_8G4L,        /**< 8 Gbps × 4 lanes = 32 Gbps           */
     UIOX_HDMI_FRL_10G4L,       /**< 10 Gbps × 4 lanes = 40 Gbps          */
     UIOX_HDMI_FRL_12G4L,       /**< 12 Gbps × 4 lanes = 48 Gbps          */
 } uiox_hdmi_frl_rate_t;
 
 /* =========================================================================
  * HDMI colour space / quantisation
  * ====================================================================== */
 
 typedef enum {
     UIOX_HDMI_CS_RGB = 0,
     UIOX_HDMI_CS_YCbCr444,
     UIOX_HDMI_CS_YCbCr422,
     UIOX_HDMI_CS_YCbCr420,
 } uiox_hdmi_colorspace_t;
 
 typedef enum {
     UIOX_HDMI_BPC_8  = 0,
     UIOX_HDMI_BPC_10,
     UIOX_HDMI_BPC_12,
     UIOX_HDMI_BPC_16,
 } uiox_hdmi_bpc_t;
 
 /* =========================================================================
  * HDMI display timing (reuses uiox_mon_timing_t structure concept)
  * ====================================================================== */
 
 typedef struct {
     uint32_t  pixel_clk_khz;
     uint16_t  h_active, h_front, h_sync, h_back;
     uint16_t  v_active, v_front, v_sync, v_back;
     bool      h_sync_pos, v_sync_pos, interlaced;
     uint8_t   refresh_hz;
     uint8_t   vic;            /**< CEA-861 Video Identification Code      */
 } uiox_hdmi_timing_t;
 
 /* =========================================================================
  * HDCP state
  * ====================================================================== */
 
 typedef enum {
     UIOX_HDCP_DISABLED = 0,
     UIOX_HDCP_AUTHENTICATING,
     UIOX_HDCP_AUTHENTICATED,
     UIOX_HDCP_FAILED,
 } uiox_hdcp_state_t;
 
 /* =========================================================================
  * Audio configuration
  * ====================================================================== */
 
 typedef enum {
     UIOX_HDMI_AUDIO_LPCM = 0, /**< Linear PCM (IEC 60958)               */
     UIOX_HDMI_AUDIO_HBR,       /**< High Bit Rate (DTS-HD, TrueHD)      */
     UIOX_HDMI_AUDIO_DSD,       /**< Direct Stream Digital               */
 } uiox_hdmi_audio_fmt_t;
 
 typedef struct {
    uiox_hdmi_audio_fmt_t fmt;
    uint32_t  sample_rate_hz;
    uint8_t   channels;
    uint8_t   bits_per_sample;
} uiox_hdmi_audio_cfg_t;

/* =========================================================================
 * Hardware device descriptor
 * ====================================================================== */

typedef struct {
    uintptr_t             base_addr;     /**< MMIO base of HDMI controller */
    uint32_t              irq_hpd;       /**< Hot-plug detect IRQ           */
    uint32_t              irq_hdcp;      /**< HDCP engine IRQ               */
    uint32_t              irq_vblank;    /**< VBlank IRQ                    */
    uint32_t              irq_audio;     /**< Audio FIFO underrun IRQ       */
    uint32_t              caps;          /**< UIOX_HDMI_CAP_* bitmask      */
    uiox_hdmi_ver_t       version;
    uiox_hdmi_link_t      link;
    uiox_hdmi_frl_rate_t  frl_rate;
    uint32_t              pll_ref_hz;    /**< PLL reference clock (Hz)      */

    /* Current config */
    uiox_hdmi_timing_t    timing;
    uiox_hdmi_colorspace_t colorspace;
    uiox_hdmi_bpc_t       bpc;
    uiox_hdmi_audio_cfg_t audio;

    /* State */
    bool                  connected;
    bool                  enabled;
    uiox_hdcp_state_t     hdcp_state;
    volatile uint32_t     vblank_count;
    bool                  flip_pending;

    /* DDC I2C */
    uint32_t              ddc_base;      /**< DDC I2C MMIO base             */

    /* CEC */
    uint32_t              cec_base;      /**< CEC controller MMIO base      */
    uint8_t               cec_la;        /**< CEC logical address           */

    /* Backlight (panel with embedded HDMI) */
    uint8_t               bl_level;

    void                 *priv;
} uiox_hdmi_hw_t;

/* =========================================================================
 * Hardware operations vtable
 * ====================================================================== */

typedef struct {
    int  (*init)           (uiox_hdmi_hw_t *hw);
    void (*deinit)         (uiox_hdmi_hw_t *hw);
    int  (*enable)         (uiox_hdmi_hw_t *hw);
    void (*disable)        (uiox_hdmi_hw_t *hw);
    int  (*set_timing)     (uiox_hdmi_hw_t *hw,
                            const uiox_hdmi_timing_t *t);
    int  (*set_colorspace) (uiox_hdmi_hw_t *hw,
                            uiox_hdmi_colorspace_t cs,
                            uiox_hdmi_bpc_t bpc);
    int  (*set_audio)      (uiox_hdmi_hw_t *hw,
                            const uiox_hdmi_audio_cfg_t *a);
    int  (*set_frl_rate)   (uiox_hdmi_hw_t *hw,
                            uiox_hdmi_frl_rate_t rate);
    int  (*phy_power)      (uiox_hdmi_hw_t *hw, bool on);
    int  (*pll_set)        (uiox_hdmi_hw_t *hw, uint32_t pixel_clk_khz);
    int  (*wait_vblank)    (uiox_hdmi_hw_t *hw, uint32_t timeout_ms);
    int  (*flip)           (uiox_hdmi_hw_t *hw,
                            uintptr_t phys, uint32_t stride);

    /* DDC / EDID */
    int  (*ddc_read)       (uiox_hdmi_hw_t *hw,
                            uint8_t dev_addr, uint8_t reg,
                            uint8_t *buf, uint16_t len);
    int  (*ddc_write)      (uiox_hdmi_hw_t *hw,
                            uint8_t dev_addr, uint8_t reg,
                            const uint8_t *buf, uint16_t len);

    /* HDCP */
    int  (*hdcp_start)     (uiox_hdmi_hw_t *hw, uint8_t version);
    void (*hdcp_stop)      (uiox_hdmi_hw_t *hw);
    int  (*hdcp_status)    (uiox_hdmi_hw_t *hw,
                            uiox_hdcp_state_t *state_out);

    /* CEC */
    int  (*cec_send)       (uiox_hdmi_hw_t *hw,
                            uint8_t dst_la,
                            const uint8_t *msg, uint8_t len);
    int  (*cec_recv)       (uiox_hdmi_hw_t *hw,
                            uint8_t *src_la,
                            uint8_t *msg, uint8_t *len);

    /* Infoframe injection */
    int  (*infoframe_send) (uiox_hdmi_hw_t *hw,
                            const uint8_t *packet, uint8_t len);

    /* Audio sample injection */
    int  (*audio_write)    (uiox_hdmi_hw_t *hw,
                            const uint8_t *samples, uint32_t bytes);

    /* Hotplug */
    bool (*hpd_state)      (uiox_hdmi_hw_t *hw);

    /* ISRs */
    void (*isr_hpd)        (uiox_hdmi_hw_t *hw);
    void (*isr_hdcp)       (uiox_hdmi_hw_t *hw);
    void (*isr_vblank)     (uiox_hdmi_hw_t *hw);
    void (*isr_audio)      (uiox_hdmi_hw_t *hw);
} uiox_hdmi_hw_ops_t;

/* =========================================================================
 * HAL public API
 * ====================================================================== */

int  uiox_hdmi_hw_init       (uiox_hdmi_hw_t *hw,
                               const uiox_hdmi_hw_ops_t *ops);
void uiox_hdmi_hw_deinit     (uiox_hdmi_hw_t *hw);
int  uiox_hdmi_hw_enable     (uiox_hdmi_hw_t *hw);
void uiox_hdmi_hw_disable    (uiox_hdmi_hw_t *hw);
int  uiox_hdmi_hw_set_timing (uiox_hdmi_hw_t *hw,
                               const uiox_hdmi_timing_t *t);
int  uiox_hdmi_hw_flip       (uiox_hdmi_hw_t *hw,
                               uintptr_t phys, uint32_t stride);
int  uiox_hdmi_hw_wait_vblank(uiox_hdmi_hw_t *hw, uint32_t timeout_ms);
int  uiox_hdmi_hw_ddc_read   (uiox_hdmi_hw_t *hw, uint8_t dev_addr,
                               uint8_t reg, uint8_t *buf, uint16_t len);
bool uiox_hdmi_hw_connected  (uiox_hdmi_hw_t *hw);

static inline uint32_t uiox_hdmi_caps(const uiox_hdmi_hw_t *hw)
{ return hw ? hw->caps : 0u; }

#ifdef __cplusplus
}
#endif
#endif /* UIOX_HDMI_HW_H */
