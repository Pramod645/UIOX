/**
 * @file    uiox_therm_if.h
 * @brief   UIOX Thermal Sensor interface driver.
 * @date    2026-06-05
 */

 #ifndef UIOX_THERM_IF_H
 #define UIOX_THERM_IF_H
 
 #include "uiox_therm_hw.h"
 #include "uiox_therm_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * PCT2075 / LM75-compatible register map
  * ====================================================================== */
 
 #define UIOX_REG_THERM_TEMP      0x00u  /**< Temperature register         */
 #define UIOX_REG_THERM_CONF      0x01u  /**< Configuration                */
 #define UIOX_REG_THERM_T_HYST   0x02u  /**< Hysteresis register           */
 #define UIOX_REG_THERM_T_OS     0x03u  /**< Over-temp shutdown register   */
 #define UIOX_REG_THERM_T_IDLE   0x04u  /**< Idle (PCT2075 specific)      */
 
 /* Configuration register bits (PCT2075/LM75) */
 #define UIOX_THERM_CONF_SD      (1u << 0)  /**< Shutdown mode             */
 #define UIOX_THERM_CONF_CMP_INT (1u << 1)  /**< Comparator/interrupt mode */
 #define UIOX_THERM_CONF_POL     (1u << 2)  /**< Alert polarity            */
 #define UIOX_THERM_CONF_FQ_MASK (3u << 3)  /**< Fault queue bits          */
 #define UIOX_THERM_CONF_RES_MASK (3u << 5)  /**< Resolution bits (TMP112)  */
 
 /* =========================================================================
  * Interface statistics
  * ====================================================================== */
 
 typedef struct {
     uint64_t  measurements;
     uint32_t  irq_count;
     uint32_t  alert_count;
     uint32_t  error_count;
     uint32_t  comm_errors;
 } uiox_therm_if_stats_t;
 
 /* =========================================================================
  * Interface descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_therm_hw_t       *hw;
     uiox_therm_if_stats_t  stats;
     uint8_t                device_id;
     bool                   primed;
 } uiox_therm_if_t;
 
 /* =========================================================================
  * Interface API
  * ====================================================================== */
 
 int  uiox_therm_if_config     (uiox_therm_if_t *tif, uiox_therm_hw_t *hw);
 int  uiox_therm_if_start      (uiox_therm_if_t *tif);
 void uiox_therm_if_stop       (uiox_therm_if_t *tif);
 
 /** Read all channels; push alert events if thresholds exceeded. */
 int  uiox_therm_if_measure    (uiox_therm_if_t *tif, uint32_t now_ms);
 
 /** Handle alert IRQ — read status, push events. */
 int  uiox_therm_if_irq_handle (uiox_therm_if_t *tif, uint32_t now_ms);
 
 /** Collect telemetry snapshot. */
 int  uiox_therm_if_telemetry  (uiox_therm_if_t *tif,
                                 uiox_therm_telem_t *out, uint32_t now_ms);
 
 /** Set alert thresholds (°C × 10). */
 int  uiox_therm_if_set_alert  (uiox_therm_if_t *tif,
                                 int16_t t_high_dc, int16_t t_hyst_dc);
 
 void uiox_therm_if_stats_get  (const uiox_therm_if_t *tif,
                                 uiox_therm_if_stats_t *out);
 void uiox_therm_if_stats_reset(uiox_therm_if_t *tif);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_THERM_IF_H */
 