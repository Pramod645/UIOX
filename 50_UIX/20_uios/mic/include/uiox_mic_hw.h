/**
 * @file    uiox_mic_hw.h
 * @brief   UIOX Microphone Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to microphone capture hardware. Owns:
 *   - I2S / PDM / TDM controller MMIO register access
 *   - DMA engine for audio sample capture (ADC → memory)
 *   - Audio PLL / MCLK / BCLK / LRCK clock programming
 *   - IRQ handling: DMA half-complete, DMA complete, overrun
 *   - I2C/SPI bus for MEMS codec configuration
 *   - GPIO for enable, mute, power-down pins
 *
 * Supports:
 *   - MEMS microphones: ICS-43434, SPH0645, MP34DT05 (I2S / PDM)
 *   - Digital MEMS: SPM0687, ADMP441, IM69D130 (I2S)
 *   - Analog MEMS via ADC codec: WM8731, TLV320, NAU8822
 *   - TDM multi-mic arrays (up to 8 channels)
 *
 * @version 1.0.0
 * @date    2026-06-03
 */

 #ifndef UIOX_MIC_HW_H
 #define UIOX_MIC_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_MIC_CAP_I2S            (1u << 0)
 #define UIOX_MIC_CAP_PDM            (1u << 1)
 #define UIOX_MIC_CAP_TDM            (1u << 2)
 #define UIOX_MIC_CAP_ANALOG         (1u << 3)
 #define UIOX_MIC_CAP_DMA            (1u << 4)
 #define UIOX_MIC_CAP_STEREO         (1u << 5)
 #define UIOX_MIC_CAP_MULTI_CH       (1u << 6)  /**< > 2 channels (array)  */
 #define UIOX_MIC_CAP_HW_GAIN        (1u << 7)  /**< Hardware gain register */
 #define UIOX_MIC_CAP_HW_MUTE        (1u << 8)  /**< Hardware mute pin      */
 #define UIOX_MIC_CAP_HW_AGC         (1u << 9)  /**< Hardware AGC engine    */
 #define UIOX_MIC_CAP_OVERRUN_IRQ    (1u << 10) /**< FIFO overrun IRQ       */
 #define UIOX_MIC_CAP_WAKEUP         (1u << 11) /**< Wake-word wake-up      */
 #define UIOX_MIC_CAP_HIGH_SNR       (1u << 12) /**< SNR > 65 dB            */
 
 /* =========================================================================
  * Microphone interface type
  * ====================================================================== */
 
 typedef enum {
     UIOX_MIC_IF_I2S = 0,    /**< I2S digital (ICS-43434, ADMP441)        */
     UIOX_MIC_IF_PDM,         /**< PDM (MP34DT05, SPH0645)                */
     UIOX_MIC_IF_TDM,         /**< TDM multi-channel array                */
     UIOX_MIC_IF_ANALOG,      /**< Analog via on-chip ADC                 */
 } uiox_mic_if_type_t;
 
 /* =========================================================================
  * Audio format
  * ====================================================================== */
 
 typedef struct {
     uint32_t  sample_rate;  /**< 8000, 16000, 44100, 48000, 96000        */
     uint8_t   channels;     /**< 1=mono, 2=stereo, 4/6/8=array           */
     uint8_t   bit_depth;    /**< 16, 24, 32                              */
     bool      is_signed;
     bool      big_endian;
 } uiox_mic_audio_fmt_t;
 
 /* =========================================================================
  * DMA descriptor
  * ====================================================================== */
 
 #define UIOX_MIC_DMA_DESC_ALIGN  32
 
 typedef struct __attribute__((packed, aligned(UIOX_MIC_DMA_DESC_ALIGN))) {
     volatile uint32_t  status;
     uint32_t           ctrl;
     uint32_t           buf_lo;
     uint32_t           buf_hi;
     uint32_t           length;
     uint32_t           reserved[3];
 } uiox_mic_dma_desc_t;
 
 #define UIOX_MIC_DESC_OWN   (1u << 31)
 #define UIOX_MIC_DESC_DONE  (1u << 1)
 #define UIOX_MIC_DESC_ERR   (1u << 0)
 #define UIOX_MIC_DESC_LAST  (1u << 30)
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 typedef struct {
     uintptr_t             base_addr;
     uint32_t              irq_dma;
     uint32_t              irq_overrun;
     uint32_t              caps;
     uiox_mic_if_type_t    if_type;
     uint32_t              mclk_hz;
     uint8_t               enable_pin;
     uint8_t               mute_pin;
     uint8_t               pwr_pin;
 
     /* I2C for codec/MEMS config */
     uint32_t              i2c_base;
     uint8_t               codec_i2c_addr;
 
     /* DMA double buffer */
     uiox_mic_dma_desc_t  *dma_ring;
     uint16_t              dma_ring_sz;
     uint16_t              dma_head;
     uint16_t              dma_tail;
 
     /* State */
     uiox_mic_audio_fmt_t  fmt;
     uint8_t               gain_db;       /**< 0..40 dB                   */
     bool                  muted;
     bool                  capturing;
     volatile uint32_t     overrun_count;
     volatile bool         dma_half;
     volatile bool         dma_done;
 
     void                 *priv;
 } uiox_mic_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     int  (*init)          (uiox_mic_hw_t *hw);
     void (*deinit)        (uiox_mic_hw_t *hw);
     int  (*start)         (uiox_mic_hw_t *hw);
     void (*stop)          (uiox_mic_hw_t *hw);
     int  (*set_format)    (uiox_mic_hw_t *hw,
                            const uiox_mic_audio_fmt_t *fmt);
     int  (*set_gain)      (uiox_mic_hw_t *hw, uint8_t gain_db);
     int  (*set_mute)      (uiox_mic_hw_t *hw, bool mute);
     int  (*dma_submit)    (uiox_mic_hw_t *hw,
                            uintptr_t phys, uint32_t bytes, bool last);
     int  (*i2c_read)      (uiox_mic_hw_t *hw,
                            uint8_t dev_addr, uint8_t reg,
                            uint8_t *buf, uint16_t len);
     int  (*i2c_write)     (uiox_mic_hw_t *hw,
                            uint8_t dev_addr, uint8_t reg,
                            const uint8_t *buf, uint16_t len);
     void (*gpio_set)      (uiox_mic_hw_t *hw, uint8_t pin, bool val);
     void (*isr_dma_half)  (uiox_mic_hw_t *hw);
     void (*isr_dma_done)  (uiox_mic_hw_t *hw);
     void (*isr_overrun)   (uiox_mic_hw_t *hw);
 } uiox_mic_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int  uiox_mic_hw_init      (uiox_mic_hw_t *hw,
                              const uiox_mic_hw_ops_t *ops);
 void uiox_mic_hw_deinit    (uiox_mic_hw_t *hw);
 int  uiox_mic_hw_start     (uiox_mic_hw_t *hw);
 void uiox_mic_hw_stop      (uiox_mic_hw_t *hw);
 int  uiox_mic_hw_set_fmt   (uiox_mic_hw_t *hw,
                              const uiox_mic_audio_fmt_t *fmt);
 int  uiox_mic_hw_set_gain  (uiox_mic_hw_t *hw, uint8_t gain_db);
 int  uiox_mic_hw_set_mute  (uiox_mic_hw_t *hw, bool mute);
 int  uiox_mic_hw_dma_submit(uiox_mic_hw_t *hw,
                              uintptr_t phys, uint32_t bytes, bool last);
 
 static inline uint32_t uiox_mic_caps(const uiox_mic_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MIC_HW_H */
 