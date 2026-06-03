/**
 * @file    uiox_tpwd_subsys.h
 * @brief   UIOX Touch-Password subsystem.
 *
 * Full pipeline:
 *   scan → debounce → gesture → hash → verify → lockout → audit log
 *
 * Features:
 *   - Enrolment workflow (set new password)
 *   - Verification workflow (enter password)
 *   - Lockout enforcement (attempts × delay)
 *   - Audit log (last N events with timestamp)
 *   - Session management (token after success)
 *   - Backlight control during entry
 *   - Event callbacks (success, fail, lockout, timeout)
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_TPWD_SUBSYS_H
 #define UIOX_TPWD_SUBSYS_H
 
 #include "uiox_tpwd_gesture.h"
 #include "uiox_tpwd_sec.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Subsystem events
  * ====================================================================== */
 
 typedef enum {
     UIOX_TPWD_EVT_AUTH_OK = 0,    /**< Authentication succeeded          */
     UIOX_TPWD_EVT_AUTH_FAIL,       /**< Wrong credential entered          */
     UIOX_TPWD_EVT_LOCKED,          /**< Account locked after max attempts */
     UIOX_TPWD_EVT_TIMEOUT,         /**< Entry timed out                   */
     UIOX_TPWD_EVT_CANCELLED,       /**< Entry cancelled by user           */
     UIOX_TPWD_EVT_ENROLLED,        /**< New credential enrolled           */
     UIOX_TPWD_EVT_DIGIT,           /**< One digit/node entered (UI hint)  */
     UIOX_TPWD_EVT_BACKSPACE,       /**< Backspace pressed (UI hint)       */
     UIOX_TPWD_EVT_LOGOUT,          /**< Session logged out                */
 } uiox_tpwd_evt_t;
 
 typedef void (*uiox_tpwd_evt_cb_t)(uiox_tpwd_evt_t evt,
                                     uint8_t attempts_remaining,
                                     void *ctx);
 
 /* =========================================================================
  * Audit log entry
  * ====================================================================== */
 
 #define UIOX_TPWD_AUDIT_MAX     16
 
 typedef struct {
     uiox_tpwd_evt_t  evt;
     uint32_t         ts_s;        /**< Timestamp (seconds since boot)     */
     char             id[UIOX_TPWD_SEC_ID_LEN];
     bool             valid;
 } uiox_tpwd_audit_entry_t;
 
 /* =========================================================================
  * Subsystem state
  * ====================================================================== */
 
 typedef enum {
     UIOX_TPWD_SUBSYS_IDLE = 0,
     UIOX_TPWD_SUBSYS_ENROLLING,
     UIOX_TPWD_SUBSYS_VERIFYING,
     UIOX_TPWD_SUBSYS_LOCKED,
     UIOX_TPWD_SUBSYS_AUTHENTICATED,
 } uiox_tpwd_subsys_state_t;
 
 /* =========================================================================
  * Subsystem descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_tpwd_if_t         tif;
     uiox_tpwd_gesture_t    gesture;
     uiox_tpwd_sec_t        sec;
     uiox_tpwd_evtbuf_t     gesture_rb;  /**< IF → gesture ring buffer     */
     uiox_tpwd_subsys_state_t state;
 
     char                   active_id[UIOX_TPWD_SEC_ID_LEN];
     uiox_tpwd_evt_cb_t     evt_cb;
     void                  *evt_ctx;
 
     uiox_tpwd_audit_entry_t audit[UIOX_TPWD_AUDIT_MAX];
     uint8_t                 audit_head;
 
     uint32_t                now_s;      /**< Updated by tick               */
 } uiox_tpwd_subsys_t;
 
 /* =========================================================================
  * Subsystem API
  * ====================================================================== */
 
 int  uiox_tpwd_subsys_init   (uiox_tpwd_subsys_t           *sys,
                                uiox_tpwd_hw_t               *hw,
                                const uiox_tpwd_gesture_cfg_t *gcfg,
                                uint32_t debounce_ms,
                                uint32_t hold_ms,
                                uint32_t timeout_ms,
                                uint32_t rng_seed);
 
 void uiox_tpwd_subsys_set_cb (uiox_tpwd_subsys_t *sys,
                                uiox_tpwd_evt_cb_t  cb, void *ctx);
 
 /** Enrol a new credential for the given ID. */
 int  uiox_tpwd_subsys_enrol_start(uiox_tpwd_subsys_t *sys,
                                    const char *id,
                                    uint32_t now_ms);
 
 /** Begin a verification session for the given ID. */
 int  uiox_tpwd_subsys_verify_start(uiox_tpwd_subsys_t *sys,
                                     const char *id,
                                     uint32_t now_ms);
 
 /**
  * @brief  Periodic tick — drives scan, gesture processing, and auth.
  * @param  now_ms  Monotonic time (ms).
  * @param  now_s   Wall-clock time (seconds) for lockout timestamps.
  */
 void uiox_tpwd_subsys_tick   (uiox_tpwd_subsys_t *sys,
                                uint32_t now_ms, uint32_t now_s);
 
 /** Logout: invalidate session token. */
 void uiox_tpwd_subsys_logout (uiox_tpwd_subsys_t *sys);
 
 /** Query current authentication state. */
 uiox_tpwd_subsys_state_t uiox_tpwd_subsys_state(
     const uiox_tpwd_subsys_t *sys);
 
 /** Return remaining attempts for given ID (255 = not locked). */
 uint8_t uiox_tpwd_subsys_attempts_left(const uiox_tpwd_subsys_t *sys,
                                         const char *id);
 
 /** Dump audit log to stdout. */
 void uiox_tpwd_subsys_print_audit(const uiox_tpwd_subsys_t *sys);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_TPWD_SUBSYS_H */
 