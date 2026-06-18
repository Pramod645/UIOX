/**
 * @file    uiox_therm_policy.h
 * @brief   UIOX Thermal policy: trip points, throttle, DVFS.
 * @date    2026-06-05
 */
//Layer 3 — Thermal Policy
 #ifndef UIOX_THERM_POLICY_H
 #define UIOX_THERM_POLICY_H
 
 #include "uiox_therm_sensor.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_THERM_MAX_ZONES        4
 #define UIOX_THERM_MAX_TRIPS        6
 
 /* =========================================================================
  * Trip point type
  * ====================================================================== */
 
 typedef enum {
     UIOX_THERM_TRIP_PASSIVE = 0,  /**< SW cooling (CPU throttle)          */
     UIOX_THERM_TRIP_ACTIVE,        /**< HW cooling (fan speed up)          */
     UIOX_THERM_TRIP_HOT,           /**< Warning alert                      */
     UIOX_THERM_TRIP_CRITICAL,      /**< Emergency shutdown                 */
 } uiox_therm_trip_type_t;
 
 /* =========================================================================
  * Trip point
  * ====================================================================== */
 
 typedef struct {
     uiox_therm_trip_type_t  type;
     int16_t                 temp_dc;      /**< Trigger temperature         */
     int16_t                 hyst_dc;      /**< Hysteresis (for clearing)   */
     bool                    active;       /**< Currently triggered         */
     void (*action)(uint8_t zone_id, uiox_therm_trip_type_t type,
                    bool crossing_up, void *ctx);
     void *action_ctx;
 } uiox_therm_trip_t;
 
 /* =========================================================================
  * Thermal zone
  * ====================================================================== */
 
 typedef struct {
     uint8_t               zone_id;
     const char           *sensor_name; /**< Controlling sensor name       */
     uiox_therm_trip_t     trips[UIOX_THERM_MAX_TRIPS];
     uint8_t               num_trips;
     int16_t               cur_dc;
     bool                  throttled;
     bool                  active;
 } uiox_therm_zone_t;
 
 /* =========================================================================
  * Policy context
  * ====================================================================== */
 
 typedef struct {
     uiox_therm_sensor_mgr_t *mgr;
     uiox_therm_zone_t         zones[UIOX_THERM_MAX_ZONES];
     uint8_t                   num_zones;
     bool                      emergency;
     bool                      throttled;
     uint32_t                  throttle_count;
     uint32_t                  emergency_count;
 } uiox_therm_policy_t;
 
 /* =========================================================================
  * Policy API
  * ====================================================================== */
 
 int  uiox_therm_policy_init     (uiox_therm_policy_t      *pol,
                                   uiox_therm_sensor_mgr_t  *mgr);
 int  uiox_therm_policy_add_zone (uiox_therm_policy_t       *pol,
                                   const uiox_therm_zone_t   *zone);
 void uiox_therm_policy_tick     (uiox_therm_policy_t *pol,
                                   uint32_t now_ms);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_THERM_POLICY_H */
 