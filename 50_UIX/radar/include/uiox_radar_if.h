/**
 * @file    uiox_radar_if.h
 * @brief   UIOX Radar interface driver (SPI config + LVDS/CSI-2 data).
 *
 * Sits between the HAL and sensor abstraction. Configures the high-speed
 * data interface (LVDS or CSI-2) and manages DMA buffer priming.
 *
 * @date    2026-05-26
 */
//Layer 2 — Interface Driver
 #ifndef UIOX_RADAR_IF_H
 #define UIOX_RADAR_IF_H
 
 #include "uiox_radar_hw.h"
 #include "uiox_radar_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_RADAR_IF_LVDS = 0,  /**< LVDS high-speed data interface            */
     UIOX_RADAR_IF_CSI2,      /**< CSI-2 data interface                      */
     UIOX_RADAR_IF_SPI,       /**< SPI (low-speed, debug only)               */
 } uiox_radar_if_type_t;
 
 typedef struct {
     uiox_radar_hw_t       *hw;
     uiox_radar_if_type_t   type;
     uint8_t                num_rx;
     uint8_t                num_tx;
     uint16_t               num_chirps;
     uint16_t               num_samples;
     uiox_radar_adc_fmt_t   adc_fmt;
     uint32_t               frame_bytes;  /**< Computed: rx × chirps × samples × width */
     uint8_t                primed;
 } uiox_radar_if_t;
 
 /**
  * @brief  Configure the radar data interface.
  * @return 0 on success, negative errno on failure.
  */
 int uiox_radar_if_config(uiox_radar_if_t    *rif,
                           uiox_radar_hw_t    *hw,
                           uiox_radar_if_type_t type,
                           uint8_t  num_rx,
                           uint8_t  num_tx,
                           uint16_t num_chirps,
                           uint16_t num_samples,
                           uiox_radar_adc_fmt_t adc_fmt);
 
 /** Prime N raw ADC buffers into the DMA ring. */
 int uiox_radar_if_prime(uiox_radar_if_t *rif, int count);
 
 /**
  * @brief  Poll for a completed raw ADC frame.
  * @return Frame pointer (caller must free), or NULL if none ready.
  */
 uiox_radar_frame_t *uiox_radar_if_dequeue(uiox_radar_if_t *rif);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RADAR_IF_H */
 