/**
 * @file    uiox_kbd_buf.h
 * @brief   UIOX Keyboard key-event ring buffer.
 *
 * Lock-free single-producer / single-consumer ring buffer for
 * key events. The scanner (ISR/task) produces; the application
 * consumer reads. Statically allocated — no heap fragmentation.
 *
 * @date    2026-05-27
 */
//Layer 1.5 — Buffer Manager
 #ifndef UIOX_KBD_BUF_H
 #define UIOX_KBD_BUF_H
 
 #include <stdint.h>
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Ring buffer configuration
  * ====================================================================== */
 
 #define UIOX_KBD_BUF_SIZE       256   /**< Must be power of 2             */
 #define UIOX_KBD_BUF_MASK       (UIOX_KBD_BUF_SIZE - 1)
 
 /* =========================================================================
  * Key event
  * ====================================================================== */
 
 typedef enum {
     UIOX_KBD_EV_PRESS   = 0,   /**< Key pressed                          */
     UIOX_KBD_EV_RELEASE,       /**< Key released                         */
     UIOX_KBD_EV_REPEAT,        /**< Key auto-repeat                      */
     UIOX_KBD_EV_SPECIAL,       /**< Special event (caps lock, etc.)      */
 } uiox_kbd_ev_type_t;
 
 typedef struct {
     uint8_t            ev_type;    /**< uiox_kbd_ev_type_t                */
     uint8_t            row;        /**< Matrix row (0xFF = direct/other)  */
     uint8_t            col;        /**< Matrix column                     */
     uint8_t            scancode;   /**< Raw hardware scancode             */
     uint16_t           keycode;    /**< Logical keycode (HID usage ID)    */
     uint32_t           unicode;    /**< Unicode codepoint (0 = none)      */
     uint8_t            modifiers;  /**< Active modifier bitmask           */
     uint64_t           ts_ns;      /**< Event timestamp (ns)              */
 } uiox_kbd_event_t;
 
 /* =========================================================================
  * Modifier bitmask
  * ====================================================================== */
 
 #define UIOX_KBD_MOD_LSHIFT     (1u << 0)
 #define UIOX_KBD_MOD_RSHIFT     (1u << 1)
 #define UIOX_KBD_MOD_LCTRL      (1u << 2)
 #define UIOX_KBD_MOD_RCTRL      (1u << 3)
 #define UIOX_KBD_MOD_LALT       (1u << 4)
 #define UIOX_KBD_MOD_RALT       (1u << 5)
 #define UIOX_KBD_MOD_LGUI       (1u << 6)
 #define UIOX_KBD_MOD_RGUI       (1u << 7)
 #define UIOX_KBD_MOD_SHIFT      (UIOX_KBD_MOD_LSHIFT | UIOX_KBD_MOD_RSHIFT)
 #define UIOX_KBD_MOD_CTRL       (UIOX_KBD_MOD_LCTRL  | UIOX_KBD_MOD_RCTRL)
 #define UIOX_KBD_MOD_ALT        (UIOX_KBD_MOD_LALT   | UIOX_KBD_MOD_RALT)
 #define UIOX_KBD_MOD_GUI        (UIOX_KBD_MOD_LGUI   | UIOX_KBD_MOD_RGUI)
 
 /* =========================================================================
  * Ring buffer descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_kbd_event_t  buf[UIOX_KBD_BUF_SIZE];
     volatile uint32_t head;     /**< Write index (producer)               */
     volatile uint32_t tail;     /**< Read index  (consumer)               */
     uint32_t          overflow; /**< Overflow event counter               */
 } uiox_kbd_ringbuf_t;
 
 /* =========================================================================
  * Ring buffer API
  * ====================================================================== */
 
 void uiox_kbd_buf_init  (uiox_kbd_ringbuf_t *rb);
 bool uiox_kbd_buf_push  (uiox_kbd_ringbuf_t *rb,
                           const uiox_kbd_event_t *ev);
 bool uiox_kbd_buf_pop   (uiox_kbd_ringbuf_t *rb, uiox_kbd_event_t *ev);
 bool uiox_kbd_buf_peek  (const uiox_kbd_ringbuf_t *rb,
                           uiox_kbd_event_t *ev);
 bool uiox_kbd_buf_empty (const uiox_kbd_ringbuf_t *rb);
 uint32_t uiox_kbd_buf_count(const uiox_kbd_ringbuf_t *rb);
 void uiox_kbd_buf_flush (uiox_kbd_ringbuf_t *rb);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KBD_BUF_H */
 