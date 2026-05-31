/**
 * @file    uiox_kbd_subsys.c
 * @brief   UIOX Keyboard subsystem implementation.
 * @date    2026-05-27
 */

 #include "uiox_kbd_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 /* =========================================================================
  * Init
  * ====================================================================== */
 
 int uiox_kbd_subsys_init(uiox_kbd_subsys_t          *sys,
                           uiox_kbd_hw_t              *hw,
                           uiox_kbd_if_type_t          if_type,
                           const uiox_kbd_event_cfg_t *ev_cfg,
                           uiox_kbd_layout_t           layout)
 {
     if (!sys || !hw || !ev_cfg) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
 
     /* Initialise ring buffers */
     uiox_kbd_buf_init(&sys->raw_rb);
     uiox_kbd_buf_init(&sys->cooked_rb);
 
     /* Initialise interface driver */
     int rc = uiox_kbd_if_config(&sys->kif, hw, if_type);
     if (rc < 0) return rc;
 
     /* Initialise event processor */
     rc = uiox_kbd_event_init(&sys->event_ctx, ev_cfg);
     if (rc < 0) return rc;
 
     /* Initialise keymap */
     rc = uiox_kbd_map_init(layout);
     if (rc < 0) return rc;
 
     sys->state           = UIOX_KBD_SUBSYS_STOPPED;
     sys->dim_timeout_ms  = 0u;
     sys->active_backlight= 255u;
     sys->dim_backlight   = 32u;
     return 0;
 }
 
 int uiox_kbd_subsys_start(uiox_kbd_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     sys->state = UIOX_KBD_SUBSYS_RUNNING;
     sys->last_activity_ms = 0u;
     return 0;
 }
 
 void uiox_kbd_subsys_stop(uiox_kbd_subsys_t *sys)
 {
     if (!sys) return;
     sys->state = UIOX_KBD_SUBSYS_STOPPED;
 }
 
 /* =========================================================================
  * Shortcut dispatch
  * ====================================================================== */
 
 static void dispatch_shortcuts(uiox_kbd_subsys_t    *sys,
                                 const uiox_kbd_event_t *ev)
 {
     if (ev->ev_type != UIOX_KBD_EV_PRESS) return;
 
     for (uint8_t i = 0; i < sys->shortcut_count; i++) {
         uiox_kbd_shortcut_t *s = &sys->shortcuts[i];
         if (!s->enabled) continue;
         if (s->keycode   != ev->keycode)   continue;
         if (s->modifiers != ev->modifiers) continue;
         if (s->fn) s->fn(s->ctx);
     }
 }
 
 /* =========================================================================
  * Callback dispatch
  * ====================================================================== */
 
 static void dispatch_callbacks(uiox_kbd_subsys_t    *sys,
                                 const uiox_kbd_event_t *ev)
 {
     for (uint8_t i = 0; i < sys->callback_count; i++) {
         uiox_kbd_callback_t *cb = &sys->callbacks[i];
         if (!cb->enabled) continue;
         if (cb->filter_type != 0u && cb->filter_type != ev->ev_type)
             continue;
         if (cb->fn) cb->fn(ev, cb->ctx);
     }
 }
 
 /* =========================================================================
  * Backlight auto-dim
  * ====================================================================== */
 
 static void update_dim(uiox_kbd_subsys_t *sys, uint32_t now_ms,
                         bool activity)
 {
     if (!sys->dim_timeout_ms) return;
 
     if (activity) {
         sys->last_activity_ms = now_ms;
         /* Restore full brightness */
         if (sys->kif.hw)
             uiox_kbd_hw_set_backlight(sys->kif.hw, sys->active_backlight);
         return;
     }
 
     if ((now_ms - sys->last_activity_ms) >= sys->dim_timeout_ms) {
         if (sys->kif.hw)
             uiox_kbd_hw_set_backlight(sys->kif.hw, sys->dim_backlight);
     }
 }
 
 /* =========================================================================
  * LED synchronisation
  * ====================================================================== */
 
 void uiox_kbd_subsys_sync_leds(uiox_kbd_subsys_t *sys)
 {
     if (!sys || !sys->kif.hw) return;
     uint8_t leds = 0u;
     if (sys->event_ctx.caps_lock)   leds |= UIOX_KBD_LED_CAPSLOCK;
     if (sys->event_ctx.num_lock)    leds |= UIOX_KBD_LED_NUMLOCK;
     if (sys->event_ctx.scroll_lock) leds |= UIOX_KBD_LED_SCROLLLOCK;
     if (leds != sys->led_state) {
         uiox_kbd_hw_set_leds(sys->kif.hw, leds);
         sys->led_state = leds;
     }
 }
 
 /* =========================================================================
  * Main tick
  * ====================================================================== */
 
 int uiox_kbd_subsys_tick(uiox_kbd_subsys_t *sys,
                           uint32_t           now_ms,
                           uint64_t           ts_ns)
 {
     if (!sys || sys->state != UIOX_KBD_SUBSYS_RUNNING) return 0;
 
     /* 1. Scan hardware → push to raw ring buffer */
     uiox_kbd_if_scan(&sys->kif, &sys->raw_rb, ts_ns);
     sys->scan_count++;
 
     /* 2. Process raw events → cooked ring buffer */
     int n = uiox_kbd_event_process(&sys->event_ctx,
                                     &sys->raw_rb,
                                     &sys->cooked_rb,
                                     now_ms);
 
     /* 3. Dispatch cooked events to shortcuts and callbacks */
     bool had_activity = false;
     uiox_kbd_event_t ev;
     while (uiox_kbd_buf_pop(&sys->cooked_rb, &ev)) {
         /* Update statistics */
         switch (ev.ev_type) {
         case UIOX_KBD_EV_PRESS:   sys->total_keypresses++; break;
         case UIOX_KBD_EV_RELEASE: sys->total_releases++;   break;
         case UIOX_KBD_EV_REPEAT:  sys->total_repeats++;    break;
         default: break;
         }
 
         had_activity = (ev.ev_type == UIOX_KBD_EV_PRESS);
         dispatch_shortcuts(sys, &ev);
         dispatch_callbacks(sys, &ev);
 
         /* Re-push to cooked buffer for application pop */
         uiox_kbd_buf_push(&sys->cooked_rb, &ev);
     }
 
     /* 4. Sync LEDs */
     uiox_kbd_subsys_sync_leds(sys);
 
     /* 5. Backlight dim */
     update_dim(sys, now_ms, had_activity);
 
     return n;
 }
 
 /* =========================================================================
  * Registration
  * ====================================================================== */
 
 int uiox_kbd_subsys_add_shortcut(uiox_kbd_subsys_t     *sys,
                                   uint8_t                modifiers,
                                   uint16_t               keycode,
                                   uiox_kbd_shortcut_fn_t fn,
                                   void                  *ctx,
                                   const char            *name)
 {
     if (!sys || !fn) return -EINVAL;
     if (sys->shortcut_count >= UIOX_KBD_MAX_SHORTCUTS) return -ENOSPC;
     uiox_kbd_shortcut_t *s = &sys->shortcuts[sys->shortcut_count++];
     s->modifiers = modifiers;
     s->keycode   = keycode;
     s->fn        = fn;
     s->ctx       = ctx;
     s->name      = name;
     s->enabled   = true;
     return 0;
 }
 
 int uiox_kbd_subsys_add_callback(uiox_kbd_subsys_t   *sys,
                                   uiox_kbd_event_fn_t  fn,
                                   void                *ctx,
                                   uint8_t              filter_type)
 {
     if (!sys || !fn) return -EINVAL;
     if (sys->callback_count >= UIOX_KBD_MAX_CALLBACKS) return -ENOSPC;
     uiox_kbd_callback_t *cb = &sys->callbacks[sys->callback_count++];
     cb->fn          = fn;
     cb->ctx         = ctx;
     cb->filter_type = filter_type;
     cb->enabled     = true;
     return 0;
 }
 
 void uiox_kbd_subsys_set_dim(uiox_kbd_subsys_t *sys,
                               uint32_t           timeout_ms,
                               uint8_t            active_level,
                               uint8_t            dim_level)
 {
     if (!sys) return;
     sys->dim_timeout_ms   = timeout_ms;
     sys->active_backlight = active_level;
     sys->dim_backlight    = dim_level;
 }
 