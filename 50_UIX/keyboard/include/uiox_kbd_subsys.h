/**
 * @file    uiox_kbd_subsys.h
 * @brief   UIOX Keyboard subsystem — shortcuts, LED sync, multi-layout.
 *
 * Top processing layer. Manages:
 *   - Full pipeline: scan → debounce → keymap → events
 *   - Keyboard shortcut registration and dispatch
 *   - LED state synchronisation with lock keys
 *   - Backlight auto-dim after inactivity
 *   - Layout hot-switch (e.g. Ctrl+Shift+F1 = QWERTY, +F2 = QWERTZ)
 *   - Key event callback registration
 *
 * @date    2026-05-27
 */
//Layer 4 — Subsystem
 #ifndef UIOX_KBD_SUBSYS_H
 #define UIOX_KBD_SUBSYS_H
 
 #include "uiox_kbd_if.h"
 #include "uiox_kbd_event.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Shortcut definition
  * ====================================================================== */
 
 #define UIOX_KBD_MAX_SHORTCUTS  16
 
 typedef void (*uiox_kbd_shortcut_fn_t)(void *ctx);

 typedef struct {
     uint8_t                modifiers;  /**< Required modifier bitmask      */
     uint16_t               keycode;    /**< Trigger keycode (HID Usage ID) */
     uiox_kbd_shortcut_fn_t fn;         /**< Callback on activation         */
     void                  *ctx;
     const char            *name;       /**< Human-readable label           */
     bool                   enabled;
 } uiox_kbd_shortcut_t;
 
 /* =========================================================================
  * Key event callback
  * ====================================================================== */
 
 typedef void (*uiox_kbd_event_fn_t)(const uiox_kbd_event_t *ev, void *ctx);
 
 #define UIOX_KBD_MAX_CALLBACKS  8
 
 typedef struct {
     uiox_kbd_event_fn_t fn;
     void               *ctx;
     uint8_t             filter_type;  /**< 0 = all, else uiox_kbd_ev_type_t */
     bool                enabled;
 } uiox_kbd_callback_t;
 
 /* =========================================================================
  * Subsystem state
  * ====================================================================== */
 
 typedef enum {
     UIOX_KBD_SUBSYS_STOPPED = 0,
     UIOX_KBD_SUBSYS_RUNNING,
 } uiox_kbd_subsys_state_t;
 
 typedef struct {
     uiox_kbd_if_t            kif;
     uiox_kbd_event_ctx_t     event_ctx;
     uiox_kbd_ringbuf_t       raw_rb;     /**< Raw events from IF layer      */
     uiox_kbd_ringbuf_t       cooked_rb;  /**< Processed events for app      */
     uiox_kbd_subsys_state_t  state;
 
     /* Shortcuts */
     uiox_kbd_shortcut_t      shortcuts[UIOX_KBD_MAX_SHORTCUTS];
     uint8_t                  shortcut_count;
 
     /* Event callbacks */
     uiox_kbd_callback_t      callbacks[UIOX_KBD_MAX_CALLBACKS];
     uint8_t                  callback_count;
 
     /* LED sync */
     uint8_t                  led_state;
 
     /* Backlight auto-dim */
     uint32_t                 last_activity_ms;
     uint32_t                 dim_timeout_ms;  /**< 0 = disabled             */
     uint8_t                  active_backlight;
     uint8_t                  dim_backlight;
 
     /* Statistics */
     uint64_t                 total_keypresses;
     uint64_t                 total_releases;
     uint64_t                 total_repeats;
     uint32_t                 scan_count;
 } uiox_kbd_subsys_t;
 
 /* =========================================================================
  * Subsystem API
  * ====================================================================== */
 
 int  uiox_kbd_subsys_init   (uiox_kbd_subsys_t          *sys,
                               uiox_kbd_hw_t              *hw,
                               uiox_kbd_if_type_t          if_type,
                               const uiox_kbd_event_cfg_t *ev_cfg,
                               uiox_kbd_layout_t           layout);
 
 int  uiox_kbd_subsys_start  (uiox_kbd_subsys_t *sys);
 void uiox_kbd_subsys_stop   (uiox_kbd_subsys_t *sys);
 
 /**
  * @brief  Perform one scan + process cycle.
  * @param  now_ms  Monotonic time (ms) — drives debounce, repeat, dim.
  * @param  ts_ns   Timestamp for event tagging (ns).
  * @return Number of processed events available in cooked ring buffer.
  */
 int  uiox_kbd_subsys_tick   (uiox_kbd_subsys_t *sys,
                               uint32_t           now_ms,
                               uint64_t           ts_ns);
 
 /** Register a keyboard shortcut. */
 int  uiox_kbd_subsys_add_shortcut(uiox_kbd_subsys_t    *sys,
                                    uint8_t               modifiers,
                                    uint16_t              keycode,
                                    uiox_kbd_shortcut_fn_t fn,
                                    void                 *ctx,
                                    const char           *name);
 
 /** Register a key event callback. */
 int  uiox_kbd_subsys_add_callback(uiox_kbd_subsys_t   *sys,
                                    uiox_kbd_event_fn_t  fn,
                                    void                *ctx,
                                    uint8_t              filter_type);
 
 /** Set backlight auto-dim parameters. */
 void uiox_kbd_subsys_set_dim(uiox_kbd_subsys_t *sys,
                               uint32_t           timeout_ms,
                               uint8_t            active_level,
                               uint8_t            dim_level);
 
 /** Sync LEDs with current lock state. */
 void uiox_kbd_subsys_sync_leds(uiox_kbd_subsys_t *sys);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KBD_SUBSYS_H */ 
 