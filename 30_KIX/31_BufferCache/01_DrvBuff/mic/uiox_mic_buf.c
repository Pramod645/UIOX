/**
 * @file    uiox_mic_buf.c
 * @brief   UIOX Microphone buffer pool and ring buffer implementation.
 * @date    2026-06-03
 */

 #include "uiox_mic_buf.h"
 #include <string.h>
 #include <assert.h>
 
 static uiox_mic_frame_t s_cap_desc [UIOX_MIC_CAP_POOL_SIZE];
 static uiox_mic_frame_t s_proc_desc[UIOX_MIC_PROC_POOL_SIZE];
 
 static uint8_t s_cap_mem [UIOX_MIC_CAP_POOL_SIZE]
                           [UIOX_MIC_FRAME_MAX_BYTES + UIOX_MIC_BUF_ALIGN];
 static uint8_t s_proc_mem[UIOX_MIC_PROC_POOL_SIZE]
                           [UIOX_MIC_FRAME_MAX_BYTES + UIOX_MIC_BUF_ALIGN];
 
 static uiox_mic_frame_t *s_cap_free  = NULL;
 static uiox_mic_frame_t *s_proc_free = NULL;
 static uint8_t           s_cap_cnt   = 0;
 
 static uintptr_t align_up(uintptr_t p, uintptr_t a)
 { return (p + a - 1u) & ~(a - 1u); }
 
 static void build_pool(uiox_mic_frame_t *descs, int n,
                         uint8_t (*mem)[], uint32_t cap,
                         uiox_mic_frame_t **list, uint8_t *cnt)
 {
     *list = NULL;
     if (cnt) *cnt = 0;
     for (int i = 0; i < n; i++) {
         uiox_mic_frame_t *f = &descs[i];
         memset(f, 0, sizeof(*f));
         uintptr_t base = (uintptr_t)mem[i];
         uintptr_t al   = align_up(base, UIOX_MIC_BUF_ALIGN);
         f->data     = (uint8_t *)al;
         f->paddr    = al;
         f->capacity = cap;
         f->state    = UIOX_MIC_FRAME_FREE;
         f->next     = *list;
         *list       = f;
         if (cnt) (*cnt)++;
     }
 }
 
 void uiox_mic_buf_init(void)
 {
     build_pool(s_cap_desc,  UIOX_MIC_CAP_POOL_SIZE,  s_cap_mem,
                UIOX_MIC_FRAME_MAX_BYTES, &s_cap_free,  &s_cap_cnt);
     build_pool(s_proc_desc, UIOX_MIC_PROC_POOL_SIZE, s_proc_mem,
                UIOX_MIC_FRAME_MAX_BYTES, &s_proc_free, NULL);
 }
 
 static uiox_mic_frame_t *pool_alloc(uiox_mic_frame_t **list, uint8_t *cnt)
 {
     if (!*list) return NULL;
     uiox_mic_frame_t *f = *list;
     *list      = f->next;
     if (cnt && *cnt > 0) (*cnt)--;
     f->next        = NULL;
     f->in_use      = 1;
     f->valid_bytes = 0;
     f->num_samples = 0;
     f->state       = UIOX_MIC_FRAME_CAPTURING;
     return f;
 }
 
 uiox_mic_frame_t *uiox_mic_buf_alloc_cap (void)
 { return pool_alloc(&s_cap_free,  &s_cap_cnt); }
 
 uiox_mic_frame_t *uiox_mic_buf_alloc_proc(void)
 { return pool_alloc(&s_proc_free, NULL); }
 
 void uiox_mic_buf_free(uiox_mic_frame_t *f)
 {
     if (!f) return;
     assert(f->in_use > 0);
     if (--f->in_use == 0) {
         f->state = UIOX_MIC_FRAME_FREE;
         bool is_cap = (f >= s_cap_desc &&
                        f <  s_cap_desc + UIOX_MIC_CAP_POOL_SIZE);
         if (is_cap) { f->next = s_cap_free;  s_cap_free  = f; s_cap_cnt++; }
         else        { f->next = s_proc_free; s_proc_free = f; }
     }
 }
 
 uint8_t uiox_mic_buf_cap_free(void) { return s_cap_cnt; }
 
 /* =========================================================================
  * SPSC ring buffer
  * ====================================================================== */
 
 void uiox_mic_ring_init(uiox_mic_ring_t *r)
 {
     if (!r) return;
     r->head = r->tail = r->overflow = r->underrun = 0;
     memset(r->buf, 0, sizeof(r->buf));
 }
 
 uint32_t uiox_mic_ring_write(uiox_mic_ring_t *r,
                               const int16_t *samples, uint32_t n)
 {
     if (!r || !samples) return 0;
     uint32_t written = 0;
     while (written < n) {
         uint32_t next = (r->head + 1u) & UIOX_MIC_RING_MASK;
         if (next == r->tail) { r->overflow++; break; }
         r->buf[r->head] = samples[written++];
         r->head = next;
     }
     return written;
 }
 
 uint32_t uiox_mic_ring_read(uiox_mic_ring_t *r, int16_t *samples, uint32_t n)
 {
     if (!r || !samples) return 0;
     uint32_t rd = 0;
     while (rd < n) {
         if (r->tail == r->head) { r->underrun++; break; }
         samples[rd++] = r->buf[r->tail];
         r->tail = (r->tail + 1u) & UIOX_MIC_RING_MASK;
     }
     return rd;
 }
 
 uint32_t uiox_mic_ring_avail(const uiox_mic_ring_t *r)
 { return r ? (r->head - r->tail) & UIOX_MIC_RING_MASK : 0u; }
 
 uint32_t uiox_mic_ring_space(const uiox_mic_ring_t *r)
 { return r ? (UIOX_MIC_RING_SAMPLES - 1u -
               ((r->head - r->tail) & UIOX_MIC_RING_MASK)) : 0u; }
 
 void uiox_mic_ring_flush(uiox_mic_ring_t *r)
 { if (r) r->tail = r->head; }
 