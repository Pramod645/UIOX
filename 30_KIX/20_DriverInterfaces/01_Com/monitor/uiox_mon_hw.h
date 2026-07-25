/**
 * @file    uiox_mon_hw.h
 * @brief   UIOX Monitor Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to display controller hardware. Owns:
 *   - MMIO register access to display engine / CRTC
 *   - PLL / pixel-clock programming
 *   - DMA / scanout engine (framebuffer → display)
 *   - HDMI/DisplayPort PHY configuration
 *   - MIPI DSI / LVDS transmitter setup
 *   - IRQ handling (VBlank, HBlank, hotplug, FIFO underrun)
 *   - I2C DDC bus for EDID read
 *   - Backlight PWM control
 *
 * Supports:
 *   - Generic MMIO display controller (embedded LCD)
 *   - HDMI 1.4 / 2.0 transmitter
 *   - DisplayPort 1.2 / 1.4 transmitter
 *   - MIPI DSI (1..4 lanes)
 *   - LVDS single / dual link
 *
 * @version 1.0.0
 * @date    2026-05-27
 */
//Layer 1 — Hardware Abstraction
 #ifndef UIOX_MON_HW_H
 #define UIOX_MON_HW_H
 
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_MON_CAP_HDMI           (1u << 0)
 #define UIOX_MON_CAP_DP             (1u << 1)
 #define UIOX_MON_CAP_MIPI_DSI       (1u << 2)
 #define UIOX_MON_CAP_LVDS           (1u << 3)
 #define UIOX_MON_CAP_VGA            (1u << 4)
 #define UIOX_MON_CAP_DMA_FLIP       (1u << 5)   /**< Hardware page flip     */
 #define UIOX_MON_CAP_OVERLAY        (1u << 6)   /**< Hardware overlay plane */
 #define UIOX_MON_CAP_CURSOR         (1u << 7)   /**< Hardware cursor        */
 #define UIOX_MON_CAP_GAMMA          (1u << 8)   /**< Hardware gamma LUT     */
 #define UIOX_MON_CAP_SCALING        (1u << 9)   /**< Hardware scaler        */
 #define UIOX_MON_CAP_HDR            (1u << 10)  /**< HDR10 / HLG support    */
 #define UIOX_MON_CAP_AUDIO          (1u << 11)  /**< Audio over HDMI/DP     */
 #define UIOX_MON_CAP_DDC            (1u << 12)  /**< I2C DDC / EDID         */
 #define UIOX_MON_CAP_BACKLIGHT_PWM  (1u << 13)  /**< PWM backlight control  */
 #define UIOX_MON_CAP_HOTPLUG_IRQ    (1u << 14)  /**< Hotplug detect IRQ     */
 #define UIOX_MON_CAP_VBLANK_IRQ    (1u << 15)  /**< VBlank IRQ             */
 
 /* =========================================================================
  * Interface types
  * ====================================================================== */
 
 typedef enum {
     UIOX_MON_IF_HDMI = 0,
     UIOX_MON_IF_DP,
     UIOX_MON_IF_MIPI_DSI,
     UIOX_MON_IF_LVDS,
     UIOX_MON_IF_VGA,
     UIOX_MON_IF_PARALLEL_RGB,   /**< Embedded parallel RGB LCD            */
 } uiox_mon_if_type_t;
 
 /* =========================================================================
  * Pixel formats
  * ====================================================================== */
 
 typedef enum {
     UIOX_MON_FMT_RGB565   = 0,  /**< 16 bpp packed                        */
     UIOX_MON_FMT_RGB888,         /**< 24 bpp packed                        */
     UIOX_MON_FMT_XRGB8888,       /**< 32 bpp, X ignored                   */
     UIOX_MON_FMT_ARGB8888,       /**< 32 bpp with alpha                   */
     UIOX_MON_FMT_YUV420,
     UIOX_MON_FMT_YUV422,
     UIOX_MON_FMT_YUV444,
 } uiox_mon_pixfmt_t;
 
 /* =========================================================================
  * Display timing (VESA / CEA-861 compatible)
  * ====================================================================== */
 
 typedef struct {
     uint32_t pixel_clk_khz;  /**< Pixel clock in kHz                      */
     uint16_t h_active;       /**< Horizontal active pixels                 */
     uint16_t h_front;        /**< H front porch                           */
     uint16_t h_sync;         /**< H sync width                            */
     uint16_t h_back;         /**< H back porch                            */
     uint16_t v_active;       /**< Vertical active lines                   */
     uint16_t v_front;        /**< V front porch                           */
     uint16_t v_sync;         /**< V sync width                            */
     uint16_t v_back;         /**< V back porch                            */
     bool     h_sync_pos;     /**< H sync polarity: true=positive          */
     bool     v_sync_pos;     /**< V sync polarity: true=positive          */
     bool     interlaced;     /**< Interlaced mode                         */
     uint8_t  refresh_hz;     /**< Refresh rate (Hz)                       */
 } uiox_mon_timing_t;
 
 /* =========================================================================
  * DPMS power state
  * ====================================================================== */
 
 typedef enum {
     UIOX_MON_DPMS_ON = 0,
     UIOX_MON_DPMS_STANDBY,
     UIOX_MON_DPMS_SUSPEND,
     UIOX_MON_DPMS_OFF,
 } uiox_mon_dpms_t;
 
 /* =========================================================================
  * DMA scanout descriptor
  * ====================================================================== */
 
 #define UIOX_MON_DMA_DESC_ALIGN  64
 
 typedef struct __attribute__((packed, aligned(UIOX_MON_DMA_DESC_ALIGN))) {
     volatile uint32_t  status;
     uint32_t           ctrl;
     uint32_t           addr_lo;    /**< Framebuffer physical address lo    */
     uint32_t           addr_hi;    /**< Framebuffer physical address hi    */
     uint32_t           stride;     /**< Bytes per line                     */
     uint32_t           format;     /**< Pixel format code                  */
     uint32_t           reserved[2];
 } uiox_mon_dma_desc_t;
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 typedef struct {
     uintptr_t           base_addr;     /**< MMIO base of display controller*/
     uint32_t            irq_vblank;    /**< VBlank IRQ                     */
     uint32_t            irq_hotplug;   /**< Hotplug detect IRQ             */
     uint32_t            caps;          /**< UIOX_MON_CAP_* bitmask        */
     uiox_mon_if_type_t  if_type;
     uint32_t            pll_ref_hz;    /**< PLL reference clock (Hz)       */
     uint8_t             mipi_lanes;    /**< MIPI DSI lane count (1..4)     */
     uint8_t             lvds_links;    /**< LVDS links (1=single, 2=dual)  */
 
     /* DDC I2C for EDID */
     uint32_t            ddc_i2c_base; /**< DDC I2C controller MMIO base   */
 
     /* Backlight */
     uint32_t            bl_pwm_base;  /**< Backlight PWM MMIO base        */
     uint8_t             bl_level;     /**< Current backlight (0..255)     */
 
     /* Current state */
     uiox_mon_timing_t   timing;
     uiox_mon_pixfmt_t   pixfmt;
     uiox_mon_dpms_t     dpms;
     bool                connected;
     bool                enabled;
 
     /* VBlank sync */
     volatile uint32_t   vblank_count;
     bool                flip_pending;
 
     void               *priv;
 } uiox_mon_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     int  (*init)          (uiox_mon_hw_t *hw);
     void (*deinit)        (uiox_mon_hw_t *hw);
     int  (*enable)        (uiox_mon_hw_t *hw);
     void (*disable)       (uiox_mon_hw_t *hw);
 
     /** Program pixel clock PLL for given timing. */
     int  (*set_timing)    (uiox_mon_hw_t *hw,
                            const uiox_mon_timing_t *t);
 
     /** Set pixel format for scanout engine. */
     int  (*set_pixfmt)    (uiox_mon_hw_t *hw, uiox_mon_pixfmt_t fmt);
 
     /**
      * Schedule a page flip to new framebuffer (takes effect on next VBlank).
      * @param phys  Physical address of new framebuffer.
      * @param stride Bytes per line.
      */
     int  (*flip)          (uiox_mon_hw_t *hw,
                            uintptr_t phys, uint32_t stride);
 
     /** Wait for VBlank (blocking up to timeout_ms). */
     int  (*wait_vblank)   (uiox_mon_hw_t *hw, uint32_t timeout_ms);
 
     /** Read EDID block via DDC I2C. buf must be 128 bytes. */
     int  (*read_edid)     (uiox_mon_hw_t *hw, uint8_t *buf);
 
     /** Set DPMS power state. */
     int  (*set_dpms)      (uiox_mon_hw_t *hw, uiox_mon_dpms_t dpms);
 
     /** Set backlight brightness (0=off, 255=max). */
     int  (*set_backlight) (uiox_mon_hw_t *hw, uint8_t level);
 
     /** Set hardware gamma LUT (256 entries × RGB16). */
     int  (*set_gamma)     (uiox_mon_hw_t *hw,
                            const uint16_t *r,
                            const uint16_t *g,
                            const uint16_t *b);
 
     /** Query hotplug state. true = display connected. */
     bool (*hotplug_state) (uiox_mon_hw_t *hw);
 
     /** Top-half VBlank ISR. */
     void (*isr_vblank)    (uiox_mon_hw_t *hw);
 
     /** Top-half hotplug ISR. */
     void (*isr_hotplug)   (uiox_mon_hw_t *hw);
 
 } uiox_mon_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int  uiox_mon_hw_init        (uiox_mon_hw_t *hw,
                                const uiox_mon_hw_ops_t *ops);
 void uiox_mon_hw_deinit      (uiox_mon_hw_t *hw);
 int  uiox_mon_hw_enable      (uiox_mon_hw_t *hw);
 void uiox_mon_hw_disable     (uiox_mon_hw_t *hw);
 int  uiox_mon_hw_set_timing  (uiox_mon_hw_t *hw,
                                const uiox_mon_timing_t *t);
 int  uiox_mon_hw_flip        (uiox_mon_hw_t *hw,
                                uintptr_t phys, uint32_t stride);
 int  uiox_mon_hw_wait_vblank (uiox_mon_hw_t *hw, uint32_t timeout_ms);
 int  uiox_mon_hw_read_edid   (uiox_mon_hw_t *hw, uint8_t *buf);
 int  uiox_mon_hw_set_dpms    (uiox_mon_hw_t *hw, uiox_mon_dpms_t dpms);
 int  uiox_mon_hw_set_backlight(uiox_mon_hw_t *hw, uint8_t level);
 bool uiox_mon_hw_connected   (uiox_mon_hw_t *hw);
 
 static inline uint32_t uiox_mon_caps(const uiox_mon_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MON_HW_H */
 