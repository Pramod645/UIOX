/**
 * @file    uiox_kbd_if.h
 * @brief   UIOX Keyboard interface driver.
 *
 * Sits between HAL and event layer. Manages:
 *   - Full matrix scan (all rows, ghost-key filtering)
 *   - Direct GPIO polling
 *   - I2C keyboard controller polling (TCA8418, PCF8574)
 *   - PS/2 scancode reception
 *   - Raw scancode → position mapping
 *   - Key state tracking (pressed/released bitmaps)
 *
 * @date    2026-05-27
 */
//Layer 2 — Interface Driver
 #ifndef UIOX_KBD_IF_H
 #define UIOX_KBD_IF_H
 
 #include "uiox_kbd_hw.h"
 #include "uiox_kbd_buf.h"
 #include "uiox_klibc.h"

 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Matrix state snapshot
  * ====================================================================== */
 
 typedef struct {
     uint16_t  row_state[UIOX_KBD_MAX_ROWS]; /**< Current col bitmask/row  */
     uint16_t  prev_state[UIOX_KBD_MAX_ROWS];/**< Previous col bitmask/row */
 } uiox_kbd_matrix_state_t;
 
 /* =========================================================================
  * Interface descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_kbd_hw_t           *hw;
     uiox_kbd_if_type_t       type;
     uiox_kbd_matrix_state_t  matrix;
     uint32_t                 direct_state;      /**< Current direct key bits */
     uint32_t                 direct_prev;       /**< Previous direct key bits*/
     uint32_t                 scan_count;        /**< Total scan cycles       */
     uint32_t                 change_count;      /**< Total state changes     */
     bool                     primed;
 } uiox_kbd_if_t;
 
 /* =========================================================================
  * Interface API
  * ====================================================================== */
 
 int  uiox_kbd_if_config (uiox_kbd_if_t    *kif,
                           uiox_kbd_hw_t    *hw,
                           uiox_kbd_if_type_t type);
 
 /**
  * @brief  Perform one full scan cycle.
  *
  * Scans all rows (matrix) or reads all GPIO/I2C pins, updates
  * internal state, and pushes raw position events to the ring buffer.
  *
  * @param  kif  Interface descriptor.
  * @param  rb   Ring buffer to push raw events into.
  * @param  ts_ns Current timestamp in nanoseconds.
  * @return Number of key state changes detected.
  */
 int  uiox_kbd_if_scan   (uiox_kbd_if_t      *kif,
    uiox_kbd_ringbuf_t *rb,
    uint64_t            ts_ns);

/** Query whether a specific matrix key is currently pressed. */
bool uiox_kbd_if_key_pressed(const uiox_kbd_if_t *kif,
        uint8_t row, uint8_t col);

/** Query whether a direct GPIO key is pressed. */
bool uiox_kbd_if_direct_pressed(const uiox_kbd_if_t *kif, uint8_t idx);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_KBD_IF_H */
