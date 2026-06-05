/**
 * @file    uiox_ram_buf.c
 * @brief   UIOX RAM region descriptor pool implementation.
 * @date    2026-06-03
 */

 #include "uiox_ram_buf.h"
 #include <string.h>
 #include <assert.h>
 
 static uiox_ram_region_t  s_pool[UIOX_RAM_REGION_POOL_SIZE];
 static uiox_ram_region_t *s_free = NULL;
 static uint16_t            s_cnt  = 0;
 
 void uiox_ram_buf_init(void)
 {
     s_free = NULL; s_cnt = 0;
     for (int i = 0; i < UIOX_RAM_REGION_POOL_SIZE; i++) {
         memset(&s_pool[i], 0, sizeof(s_pool[i]));
         s_pool[i].type   = UIOX_RAM_REGION_FREE;
         s_pool[i].next   = s_free;
         s_free           = &s_pool[i];
         s_cnt++;
     }
 }
 
 uiox_ram_region_t *uiox_ram_buf_alloc(void)
 {
     if (!s_free) return NULL;
     uiox_ram_region_t *r = s_free;
     s_free   = r->next;
     s_cnt--;
     r->next   = NULL;
     r->in_use = 1;
     r->type   = UIOX_RAM_REGION_HEAP;
     return r;
 }
 
 void uiox_ram_buf_free(uiox_ram_region_t *r)
 {
     if (!r) return;
     assert(r->in_use > 0);
     if (--r->in_use == 0) {
         r->type  = UIOX_RAM_REGION_FREE;
         r->next  = s_free;
         s_free   = r;
         s_cnt++;
     }
 }
 
 uint16_t uiox_ram_buf_free_cnt(void) { return s_cnt; }
 