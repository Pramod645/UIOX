/**
 * @file    uiox_fan_thermal.h
 * @brief   UIOX Fan thermal control engine (PID, step, hysteresis).
 * @date    2026-06-05
 */
//Layer 3 — Thermal Engine
 #ifndef UIOX_FAN_THERMAL_H
 #define UIOX_FAN_THERMAL_H
 
 #include "uiox_fan_drv.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_FAN_TEMP_ZONE_MAX      4
 #define UIOX_FAN_TRIP_POINTS_MAX    6
 
 /* =========================================================================
  * Controller type
  * ====================================================================== */
 
 typedef enum {
     UIOX_FAN_CTRL_STEP       = 0,  /**< Step table (fan curve)            */
     UIOX_FAN_CTRL_HYSTERESIS,       /**< Hysteresis on/off                */
     UIOX_FAN_CTRL_PID,              /**< PID controller                   */
 } uiox_fan_ctrl_type_t;
 
 /* =========================================================================
  * Fan curve (step table)
  * ====================================================================== */
 
 typedef struct {
     int16_t  temp_dc;   /**< Temperature threshold (°C × 10)              */
     uint8_t  duty;      /**< PWM duty (0..255) at this temp               */
 } uiox_fan_trip_t;
 
 /* =========================================================================
  * PID state
  * ====================================================================== */
 
 typedef struct {
     float  kp, ki, kd;          /**< PID gains                            */
     float  integral;
     float  prev_error;
     float  output_min;           /**< Min output duty (0..255)             */
     float  output_max;           /**< Max output duty (0..255)             */
     float  integral_limit;       /**< Anti-windup clamp                   */
     int16_t setpoint_dc;         /**< Target temperature (°C × 10)        */
 } uiox_fan_pid_t;
 
 /* =========================================================================
  * Thermal zone
  * ====================================================================== */
 
 typedef struct {
     uint8_t            zone_id;
     uint8_t            temp_sensor_id;  /**< Temp sensor channel index     */
     uint8_t            fan_id;          /**< Controlled fan channel        */
     uiox_fan_ctrl_type_t ctrl_type;
     uiox_fan_trip_t    trips[UIOX_FAN_TRIP_POINTS_MAX];
     uint8_t            num_trips;
     int16_t            hyst_on_dc;     /**< Hysteresis ON threshold        */
     int16_t            hyst_off_dc;    /**< Hysteresis OFF threshold       */
     uiox_fan_pid_t     pid;
     int16_t            cur_temp_dc;    /**< Last read temperature          */
     uint8_t            cur_duty;       /**< Current output duty            */
     bool               active;
 } uiox_fan_zone_t;
 
 /* =========================================================================
  * Thermal engine
  * ====================================================================== */
 
 typedef struct {
     uiox_fan_drv_t   *drv;
     uiox_fan_zone_t   zones[UIOX_FAN_TEMP_ZONE_MAX];
     uint8_t           num_zones;
     int16_t           critical_temp_dc; /**< Emergency full-speed temp     */
     bool              emergency;        /**< True if critical temp hit     */
 } uiox_fan_thermal_t;
 
 /* =========================================================================
  * Thermal API
  * ====================================================================== */
 
 int  uiox_fan_thermal_init       (uiox_fan_thermal_t *th,
                                    uiox_fan_drv_t     *drv,
                                    int16_t             critical_temp_dc);
 int  uiox_fan_thermal_add_zone   (uiox_fan_thermal_t *th,
                                    const uiox_fan_zone_t *zone);
 
 /** Thermal tick: read temps, compute output, apply to fans. */
 void uiox_fan_thermal_tick       (uiox_fan_thermal_t *th, uint32_t now_ms);
 
 /** PID initialiser. */
 void uiox_fan_pid_init           (uiox_fan_pid_t *pid,
                                    float kp, float ki, float kd,
                                    int16_t setpoint_dc,
                                    float out_min, float out_max);
 
 /** PID compute (returns duty 0..255). */
 uint8_t uiox_fan_pid_update      (uiox_fan_pid_t *pid,
                                    int16_t measured_dc, float dt_s);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FAN_THERMAL_H */
 