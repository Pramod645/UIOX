/**
 * @file    uiox_mouse_if.h
 * @brief   UIOX Mouse interface driver (report parsing, polling).
 *
 * Manages:
 *   - Continuous hardware polling or IRQ-driven report fetch
 *   - Raw report validation and sanity check
 *   - Button state change detection
 *   - Overflow / disconnect detection
 *   - Interface statistics
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_MOUSE_IF_H
 #define UIOX_MOUSE_IF_H
 
 #include "uiox_mouse_hw.h"
 #include "uiox_mouse_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uint64_t  reports_received;
     uint64_t  reports_dropped;
     uint64_t  connect_events;
     uint64_t  disconnect_events;
     uint32_t  poll_count;
 } uiox_mouse_if_stats_t;
 
 typedef struct {
     uiox_mouse_hw_t      *hw;
     uint8_t               prev_buttons;  /**< Previous button state       */
     bool                  prev_connected;
     uiox_mouse_if_stats_t stats;
     bool                  primed;
 } uiox_mouse_if_t;
 
 int  uiox_mouse_if_config (uiox_mouse_if_t *mif, uiox_mouse_hw_t *hw);
 
 /**
  * @brief  Poll for new data and push events to dst_rb.
  * @return Number of events pushed, or <0 on error.
  */
 int  uiox_mouse_if_poll   (uiox_mouse_if_t    *mif,
                             uiox_mouse_ringbuf_t *dst_rb,
                             uint32_t             now_ms);
 
 void uiox_mouse_if_stats_get  (const uiox_mouse_if_t *mif,
                                 uiox_mouse_if_stats_t *out);
 void uiox_mouse_if_stats_reset(uiox_mouse_if_t *mif);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MOUSE_IF_H */
 