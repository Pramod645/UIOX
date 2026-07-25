/**
 * @file    uiox_mon_buf.c
 * @brief   UIOX Monitor framebuffer pool implementation.
 * @date    2026-05-27
 */

 #include "uiox_mon_buf.h"

 #ifndef UIOX_ASSERT
 #  define UIOX_ASSERT(cond)  do { if (!(cond)) __builtin_trap(); } while (0)
 #endif
 
 #define PTR_TO_UINTPTR(dst, src)                        \
     do { const void *_q = (const void *)(src);          \
          memcpy(&(dst), &_q, sizeof(dst)); } while (0)
 
 #define UINTPTR_TO_PTR(dst, src)                        \
     do { uintptr_t _u = (src);                          \
          memcpy(&(dst), &_u, sizeof(dst)); } while (0)
 
 static uiox_mon_fb_t  s_desc[UIOX_MON_BUF_POOL_SIZE];
 static uint8_t        s_mem [UIOX_MON_BUF_POOL_SIZE]
                              [UIOX_MON_BUF_MAX_BYTES + UIOX_MON_BUF_ALIGN];
 static uiox_mon_fb_t *s_free = NULL;
 static uint8_t        s_free_cnt = 0;
 static uint16_t       s_w, s_h;
 static uint32_t       s_stride;
 static uiox_mon_pixfmt_t s_fmt;
 
 static uintptr_t align_up(uintptr_t p, uintptr_t a)
 { return (p + a - 1u) & ~(a - 1u); }
 
 void uiox_mon_buf_init(uint16_t width, uint16_t height,
                         uint32_t stride, uiox_mon_pixfmt_t fmt)
 {
     s_free = NULL; s_free_cnt = 0;
     s_w = width; s_h = height; s_stride = stride; s_fmt = fmt;
     uint8_t bpp = (fmt == UIOX_MON_FMT_RGB565) ? 2u : 4u;
     for (int i = 0; i < UIOX_MON_BUF_POOL_SIZE; i++) {
         uiox_mon_fb_t *fb = &s_desc[i];
         memset(fb, 0, sizeof(*fb));
 
         /* line 35 fix: pointer → uintptr_t via memcpy */
         uintptr_t base;
         uint8_t  *mem_i = s_mem[i];
         PTR_TO_UINTPTR(base, mem_i);
         uintptr_t al = align_up(base, UIOX_MON_BUF_ALIGN);
 
         /* line 36 fix: uintptr_t → uint8_t * via memcpy */
         uint8_t *vaddr;
         UINTPTR_TO_PTR(vaddr, al);
         fb->vaddr    = vaddr;
         fb->paddr    = al;          /* uintptr_t = uintptr_t — no warning */
 
         fb->capacity = UIOX_MON_BUF_MAX_BYTES;
         fb->width    = width;
         fb->height   = height;
         fb->stride   = stride;
         fb->bpp      = bpp;
         fb->fmt      = fmt;
         fb->state    = UIOX_MON_BUF_FREE;
         fb->in_use   = 0;
         fb->next     = s_free;
         s_free       = fb;
         s_free_cnt++;
     }
 }
 
 uiox_mon_fb_t *uiox_mon_buf_alloc(void)
 {
     if (!s_free) return NULL;
     uiox_mon_fb_t *fb = s_free;
     s_free = fb->next; s_free_cnt--;
     fb->next = NULL; fb->in_use = 1;
     fb->state = UIOX_MON_BUF_RENDERING; fb->frame_id = 0;
     return fb;
 }
 
 void uiox_mon_buf_ref(uiox_mon_fb_t *fb) { if (fb) fb->in_use++; }
 
 void uiox_mon_buf_free(uiox_mon_fb_t *fb)
 {
     if (!fb) return;
     UIOX_ASSERT(fb->in_use > 0);
     if (--fb->in_use == 0) {
         fb->state = UIOX_MON_BUF_FREE;
         fb->next = s_free; s_free = fb; s_free_cnt++;
     }
 }
 
 uint8_t uiox_mon_buf_free_count(void) { return s_free_cnt; }
 
 void uiox_mon_buf_clear(uiox_mon_fb_t *fb, uint32_t colour)
 {
     if (!fb || !fb->vaddr) return;
     uint32_t *px = (uint32_t *)fb->vaddr;
     uint32_t  n  = (fb->stride * fb->height) / 4u;
     for (uint32_t i = 0; i < n; i++) px[i] = colour;
 }
 
 void uiox_mon_buf_copy(uiox_mon_fb_t *dst, const uiox_mon_fb_t *src)
 {
     if (!dst || !src || !dst->vaddr || !src->vaddr) return;
     uint32_t bytes = src->stride * src->height;
     if (bytes > dst->capacity) bytes = dst->capacity;
     memcpy(dst->vaddr, src->vaddr, bytes);
 }
 