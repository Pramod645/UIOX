/**
 * @file    uiox_tpwd_if.h
 * @brief   UIOX Touch-Password interface driver.
 *
 * Sits between HAL and gesture engine. Manages:
 *   - Continuous touch scanning (polling or interrupt-driven)
 *   - Raw coordinate normalisation to logical grid
 *   - Debounce (configurable window)
 *   - Lift-off detection and timeout
 *   - Touch-cell mapping (numpad, pattern grid, swipe zone)
 *   - Interface statistics
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_TPWD_IF_H
 #define UIOX_TPWD_IF_H
 
 #include "uiox_tpwd_hw.h"
 #include "uiox_tpwd_buf.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Logical grid (mapped from raw touch coordinates)
  * ====================================================================== */
 
 #define UIOX_TPWD_GRID_COLS     3
 #define UIOX_TPWD_GRID_ROWS     4    /**< 0-9 + * + # like numpad        */
 #define UIOX_TPWD_GRID_CELLS    (UIOX_TPWD_GRID_COLS * UIOX_TPWD_GRID_ROWS)

 /* =========================================================================
  * Logical touch event (post-normalisation)
  * ====================================================================== */
 
 typedef enum {
     UIOX_TPWD_TE_PRESS   = 0,
     UIOX_TPWD_TE_RELEASE,
     UIOX_TPWD_TE_MOVE,
     UIOX_TPWD_TE_HOLD,
     UIOX_TPWD_TE_TIMEOUT,
 } uiox_tpwd_te_type_t;
 
 typedef struct {
     uiox_tpwd_te_type_t type;
     uint8_t             cell;        /**< Grid cell index (0..N-1)        */
     uint8_t             col, row;    /**< Logical grid column and row      */
     uint16_t            x_norm;      /**< Normalised X (0..1000)          */
     uint16_t            y_norm;      /**< Normalised Y (0..1000)          */
     uint8_t             pressure;
     uint64_t            ts_ns;
     uint32_t            hold_ms;     /**< Duration held (for hold events) */
 } uiox_tpwd_touch_evt_t;
 
 /* =========================================================================
  * Interface statistics
  * ====================================================================== */
 
 typedef struct {
     uint64_t  total_touches;
     uint64_t  total_releases;
     uint64_t  debounce_filtered;
     uint64_t  overflow_events;
     uint32_t  scan_count;
 } uiox_tpwd_if_stats_t;
 
 /* =========================================================================
  * Interface descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_tpwd_hw_t       *hw;
     uiox_tpwd_evtbuf_t    raw_rb;       /**< Raw event ring buffer        */
     uiox_tpwd_if_stats_t  stats;
 
     /* Debounce state */
     uint32_t              debounce_ms;
     uint32_t              last_event_ms;
     bool                  prev_pressed;
 
     /* Hold detection */
     uint32_t              press_start_ms;
     uint32_t              hold_threshold_ms;
 
     /* Touch timeout (lift-off after no touch) */
     uint32_t              timeout_ms;
     uint32_t              last_touch_ms;
 
     bool                  primed;
 } uiox_tpwd_if_t;
 
 /* =========================================================================
  * Interface API
  * ====================================================================== */
 
 int  uiox_tpwd_if_config   (uiox_tpwd_if_t *tif,
                              uiox_tpwd_hw_t *hw,
                              uint32_t debounce_ms,
                              uint32_t hold_threshold_ms,
                              uint32_t timeout_ms);
 
 /**
  * @brief  Perform one scan cycle.
  *
  * Reads raw touch, debounces, normalises to logical grid,
  * and pushes touch events to dst_rb.
  *
  * @param  tif     Interface descriptor.
  * @param  dst_rb  Destination event ring buffer (for gesture layer).
  * @param  now_ms  Monotonic time (ms).
  * @return Number of events pushed, or <0 on error.
  */
 int  uiox_tpwd_if_scan     (uiox_tpwd_if_t *tif,
                              uiox_tpwd_evtbuf_t *dst_rb,
                              uint32_t now_ms);
 
 /** Map raw (x,y) to logical grid cell. Returns 0xFF if outside grid. */
 uint8_t uiox_tpwd_if_map_cell(const uiox_tpwd_hw_t *hw,
                                uint16_t x, uint16_t y,
                                uint8_t *col_out, uint8_t *row_out);
 
 void uiox_tpwd_if_stats_get  (const uiox_tpwd_if_t *tif,
                                uiox_tpwd_if_stats_t *out);
 void uiox_tpwd_if_stats_reset(uiox_tpwd_if_t *tif);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_TPWD_IF_H */
 
 