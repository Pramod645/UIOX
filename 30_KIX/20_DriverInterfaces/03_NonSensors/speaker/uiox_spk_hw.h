/**
 * @file    uiox_spk_hw.h
 * @brief   UIOX Music Speaker Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to audio output hardware. Owns:
 *   - I2S / PCM / PDM controller MMIO register access
 *   - DMA engine for audio sample transfer (CPU → DAC)
 *   - Audio PLL / MCLK / BCLK / LRCK clock programming
 *   - IRQ handling: DMA half-complete, DMA complete, underrun
 *   - I2C/SPI bus for codec / amplifier configuration
 *   - GPIO for mute, power-down, fault pins
 *
 * Supports:
 *   - I2S (Philips format, left/right justified)
 *   - PCM (short/long frame sync)
 *   - PDM (pulse-density modulation, e.g. MEMS mic out / speaker in)
 *   - TDM (time-division multiplexed, multi-channel)
 *   - Codecs: TI TAS5756, MAX98357, NAU8822, WM8960
 *   - Class-D amplifier with I2C gain control
 *
 * @version 1.0.0
 * @date    2026-06-01
 */

 #ifndef UIOX_SPK_HW_H
 #define UIOX_SPK_HW_H
 
#include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_SPK_CAP_I2S            (1u << 0)
 #define UIOX_SPK_CAP_PCM            (1u << 1)
 #define UIOX_SPK_CAP_PDM            (1u << 2)
 #define UIOX_SPK_CAP_TDM            (1u << 3)
 #define UIOX_SPK_CAP_DMA            (1u << 4)
 #define UIOX_SPK_CAP_HW_VOLUME      (1u << 5)  /**< HW volume register    */
 #define UIOX_SPK_CAP_HW_MUTE        (1u << 6)  /**< HW mute pin           */
 #define UIOX_SPK_CAP_HW_EQ          (1u << 7)  /**< Codec HW EQ bands     */
 #define UIOX_SPK_CAP_STEREO         (1u << 8)
 #define UIOX_SPK_CAP_SURROUND       (1u << 9)  /**< 5.1 / 7.1             */
 #define UIOX_SPK_CAP_BASS_BOOST     (1u << 10)
 #define UIOX_SPK_CAP_3D_EFFECT      (1u << 11)
 #define UIOX_SPK_CAP_SPDIF_OUT      (1u << 12)
 #define UIOX_SPK_CAP_FAULT_IRQ      (1u << 13) /**< Amplifier fault IRQ   */
 #define UIOX_SPK_CAP_THERMAL        (1u << 14) /**< Thermal monitor       */
 
 /* =========================================================================
  * Audio interface type
  * ====================================================================== */
 
 typedef enum {
     UIOX_SPK_IF_I2S = 0,
     UIOX_SPK_IF_PCM,
     UIOX_SPK_IF_PDM,
     UIOX_SPK_IF_TDM,
     UIOX_SPK_IF_PWM,       /**< Simple PWM buzzer                         */
 } uiox_spk_if_type_t;
 
 /* =========================================================================
  * Audio format
  * ====================================================================== */
 
 typedef struct {
     uint32_t  sample_rate;  /**< 8000, 16000, 44100, 48000, 96000, 192000  */
     uint8_t   channels;     /**< 1=mono, 2=stereo, 6=5.1, 8=7.1           */
     uint8_t   bit_depth;    /**< 8, 16, 24, 32                             */
     bool      is_signed;    /**< true = signed PCM (standard)              */
     bool      big_endian;   /**< false = little-endian (standard)          */
 } uiox_spk_audio_fmt_t;
 
 /* =========================================================================
  * DMA descriptor
  * ====================================================================== */
 
 #define UIOX_SPK_DMA_DESC_ALIGN  32
 
 typedef struct __attribute__((packed, aligned(UIOX_SPK_DMA_DESC_ALIGN))) {
     volatile uint32_t  status;
     uint32_t           ctrl;
     uint32_t           buf_lo;
     uint32_t           buf_hi;
     uint32_t           length;      /**< Bytes in this descriptor          */
     uint32_t           reserved[3];
 } uiox_spk_dma_desc_t;
 
 #define UIOX_SPK_DESC_OWN   (1u << 31)
 #define UIOX_SPK_DESC_DONE  (1u << 1)
 #define UIOX_SPK_DESC_ERR   (1u << 0)
 #define UIOX_SPK_DESC_LAST  (1u << 30)  /**< Last in chain → wrap          */
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 typedef struct {
     uintptr_t             base_addr;    /**< I2S/PCM MMIO base             */
     uint32_t              irq_dma;      /**< DMA complete IRQ              */
     uint32_t              irq_fault;    /**< Amplifier fault IRQ           */
     uint32_t              caps;
     uiox_spk_if_type_t    if_type;
     uint32_t              mclk_hz;      /**< Master clock frequency        */
     uint8_t               mute_pin;     /**< GPIO pin for mute (0=none)    */
     uint8_t               pwr_pin;      /**< GPIO power-down pin (0=none)  */
 
     /* I2C for codec */
     uint32_t              i2c_base;
     uint8_t               codec_i2c_addr;
 
     /* DMA double buffer */
     uiox_spk_dma_desc_t  *dma_ring;
     uint16_t              dma_ring_sz;
     uint16_t              dma_head;
     uint16_t              dma_tail;
 
     /* State */
     uiox_spk_audio_fmt_t  fmt;
     uint8_t               volume;       /**< 0..100 %                      */
     bool                  muted;
     bool                  playing;
     volatile uint32_t     underrun_count;
     volatile bool         dma_half;     /**< Half-complete flag            */
     volatile bool         dma_done;     /**< Full-complete flag            */
 
     void                 *priv;
 } uiox_spk_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     int  (*init)          (uiox_spk_hw_t *hw);
     void (*deinit)        (uiox_spk_hw_t *hw);
     int  (*start)         (uiox_spk_hw_t *hw);
     void (*stop)          (uiox_spk_hw_t *hw);
     int  (*set_format)    (uiox_spk_hw_t *hw,
                            const uiox_spk_audio_fmt_t *fmt);
     int  (*set_volume)    (uiox_spk_hw_t *hw, uint8_t vol_pct);
     int  (*set_mute)      (uiox_spk_hw_t *hw, bool mute);
     int  (*dma_submit)    (uiox_spk_hw_t *hw,
                            uintptr_t phys, uint32_t bytes, bool last);
     int  (*dma_flush)     (uiox_spk_hw_t *hw);
     int  (*i2c_read)      (uiox_spk_hw_t *hw,
                            uint8_t dev_addr, uint8_t reg,
                            uint8_t *buf, uint16_t len);
     int  (*i2c_write)     (uiox_spk_hw_t *hw,
                            uint8_t dev_addr, uint8_t reg,
                            const uint8_t *buf, uint16_t len);
     void (*gpio_set)      (uiox_spk_hw_t *hw, uint8_t pin, bool val);
     void (*isr_dma_half)  (uiox_spk_hw_t *hw);
     void (*isr_dma_done)  (uiox_spk_hw_t *hw);
     void (*isr_fault)     (uiox_spk_hw_t *hw);
 } uiox_spk_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int  uiox_spk_hw_init      (uiox_spk_hw_t *hw,
                              const uiox_spk_hw_ops_t *ops);
 void uiox_spk_hw_deinit    (uiox_spk_hw_t *hw);
 int  uiox_spk_hw_start     (uiox_spk_hw_t *hw);
 void uiox_spk_hw_stop      (uiox_spk_hw_t *hw);
 int  uiox_spk_hw_set_fmt   (uiox_spk_hw_t *hw,
                              const uiox_spk_audio_fmt_t *fmt);
 int  uiox_spk_hw_set_volume(uiox_spk_hw_t *hw, uint8_t vol_pct);
 int  uiox_spk_hw_set_mute  (uiox_spk_hw_t *hw, bool mute);
 int  uiox_spk_hw_dma_submit(uiox_spk_hw_t *hw,
                              uintptr_t phys, uint32_t bytes, bool last);
 
 static inline uint32_t uiox_spk_caps(const uiox_spk_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SPK_HW_H */
 