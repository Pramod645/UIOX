/**
 * @file    uiox_us_if.h
 * @brief   UIOX Ultrasonic interface driver (GPIO / SPI / UART / ADC).
 *
 * Sits between HAL and sensor abstraction. Manages:
 *   - Trigger pulse generation per channel
 *   - Echo capture (pulse-width or ADC stream)
 *   - Multi-channel multiplexing
 *
 * @date    2026-05-26
 */
//Layer 2 — Interface Driver
 #ifndef UIOX_US_IF_H
 #define UIOX_US_IF_H
 
 #include "uiox_us_hw.h"
 #include "uiox_us_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_us_hw_t       *hw;
     uiox_us_if_type_t   type;
     uint8_t             num_channels;
     uiox_us_trig_cfg_t  trig_cfg;
     uint32_t            sample_rate_hz;  /**< ADC rate (ADC mode only)      */
     uint32_t            echo_window_us;  /**< ADC capture window (µs)       */
     bool                primed;
 } uiox_us_if_t;
 
 int uiox_us_if_config(uiox_us_if_t    *uif,
                        uiox_us_hw_t    *hw,
                        uiox_us_if_type_t type,
                        uint8_t          num_channels,
                        uint32_t         pulse_width_us,
                        uint32_t         period_ms,
                        uint32_t         timeout_ms,
                        uint32_t         sample_rate_hz,
                        uint32_t         echo_window_us);
 
 /**
  * @brief  Fire trigger on channel ch and capture echo.
  *
  * GPIO mode : returns echo pulse width in timer ticks.
  * ADC mode  : fills a raw frame with ADC samples.
  *
  * @return Echo ticks (GPIO) or bytes captured (ADC), <0 on error.
  */
 int64_t uiox_us_if_measure(uiox_us_if_t *uif, uint8_t ch,
                              uiox_us_frame_t **raw_out);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_US_IF_H */
 