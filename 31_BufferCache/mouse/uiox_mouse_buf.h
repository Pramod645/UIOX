/**
 * @file    uiox_mouse_buf.h
 * @brief   UIOX Mouse event ring buffer (SPSC lock-free).
 * @date    2026-06-01
 */

 #ifndef UIOX_MOUSE_BUF_H
 #define UIOX_MOUSE_BUF_H
 
 #include "uiox_mouse_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_MOUSE_BUF_SIZE    128   /**< Must be power of 2              */
 #define UIOX_MOUSE_BUF_MASK    (UIOX_MOUSE_BUF_SIZE - 1)
 
 /* =========================================================================
  * Logical mouse event (post-processing)
  * ====================================================================== */
 
 typedef enum {
     UIOX_MOUSE_EV_MOVE       = 0,
     UIOX_MOUSE_EV_BTN_PRESS,
     UIOX_MOUSE_EV_BTN_RELEASE,
     UIOX_MOUSE_EV_CLICK,
     UIOX_MOUSE_EV_DBLCLICK,
     UIOX_MOUSE_EV_SCROLL_V,
     UIOX_MOUSE_EV_SCROLL_H,
     UIOX_MOUSE_EV_CONNECT,
     UIOX_MOUSE_EV_DISCONNECT,
     UIOX_MOUSE_EV_GESTURE,
 } uiox_mouse_ev_type_t;
 
 /* Button indices */
 #define UIOX_MOUSE_BTN_LEFT    0
 #define UIOX_MOUSE_BTN_RIGHT   1
 #define UIOX_MOUSE_BTN_MIDDLE  2
 #define UIOX_MOUSE_BTN_BACK    3
 #define UIOX_MOUSE_BTN_FORWARD 4
 
 /* Gesture codes */
 #define UIOX_MOUSE_GESTURE_SWIPE_LEFT  1
 #define UIOX_MOUSE_GESTURE_SWIPE_RIGHT 2
 #define UIOX_MOUSE_GESTURE_SWIPE_UP    3
 #define UIOX_MOUSE_GESTURE_SWIPE_DOWN  4
 #define UIOX_MOUSE_GESTURE_PINCH_IN    5
 #define UIOX_MOUSE_GESTURE_PINCH_OUT   6
 
 typedef struct {
     uiox_mouse_ev_type_t type;
     int32_t   x;          /**< Absolute cursor X (after accel)            */
     int32_t   y;          /**< Absolute cursor Y (after accel)            */
     int16_t   dx;         /**< Relative X delta                           */
     int16_t   dy;         /**< Relative Y delta                           */
     int8_t    dz;         /**< Scroll vertical                            */
     int8_t    dw;         /**< Scroll horizontal                          */
     uint8_t   button;     /**< Button index for BTN_* events              */
     uint8_t   buttons;    /**< Full button bitmask at event time          */
     uint8_t   gesture;    /**< Gesture code for GESTURE events            */
     uint32_t  click_count;/**< 1=click, 2=double-click                   */
     uint64_t  ts_ns;
 } uiox_mouse_event_t;
 
 /* =========================================================================
  * Ring buffer
  * ====================================================================== */
 
 typedef struct {
     uiox_mouse_event_t   buf[UIOX_MOUSE_BUF_SIZE];
     volatile uint32_t    head;
     volatile uint32_t    tail;
     uint32_t             overflow;
 } uiox_mouse_ringbuf_t;
 
 void uiox_mouse_buf_init (uiox_mouse_ringbuf_t *rb);
 bool uiox_mouse_buf_push (uiox_mouse_ringbuf_t *rb,
                            const uiox_mouse_event_t *ev);
 bool uiox_mouse_buf_pop  (uiox_mouse_ringbuf_t *rb, uiox_mouse_event_t *ev);
 bool uiox_mouse_buf_peek (const uiox_mouse_ringbuf_t *rb,
                            uiox_mouse_event_t *ev);
 bool uiox_mouse_buf_empty(const uiox_mouse_ringbuf_t *rb);
 uint32_t uiox_mouse_buf_count(const uiox_mouse_ringbuf_t *rb);
 void uiox_mouse_buf_flush(uiox_mouse_ringbuf_t *rb);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MOUSE_BUF_H */
 