/**
 * @file    uiox_kbd_buf.c
 * @brief   UIOX Keyboard ring buffer implementation.
 * @date    2026-05-27
 */

 #include "uiox_kbd_buf.h"
 
 void uiox_kbd_buf_init(uiox_kbd_ringbuf_t *rb)
 {
     if (!rb) return;
     rb->head     = 0;
     rb->tail     = 0;
     rb->overflow = 0;
     memset(rb->buf, 0, sizeof(rb->buf));
 }
 
 bool uiox_kbd_buf_push(uiox_kbd_ringbuf_t *rb,
                         const uiox_kbd_event_t *ev)
 {
     if (!rb || !ev) return false;
     uint32_t next = (rb->head + 1u) & UIOX_KBD_BUF_MASK;
     if (next == rb->tail) {
         rb->overflow++;
         return false;  /* Buffer full */
     }
     rb->buf[rb->head] = *ev;
     rb->head = next;
     return true;
 }
 
 bool uiox_kbd_buf_pop(uiox_kbd_ringbuf_t *rb, uiox_kbd_event_t *ev)
 {
     if (!rb || !ev) return false;
     if (rb->tail == rb->head) return false;  /* Empty */
     *ev      = rb->buf[rb->tail];
     rb->tail = (rb->tail + 1u) & UIOX_KBD_BUF_MASK;
     return true;
 }
 
 bool uiox_kbd_buf_peek(const uiox_kbd_ringbuf_t *rb,
                         uiox_kbd_event_t *ev)
 {
     if (!rb || !ev) return false;
     if (rb->tail == rb->head) return false;
     *ev = rb->buf[rb->tail];
     return true;
 }
 
 bool uiox_kbd_buf_empty(const uiox_kbd_ringbuf_t *rb)
 {
     return rb ? (rb->tail == rb->head) : true;
 }
 
 uint32_t uiox_kbd_buf_count(const uiox_kbd_ringbuf_t *rb)
 {
     if (!rb) return 0u;
     return (rb->head - rb->tail) & UIOX_KBD_BUF_MASK;
 }
 
 void uiox_kbd_buf_flush(uiox_kbd_ringbuf_t *rb)
 {
     if (rb) rb->tail = rb->head;
 }
 