/**
 * @file    uiox_therm_sensor.h
 * @brief   UIOX Thermal Sensor abstraction (NTC, digital ICs).
 * @date    2026-06-05
 */
//Layer 2b — Sensor Abstraction
 #ifndef UIOX_THERM_SENSOR_H
 #define UIOX_THERM_SENSOR_H
 
 #include "uiox_therm_if.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_THERM_SENSOR_NAME_MAX  16
 #define UIOX_THERM_MAX_SENSORS      8
 
 typedef struct {
     char               name[UIOX_THERM_SENSOR_NAME_MAX];
     uint8_t            sensor_id;
     uint8_t            hw_channel;  /**< HW channel index                 */
     uiox_therm_type_t  type;
     int16_t            offset_dc;   /**< Calibration offset (°C × 10)    */
     int16_t            min_dc;      /**< Minimum valid reading            */
     int16_t            max_dc;      /**< Maximum valid reading            */
     int16_t            cur_dc;      /**< Last filtered reading            */
     int16_t            avg_dc;      /**< Running average (°C × 10)       */
     uint8_t            avg_samples; /**< Averaging window size            */
     int32_t            avg_acc;     /**< Accumulator for averaging        */
     uint8_t            avg_count;
     bool               enabled;
     bool               error;
 } uiox_therm_sensor_t;
 
 typedef struct {
     uiox_therm_if_t       *tif;
     uiox_therm_sensor_t    sensors[UIOX_THERM_MAX_SENSORS];
     uint8_t                num_sensors;
 } uiox_therm_sensor_mgr_t;
 
 /* =========================================================================
  * NTC conversion (Steinhart-Hart Beta equation)
  * ====================================================================== */
 
 /**
  * @brief Convert ADC raw count to temperature (°C × 10) for NTC.
  *
  * Uses simplified Beta equation:
  *   1/T = 1/T_nom + (1/B) × ln(R/R_nom)
  *
  * @param raw    ADC count.
  * @param cfg    NTC parameters.
  * @return       Temperature in °C × 10, or INT16_MIN on error.
  */
 int16_t uiox_therm_ntc_convert(uint16_t raw,
                                  const uiox_therm_ntc_cfg_t *cfg);
 
 /* =========================================================================
  * Sensor manager API
  * ====================================================================== */
 
 int  uiox_therm_sensor_init    (uiox_therm_sensor_mgr_t *mgr,
                                  uiox_therm_if_t *tif);
 int  uiox_therm_sensor_register(uiox_therm_sensor_mgr_t *mgr,
                                  const uiox_therm_sensor_t *s);
 
 /** Read and update all registered sensors. */
 int  uiox_therm_sensor_update  (uiox_therm_sensor_mgr_t *mgr,
                                  uint32_t now_ms);
 
 /** Get current temperature for a sensor by name. */
 int16_t uiox_therm_sensor_get  (const uiox_therm_sensor_mgr_t *mgr,
                                  const char *name);
 
 uiox_therm_sensor_t *uiox_therm_sensor_find(
     uiox_therm_sensor_mgr_t *mgr, const char *name);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_THERM_SENSOR_H */
 