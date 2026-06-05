/**
 * @file    uiox_bios_buf.c
 * @brief   UIOX BIOS buffer pool implementation.
 * @date    2026-06-04
 */

 #include "uiox_bios_buf.h"
 #include <string.h>
 #include <assert.h>
 #include <stdint.h>
 
 static uiox_bios_buf_t  s_pool[UIOX_BIOS_BUF_POOL_SIZE];
 static uiox_bios_buf_t *s_free = NULL;
 static uint8_t           s_cnt  = 0;
 
 static uintptr_t align_up(uintptr_t p, uintptr_t a)
 { return (p + a - 1u) & ~(a - 1u); }
 
 void uiox_bios_buf_init(void)
 {
     s_free = NULL; s_cnt = 0;
     for (int i = 0; i < UIOX_BIOS_BUF_POOL_SIZE; i++) {
         uiox_bios_buf_t *b = &s_pool[i];
         memset(b, 0, sizeof(*b));
         b->aligned = (uint8_t *)align_up(
             (uintptr_t)b->data, UIOX_BIOS_BUF_ALIGN);
         b->use     = UIOX_BIOS_BUF_FREE;
         b->next    = s_free;
         s_free     = b;
         s_cnt++;
     }
 }
 
 uiox_bios_buf_t *uiox_bios_buf_alloc(uiox_bios_buf_use_t use)
 {
     if (!s_free) return NULL;
     uiox_bios_buf_t *b = s_free;
     s_free   = b->next;
     s_cnt--;
     b->next        = NULL;
     b->in_use      = 1;
     b->use         = use;
     b->valid_bytes = 0;
     b->flash_offset= 0;
     return b;
 }
 
 void uiox_bios_buf_free(uiox_bios_buf_t *b)
 {
     if (!b) return;
     assert(b->in_use > 0);
     if (--b->in_use == 0) {
         /* Zero sensitive flash contents before releasing */
         memset(b->aligned, 0, UIOX_BIOS_SECTOR_SIZE);
         b->use  = UIOX_BIOS_BUF_FREE;
         b->next = s_free;
         s_free  = b;
         s_cnt++;
     }
 }
 
 uint8_t uiox_bios_buf_free_count(void) { return s_cnt; }
 