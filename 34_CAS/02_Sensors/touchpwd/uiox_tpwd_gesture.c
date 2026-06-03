/**
 * @file    uiox_tpwd_gesture.c
 * @brief   UIOX Touch-Password gesture engine implementation.
 * @date    2026-06-01
 */

 #include "uiox_tpwd_gesture.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_tpwd_gesture_init(uiox_tpwd_gesture_t           *g,
                             uiox_tpwd_evtbuf_t            *src_rb,
                             const uiox_tpwd_gesture_cfg_t *cfg)
 {
     if (!g || !src_rb || !cfg) return -EINVAL;
     memset(g, 0, sizeof(*g));
     memcpy(&g->cfg, cfg, sizeof(*cfg));
     g->src_rb = src_rb;
     g->running = false;
     return 0;
 }
 
 int uiox_tpwd_gesture_start(uiox_tpwd_gesture_t *g, uint32_t now_ms)
 {
     if (!g) return -EINVAL;
     memset(&g->current, 0, sizeof(g->current));
     g->current.mode      = g->cfg.mode;
     g->current.start_ts_ns = (uint64_t)now_ms * 1000000ULL;
     g->start_ms          = now_ms;
     g->last_cell         = 0xFFu;
     g->running           = true;
     return 0;
 }
 
 void uiox_tpwd_gesture_cancel(uiox_tpwd_gesture_t *g)
 {
     if (!g) return;
     /* Zero any partial credential before clearing */
     volatile uint8_t *p = (volatile uint8_t *)g->current.pin;
     for (size_t i = 0; i < sizeof(g->current.pin); i++) p[i] = 0;
     p = (volatile uint8_t *)g->current.pattern;
     for (size_t i = 0; i < sizeof(g->current.pattern); i++) p[i] = 0;
     g->current.cancelled = true;
     g->current.complete  = false;
     g->current.pin_len   = 0;
     g->current.pattern_len = 0;
     g->running = false;
 }
 
 /* -------------------------------------------------------------------------
  * PIN mode: each PRESS on a valid cell appends the digit.
  * '#' (cell 11) = confirm/submit.
  * '*' (cell 9)  = backspace.
  * ---------------------------------------------------------------------- */
 
 static void pin_on_press(uiox_tpwd_gesture_t *g, uint8_t cell)
 {
     if (cell >= UIOX_TPWD_GRID_CELLS) return;
     char digit = UIOX_TPWD_CELL_TO_DIGIT(cell);
 
     if (digit == '#') {
         /* Submit */
         if (g->current.pin_len >= g->cfg.min_pin_len)
             g->current.complete = true;
         return;
     }
     if (digit == '*') {
         /* Backspace */
         if (g->current.pin_len > 0) {
             g->current.pin_len--;
             g->current.pin[g->current.pin_len] = '\0';
         }
         return;
     }
     if (g->current.pin_len < g->cfg.max_pin_len) {
         g->current.pin[g->current.pin_len++] = digit;
         g->current.pin[g->current.pin_len]   = '\0';
     }
 }
 
 /* -------------------------------------------------------------------------
  * Pattern mode: each unique cell visited (while finger is down) is appended.
  * Completion triggered when finger lifts and min_pattern_pts met.
  * ---------------------------------------------------------------------- */
 
 static void pattern_on_press(uiox_tpwd_gesture_t *g, uint8_t cell)
 {
     if (cell >= UIOX_TPWD_GRID_CELLS) return;
     if (cell == g->last_cell) return;  /* same cell — ignore */
 
     /* Check if cell already in pattern */
     for (uint8_t i = 0; i < g->current.pattern_len; i++)
         if (g->current.pattern[i] == cell) return;
 
     if (g->current.pattern_len < UIOX_TPWD_MAX_PATTERN_PTS) {
         g->current.pattern[g->current.pattern_len++] = cell;
         g->last_cell = cell;
     }
 }
 
 static void pattern_on_release(uiox_tpwd_gesture_t *g)
 {
     if (g->current.pattern_len >= g->cfg.min_pattern_pts)
         g->current.complete = true;
     else
         uiox_tpwd_gesture_cancel(g);
 }
 
 /* -------------------------------------------------------------------------
  * Swipe mode: compare start and end coordinates to determine direction.
  * ---------------------------------------------------------------------- */
 
 static uint16_t s_swipe_start_x = 0;
 static uint16_t s_swipe_start_y = 0;
 
 static void swipe_on_press(uint16_t xn, uint16_t yn)
 {
     s_swipe_start_x = xn;
     s_swipe_start_y = yn;
 }
 
 static void swipe_on_release(uiox_tpwd_gesture_t *g,
                               uint16_t xn, uint16_t yn)
 {
     int16_t dx = (int16_t)(xn - s_swipe_start_x);
     int16_t dy = (int16_t)(yn - s_swipe_start_y);
     uint16_t adx = (uint16_t)(dx < 0 ? -dx : dx);
     uint16_t ady = (uint16_t)(dy < 0 ? -dy : dy);
 
     if (adx < 100 && ady < 100) return; /* not a swipe */
 
     if (adx > ady)
         g->current.swipe = (dx > 0) ? UIOX_TPWD_SWIPE_RIGHT
                                      : UIOX_TPWD_SWIPE_LEFT;
     else
         g->current.swipe = (dy > 0) ? UIOX_TPWD_SWIPE_DOWN
                                      : UIOX_TPWD_SWIPE_UP;
     g->current.complete = true;
 }
 
 /* =========================================================================
  * Main process function
  * ====================================================================== */
 
 int uiox_tpwd_gesture_process(uiox_tpwd_gesture_t *g, uint32_t now_ms)
 {
     if (!g || !g->src_rb || !g->running) return -EINVAL;
 
     /* Entry timeout check */
     if (g->cfg.entry_timeout_ms &&
         (now_ms - g->start_ms) >= g->cfg.entry_timeout_ms) {
         uiox_tpwd_gesture_cancel(g);
         return 1;   /* "complete" — with cancelled flag set */
     }
 
     uiox_tpwd_raw_evt_t raw;
     while (uiox_tpwd_evtbuf_pop(g->src_rb, &raw)) {
 
         /* Timeout sentinel from IF layer */
         if (raw.gesture_id == 0xFFu) {
             if (g->cfg.mode == UIOX_TPWD_MODE_PATTERN)
                 pattern_on_release(g);
             else if (g->cfg.mode == UIOX_TPWD_MODE_SWIPE)
                 swipe_on_release(g,
                     raw.pts[0].x, raw.pts[0].y);
             if (g->current.complete || g->current.cancelled) {
                 g->current.end_ts_ns =
                     (uint64_t)now_ms * 1000000ULL;
                 g->running = false;
                 return 1;
             }
             continue;
         }
 
         bool pressed = raw.num_points > 0 && raw.pts[0].active;
         uint8_t cell = raw.gesture_id;  /* IF layer put cell index here */
         uint16_t xn  = raw.pts[0].x;   /* IF layer normalised to 0..1000 */
         uint16_t yn  = raw.pts[0].y;
 
         if (pressed) {
             switch (g->cfg.mode) {
             case UIOX_TPWD_MODE_PIN:
                 pin_on_press(g, cell);
                 break;
             case UIOX_TPWD_MODE_PATTERN:
                 pattern_on_press(g, cell);
                 break;
             case UIOX_TPWD_MODE_SWIPE:
                 swipe_on_press(xn, yn);
                 break;
             }
         } else {
             /* Release */
             if (g->cfg.mode == UIOX_TPWD_MODE_PATTERN)
                 pattern_on_release(g);
             else if (g->cfg.mode == UIOX_TPWD_MODE_SWIPE)
                 swipe_on_release(g, xn, yn);
         }
 
         if (g->current.complete || g->current.cancelled) {
             g->current.end_ts_ns = (uint64_t)now_ms * 1000000ULL;
             g->running = false;
             return 1;
         }
     }
     return 0;
 }
 
 int uiox_tpwd_gesture_serialise(const uiox_tpwd_credential_t *cred,
                                   uint8_t *buf, uint16_t buf_len)
 {
     if (!cred || !buf || !buf_len) return -EINVAL;
     switch (cred->mode) {
     case UIOX_TPWD_MODE_PIN:
         if (cred->pin_len == 0 || cred->pin_len > buf_len) return -EINVAL;
         memcpy(buf, cred->pin, cred->pin_len);
         return (int)cred->pin_len;
 
     case UIOX_TPWD_MODE_PATTERN:
         if (cred->pattern_len == 0 ||
             cred->pattern_len > buf_len) return -EINVAL;
         memcpy(buf, cred->pattern, cred->pattern_len);
         return (int)cred->pattern_len;
 
     case UIOX_TPWD_MODE_SWIPE:
         buf[0] = (uint8_t)cred->swipe;
         return 1;
 
     default:
         return -EINVAL;
     }
 }
 