/**
 * @file    uiox_us_buf.c
 * @brief   UIOX Ultrasonic buffer pool implementation.
 * @date    2026-05-26
 */

 #include "uiox_us_buf.h"
 #include <string.h>
 #include <assert.h>
 
 static uiox_us_frame_t s_raw_desc[UIOX_US_RAW_POOL_SIZE];
 static uint8_t         s_raw_mem [UIOX_US_RAW_POOL_SIZE]
                                   [UIOX_US_RAW_FRAME_MAX + UIOX_US_BUF_ALIGN];
 
 static uiox_us_frame_t s_dsp_desc[UIOX_US_DSP_POOL_SIZE];
 static uint8_t         s_dsp_mem [UIOX_US_DSP_POOL_SIZE]
                                   [UIOX_US_DSP_FRAME_MAX + UIOX_US_BUF_ALIGN];
 
 static uiox_us_frame_t *s_raw_free = NULL;
 static uiox_us_frame_t *s_dsp_free = NULL;
 static uint8_t          s_raw_cnt  = 0;
 static uint8_t          s_dsp_cnt  = 0;
 
 static uintptr_t align_up(uintptr_t p, uintptr_t a)
 { return (p + a - 1u) & ~(a - 1u); }
 
 static void build_pool(uiox_us_frame_t *descs, int n,
                         uint8_t (*mem)[], uint32_t cap,
                         uiox_us_frame_t **list, uint8_t *cnt)
 {
     *list = NULL; *cnt = 0;
     for (int i = 0; i < n; i++) {
         uiox_us_frame_t *f = &descs[i];
         memset(f, 0, sizeof(*f));
         uintptr_t base = (uintptr_t)mem[i];
         uintptr_t al   = align_up(base, UIOX_US_BUF_ALIGN);
         f->vaddr    = (uint8_t *)al;
         f->paddr    = al;
         f->capacity = cap;
         f->next     = *list;
         *list       = f;
         (*cnt)++;
     }
 }
 
 void uiox_us_buf_init(void)
 {
     build_pool(s_raw_desc, UIOX_US_RAW_POOL_SIZE,
                s_raw_mem,  UIOX_US_RAW_FRAME_MAX,
                &s_raw_free, &s_raw_cnt);
     build_pool(s_dsp_desc, UIOX_US_DSP_POOL_SIZE,
                s_dsp_mem,  UIOX_US_DSP_FRAME_MAX,
                &s_dsp_free, &s_dsp_cnt);
 }
 
 static uiox_us_frame_t *pool_alloc(uiox_us_frame_t **list, uint8_t *cnt)
 {
     if (!*list) return NULL;
     uiox_us_frame_t *f = *list;
     *list = f->next;
     (*cnt)--;
     f->next   = NULL;
     f->in_use = 1;
     f->used   = 0;
     return f;
 }
 
 uiox_us_frame_t *uiox_us_buf_alloc_raw(void)
 {
     uiox_us_frame_t *f = pool_alloc(&s_raw_free, &s_raw_cnt);
     if (f) f->type = UIOX_US_FRAME_RAW;
     return f;
 }
 
 uiox_us_frame_t *uiox_us_buf_alloc_dsp(void)
 {
     uiox_us_frame_t *f = pool_alloc(&s_dsp_free, &s_dsp_cnt);
     if (f) f->type = UIOX_US_FRAME_RESULT;
     return f;
 }
 
 void uiox_us_buf_ref(uiox_us_frame_t *f)
 { if (f) f->in_use++; }
 
 void uiox_us_buf_free(uiox_us_frame_t *f)
 {
     if (!f) return;
     assert(f->in_use > 0);
     if (--f->in_use == 0) {
         bool is_raw = (f->capacity == UIOX_US_RAW_FRAME_MAX);
         if (is_raw) { f->next = s_raw_free; s_raw_free = f; s_raw_cnt++; }
         else        { f->next = s_dsp_free; s_dsp_free = f; s_dsp_cnt++; }
     }
 }
 
 uint8_t uiox_us_buf_raw_free(void) { return s_raw_cnt; }
 uint8_t uiox_us_buf_dsp_free(void) { return s_dsp_cnt; }
 