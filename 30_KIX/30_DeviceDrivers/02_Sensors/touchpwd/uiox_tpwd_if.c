/**
 * @file    uiox_tpwd_if.c
 * @brief   UIOX Touch-Password interface driver implementation.
 * @date    2026-06-01
 */

 #include "uiox_tpwd_if.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_tpwd_if_config(uiox_tpwd_if_t *tif,
                          uiox_tpwd_hw_t *hw,
                          uint32_t debounce_ms,
                          uint32_t hold_threshold_ms,
                          uint32_t timeout_ms)
 {
     if (!tif || !hw) return -EINVAL;
     memset(tif, 0, sizeof(*tif));
     tif->hw                 = hw;
     tif->debounce_ms        = debounce_ms;
     tif->hold_threshold_ms  = hold_threshold_ms;
     tif->timeout_ms         = timeout_ms;
     uiox_tpwd_evtbuf_init(&tif->raw_rb);
     tif->primed = true;
     return 0;
 }
 
 uint8_t uiox_tpwd_if_map_cell(const uiox_tpwd_hw_t *hw,
                                 uint16_t x, uint16_t y,
                                 uint8_t *col_out, uint8_t *row_out)
 {
     if (!hw) return 0xFFu;
 
     uint16_t W = hw->panel.width;
     uint16_t H = hw->panel.height;
     if (!W || !H || x >= W || y >= H) return 0xFFu;
 
     /* Apply rotation */
     uint16_t lx = x, ly = y;
     switch (hw->panel.rotation) {
     case UIOX_TPWD_ROT_90:
         lx = y; ly = (uint16_t)(W - 1u - x); break;
     case UIOX_TPWD_ROT_180:
         lx = (uint16_t)(W - 1u - x);
         ly = (uint16_t)(H - 1u - y); break;
     case UIOX_TPWD_ROT_270:
         lx = (uint16_t)(H - 1u - y); ly = x; break;
     default: break;
     }
     if (hw->panel.flip_x) lx = (uint16_t)(W - 1u - lx);
     if (hw->panel.flip_y) ly = (uint16_t)(H - 1u - ly);
 
     uint8_t col = (uint8_t)(lx * UIOX_TPWD_GRID_COLS / W);
     uint8_t row = (uint8_t)(ly * UIOX_TPWD_GRID_ROWS / H);
     if (col >= UIOX_TPWD_GRID_COLS) col = UIOX_TPWD_GRID_COLS - 1u;
     if (row >= UIOX_TPWD_GRID_ROWS) row = UIOX_TPWD_GRID_ROWS - 1u;
 
     if (col_out) *col_out = col;
     if (row_out) *row_out = row;
     return (uint8_t)(row * UIOX_TPWD_GRID_COLS + col);
 }
 
 int uiox_tpwd_if_scan(uiox_tpwd_if_t     *tif,
                        uiox_tpwd_evtbuf_t *dst_rb,
                        uint32_t            now_ms)
 {
     if (!tif || !dst_rb || !tif->primed) return -EINVAL;
     tif->stats.scan_count++;
 
     uiox_tpwd_raw_evt_t raw;
     int n = uiox_tpwd_hw_read_touch(tif->hw, &raw);
     if (n < 0) return n;
 
     bool pressed = (n > 0 && raw.pts[0].active);
 
     /* Debounce */
     if (pressed != tif->prev_pressed) {
         if ((now_ms - tif->last_event_ms) < tif->debounce_ms) {
             tif->stats.debounce_filtered++;
             return 0;
         }
         tif->last_event_ms = now_ms;
         tif->prev_pressed  = pressed;
     }
 
     if (pressed) {
         tif->last_touch_ms = now_ms;
         uint16_t x = raw.pts[0].x;
         uint16_t y = raw.pts[0].y;
         uint8_t col, row;
         uint8_t cell = uiox_tpwd_if_map_cell(tif->hw, x, y, &col, &row);
 
         uint16_t W = tif->hw->panel.width;
         uint16_t H = tif->hw->panel.height;
         uint16_t xn = W ? (uint16_t)((uint32_t)x * 1000u / W) : 0u;
         uint16_t yn = H ? (uint16_t)((uint32_t)y * 1000u / H) : 0u;
 
         uiox_tpwd_touch_evt_t ev = {
             .type     = UIOX_TPWD_TE_PRESS,
             .cell     = cell,
             .col      = col,
             .row      = row,
             .x_norm   = xn,
             .y_norm   = yn,
             .pressure = raw.pts[0].pressure,
             .ts_ns    = raw.ts_ns,
             .hold_ms  = 0,
         };
 
         /* Check for hold */
         if (tif->press_start_ms == 0) tif->press_start_ms = now_ms;
         uint32_t held = now_ms - tif->press_start_ms;
         if (held >= tif->hold_threshold_ms) {
             ev.type    = UIOX_TPWD_TE_HOLD;
             ev.hold_ms = held;
         }
 
         /* Push as raw_evt carrying logical info in x/y fields */
         uiox_tpwd_raw_evt_t logical_ev;
         memcpy(&logical_ev, &raw, sizeof(raw));
         logical_ev.pts[0].x = xn;
         logical_ev.pts[0].y = yn;
         logical_ev.gesture_id = cell;
 
         if (!uiox_tpwd_evtbuf_push(dst_rb, &logical_ev))
             tif->stats.overflow_events++;
         else
             tif->stats.total_touches++;
 
     } else if (tif->prev_pressed == false &&
                tif->last_touch_ms &&
                (now_ms - tif->last_touch_ms) >= tif->timeout_ms) {
         /* Timeout — no touch for timeout_ms */
         uiox_tpwd_raw_evt_t timeout_ev;
         memset(&timeout_ev, 0, sizeof(timeout_ev));
         timeout_ev.gesture_id = 0xFFu;  /* sentinel for timeout */
         uiox_tpwd_evtbuf_push(dst_rb, &timeout_ev);
         tif->last_touch_ms  = 0;
         tif->press_start_ms = 0;
 
     } else if (!pressed && tif->prev_pressed) {
         /* Release */
         tif->press_start_ms = 0;
         tif->stats.total_releases++;
     }
 
     return (int)uiox_tpwd_evtbuf_count(dst_rb);
 }
 
 void uiox_tpwd_if_stats_get(const uiox_tpwd_if_t *tif,
                               uiox_tpwd_if_stats_t *out)
 {
     if (!tif || !out) return;
     memcpy(out, &tif->stats, sizeof(*out));
 }
 
 void uiox_tpwd_if_stats_reset(uiox_tpwd_if_t *tif)
 {
     if (!tif) return;
     memset(&tif->stats, 0, sizeof(tif->stats));
 }
 