/**
 * @file    uiox_mouse_event.c
 * @brief   UIOX Mouse event processing implementation.
 * @date    2026-06-01
 */

 #include "uiox_mouse_event.h"
 #include <string.h>
 #include <math.h>
 #include <errno.h>
 
 int uiox_mouse_event_init(uiox_mouse_event_ctx_t       *ctx,
                            const uiox_mouse_event_cfg_t *cfg)
 {
     if (!ctx || !cfg) return -EINVAL;
     memset(ctx, 0, sizeof(*ctx));
     memcpy(&ctx->cfg, cfg, sizeof(*cfg));
     ctx->cursor_x = cfg->screen_w / 2;
     ctx->cursor_y = cfg->screen_h / 2;
     return 0;
 }
 
 /* -------------------------------------------------------------------------
  * Pointer acceleration: applies velocity-based multiplier
  * ---------------------------------------------------------------------- */
 
 static float accelerate(float delta, float factor, float threshold)
 {
     float speed = fabsf(delta);
     if (speed <= threshold) return delta;
     float scale = 1.0f + (speed - threshold) * factor / threshold;
     return delta * scale;
 }
 
 /* -------------------------------------------------------------------------
  * Clamp cursor to screen bounds
  * ---------------------------------------------------------------------- */
 
 static int32_t clamp32(int32_t v, int32_t lo, int32_t hi)
 {
     if (v < lo) return lo;
     if (v > hi) return hi;
     return v;
 }
 
 /* =========================================================================
  * Main process function
  * ====================================================================== */
 
 int uiox_mouse_event_process(uiox_mouse_event_ctx_t *ctx,
                               uiox_mouse_ringbuf_t   *src_rb,
                               uiox_mouse_ringbuf_t   *dst_rb,
                               uint32_t                now_ms)
 {
     if (!ctx || !src_rb || !dst_rb) return -EINVAL;
     int emitted = 0;
 
     uiox_mouse_event_t ev;
     while (uiox_mouse_buf_pop(src_rb, &ev)) {
 
         switch (ev.type) {
 
         case UIOX_MOUSE_EV_MOVE: {
             /* Apply inversion */
             float fdx = (float)ev.dx;
             float fdy = ctx->cfg.invert_y ? -(float)ev.dy : (float)ev.dy;
 
             /* Pointer acceleration */
             fdx = accelerate(fdx, ctx->cfg.accel_factor,
                              ctx->cfg.accel_threshold);
             fdy = accelerate(fdy, ctx->cfg.accel_factor,
                              ctx->cfg.accel_threshold);
 
             /* Sub-pixel accumulation */
             ctx->accel_x += fdx;
             ctx->accel_y += fdy;
             int32_t ix = (int32_t)ctx->accel_x;
             int32_t iy = (int32_t)ctx->accel_y;
             ctx->accel_x -= (float)ix;
             ctx->accel_y -= (float)iy;
 
             /* Clamp cursor */
             ctx->cursor_x = clamp32(ctx->cursor_x + ix,
                                     0, ctx->cfg.screen_w - 1);
             ctx->cursor_y = clamp32(ctx->cursor_y + iy,
                                     0, ctx->cfg.screen_h - 1);
 
             ev.x  = ctx->cursor_x;
             ev.y  = ctx->cursor_y;
             ev.dx = (int16_t)ix;
             ev.dy = (int16_t)iy;
 
             if (uiox_mouse_buf_push(dst_rb, &ev)) emitted++;
             break;
         }
 
         case UIOX_MOUSE_EV_BTN_PRESS: {
             uint8_t b = ev.button;
             if (b >= UIOX_MOUSE_MAX_BUTTONS) break;
             uiox_mouse_btn_state_t *bs = &ctx->buttons[b];
 
             /* Debounce check */
             if (!bs->confirmed ||
                 (now_ms - bs->change_ts_ms) >= ctx->cfg.debounce_ms) {
                 bs->pressed      = true;
                 bs->confirmed    = true;
                 bs->press_ts_ms  = now_ms;
                 bs->change_ts_ms = now_ms;
                 ev.x = ctx->cursor_x;
                 ev.y = ctx->cursor_y;
                 if (uiox_mouse_buf_push(dst_rb, &ev)) emitted++;
             }
             break;
         }
 
         case UIOX_MOUSE_EV_BTN_RELEASE: {
             uint8_t b = ev.button;
             if (b >= UIOX_MOUSE_MAX_BUTTONS) break;
             uiox_mouse_btn_state_t *bs = &ctx->buttons[b];
             if (!bs->pressed) break;
 
             bs->pressed      = false;
             bs->change_ts_ms = now_ms;
             ev.x = ctx->cursor_x;
             ev.y = ctx->cursor_y;
             if (uiox_mouse_buf_push(dst_rb, &ev)) emitted++;
 
             /* Click detection */
             uint32_t hold_ms = now_ms - bs->press_ts_ms;
             if (hold_ms <= ctx->cfg.click_timeout_ms) {
                 uiox_mouse_event_t click_ev = ev;
                 click_ev.type = UIOX_MOUSE_EV_CLICK;
 
                 /* Double-click detection */
                 uint32_t gap = now_ms - bs->last_click_ts_ms;
                 if (bs->click_count > 0 &&
                     gap <= ctx->cfg.dblclick_timeout_ms) {
                     click_ev.type         = UIOX_MOUSE_EV_DBLCLICK;
                     click_ev.click_count  = 2u;
                     bs->click_count       = 0;
                 } else {
                     click_ev.click_count  = 1u;
                     bs->click_count       = 1;
                 }
                 bs->last_click_ts_ms = now_ms;
                 if (uiox_mouse_buf_push(dst_rb, &click_ev)) emitted++;
             }
             break;
         }
 
         case UIOX_MOUSE_EV_SCROLL_V: {
             if (ctx->cfg.invert_scroll) ev.dz = -ev.dz;
             if (uiox_mouse_buf_push(dst_rb, &ev)) emitted++;
             break;
         }
 
         case UIOX_MOUSE_EV_SCROLL_H: {
             if (ctx->cfg.invert_scroll) ev.dw = -ev.dw;
             if (uiox_mouse_buf_push(dst_rb, &ev)) emitted++;
             break;
         }
 
         default:
             /* CONNECT / DISCONNECT / GESTURE — pass through unchanged */
             if (uiox_mouse_buf_push(dst_rb, &ev)) emitted++;
             break;
         }
     }
     return emitted;
 }
 
 void uiox_mouse_event_cursor(const uiox_mouse_event_ctx_t *ctx,
                               int32_t *x, int32_t *y)
 {
     if (!ctx) return;
     if (x) *x = ctx->cursor_x;
     if (y) *y = ctx->cursor_y;
 }
 
 void uiox_mouse_event_warp(uiox_mouse_event_ctx_t *ctx,
                             int32_t x, int32_t y)
 {
     if (!ctx) return;
     ctx->cursor_x = clamp32(x, 0, ctx->cfg.screen_w - 1);
     ctx->cursor_y = clamp32(y, 0, ctx->cfg.screen_h - 1);
     ctx->accel_x  = 0.0f;
     ctx->accel_y  = 0.0f;
 }
 