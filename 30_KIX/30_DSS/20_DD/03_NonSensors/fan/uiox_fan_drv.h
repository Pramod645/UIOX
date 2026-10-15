/**
 * @file    uiox_fan_drv.h
 * @brief   UIOX Fan driver abstraction (RPM/PWM/tach/stall).
 * @date    2026-06-05
 */
//Layer 2b — Fan Driver
 #ifndef UIOX_FAN_DRV_H
 #define UIOX_FAN_DRV_H
 
 #include "uiox_fan_if.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_FAN_MAX_FAN_COUNT      UIOX_FAN_MAX_CHANNELS
 #define UIOX_FAN_NAME_MAX           16
 #define UIOX_FAN_STALL_RPM_MIN      200u  /**< Below this = stalled        */
 #define UIOX_FAN_SPIN_UP_DUTY       200u  /**< Spin-up duty (78 %)         */
 #define UIOX_FAN_SPIN_UP_MS         500u  /**< Spin-up duration (ms)       */
 
 typedef struct {
     char      name[UIOX_FAN_NAME_MAX];
     uint8_t   fan_id;
     uint16_t  min_rpm;      /**< Minimum operational RPM                   */
     uint16_t  max_rpm;      /**< Maximum rated RPM                         */
     uint8_t   min_duty;     /**< Minimum PWM duty (0..255)                 */
     uint8_t   max_duty;     /**< Maximum PWM duty (0..255)                 */
     uint8_t   cur_duty;     /**< Current duty                              */
     uint16_t  cur_rpm;
     bool      stalled;
     bool      enabled;
     bool      manual;       /**< true = ignore thermal policy              */
     uint32_t  stall_ts_ms;  /**< Timestamp when stall detected             */
     uint32_t  spin_up_ts_ms;/**< Timestamp when spin-up started            */
     bool      spinning_up;
 } uiox_fan_ch_t;
 
 typedef struct {
     uiox_fan_if_t   *fif;
     uiox_fan_ch_t    fans[UIOX_FAN_MAX_FAN_COUNT];
     uint8_t          num_fans;
 } uiox_fan_drv_t;
 
 int  uiox_fan_drv_init      (uiox_fan_drv_t *drv, uiox_fan_if_t *fif);
 int  uiox_fan_drv_register  (uiox_fan_drv_t *drv, const uiox_fan_ch_t *ch);
 
 /** Set duty cycle (0..255). Performs spin-up if starting from stopped. */
 int  uiox_fan_drv_set_duty  (uiox_fan_drv_t *drv, uint8_t fan_id,
                               uint8_t duty, uint32_t now_ms);
 
 /** Set duty as percent (0..100). */
 int  uiox_fan_drv_set_pct   (uiox_fan_drv_t *drv, uint8_t fan_id,
                               uint8_t pct, uint32_t now_ms);
 
 /** Periodic tick: check stall, handle spin-up timeout. */
 void uiox_fan_drv_tick      (uiox_fan_drv_t *drv, uint32_t now_ms);
 
 /** Enable/disable manual override (bypass thermal policy). */
 void uiox_fan_drv_set_manual(uiox_fan_drv_t *drv, uint8_t fan_id,
                               bool manual, uint8_t duty, uint32_t now_ms);
 
 uiox_fan_ch_t *uiox_fan_drv_find(uiox_fan_drv_t *drv, uint8_t fan_id);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FAN_DRV_H */
 