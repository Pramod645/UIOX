/**
 * @file    uiox_can_buf.c
 * @brief   UIOX CAN buffer pool implementation.
 * @date    2026-05-26
 */

 #include "uiox_can_buf.h"
 #include <string.h>
 #include <assert.h>
 
 static uiox_can_msg_t  s_tx_pool[UIOX_CAN_TX_POOL_SIZE];
 static uiox_can_msg_t  s_rx_pool[UIOX_CAN_RX_POOL_SIZE];
 
 static uiox_can_msg_t *s_tx_free = NULL;
 static uiox_can_msg_t *s_rx_free = NULL;
 static uint16_t        s_tx_cnt  = 0;
 static uint16_t        s_rx_cnt  = 0;
 
 static void build_pool(uiox_can_msg_t *pool, int n,
                         uiox_can_msg_t **list, uint16_t *cnt)
 {
     *list = NULL; *cnt = 0;
     for (int i = 0; i < n; i++) {
         memset(&pool[i], 0, sizeof(pool[i]));
         pool[i].next = *list;
         *list        = &pool[i];
         (*cnt)++;
     }
 }
 
 void uiox_can_buf_init(void)
 {
     build_pool(s_tx_pool, UIOX_CAN_TX_POOL_SIZE, &s_tx_free, &s_tx_cnt);
     build_pool(s_rx_pool, UIOX_CAN_RX_POOL_SIZE, &s_rx_free, &s_rx_cnt);
 }
 
 static uiox_can_msg_t *pool_alloc(uiox_can_msg_t **list, uint16_t *cnt)
 {
     if (!*list) return NULL;
     uiox_can_msg_t *m = *list;
     *list  = m->next;
     (*cnt)--;
     m->next   = NULL;
     m->in_use = 1;
     m->dlc    = 0;
     m->id     = 0;
     m->ts_ns  = 0;
     memset(m->data, 0, sizeof(m->data));
     return m;
 }
 
 uiox_can_msg_t *uiox_can_buf_alloc_tx(void)
 { return pool_alloc(&s_tx_free, &s_tx_cnt); }
 
 uiox_can_msg_t *uiox_can_buf_alloc_rx(void)
 { return pool_alloc(&s_rx_free, &s_rx_cnt); }
 
 void uiox_can_buf_ref(uiox_can_msg_t *msg)
 { if (msg) msg->in_use++; }
 
 void uiox_can_buf_free(uiox_can_msg_t *msg)
 {
     if (!msg) return;
     assert(msg->in_use > 0);
     if (--msg->in_use == 0) {
         /* Determine pool by pointer range */
         bool is_tx = (msg >= s_tx_pool &&
                       msg < s_tx_pool + UIOX_CAN_TX_POOL_SIZE);
         if (is_tx) { msg->next = s_tx_free; s_tx_free = msg; s_tx_cnt++; }
         else       { msg->next = s_rx_free; s_rx_free = msg; s_rx_cnt++; }
     }
 }
 
 uint16_t uiox_can_buf_tx_free(void) { return s_tx_cnt; }
 uint16_t uiox_can_buf_rx_free(void) { return s_rx_cnt; }
 