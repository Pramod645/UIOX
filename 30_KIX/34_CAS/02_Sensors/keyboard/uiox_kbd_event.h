/**
 * @file    uiox_kbd_event.h
 * @brief   UIOX Keyboard event processing layer.
 *
 * Processes raw scan events into fully-resolved key events:
 *   - Hardware debouncing (configurable window)
 *   - Modifier state tracking
 *   - Lock key toggle (CapsLock, NumLock, ScrollLock)
 *   - Key auto-repeat (delay + interval)
 *   - Keycode + unicode resolution via keymap
 *   - Rollover tracking (up to N-KRO)
 *
 * @date    2026-05-27
 */
//Layer 3 — Event Processing
 #ifndef UIOX_KBD_EVENT_H
 #define UIOX_KBD_EVENT_H
 
 #include "uiox_kbd_buf.h"
 #include "uiox_kbd_map.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Event processing configuration
  * ====================================================================== */
 
 #define UIOX_KBD_MAX_ROLLOVER       6   /**< Max simultaneous key presses  */
 #define UIOX_KBD_DEBOUNCE_MS        10  /**< Debounce window (ms)          */
 #define UIOX_KBD_REPEAT_DELAY_MS    500 /**< Delay before auto-repeat (ms) */
 #define UIOX_KBD_REPEAT_INTERVAL_MS 33  /**< Auto-repeat interval (ms)     */
 
 typedef struct {
     uint32_t debounce_ms;        /**< Debounce window                      */
     uint32_t repeat_delay_ms;    /**< Initial repeat delay                 */
     uint32_t repeat_interval_ms; /**< Subsequent repeat interval           */
     bool     repeat_enabled;     /**< Enable key auto-repeat               */
 } uiox_kbd_event_cfg_t;
 
 /* =========================================================================
  * Per-key debounce state
  * ====================================================================== */
 
 typedef struct {
     uint8_t   scancode;
     bool      pressed;          /**< Confirmed pressed state               */
     bool      raw;              /**< Current raw state                     */
     uint32_t  change_ts_ms;     /**< Timestamp of last raw state change    */
     bool      confirmed;        /**< Debounce confirmation complete        */
 } uiox_kbd_debounce_t;
 
 /* =========================================================================
  * Event processor context
  * ====================================================================== */
 
 #define UIOX_KBD_MAX_DEBOUNCE_SLOTS  (UIOX_KBD_MAX_ROWS * UIOX_KBD_MAX_COLS)
 
 typedef struct {
     uiox_kbd_event_cfg_t  cfg;
 
     /* Modifier and lock state */
     uint8_t   modifiers;         /**< Active modifier bitmask              */
     bool      caps_lock;
     bool      num_lock;
     bool      scroll_lock;
 
     /* Rollover: currently pressed keys */
     uint8_t   held_scancodes[UIOX_KBD_MAX_ROLLOVER];
     uint8_t   held_count;
 
     /* Auto-repeat */
     uint8_t   repeat_scancode;   /**< Key being repeated (0 = none)       */
     uint32_t  repeat_next_ms;    /**< Next repeat event timestamp          */
     bool      repeat_started;
 
     /* Debounce slots */
     uiox_kbd_debounce_t debounce[UIOX_KBD_MAX_DEBOUNCE_SLOTS];
     uint16_t            debounce_count;
 
 } uiox_kbd_event_ctx_t;
 
 /* =========================================================================
  * Event processing API
  * ====================================================================== */
 
 int  uiox_kbd_event_init   (uiox_kbd_event_ctx_t       *ctx,
                              const uiox_kbd_event_cfg_t *cfg);
 
 /**
  * @brief  Process raw events from ring buffer.
  *
  * Reads raw events from src_rb, applies debounce, resolves keycode
  * and unicode, then pushes fully processed events to dst_rb.
  *
  * @param  ctx      Event context.
  * @param  src_rb   Raw event ring buffer (from IF layer).
  * @param  dst_rb   Processed event ring buffer (for application).
  * @param  now_ms   Current monotonic time (ms).
  * @return Number of events pushed to dst_rb.
  */
 int  uiox_kbd_event_process(uiox_kbd_event_ctx_t *ctx,
                              uiox_kbd_ringbuf_t   *src_rb,
                              uiox_kbd_ringbuf_t   *dst_rb,
                              uint32_t              now_ms);
 
 /** Query current modifier bitmask. */
 uint8_t uiox_kbd_event_modifiers(const uiox_kbd_event_ctx_t *ctx);
 
 /** Query a specific lock state. */
 bool uiox_kbd_event_lock(const uiox_kbd_event_ctx_t *ctx, uint8_t led_bit);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KBD_EVENT_H */
 