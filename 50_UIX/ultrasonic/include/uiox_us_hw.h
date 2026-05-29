/**
 * @file    uiox_us_hw.h
 * @brief   UIOX Ultrasonic Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to ultrasonic hardware. Owns:
 *   - GPIO trigger pulse generation (via hardware timer)
 *   - Echo capture (input capture timer or DMA-driven ADC)
 *   - DMA engine for continuous echo sampling
 *   - IRQ handling (echo rise/fall edge detection)
 *   - Clock and reset control
 *
 * Supports:
 *   - GPIO-based sensors (HC-SR04, JSN-SR04T)
 *   - SPI/UART digital sensors (TDC1000, MaxSonar MB series)
 *   - Analog sensors with on-chip ADC
 *
 * @version 1.0.0
 * @date    2026-05-26
 */
//Layer 1 — Hardware Abstraction
 #ifndef UIOX_US_HW_H
 #define UIOX_US_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_US_CAP_GPIO_TRIG       (1u << 0)  /**< GPIO trigger output       */
 #define UIOX_US_CAP_GPIO_ECHO       (1u << 1)  /**< GPIO echo input capture   */
 #define UIOX_US_CAP_ADC_ECHO        (1u << 2)  /**< ADC-based echo sampling   */
 #define UIOX_US_CAP_SPI_SENSOR      (1u << 3)  /**< SPI digital sensor        */
 #define UIOX_US_CAP_UART_SENSOR     (1u << 4)  /**< UART digital sensor       */
 #define UIOX_US_CAP_DMA             (1u << 5)  /**< DMA echo capture          */
 #define UIOX_US_CAP_MULTI_SENSOR    (1u << 6)  /**< Multiple sensor channels  */
 #define UIOX_US_CAP_TEMP_SENSOR     (1u << 7)  /**< On-chip temperature sensor*/
 
 /* =========================================================================
  * Interface types
  * ====================================================================== */
 
 typedef enum {
     UIOX_US_IF_GPIO = 0,   /**< GPIO trigger + echo (HC-SR04 style)         */
     UIOX_US_IF_SPI,        /**< SPI digital interface (TDC1000)             */
     UIOX_US_IF_UART,       /**< UART interface (MaxSonar MB series)         */
     UIOX_US_IF_ADC,        /**< Analog frontend + on-chip ADC               */
 } uiox_us_if_type_t;
 
 /* =========================================================================
  * Trigger pulse configuration
  * ====================================================================== */
 
 typedef struct {
     uint32_t  pulse_width_us;   /**< Trigger pulse width (typ. 10 µs)       */
     uint32_t  period_ms;        /**< Measurement period (ms)                */
     uint32_t  timeout_ms;       /**< Echo timeout (max range equivalent)    */
 } uiox_us_trig_cfg_t;
 
 /* =========================================================================
  * Echo capture mode
  * ====================================================================== */
 
 typedef enum {
     UIOX_US_ECHO_PULSE_WIDTH = 0, /**< Measure echo pulse width (GPIO)      */
     UIOX_US_ECHO_ADC_STREAM,      /**< Stream ADC samples during echo window*/
 } uiox_us_echo_mode_t;
 
 /* =========================================================================
  * DMA descriptor
  * ====================================================================== */
 
 #define UIOX_US_DMA_DESC_ALIGN  32
 
 typedef struct __attribute__((packed, aligned(UIOX_US_DMA_DESC_ALIGN))) {
     volatile uint32_t  status;   /**< OWN bit + done/error flags             */
     uint32_t           ctrl;     /**< Length + interrupt enable              */
     uint32_t           buf_lo;   /**< Buffer physical address (lo 32)        */
     uint32_t           buf_hi;   /**< Buffer physical address (hi 32)        */
     uint32_t           bytes_done; /**< Written by HW on completion          */
     uint32_t           reserved[3];
 } uiox_us_dma_desc_t;
 
 /* Descriptor status bits */
 #define UIOX_US_DESC_OWN    (1u << 31)
 #define UIOX_US_DESC_DONE   (1u << 1)
 #define UIOX_US_DESC_ERR    (1u << 0)
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 typedef struct {
     uintptr_t           base_addr;   /**< MMIO base (timer / capture unit)  */
     uint32_t            irq_echo;    /**< IRQ for echo edge / DMA done       */
     uint32_t            irq_trig;    /**< IRQ for trigger timer              */
     uint32_t            caps;        /**< UIOX_US_CAP_* bitmask             */
     uiox_us_if_type_t   if_type;     /**< Interface type                    */
     uiox_us_echo_mode_t echo_mode;   /**< Echo capture mode                 */
     uint32_t            timer_freq_hz; /**< Timer clock frequency (Hz)      */
     uint8_t             num_channels;  /**< Number of sensor channels        */
 
     /* GPIO pin IDs (platform-specific) */
     uint32_t            trig_pin[4]; /**< Trigger GPIO pins per channel      */
     uint32_t            echo_pin[4]; /**< Echo GPIO pins per channel         */
 
     /* Echo capture state (filled by HAL at ISR time) */
     uint64_t            echo_rise_ticks[4]; /**< Timer ticks at echo rise    */
     uint64_t            echo_fall_ticks[4]; /**< Timer ticks at echo fall    */
     bool                echo_done[4];       /**< Echo capture complete flag  */
     bool                echo_timeout[4];    /**< Echo timeout flag           */
 
     /* DMA (ADC mode) */
     uiox_us_dma_desc_t *rx_ring;
     uint16_t            rx_ring_sz;
     uint16_t            rx_head;
     uint16_t            rx_tail;
 
     void               *priv;        /**< Driver-private data               */
 } uiox_us_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     /** One-time hardware init: clocks, GPIO direction, timer config. */
     int  (*init)          (uiox_us_hw_t *hw);
 
     /** Release all hardware resources. */
     void (*deinit)        (uiox_us_hw_t *hw);
 
     /**
      * Fire a single trigger pulse on the given channel.
      * @param ch  Channel index (0-based).
      */
     int  (*trigger)       (uiox_us_hw_t *hw, uint8_t ch,
                            const uiox_us_trig_cfg_t *cfg);
 
     /**
      * Wait for echo capture complete on channel ch.
      * Blocks up to timeout_ms; fills hw->echo_rise_ticks / fall_ticks.
      * Returns echo width in timer ticks, or <0 on timeout/error.
      */
     int64_t (*echo_wait)  (uiox_us_hw_t *hw, uint8_t ch,
                            uint32_t timeout_ms);
 
     /** Read current timer counter value. */
     uint64_t (*timer_now) (uiox_us_hw_t *hw);
 
     /** Queue a DMA buffer for ADC echo capture (ADC mode). */
     int  (*dma_queue)     (uiox_us_hw_t *hw,
                            uintptr_t phys, uint32_t length);
 
     /** Poll for completed ADC echo buffer. */
     int  (*dma_complete)  (uiox_us_hw_t *hw,
                            uintptr_t *phys_out, uint32_t *bytes_out);
 
     /** Read temperature from on-chip sensor (milli-Celsius). */
     int  (*read_temp_mc)  (uiox_us_hw_t *hw, int32_t *milli_celsius);
 
     /** SPI read (digital sensor config). */
     int  (*spi_read)      (uiox_us_hw_t *hw, uint8_t addr, uint8_t *val);
 
     /** SPI write (digital sensor config). */
     int  (*spi_write)     (uiox_us_hw_t *hw, uint8_t addr, uint8_t val);
 
     /** UART read (digital sensor data byte). */
     int  (*uart_read)     (uiox_us_hw_t *hw, uint8_t *buf, uint16_t len);
 
     /** UART write (digital sensor command). */
     int  (*uart_write)    (uiox_us_hw_t *hw,
                            const uint8_t *buf, uint16_t len);
 
     /** Top-half ISR (called from interrupt context). */
     void (*isr)           (uiox_us_hw_t *hw);
 
 } uiox_us_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int      uiox_us_hw_init   (uiox_us_hw_t *hw, const uiox_us_hw_ops_t *ops);
 int      uiox_us_hw_trigger(uiox_us_hw_t *hw, uint8_t ch,
                              const uiox_us_trig_cfg_t *cfg);
 int64_t  uiox_us_hw_echo_wait(uiox_us_hw_t *hw, uint8_t ch,
                                uint32_t timeout_ms);
 int      uiox_us_hw_read_temp(uiox_us_hw_t *hw, int32_t *milli_celsius);
 void     uiox_us_hw_deinit (uiox_us_hw_t *hw);
 
 static inline uint32_t uiox_us_caps(const uiox_us_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_US_HW_H */
 