/**
 * @file    uiox_mouse_if.c
 * @brief   UIOX Mouse interface driver implementation.
 * @date    2026-06-01
 */

 #include "uiox_mouse_if.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_mouse_if_config(uiox_mouse_if_t *mif, uiox_mouse_hw_t *hw)
 {
     if (!mif || !hw) return -EINVAL;
     memset(mif, 0, sizeof(*mif));
     mif->hw     = hw;
     mif->primed = true;
     return 0;
 }
 
 int uiox_mouse_if_poll(uiox_mouse_if_t    *mif,
                         uiox_mouse_ringbuf_t *dst_rb,
                         uint32_t             now_ms)
 {
     if (!mif || !dst_rb || !mif->primed) return -EINVAL;
     (void)now_ms;
     mif->stats.poll_count++;
 
     /* Connection check */
     bool connected = uiox_mouse_hw_connected(mif->hw);
     if (connected != mif->prev_connected) {
         mif->prev_connected = connected;
         uiox_mouse_event_t ev = {
             .type  = connected ? UIOX_MOUSE_EV_CONNECT
                                : UIOX_MOUSE_EV_DISCONNECT,
             .ts_ns = 0,
         };
         if (!uiox_mouse_buf_push(dst_rb, &ev))
             mif->stats.reports_dropped++;
         else
             connected ? mif->stats.connect_events++
                       : mif->stats.disconnect_events++;
         if (!connected) return 1;
     }
     if (!connected) return 0;
 
     /* Read report from hardware */
     uiox_mouse_raw_t raw;
     int rc = uiox_mouse_hw_read_report(mif->hw, &raw);
     if (rc <= 0) return rc;
 
     mif->stats.reports_received++;
     int pushed = 0;
 
     /* Movement event */
     if (raw.dx || raw.dy) {
         uiox_mouse_event_t ev = {
             .type    = UIOX_MOUSE_EV_MOVE,
             .dx      = raw.dx,
             .dy      = raw.dy,
             .buttons = raw.buttons,
             .ts_ns   = raw.ts_ns,
         };
         if (uiox_mouse_buf_push(dst_rb, &ev)) pushed++;
         else mif->stats.reports_dropped++;
     }
 
     /* Button state changes */
     uint8_t changed = raw.buttons ^ mif->prev_buttons;
     for (uint8_t b = 0; b < UIOX_MOUSE_MAX_BUTTONS; b++) {
         if (!(changed & (1u << b))) continue;
         bool pressed = (raw.buttons >> b) & 1u;
         uiox_mouse_event_t ev = {
             .type    = pressed ? UIOX_MOUSE_EV_BTN_PRESS
                                : UIOX_MOUSE_EV_BTN_RELEASE,
             .button  = b,
             .buttons = raw.buttons,
             .ts_ns   = raw.ts_ns,
         };
         if (uiox_mouse_buf_push(dst_rb, &ev)) pushed++;
         else mif->stats.reports_dropped++;
     }
     mif->prev_buttons = raw.buttons;
 
     /* Scroll events */
     if (raw.dz) {
         uiox_mouse_event_t ev = {
             .type    = UIOX_MOUSE_EV_SCROLL_V,
             .dz      = raw.dz,
             .buttons = raw.buttons,
             .ts_ns   = raw.ts_ns,
         };
         if (uiox_mouse_buf_push(dst_rb, &ev)) pushed++;
     }
     if (raw.dw) {
         uiox_mouse_event_t ev = {
             .type    = UIOX_MOUSE_EV_SCROLL_H,
             .dw      = raw.dw,
             .buttons = raw.buttons,
             .ts_ns   = raw.ts_ns,
         };
         if (uiox_mouse_buf_push(dst_rb, &ev)) pushed++;
     }
 
     return pushed;
 }
 
 void uiox_mouse_if_stats_get(const uiox_mouse_if_t *mif,
                                uiox_mouse_if_stats_t *out)
 {
     if (!mif || !out) return;
     memcpy(out, &mif->stats, sizeof(*out));
 }
 
 void uiox_mouse_if_stats_reset(uiox_mouse_if_t *mif)
 {
     if (!mif) return;
     memset(&mif->stats, 0, sizeof(mif->stats));
 }
 