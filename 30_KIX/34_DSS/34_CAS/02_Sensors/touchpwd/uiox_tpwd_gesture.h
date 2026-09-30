/**
 * @file    uiox_tpwd_gesture.h
 * @brief   UIOX Touch-Password gesture recognition engine.
 *
 * Recognises:
 *   - PIN entry  (digit sequence 0-9, *, #)
 *   - Pattern    (3×3 Android-style connect-the-dots)
 *   - Swipe      (directional gesture for navigation)
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_TPWD_GESTURE_H
 #define UIOX_TPWD_GESTURE_H
 
 #include "uiox_tpwd_if.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * PIN digit map (cell → character)
  * Grid layout (3 cols × 4 rows):
  *   [1][2][3]
  *   [4][5][6]
  *   [7][8][9]
  *   [*][0][#]
  * ====================================================================== */
 
 #define UIOX_TPWD_CELL_TO_DIGIT(cell) \
     ((const char[]){"123456789*0#"})[(cell) < 12u ? (cell) : 11u]
 
 /* =========================================================================
  * Input mode
  * ====================================================================== */
 
 typedef enum {
     UIOX_TPWD_MODE_PIN     = 0,  /**< Digit PIN (4..16 digits)           */
     UIOX_TPWD_MODE_PATTERN,       /**< Connect-the-dots (3..9 nodes)     */
     UIOX_TPWD_MODE_SWIPE,         /**< Directional swipe gesture          */
 } uiox_tpwd_input_mode_t;
 
 /* =========================================================================
  * Swipe direction
  * ====================================================================== */
 
 typedef enum {
     UIOX_TPWD_SWIPE_NONE  = 0,
     UIOX_TPWD_SWIPE_UP,
     UIOX_TPWD_SWIPE_DOWN,
     UIOX_TPWD_SWIPE_LEFT,
     UIOX_TPWD_SWIPE_RIGHT,
 } uiox_tpwd_swipe_dir_t;
 
 /* =========================================================================
  * Collected credential (before hashing)
  * ====================================================================== */
 
 typedef struct {
     uiox_tpwd_input_mode_t mode;
 
     /* PIN */
     char    pin[UIOX_TPWD_MAX_PIN_LEN + 1];
     uint8_t pin_len;
 
     /* Pattern */
     uint8_t pattern[UIOX_TPWD_MAX_PATTERN_PTS];
     uint8_t pattern_len;
 
     /* Swipe */
     uiox_tpwd_swipe_dir_t swipe;
 
     /* Status */
     bool    complete;       /**< Entry is complete and ready to verify    */
     bool    cancelled;      /**< User cancelled entry                     */
     uint64_t start_ts_ns;
     uint64_t end_ts_ns;
 } uiox_tpwd_credential_t;
 
 /* =========================================================================
  * Gesture configuration
  * ====================================================================== */
 
 typedef struct {
     uiox_tpwd_input_mode_t mode;
     uint8_t  min_pin_len;     /**< Minimum PIN digits (default 4)        */
     uint8_t  max_pin_len;     /**< Maximum PIN digits (default 16)       */
     uint8_t  min_pattern_pts; /**< Min pattern nodes (default 4)         */
     uint32_t entry_timeout_ms;/**< Max time for full entry               */
     bool     mask_input;      /**< Replace digits with '*' in display    */
 } uiox_tpwd_gesture_cfg_t;
 
 /* =========================================================================
  * Gesture engine context
  * ====================================================================== */
 
 typedef struct {
     uiox_tpwd_gesture_cfg_t  cfg;
     uiox_tpwd_credential_t   current;
     uiox_tpwd_evtbuf_t      *src_rb;
     uint32_t                  start_ms;
     uint8_t                   last_cell;  /**< Prev cell for pattern dedup*/
     bool                      running;
 } uiox_tpwd_gesture_t;
 
 /* =========================================================================
  * Gesture API
  * ====================================================================== */
 
 int  uiox_tpwd_gesture_init   (uiox_tpwd_gesture_t          *g,
                                 uiox_tpwd_evtbuf_t           *src_rb,
                                 const uiox_tpwd_gesture_cfg_t *cfg);
 
 /** Begin collecting a new credential entry. */
 int  uiox_tpwd_gesture_start  (uiox_tpwd_gesture_t *g, uint32_t now_ms);
 
 /** Cancel ongoing entry and clear collected data. */
 void uiox_tpwd_gesture_cancel (uiox_tpwd_gesture_t *g);
 
 /**
  * @brief  Process pending events from source ring buffer.
  *
  * @param  g       Gesture engine.
  * @param  now_ms  Current time (ms) for timeout detection.
  * @return 1 = entry complete (check g->current.complete/cancelled)
  *         0 = still collecting
  *        <0 = error
  */
 int  uiox_tpwd_gesture_process(uiox_tpwd_gesture_t *g, uint32_t now_ms);
 
/**
 * @brief  Serialise credential to raw bytes for hashing.
 *         PIN  → ASCII digit string bytes
 *         Pattern → node index byte sequence
 *         Swipe   → single direction byte
 *
 * @param  cred     Completed credential.
 * @param  buf      Output buffer (caller must supply ≥ UIOX_TPWD_MAX_PIN_LEN).
 * @param  buf_len  Buffer size.
 * @return Bytes written, or <0 on error.
 */
int  uiox_tpwd_gesture_serialise(const uiox_tpwd_credential_t *cred,
    uint8_t *buf, uint16_t buf_len);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_TPWD_GESTURE_H */

 