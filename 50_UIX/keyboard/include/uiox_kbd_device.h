/**
 * @file    uiox_kbd_device.h
 * @brief   UIOX Keyboard top-level application-facing device API.
 *
 * Single include for application code. Wraps the entire keyboard
 * stack: HAL → IF driver → keymap → event processing → subsystem.
 *
 * @date    2026-05-27
 */
//Layer 5 — Device API
 #ifndef UIOX_KBD_DEVICE_H
 #define UIOX_KBD_DEVICE_H
 
 #include "uiox_kbd_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Open parameters
  * ====================================================================== */
 
 typedef struct {
     uiox_kbd_hw_t             *hw;
     const uiox_kbd_hw_ops_t   *hw_ops;
     uiox_kbd_if_type_t         if_type;
     uiox_kbd_layout_t          layout;
     uiox_kbd_event_cfg_t       event_cfg;
     uint32_t                   dim_timeout_ms;
     uint8_t                    active_backlight;
     uint8_t                    dim_backlight;
 } uiox_kbd_open_params_t;
 
 /* =========================================================================
  * Device handle
  * ====================================================================== */
 
 typedef struct {
     uiox_kbd_subsys_t  subsys;
     uiox_kbd_hw_t     *hw;
     bool               open;
 } uiox_kbd_device_t;
 
 /* =========================================================================
  * Application API
  * ====================================================================== */
 
 int  uiox_kbd_open        (uiox_kbd_device_t           *dev,
                             const uiox_kbd_open_params_t *p);
 int  uiox_kbd_start       (uiox_kbd_device_t *dev);
 void uiox_kbd_stop        (uiox_kbd_device_t *dev);
 void uiox_kbd_close       (uiox_kbd_device_t *dev);
 
 /** Scan + process one tick. Call periodically (e.g. every 5–10 ms). */
 int  uiox_kbd_tick        (uiox_kbd_device_t *dev,
                             uint32_t           now_ms,
                             uint64_t           ts_ns);
 
 /** Non-blocking: pop one processed event. Returns false if none ready. */
 bool uiox_kbd_poll        (uiox_kbd_device_t *dev,
                             uiox_kbd_event_t  *ev_out);
 
 /** Register shortcut callback. */
 int  uiox_kbd_add_shortcut(uiox_kbd_device_t     *dev,
                             uint8_t                modifiers,
                             uint16_t               keycode,
                             uiox_kbd_shortcut_fn_t fn,
                             void                  *ctx,
                             const char            *name);
 
 /** Register event callback (filter_type 0 = all events). */
 int  uiox_kbd_add_callback(uiox_kbd_device_t   *dev,
                             uiox_kbd_event_fn_t  fn,
                             void                *ctx,
                             uint8_t              filter_type);
 
 /** Switch keymap layout at runtime. */
 int  uiox_kbd_set_layout  (uiox_kbd_device_t *dev,
                             uiox_kbd_layout_t  layout);
 
 /** Set LED state directly (bypasses lock-key logic). */
 int  uiox_kbd_set_leds    (uiox_kbd_device_t *dev, uint8_t led_mask);
 
 /** Set backlight brightness (0=off, 255=max). */
 int  uiox_kbd_set_backlight(uiox_kbd_device_t *dev, uint8_t level);
 
 /** Query current modifier bitmask. */
 uint8_t uiox_kbd_modifiers(const uiox_kbd_device_t *dev);
 
 /** Query lock key state. */
 bool uiox_kbd_lock_state  (const uiox_kbd_device_t *dev, uint8_t led_bit);
 
 /** Return current layout name string. */
 const char *uiox_kbd_layout_name(void);
 
 /** Print statistics to stdout. */
 void uiox_kbd_print_stats (const uiox_kbd_device_t *dev);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KBD_DEVICE_H */
 