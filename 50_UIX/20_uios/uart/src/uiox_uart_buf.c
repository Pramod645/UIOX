/**
 * @file  uiox_uart_buf.c
 * @brief UIOX UART — TX/RX ring buffer pool and event pool.
 * @date  2026-07-05
 */

 #include "uiox_uart_device.h"
 #include <string.h>
 #include <assert.h>
 
 /* =========================================================================
  * Ring buffer operations
  * ====================================================================== */
 
 void uiox_uart_ring_init(uiox_uart_ring_t *r, uint32_t size)
 {
     if (!r) return;
     memset(r->data, 0, sizeof(r->data));
     r->head     = 0u;
     r->tail     = 0u;
     r->size     = (size <= UIOX_UART_RX_BUF_SIZE) ? size
                                                     : UIOX_UART_RX_BUF_SIZE;
     r->count    = 0u;
     r->overruns = 0u;
 }
 
 int uiox_uart_ring_put(uiox_uart_ring_t *r, uint8_t c)
 {
     if (!r) return -1;
     if (r->count >= r->size) { r->overruns++; return -1; }
     r->data[r->tail] = c;
     r->tail = (r->tail + 1u) % r->size;
     r->count++;
     return 0;
 }
 
 int uiox_uart_ring_get(uiox_uart_ring_t *r, uint8_t *c)
 {
     if (!r || !c || r->count == 0u) return -1;
     *c      = r->data[r->head];
     r->head = (r->head + 1u) % r->size;
     r->count--;
     return 0;
 }
 
 uint32_t uiox_uart_ring_avail(const uiox_uart_ring_t *r)
 { return r ? r->count : 0u; }
 
 uint32_t uiox_uart_ring_free(const uiox_uart_ring_t *r)
 { return r ? (r->size - r->count) : 0u; }
 
 bool uiox_uart_ring_empty(const uiox_uart_ring_t *r)
 { return !r || r->count == 0u; }
 
 bool uiox_uart_ring_full(const uiox_uart_ring_t *r)
 { return r && r->count >= r->size; }
 
 void uiox_uart_ring_flush(uiox_uart_ring_t *r)
 {
     if (!r) return;
     r->head  = 0u;
     r->tail  = 0u;
     r->count = 0u;
 }
 
 /* =========================================================================
  * Event pool
  * ====================================================================== */
 
 static uiox_uart_evt_t s_evt_pool[UIOX_UART_EVT_POOL_SIZE];
 static uint8_t         s_evt_cnt = 0u;
 
 void uiox_uart_buf_init(void)
 {
     memset(s_evt_pool, 0, sizeof(s_evt_pool));
     s_evt_cnt = UIOX_UART_EVT_POOL_SIZE;
 }
 
 uiox_uart_evt_t *uiox_uart_evt_alloc(void)
 {
     for (uint8_t i = 0u; i < UIOX_UART_EVT_POOL_SIZE; i++) {
         if (!s_evt_pool[i].in_use) {
             memset(&s_evt_pool[i], 0, sizeof(s_evt_pool[i]));
             s_evt_pool[i].in_use = 1u;
             s_evt_cnt--;
             return &s_evt_pool[i];
         }
     }
     return NULL;
 }
 
 void uiox_uart_evt_free(uiox_uart_evt_t *e)
 {
     if (!e) return;
     assert(e->in_use > 0u);
     e->in_use = 0u;
     s_evt_cnt++;
 }
 
 uint8_t uiox_uart_evt_free_cnt(void) { return s_evt_cnt; }
 