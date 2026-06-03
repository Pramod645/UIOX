/**
 * @file    uiox_mouse_buf.c
 * @brief   UIOX Mouse ring buffer implementation.
 * @date    2026-06-01
 */

 #include "uiox_mouse_buf.h"
 #include <string.h>
 
 void uiox_mouse_buf_init(uiox_mouse_ringbuf_t *rb)
 {
     if (!rb) return;
     rb->head = rb->tail = rb->overflow = 0;
     memset(rb->buf, 0, sizeof(rb->buf));
 }
 
 bool uiox_mouse_buf_push(uiox_mouse_ringbuf_t *rb,
                           const uiox_mouse_event_t *ev)
 {
     if (!rb || !ev) return false;
     uint32_t next = (rb->head + 1u) & UIOX_MOUSE_BUF_MASK;
     if (next == rb->tail) { rb->overflow++; return false; }
     rb->buf[rb->head] = *ev;
     rb->head = next;
     return true;
 }
 
 bool uiox_mouse_buf_pop(uiox_mouse_ringbuf_t *rb, uiox_mouse_event_t *ev)
 {
     if (!rb || !ev || rb->head == rb->tail) return false;
     *ev      = rb->buf[rb->tail];
     rb->tail = (rb->tail + 1u) & UIOX_MOUSE_BUF_MASK;
     return true;
 }
 
 bool uiox_mouse_buf_peek(const uiox_mouse_ringbuf_t *rb,
                           uiox_mouse_event_t *ev)
 {
     if (!rb || !ev || rb->head == rb->tail) return false;
     *ev = rb->buf[rb->tail];
     return true;
 }
 
 bool uiox_mouse_buf_empty(const uiox_mouse_ringbuf_t *rb)
 { return rb ? (rb->head == rb->tail) : true; }
 
 uint32_t uiox_mouse_buf_count(const uiox_mouse_ringbuf_t *rb)
 {
     if (!rb) return 0;
     return (rb->head - rb->tail) & UIOX_MOUSE_BUF_MASK;
 }
 
 void uiox_mouse_buf_flush(uiox_mouse_ringbuf_t *rb)
 { if (rb) rb->tail = rb->head; }
 