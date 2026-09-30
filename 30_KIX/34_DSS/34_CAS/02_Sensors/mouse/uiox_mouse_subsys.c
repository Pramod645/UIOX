/**
 * @file    uiox_mouse_subsys.c
 * @brief   UIOX Mouse subsystem implementation.
 * @date    2026-06-01
 */

 #include "uiox_mouse_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_mouse_subsys_init(uiox_mouse_subsys_t          *sys,
                             uiox_mouse_hw_t              *hw,
                             uiox_mouse_proto_t            proto,
                             const uiox_mouse_event_cfg_t *ev_cfg)
 {
     if (!sys || !hw || !ev_cfg) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
 
     uiox_mouse_buf_init(&sys->raw_rb);
     uiox_mouse_buf_init(&sys->cooked_rb);
 
     int rc = uiox_mouse_if_config(&sys->mif, hw);
     if (rc < 0) return rc;
 
     rc = uiox_mouse_proto_init(&sys->proto, &sys->mif, proto);
     if (rc < 0) return rc;
 
     rc = uiox_mouse_event_init(&sys->event, ev_cfg);
     if (rc < 0) return rc;
 
     sys->state = UIOX_MOUSE_SUBSYS_STOPPED;
     return 0;
 }
 
 int uiox_mouse_subsys_start(uiox_mouse_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     int rc = uiox_mouse_hw_enable(sys->mif.hw);
     if (rc < 0) return rc;
     sys->state = UIOX_MOUSE_SUBSYS_RUNNING;
     return 0;
 }
 
 void uiox_mouse_subsys_stop(uiox_mouse_subsys_t *sys)
 {
     if (!sys) return;
     uiox_mouse_hw_disable(sys->mif.hw);
     sys->state = UIOX_MOUSE_SUBSYS_STOPPED;
 }
 
 /* -------------------------------------------------------------------------
  * Zone hit-test and dispatch
  * ---------------------------------------------------------------------- */
 
 static void zone_check(uiox_mouse_subsys_t    *sys,
                         const uiox_mouse_event_t *ev)
 {
     for (uint8_t i = 0; i < sys->zone_count; i++) {
         uiox_mouse_zone_t *z = &sys->zones[i];
         if (!z->active) continue;
 
         bool inside = (ev->x >= z->x && ev->x < z->x + z->w &&
                        ev->y >= z->y && ev->y < z->y + z->h);
 
         if (inside && !z->cursor_inside) {
             z->cursor_inside = true;
             if (z->on_enter) z->on_enter(ev->x, ev->y, ev->buttons, z->ctx);
         } else if (!inside && z->cursor_inside) {
             z->cursor_inside = false;
             if (z->on_leave) z->on_leave(ev->x, ev->y, ev->buttons, z->ctx);
         }
 
         if (inside && ev->type == UIOX_MOUSE_EV_CLICK)
             if (z->on_click) z->on_click(ev->x, ev->y, ev->buttons, z->ctx);
     }
 }
 
 /* -------------------------------------------------------------------------
  * Callback dispatch
  * ---------------------------------------------------------------------- */
 
 static void cb_dispatch(uiox_mouse_subsys_t      *sys,
                          const uiox_mouse_event_t *ev)
 {
     for (uint8_t i = 0; i < sys->cb_count; i++) {
         uiox_mouse_cb_entry_t *cb = &sys->callbacks[i];
         if (!cb->active) continue;
         if (cb->filter != 0u && cb->filter != (uint8_t)ev->type) continue;
         if (cb->fn) cb->fn(ev, cb->ctx);
     }
 }
 
 /* =========================================================================
  * Tick — full pipeline
  * ====================================================================== */
 
 void uiox_mouse_subsys_tick(uiox_mouse_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state == UIOX_MOUSE_SUBSYS_STOPPED) return;
     sys->tick_count++;
 
     /* 1. Poll hardware → raw ring buffer */
     uiox_mouse_if_poll(&sys->mif, &sys->raw_rb, now_ms);
 
     /* 2. Process events: debounce, accel, click, dblclick */
     uiox_mouse_event_process(&sys->event, &sys->raw_rb,
                               &sys->cooked_rb, now_ms);
 
     /* 3. Dispatch cooked events */
     uiox_mouse_event_t ev;
     while (uiox_mouse_buf_pop(&sys->cooked_rb, &ev)) {
         /* Statistics */
         switch (ev.type) {
         case UIOX_MOUSE_EV_MOVE:       sys->total_moves++;     break;
         case UIOX_MOUSE_EV_CLICK:      sys->total_clicks++;    break;
         case UIOX_MOUSE_EV_DBLCLICK:   sys->total_dblclicks++; break;
         case UIOX_MOUSE_EV_SCROLL_V:
         case UIOX_MOUSE_EV_SCROLL_H:   sys->total_scrolls++;   break;
         case UIOX_MOUSE_EV_DISCONNECT:
             sys->state = UIOX_MOUSE_SUBSYS_DISCONNECTED; break;
         case UIOX_MOUSE_EV_CONNECT:
             sys->state = UIOX_MOUSE_SUBSYS_RUNNING; break;
         default: break;
         }
 
         /* Zone check (for MOVE and CLICK events) */
         if (ev.type == UIOX_MOUSE_EV_MOVE ||
             ev.type == UIOX_MOUSE_EV_CLICK)
             zone_check(sys, &ev);
 
         /* Callback dispatch */
         cb_dispatch(sys, &ev);
 
         /* Re-push to cooked buffer for application poll */
         uiox_mouse_buf_push(&sys->cooked_rb, &ev);
     }
 }
 
 int uiox_mouse_subsys_add_zone(uiox_mouse_subsys_t     *sys,
                                 const uiox_mouse_zone_t *zone)
 {
     if (!sys || !zone) return -EINVAL;
     if (sys->zone_count >= UIOX_MOUSE_MAX_ZONES) return -ENOSPC;
     memcpy(&sys->zones[sys->zone_count], zone, sizeof(*zone));
     sys->zones[sys->zone_count].active = true;
     sys->zones[sys->zone_count].cursor_inside = false;
     sys->zone_count++;
     return 0;
 }
 
 int uiox_mouse_subsys_add_cb(uiox_mouse_subsys_t *sys,
                               uiox_mouse_evt_cb_t  fn,
                               void                *ctx,
                               uint8_t              filter)
 {
     if (!sys || !fn) return -EINVAL;
     if (sys->cb_count >= UIOX_MOUSE_MAX_CALLBACKS) return -ENOSPC;
     uiox_mouse_cb_entry_t *cb = &sys->callbacks[sys->cb_count++];
     cb->fn     = fn;
     cb->ctx    = ctx;
     cb->filter = filter;
     cb->active = true;
     return 0;
 }
 
 void uiox_mouse_subsys_cursor(const uiox_mouse_subsys_t *sys,
                                int32_t *x, int32_t *y)
 {
     if (!sys) return;
     uiox_mouse_event_cursor(&sys->event, x, y);
 }
 
 void uiox_mouse_subsys_warp(uiox_mouse_subsys_t *sys, int32_t x, int32_t y)
 {
     if (!sys) return;
     uiox_mouse_event_warp(&sys->event, x, y);
 }
 