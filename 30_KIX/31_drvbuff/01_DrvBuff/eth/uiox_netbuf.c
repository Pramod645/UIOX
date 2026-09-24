/**
 * @file    uiox_netbuf.c
 * @brief   UIOX network buffer pool implementation.
 * @date    2026-05-25
 */

 #include "uiox_netbuf.h"

 /* Freestanding assert — no libc assert.h available under -nostdinc */
 #ifndef UIOX_ASSERT
 #  define UIOX_ASSERT(cond)  do { if (!(cond)) __builtin_trap(); } while (0)
 #endif
 
 static uiox_netbuf_t  s_desc_pool [UIOX_NETBUF_POOL_SIZE];
 static uint8_t        s_data_pool [UIOX_NETBUF_POOL_SIZE]
                                   [UIOX_NETBUF_HEADROOM + UIOX_NETBUF_DATA_SIZE];
 static uiox_netbuf_t *s_free_list  = NULL;
 static uint16_t       s_free_count = 0;
 
 void uiox_netbuf_pool_init(void)
 {
     s_free_list = NULL; s_free_count = 0;
     for (int i = 0; i < UIOX_NETBUF_POOL_SIZE; i++) {
         uiox_netbuf_t *b = &s_desc_pool[i];
         uint8_t *storage  = s_data_pool[i];
         b->buf_start = storage;
         b->buf_end   = storage + UIOX_NETBUF_HEADROOM + UIOX_NETBUF_DATA_SIZE;
         b->data      = storage + UIOX_NETBUF_HEADROOM;
         b->len = 0; b->total_len = 0; b->next = NULL;
         b->ref = 0; b->flags = 0; b->proto = 0; b->priv = NULL;
         b->next_free = s_free_list; s_free_list = b; s_free_count++;
     }
 }
 
 uint16_t uiox_netbuf_pool_free(void) { return s_free_count; }
 
 uiox_netbuf_t *uiox_netbuf_alloc(void)
 {
     if (!s_free_list) return NULL;
     uiox_netbuf_t *b = s_free_list;
     s_free_list = b->next_free; s_free_count--;
     b->data      = b->buf_start + UIOX_NETBUF_HEADROOM;
     b->len = 0; b->total_len = 0; b->next = NULL;
     b->ref = 1; b->flags = 0; b->proto = 0; b->priv = NULL;
     return b;
 }
 
 void uiox_netbuf_free(uiox_netbuf_t *buf)
 {
     while (buf) {
         uiox_netbuf_t *next = buf->next;
         UIOX_ASSERT(buf->ref > 0);
         if (--buf->ref == 0) {
             buf->next_free = s_free_list;
             s_free_list = buf; s_free_count++;
         }
         buf = next;
     }
 }
 
 void uiox_netbuf_ref(uiox_netbuf_t *buf) { if (buf) buf->ref++; }
 
 void *uiox_netbuf_push(uiox_netbuf_t *buf, uint16_t len)
 {
     if (!buf || buf->data - len < buf->buf_start) return NULL;
     buf->data -= len; buf->len += len; return buf->data;
 }
 
 void *uiox_netbuf_pull(uiox_netbuf_t *buf, uint16_t len)
 {
     if (!buf || len > buf->len) return NULL;
     buf->data += len; buf->len -= len; return buf->data;
 }
 
 void *uiox_netbuf_put(uiox_netbuf_t *buf, uint16_t len)
 {
     if (!buf) return NULL;
     uint8_t *tail = buf->data + buf->len;
     if (tail + len > buf->buf_end) return NULL;
     buf->len += len; return tail;
 }
 
 void uiox_netbuf_trim(uiox_netbuf_t *buf, uint16_t len)
 { if (!buf || len > buf->len) return; buf->len -= len; }
 
 void uiox_netbuf_chain(uiox_netbuf_t *head, uiox_netbuf_t *tail)
 {
     uiox_netbuf_t *cur = head;
     while (cur->next) cur = cur->next;
     cur->next = tail;
     head->total_len = uiox_netbuf_total_len(head);
 }
 
 uint16_t uiox_netbuf_total_len(const uiox_netbuf_t *head)
 {
     uint16_t total = 0;
     for (const uiox_netbuf_t *b = head; b; b = b->next) total += b->len;
     return total;
 }
 