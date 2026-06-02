/**
 * @file    uiox_tpwd_buf.c
 * @brief   UIOX Touch-Password buffer pool implementation.
 * @date    2026-06-01
 */

 #include "uiox_tpwd_buf.h"
 #include <string.h>
 #include <assert.h>
 
 /* -------------------------------------------------------------------------
  * Event ring buffer
  * ---------------------------------------------------------------------- */
 
 void uiox_tpwd_evtbuf_init(uiox_tpwd_evtbuf_t *rb)
 {
     if (!rb) return;
     rb->head = rb->tail = rb->overflow = 0;
     memset(rb->buf, 0, sizeof(rb->buf));
 }
 
 bool uiox_tpwd_evtbuf_push(uiox_tpwd_evtbuf_t *rb,
                              const uiox_tpwd_raw_evt_t *ev)
 {
     if (!rb || !ev) return false;
     uint32_t next = (rb->head + 1u) & UIOX_TPWD_EVT_BUF_MASK;
     if (next == rb->tail) { rb->overflow++; return false; }
     rb->buf[rb->head] = *ev;
     rb->head = next;
     return true;
 }
 
 bool uiox_tpwd_evtbuf_pop(uiox_tpwd_evtbuf_t *rb,
                             uiox_tpwd_raw_evt_t *ev)
 {
     if (!rb || !ev || rb->head == rb->tail) return false;
     *ev      = rb->buf[rb->tail];
     rb->tail = (rb->tail + 1u) & UIOX_TPWD_EVT_BUF_MASK;
     return true;
 }
 
 bool uiox_tpwd_evtbuf_empty(const uiox_tpwd_evtbuf_t *rb)
 {
     return rb ? (rb->head == rb->tail) : true;
 }
 
 uint32_t uiox_tpwd_evtbuf_count(const uiox_tpwd_evtbuf_t *rb)
 {
     if (!rb) return 0;
     return (rb->head - rb->tail) & UIOX_TPWD_EVT_BUF_MASK;
 }
 
 /* -------------------------------------------------------------------------
  * Credential pool  (memory is zeroed on free — security requirement)
  * ---------------------------------------------------------------------- */
 
 static uiox_tpwd_cred_t  s_cred_pool[UIOX_TPWD_CRED_POOL_SIZE];
 static uiox_tpwd_cred_t *s_cred_free = NULL;
 static uint8_t            s_cred_cnt  = 0;
 
 void uiox_tpwd_cred_pool_init(void)
 {
     s_cred_free = NULL; s_cred_cnt = 0;
     for (int i = 0; i < UIOX_TPWD_CRED_POOL_SIZE; i++) {
         memset(&s_cred_pool[i], 0, sizeof(s_cred_pool[i]));
         s_cred_pool[i].next = s_cred_free;
         s_cred_free = &s_cred_pool[i];
         s_cred_cnt++;
     }
 }
 
 uiox_tpwd_cred_t *uiox_tpwd_cred_alloc(void)
 {
     if (!s_cred_free) return NULL;
     uiox_tpwd_cred_t *c = s_cred_free;
     s_cred_free = c->next;
     s_cred_cnt--;
     c->next     = NULL;
     c->in_use   = 1;
     c->attempts = 0;
     return c;
 }
 
 void uiox_tpwd_cred_free(uiox_tpwd_cred_t *c)
 {
     if (!c) return;
     assert(c->in_use > 0);
     /* Zero all sensitive material before returning to pool */
     volatile uint8_t *p = (volatile uint8_t *)c->hash;
     for (size_t i = 0; i < UIOX_TPWD_HASH_LEN; i++) p[i] = 0;
     p = (volatile uint8_t *)c->salt;
     for (size_t i = 0; i < UIOX_TPWD_SALT_LEN; i++) p[i] = 0;
     c->attempts = 0;
     c->in_use   = 0;
     c->next     = s_cred_free;
     s_cred_free = c;
     s_cred_cnt++;
 }
 
 uint8_t uiox_tpwd_cred_free_count(void) { return s_cred_cnt; }
 