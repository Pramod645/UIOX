/**
 * @file    uiox_mouse_subsys.h
 * @brief   UIOX Mouse subsystem — cursor, zones, gestures, callbacks.
 *
 * Top subsystem layer. Manages:
 *   - Full pipeline: poll → proto decode → event process → deliver
 *   - Hot-zone callbacks (screen regions trigger actions)
 *   - Gesture recognition (swipe direction from movement)
 *   - Event callback registration
 *   - Connection/disconnection handling
 *   - Per-frame statistics
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_MOUSE_SUBSYS_H
 #define UIOX_MOUSE_SUBSYS_H
 
 #include "uiox_mouse_proto.h"
 #include "uiox_mouse_event.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hot zone (screen region with callback)
  * ====================================================================== */
 
 #define UIOX_MOUSE_MAX_ZONES    8
 
 typedef void (*uiox_mouse_zone_cb_t)(int32_t x, int32_t y,
                                       uint8_t buttons, void *ctx);
 
 typedef struct {
     int32_t              x, y, w, h;
     uiox_mouse_zone_cb_t on_enter;
     uiox_mouse_zone_cb_t on_leave;
     uiox_mouse_zone_cb_t on_click;
     void                *ctx;
     bool                 cursor_inside;
     bool                 active;
 } uiox_mouse_zone_t;
 
 /* =========================================================================
  * Event callback
  * ====================================================================== */
 
 typedef void (*uiox_mouse_evt_cb_t)(const uiox_mouse_event_t *ev, void *ctx);
 
 #define UIOX_MOUSE_MAX_CALLBACKS  8
 
 typedef struct {
     uiox_mouse_evt_cb_t fn;
     void               *ctx;
     uint8_t             filter;   /**< 0 = all, else uiox_mouse_ev_type_t */
     bool                active;
 } uiox_mouse_cb_entry_t;
 
 /* =========================================================================
  * Subsystem state
  * ====================================================================== */
 
 typedef enum {
     UIOX_MOUSE_SUBSYS_STOPPED = 0,
     UIOX_MOUSE_SUBSYS_RUNNING,
     UIOX_MOUSE_SUBSYS_DISCONNECTED,
 } uiox_mouse_subsys_state_t;
 
 /* =========================================================================
  * Subsystem descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_mouse_if_t        mif;
     uiox_mouse_proto_ctx_t proto;
     uiox_mouse_event_ctx_t event;
     uiox_mouse_ringbuf_t   raw_rb;    
     uiox_mouse_ringbuf_t   cooked_rb;
     uiox_mouse_subsys_state_t state;
 
     /* Hot zones */
     uiox_mouse_zone_t      zones[UIOX_MOUSE_MAX_ZONES];
     uint8_t                zone_count;
 
     /* Event callbacks */
     uiox_mouse_cb_entry_t  callbacks[UIOX_MOUSE_MAX_CALLBACKS];
     uint8_t                cb_count;
 
     /* Statistics */
     uint64_t               total_moves;
     uint64_t               total_clicks;
     uint64_t               total_dblclicks;
     uint64_t               total_scrolls;
     uint32_t               tick_count;
 } uiox_mouse_subsys_t;
 
 /* =========================================================================
  * Subsystem API
  * ====================================================================== */
 
 int  uiox_mouse_subsys_init    (uiox_mouse_subsys_t          *sys,
                                  uiox_mouse_hw_t              *hw,
                                  uiox_mouse_proto_t            proto,
                                  const uiox_mouse_event_cfg_t *ev_cfg);
 
 int  uiox_mouse_subsys_start   (uiox_mouse_subsys_t *sys);
 void uiox_mouse_subsys_stop    (uiox_mouse_subsys_t *sys);
 
 /**
  * @brief  Periodic tick — poll hardware, process events, dispatch.
  * @param  now_ms  Monotonic time (ms).
  */
 void uiox_mouse_subsys_tick    (uiox_mouse_subsys_t *sys, uint32_t now_ms);
 
 /** Register a hot zone. */
 int  uiox_mouse_subsys_add_zone(uiox_mouse_subsys_t      *sys,
                                  const uiox_mouse_zone_t  *zone);
 
 /** Register an event callback (filter=0 = all events). */
 int  uiox_mouse_subsys_add_cb  (uiox_mouse_subsys_t  *sys,
                                  uiox_mouse_evt_cb_t   fn,
                                  void                 *ctx,
                                  uint8_t               filter);
 
 /** Get current cursor position. */
 void uiox_mouse_subsys_cursor  (const uiox_mouse_subsys_t *sys,
                                  int32_t *x, int32_t *y);
 
 /** Warp cursor to absolute position. */
 void uiox_mouse_subsys_warp    (uiox_mouse_subsys_t *sys,
                                  int32_t x, int32_t y);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MOUSE_SUBSYS_H */
  