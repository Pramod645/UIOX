/**
 * @file    uiox_radar_hw.h
 * @brief   UIOX Radar Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to radar frontend hardware. Owns:
 *   - MMIO register access to radar baseband / DSP accelerator
 *   - DMA engine for ADC raw data capture
 *   - Clock / PLL / reset control
 *   - IRQ top-half handling
 *
 * Supports TI AWR / IWR class mmWave radar SoCs and generic
 * FMCW radar frontends connected over SPI + LVDS / CSI-2.
 *
 * @version 1.0.0
 * @date    2026-05-26
 */
//Layer 1 — Hardware Abstraction
 #ifndef UIOX_RADAR_HW_H
 #define UIOX_RADAR_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_RADAR_CAP_LVDS         (1u << 0)  /**< LVDS data interface       */
 #define UIOX_RADAR_CAP_CSI2         (1u << 1)  /**< CSI-2 data interface      */
 #define UIOX_RADAR_CAP_SPI_CFG      (1u << 2)  /**< SPI config interface       */
 #define UIOX_RADAR_CAP_DMA_CONTIG   (1u << 3)  /**< Contiguous DMA            */
 #define UIOX_RADAR_CAP_HW_FFT       (1u << 4)  /**< Hardware FFT accelerator  */
 #define UIOX_RADAR_CAP_HW_CFAR      (1u << 5)  /**< Hardware CFAR engine      */
 #define UIOX_RADAR_CAP_MULTI_RX     (1u << 6)  /**< Multiple RX antennas      */
 #define UIOX_RADAR_CAP_MULTI_TX     (1u << 7)  /**< Multiple TX antennas      */
 
 /* =========================================================================
  * ADC data format
  * ====================================================================== */
 
 typedef enum {
     UIOX_RADAR_ADC_COMPLEX_1X = 0,  /**< I+Q interleaved, 1 sample/clock    */
     UIOX_RADAR_ADC_REAL_1X,         /**< Real only, 1 sample/clock          */
     UIOX_RADAR_ADC_COMPLEX_2X,      /**< I+Q, 2 samples/clock (LVDS DDR)    */
 } uiox_radar_adc_fmt_t;
 
 /* =========================================================================
  * DMA descriptor
  * ====================================================================== */
 
 #define UIOX_RADAR_DMA_DESC_ALIGN   64
 
 typedef struct __attribute__((packed, aligned(UIOX_RADAR_DMA_DESC_ALIGN))) {
     volatile uint32_t   status;      /**< OWN bit + error flags              */
     uint32_t            ctrl;        /**< Length + interrupt enable          */
     uint32_t            buf_lo;      /**< Buffer physical address (lo 32)    */
     uint32_t            buf_hi;      /**< Buffer physical address (hi 32)    */
     uint32_t            next_lo;     /**< Next descriptor physical addr (lo) */
     uint32_t            next_hi;
     uint32_t            bytes_done;  /**< Written by HW on completion        */
     uint32_t            reserved;
 } uiox_radar_dma_desc_t;
 
 /* Descriptor status bits */
 #define UIOX_RADAR_DESC_OWN     (1u << 31)
 #define UIOX_RADAR_DESC_ERR     (1u << 0)
 #define UIOX_RADAR_DESC_DONE    (1u << 1)
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 typedef struct {
     uintptr_t               base_addr;  /**< MMIO base of radar baseband    */
     uint32_t                irq;        /**< IRQ line for DMA/frame-done     */
     uint32_t                caps;       /**< UIOX_RADAR_CAP_* bitmask       */
     uint8_t                 num_rx;     /**< Number of RX antennas           */
     uint8_t                 num_tx;     /**< Number of TX antennas           */
     uint16_t                adc_bits;   /**< ADC resolution (12, 14, 16)     */
     uiox_radar_adc_fmt_t    adc_fmt;    /**< ADC data format                 */
 
     /* DMA rings */
     uiox_radar_dma_desc_t  *rx_ring;    /**< RX DMA descriptor ring          */
     uint16_t                rx_ring_sz; /**< Number of descriptors           */
     uint16_t                rx_head;    /**< Next descriptor to fill         */
     uint16_t                rx_tail;    /**< Next descriptor to reclaim      */
 
     void                   *priv;       /**< Driver-private data             */
 } uiox_radar_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     /** One-time HW init: clocks, PLLs, resets, DMA ring setup. */
     int  (*init)         (uiox_radar_hw_t *hw);
 
     /** Release all HW resources. */
     void (*deinit)       (uiox_radar_hw_t *hw);
 
     /** Start ADC capture engine and enable DMA. */
     int  (*start)        (uiox_radar_hw_t *hw);
 
     /** Stop ADC capture engine. */
     void (*stop)         (uiox_radar_hw_t *hw);
 
     /**
      * Configure ADC capture parameters.
      * @param num_samples   ADC samples per chirp per RX channel.
      * @param num_chirps    Chirps per frame.
      * @param fmt           ADC data format.
      */
     int  (*set_adc)      (uiox_radar_hw_t *hw,
                           uint16_t num_samples, uint16_t num_chirps,
                           uiox_radar_adc_fmt_t fmt);
 
     /**
      * Queue a DMA buffer for the next ADC frame.
      * @param phys   Physical address of buffer.
      * @param length Buffer size in bytes.
      */
     int  (*dma_queue)    (uiox_radar_hw_t *hw,
                           uintptr_t phys, uint32_t length);
 
     /**
      * Poll for a completed ADC frame.
      * @param phys_out   Physical address of completed buffer.
      * @param bytes_out  Bytes captured.
      * @return           >0 bytes captured, 0 if none ready, <0 on error.
      */
     int  (*dma_complete) (uiox_radar_hw_t *hw,
                           uintptr_t *phys_out, uint32_t *bytes_out);
 
     /** Top-half ISR (called from interrupt context). */
     void (*isr)          (uiox_radar_hw_t *hw);
 
     /** Read SPI configuration register from radar frontend. */
     int  (*spi_read)     (uiox_radar_hw_t *hw,
                           uint16_t addr, uint32_t *val);
 
     /** Write SPI configuration register to radar frontend. */
     int  (*spi_write)    (uiox_radar_hw_t *hw,
                           uint16_t addr, uint32_t val);
 
 } uiox_radar_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int  uiox_radar_hw_init  (uiox_radar_hw_t *hw, const uiox_radar_hw_ops_t *ops);
 int  uiox_radar_hw_start (uiox_radar_hw_t *hw);
 void uiox_radar_hw_stop  (uiox_radar_hw_t *hw);
 void uiox_radar_hw_deinit(uiox_radar_hw_t *hw);
 
 static inline uint32_t uiox_radar_caps(const uiox_radar_hw_t *hw)
 { return hw ? hw->caps : 0; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RADAR_HW_H */
 