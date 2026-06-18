/**
 * @file    uiox_tpwd_device.h
 * @brief   UIOX Touch-Password top-level application-facing device API.
 *
 * Single include for application code. Wraps the entire stack:
 * HAL → IF driver → gesture engine → security engine → subsystem.
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_TPWD_DEVICE_H
 #define UIOX_TPWD_DEVICE_H
 
 #include "uiox_tpwd_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Open parameters
  * ====================================================================== */
 
 typedef struct {
     /* HAL */
     uiox_tpwd_hw_t             *hw;
     const uiox_tpwd_hw_ops_t   *hw_ops;
 
     /* Gesture config */
     uiox_tpwd_gesture_cfg_t     gesture;
 
     /* Interface timings */
     uint32_t  debounce_ms;
     uint32_t  hold_ms;
     uint32_t  timeout_ms;
 
     /* Security */
     uint32_t  rng_seed;      /**< Seed for PRNG (use TRNG value if avail) */
 
     /* Events */
     uiox_tpwd_evt_cb_t   evt_cb;
     void                *evt_ctx;
 } uiox_tpwd_open_params_t;
 
 /* =========================================================================
  * Device handle
  * ====================================================================== */
 
 typedef struct {
     uiox_tpwd_subsys_t  subsys;
     uiox_tpwd_hw_t     *hw;
     bool                open;
 } uiox_tpwd_device_t;
 
 /* =========================================================================
  * Application API
  * ====================================================================== */
 
 /** Open and fully initialise the touch-password device. */
 int  uiox_tpwd_open        (uiox_tpwd_device_t           *dev,
                              const uiox_tpwd_open_params_t *p);
 
 /** Power up hardware and prepare for use. */
 int  uiox_tpwd_start       (uiox_tpwd_device_t *dev);
 
 /** Power down hardware. */
 void uiox_tpwd_stop        (uiox_tpwd_device_t *dev);
 
 /** Release all resources. */
 void uiox_tpwd_close       (uiox_tpwd_device_t *dev);
 
 /**
  * @brief  Periodic tick — drives scan, gesture, auth pipeline.
  * @param  now_ms  Monotonic time (ms).
  * @param  now_s   Wall-clock seconds (for lockout timestamps).
  */
 void uiox_tpwd_tick        (uiox_tpwd_device_t *dev,
                              uint32_t now_ms, uint32_t now_s);
 
 /** Enrol a new credential for the given user ID. */
 int  uiox_tpwd_enrol       (uiox_tpwd_device_t *dev,
                              const char *id, uint32_t now_ms);
 
 /** Start a verification session for the given user ID. */
 int  uiox_tpwd_verify      (uiox_tpwd_device_t *dev,
                              const char *id, uint32_t now_ms);
 
 /** Delete a stored credential. */
 int  uiox_tpwd_delete      (uiox_tpwd_device_t *dev, const char *id);
 
 /** Logout: invalidate session token. */
 void uiox_tpwd_logout      (uiox_tpwd_device_t *dev);
 
 /** Query whether a valid session is active. */
 bool uiox_tpwd_authenticated(const uiox_tpwd_device_t *dev, uint32_t now_s);
 
 /** Query current state. */
 uiox_tpwd_subsys_state_t uiox_tpwd_state(const uiox_tpwd_device_t *dev);
 
 /** Return remaining attempts before lockout. */
 uint8_t uiox_tpwd_attempts_left(const uiox_tpwd_device_t *dev,
                                  const char *id);
 
 /** Set backlight brightness (0=off, 255=max). */
 int  uiox_tpwd_set_backlight(uiox_tpwd_device_t *dev, uint8_t level);
 
 /** Print audit log to stdout. */
 void uiox_tpwd_print_audit  (const uiox_tpwd_device_t *dev);
 
 /** Print interface statistics. */
 void uiox_tpwd_print_stats  (const uiox_tpwd_device_t *dev);
 
 /** Human-readable state name. */
 const char *uiox_tpwd_state_name(uiox_tpwd_subsys_state_t s);
 
 /** Human-readable event name. */
 const char *uiox_tpwd_evt_name(uiox_tpwd_evt_t evt);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_TPWD_DEVICE_H */
 