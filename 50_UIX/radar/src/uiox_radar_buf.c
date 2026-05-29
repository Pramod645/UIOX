/**
 * @file    uiox_radar_buf.c
 * @brief   UIOX Radar buffer pool implementation.
 * @date    2026-05-26
 */

 #include "uiox_radar_buf.h"
 #include <string.h>
 #include <assert.h>
 
 /* -------------------------------------------------------------------------
  * Static storage
  * ---------------------------------------------------------------------- */
 
 static uiox_radar_frame_t s_raw_desc[UIOX_RADAR_RAW_POOL_SIZE];
 static uint8_t            s_raw_mem [UIOX_RADAR_RAW_POOL_SIZE]
                                     [UIOX_RADAR_RAW_FRAME_MAX + UIOX_RADAR_BUF_ALIGN];
 
 static uiox_radar_frame_t s_dsp_desc[UIOX_RADAR_DSP_POOL_SIZE];
 static uint8_t            s_dsp_mem [UIOX_RADAR_DSP_POOL_SIZE]
                                     [UIOX_RADAR_DSP_FRAME_MAX + UIOX_RADAR_BUF_ALIGN];
 
 static uiox_radar_frame_t *s_raw_free = NULL;
 static uiox_radar_frame_t *s_dsp_free = NULL;
 static uint8_t             s_raw_free_cnt = 0;
 static uint8_t             s_dsp_free_cnt = 0;
 
 /* -------------------------------------------------------------------------
  * Helpers
  * ---------------------------------------------------------------------- */
 
 static uintptr_t align_up(uintptr_t p, uintptr_t a)
 {
     return (p + (a - 1)) & ~(a - 1);
 }
 
 static void pool_build(uiox_radar_frame_t *descs, int count,
                        uint8_t (*mem_pool)[],
                        uint32_t frame_max,
                        uiox_radar_frame_t **free_list,
                        uint8_t *free_cnt)
 {
     *free_list = NULL;
     *free_cnt  = 0;
     for (int i = 0; i < count; i++) {
         uiox_radar_frame_t *f = &descs[i];
         memset(f, 0, sizeof(*f));
         uintptr_t base    = (uintptr_t)mem_pool[i];
         uintptr_t aligned = align_up(base, UIOX_RADAR_BUF_ALIGN);
         f->vaddr    = (uint8_t *)aligned;
         f->paddr    = aligned;         /* identity map — replace on MMU system */
         f->capacity = frame_max;
         f->used     = 0;
         f->in_use   = 0;
         f->next     = *free_list;
         *free_list  = f;
         (*free_cnt)++;
     }
 }
 
 /* -------------------------------------------------------------------------
  * Public API
  * ---------------------------------------------------------------------- */
 
 void uiox_radar_buf_init(void)
 {
     pool_build(s_raw_desc, UIOX_RADAR_RAW_POOL_SIZE,
                s_raw_mem,  UIOX_RADAR_RAW_FRAME_MAX,
                &s_raw_free, &s_raw_free_cnt);
 
     pool_build(s_dsp_desc, UIOX_RADAR_DSP_POOL_SIZE,
                s_dsp_mem,  UIOX_RADAR_DSP_FRAME_MAX,
                &s_dsp_free, &s_dsp_free_cnt);
 }
 
 static uiox_radar_frame_t *pool_alloc(uiox_radar_frame_t **free_list,
                                        uint8_t *free_cnt)
 {
     if (!*free_list) return NULL;
     uiox_radar_frame_t *f = *free_list;
     *free_list = f->next;
     (*free_cnt)--;
     f->next   = NULL;
     f->in_use = 1;
     f->used   = 0;
     return f;
 }
 
 uiox_radar_frame_t *uiox_radar_buf_alloc_raw(void)
 {
     uiox_radar_frame_t *f = pool_alloc(&s_raw_free, &s_raw_free_cnt);
     if (f) f->type = UIOX_RADAR_FRAME_RAW;
     return f;
 }
 
 uiox_radar_frame_t *uiox_radar_buf_alloc_dsp(void)
 {
     uiox_radar_frame_t *f = pool_alloc(&s_dsp_free, &s_dsp_free_cnt);
     if (f) f->type = UIOX_RADAR_FRAME_RANGE;
     return f;
 }
 
 void uiox_radar_buf_ref(uiox_radar_frame_t *f)
 {
     if (f) f->in_use++;
 }
 
 void uiox_radar_buf_free(uiox_radar_frame_t *f)
 {
     if (!f) return;
     assert(f->in_use > 0);
     if (--f->in_use == 0) {
         /* Determine which pool it belongs to */
         bool is_raw = (f->capacity == UIOX_RADAR_RAW_FRAME_MAX);
         if (is_raw) {
             f->next = s_raw_free;
             s_raw_free = f;
             s_raw_free_cnt++;
         } else {
             f->next = s_dsp_free;
             s_dsp_free = f;
             s_dsp_free_cnt++;
         }
     }
 }
 
 uint8_t uiox_radar_buf_raw_free_count(void) { return s_raw_free_cnt; }
 uint8_t uiox_radar_buf_dsp_free_count(void) { return s_dsp_free_cnt; }
 