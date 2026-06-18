/**
 * @file    uiox_mouse_device.h
 * @brief   UIOX Mouse top-level application-facing device API.
 * @date    2026-06-01
 */

 #ifndef UIOX_MOUSE_DEVICE_H
 #define UIOX_MOUSE_DEVICE_H
 
 #include "uiox_mouse_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_mouse_hw_t              *hw;
     const uiox_mouse_hw_ops_t    *hw_ops;
     uiox_mouse_proto_t            proto;
     uiox_mouse_event_cfg_t        event_cfg;
 } uiox_mouse_open_params_t;
 
 typedef struct {
     uiox_mouse_subsys_t  subsys;
     uiox_mouse_hw_t     *hw;
     bool                 open;
 } uiox_mouse_device_t;
 
 /* =========================================================================
  * Application API
  * ====================================================================== */
 
 int  uiox_mouse_open       (uiox_mouse_device_t           *dev,
                              const uiox_mouse_open_params_t *p);
 int  uiox_mouse_start      (uiox_mouse_device_t *dev);
 void uiox_mouse_stop       (uiox_mouse_device_t *dev);
 void uiox_mouse_close      (uiox_mouse_device_t *dev);
 
 /** Periodic tick — call from main loop or dedicated task. */
 void uiox_mouse_tick       (uiox_mouse_device_t *dev, uint32_t now_ms);
 
 /** Non-blocking poll: pop one processed event. */
 bool uiox_mouse_poll       (uiox_mouse_device_t  *dev,
                              uiox_mouse_event_t   *ev_out);
 
 /** Register a hot zone. */
 int  uiox_mouse_add_zone   (uiox_mouse_device_t     *dev,
                              const uiox_mouse_zone_t *zone);
 
 /** Register an event callback (filter=0 = all). */
 int  uiox_mouse_add_callback(uiox_mouse_device_t *dev,
                               uiox_mouse_evt_cb_t  fn,
                               void                *ctx,
                               uint8_t              filter);
 
 /** Get current cursor position. */
 void uiox_mouse_cursor     (const uiox_mouse_device_t *dev,
                              int32_t *x, int32_t *y);
 
 /** Warp cursor to absolute position. */
 void uiox_mouse_warp       (uiox_mouse_device_t *dev, int32_t x, int32_t y);
 
 /** Query connection state. */
 bool uiox_mouse_connected  (const uiox_mouse_device_t *dev);
 
 /** Print interface and subsystem statistics. */
 void uiox_mouse_print_stats(const uiox_mouse_device_t *dev);
 
 /** Human-readable event type name. */
 const char *uiox_mouse_evt_name(uiox_mouse_ev_type_t type);
 
 /** Human-readable state name. */
 const char *uiox_mouse_state_name(uiox_mouse_subsys_state_t s);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MOUSE_DEVICE_H */
 