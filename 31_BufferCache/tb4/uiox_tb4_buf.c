/**
 * @file    uiox_tb4_buf.c
 * @brief   UIOX Thunderbolt 4 buffer pool implementation.
 * @date    2026-06-08
 */

 #include "uiox_tb4_buf.h"
 #include <string.h>
 #include <assert.h>
 
 static uiox_tb4_frame_t s_tx_desc[UIOX_TB4_TX_POOL_SIZE];
 static uiox_tb4_frame_t s_rx_desc[UIOX_TB4_RX_POOL_SIZE];
 static uint8_t s_tx_mem[UIOX_TB4_TX_POOL_SIZE]
                         [UIOX_TB4_FRAME_MAX + UIOX_TB4_BUF_ALIGN];
 static uint8_t s_rx_mem[UIOX_TB4_RX_POOL_SIZE]
                         [UIOX_TB4_FRAME_MAX + UIOX_TB4_BUF_ALIGN];
 
 static uiox_tb4_frame_t *s_tx_free = NULL;
 static uiox_tb4_frame_t *s_rx_free = NULL;
 static uint8_t s_tx_cnt = 0, s_rx_cnt = 0;
 
 static uintptr_t align_up(uintptr_t p, uintptr_t a)
 { return (p + a - 1u) & ~(a - 1u); }
 #if 0
 static void build_pool(uiox_tb4_frame_t *descs, int n,
                         uint8_t (*mem)[], uint32_t cap,
                         uiox_tb4_frame_t **list, uint8_t *cnt)
 {
     *list = NULL; *cnt = 0;
     for (int i = 0; i < n; i++) {
         uiox_tb4_frame_t *f = &descs[i];
         memset(f, 0, sizeof(*f));
         uintptr_t base = (uintptr_t)mem[i];
         uintptr_t al   = align_up(base, UIOX_TB4_BUF_ALIGN);
         f->data     = (uint8_t *)al;
         f->paddr    = al;
         f->capacity = cap;
         f->state    = UIOX_TB4_BUF_FREE;
         f->next     = *list;
         *list       = f;
         (*cnt)++;
     }
 }
 #endif
 void uiox_tb4_buf_init(void)
 {
     //build_pool(s_tx_desc, UIOX_TB4_TX_POOL_SIZE, s_tx_mem,
               // UIOX_TB4_FRAME_MAX, &s_tx_free, &s_tx_cnt);
     //build_pool(s_rx_desc, UIOX_TB4_RX_POOL_SIZE, s_rx_mem,
              //  UIOX_TB4_FRAME_MAX, &s_rx_free, &s_rx_cnt);
 }
 
 static uiox_tb4_frame_t *pool_alloc(uiox_tb4_frame_t **list, uint8_t *cnt)
 {
     if (!*list) return NULL;
     uiox_tb4_frame_t *f = *list;
     *list   = f->next;
     (*cnt)--;
     f->next   = NULL;
     f->in_use = 1;
     f->len    = 0;
     return f;
 }
 
 uiox_tb4_frame_t *uiox_tb4_buf_alloc_tx(void)
 {
     uiox_tb4_frame_t *f = pool_alloc(&s_tx_free, &s_tx_cnt);
     if (f) f->state = UIOX_TB4_BUF_TX_PENDING;
     return f;
 }
 
 uiox_tb4_frame_t *uiox_tb4_buf_alloc_rx(void)
 {
     uiox_tb4_frame_t *f = pool_alloc(&s_rx_free, &s_rx_cnt);
     if (f) f->state = UIOX_TB4_BUF_RX_READY;
     return f;
 }
 
 void uiox_tb4_buf_free(uiox_tb4_frame_t *f)
 {
     if (!f) return;
     assert(f->in_use > 0);
     if (--f->in_use == 0) {
         f->state = UIOX_TB4_BUF_FREE;
         bool is_tx = (f >= s_tx_desc &&
                       f < s_tx_desc + UIOX_TB4_TX_POOL_SIZE);
         if (is_tx) { f->next = s_tx_free; s_tx_free = f; s_tx_cnt++; }
         else        { f->next = s_rx_free; s_rx_free = f; s_rx_cnt++; }
     }
 }
 
 uint8_t uiox_tb4_buf_tx_free(void) { return s_tx_cnt; }
 uint8_t uiox_tb4_buf_rx_free(void) { return s_rx_cnt; }
 