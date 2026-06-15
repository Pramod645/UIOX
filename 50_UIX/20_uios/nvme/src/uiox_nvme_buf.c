/**
 * @file  uiox_nvme_buf.c
 * @brief UIOX NVMe command / block / event pool.
 * @date  2026-06-12
 */

 #include "uiox_nvme_buf.h"
 #include <string.h>
 #include <assert.h>
 
 static uiox_nvme_cmd_t s_cmd_pool[UIOX_NVME_CMD_POOL_SIZE];
 static uiox_nvme_blk_t s_blk_pool[UIOX_NVME_BLK_POOL_SIZE];
 static uiox_nvme_evt_t s_evt_pool[UIOX_NVME_EVT_POOL_SIZE];
 
 static uint8_t s_cmd_cnt = 0u;
 static uint8_t s_blk_cnt = 0u;
 static uint8_t s_evt_cnt = 0u;
 
 void uiox_nvme_buf_init(void)
 {
     memset(s_cmd_pool, 0, sizeof(s_cmd_pool));
     memset(s_blk_pool, 0, sizeof(s_blk_pool));
     memset(s_evt_pool, 0, sizeof(s_evt_pool));
     s_cmd_cnt = UIOX_NVME_CMD_POOL_SIZE;
     s_blk_cnt = UIOX_NVME_BLK_POOL_SIZE;
     s_evt_cnt = UIOX_NVME_EVT_POOL_SIZE;
 }
 
 uiox_nvme_cmd_t *uiox_nvme_cmd_alloc(void)
 {
     for (uint8_t i = 0u; i < UIOX_NVME_CMD_POOL_SIZE; i++) {
         if (!s_cmd_pool[i].in_use) {
             memset(&s_cmd_pool[i], 0, sizeof(s_cmd_pool[i]));
             s_cmd_pool[i].in_use = 1u;
             s_cmd_pool[i].state  = UIOX_NVME_CMD_PENDING;
             s_cmd_cnt--;
             return &s_cmd_pool[i];
         }
     }
     return NULL;
 }
 void uiox_nvme_cmd_free(uiox_nvme_cmd_t *c)
 {
     if (!c) return;
     assert(c->in_use > 0u);
     c->in_use = 0u; c->state = UIOX_NVME_CMD_FREE; s_cmd_cnt++;
 }
 uint8_t uiox_nvme_cmd_free_cnt(void) { return s_cmd_cnt; }
 
 uiox_nvme_blk_t *uiox_nvme_blk_alloc(void)
 {
     for (uint8_t i = 0u; i < UIOX_NVME_BLK_POOL_SIZE; i++) {
         if (!s_blk_pool[i].in_use) {
             memset(&s_blk_pool[i], 0, sizeof(s_blk_pool[i]));
             s_blk_pool[i].in_use = 1u;
             s_blk_cnt--;
             return &s_blk_pool[i];
         }
     }
     return NULL;
 }
 void uiox_nvme_blk_free(uiox_nvme_blk_t *b)
 {
     if (!b) return;
     assert(b->in_use > 0u);
     b->in_use = 0u; s_blk_cnt++;
 }
 uint8_t uiox_nvme_blk_free_cnt(void) { return s_blk_cnt; }
 
 uiox_nvme_evt_t *uiox_nvme_evt_alloc(void)
 {
     for (uint8_t i = 0u; i < UIOX_NVME_EVT_POOL_SIZE; i++) {
         if (!s_evt_pool[i].in_use) {
             memset(&s_evt_pool[i], 0, sizeof(s_evt_pool[i]));
             s_evt_pool[i].in_use = 1u;
             s_evt_cnt--;
             return &s_evt_pool[i];
         }
     }
     return NULL;
 }
 void uiox_nvme_evt_free(uiox_nvme_evt_t *e)
 {
     if (!e) return;
     assert(e->in_use > 0u);
     e->in_use = 0u; s_evt_cnt++;
 }
 uint8_t uiox_nvme_evt_free_cnt(void) { return s_evt_cnt; }
 