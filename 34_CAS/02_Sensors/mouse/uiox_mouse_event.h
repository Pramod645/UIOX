/**
 * @file    uiox_mouse_event.h
 * @brief   UIOX Mouse event processing (debounce, click, double-click).
 *
 * Adds:
 *   - Button debounce (configurable window)
 *   - Click detection (press + release < click_timeout_ms)
 *   - Double-click detection (two clicks < dblclick_timeout_ms)
 *   - Pointer acceleration (configurable curve)
 *   - Scroll acceleration
 *   - Absolute cursor clamping to screen bounds
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_MOUSE_EVENT_H
 #define UIOX_MOUSE_EVENT_H
 
 #include "uiox_mouse_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Event processing configuration
  * ====================================================================== */
 
 typedef struct {
     uint32_t  debounce_ms;        /**< Button debounce window (ms)        */
     uint32_t  click_timeout_ms;   /**< Max press→release for click (ms)   */
     uint32_t  dblclick_timeout_ms;/**< Max click→click for dblclick (ms)  */
     float     accel_factor;       /**< Pointer acceleration multiplier     */
     float     accel_threshold;    /**< Velocity threshold to start accel  */
     int32_t   screen_w;           /**< Screen width for cursor clamping   */
     int32_t   screen_h;           /**< Screen height                      */
     bool      invert_y;           /**< Invert Y axis                      */
     bool      invert_scroll;      /**< Natural scrolling                  */
 } uiox_mouse_event_cfg_t;
 
 /* =========================================================================
  * Per-button debounce state
  * ====================================================================== */
 
 typedef struct {
     bool     pressed;
     bool     raw;
     uint32_t change_ts_ms;
     uint32_t press_ts_ms;
     uint32_t last_click_ts_ms;
     uint32_t click_count;
     bool     confirmed;
 } uiox_mouse_btn_state_t;
 
 /* =========================================================================
  * Event processor context
  * ====================================================================== */
 
 typedef struct {
     uiox_mouse_event_cfg_t cfg;
     uiox_mouse_btn_state_t buttons[UIOX_MOUSE_MAX_BUTTONS];
     int32_t                cursor_x;
     int32_t                cursor_y;
     float                  accel_x;   /**< Accumulated sub-pixel X       */
     float                  accel_y;   /**< Accumulated sub-pixel Y       */
 } uiox_mouse_event_ctx_t;
 
 /* =========================================================================
  * Event processor API
  * ====================================================================== */
 
 int  uiox_mouse_event_init   (uiox_mouse_event_ctx_t       *ctx,
                                const uiox_mouse_event_cfg_t *cfg);
 
 /**
  * @brief  Process raw events from src_rb, push processed events to dst_rb.
  * @return Number of events pushed.
  */
 int  uiox_mouse_event_process(uiox_mouse_event_ctx_t *ctx,
                                uiox_mouse_ringbuf_t   *src_rb,
                                uiox_mouse_ringbuf_t   *dst_rb,
                                uint32_t                now_ms);
 
 /** Get current cursor position. */
 void uiox_mouse_event_cursor (const uiox_mouse_event_ctx_t *ctx,
                                int32_t *x, int32_t *y);
 
 /** Warp cursor to absolute position (for programmatic repositioning). */
 void uiox_mouse_event_warp   (uiox_mouse_event_ctx_t *ctx,
                                int32_t x, int32_t y);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MOUSE_EVENT_H */
 