/**
 * @file    uiox_hdmi_buf.c
 * @brief   UIOX HDMI buffer pool implementation.
 * @date    2026-05-28
 */

 #include "uiox_hdmi_buf.h"

 /* Freestanding assert — no libc assert.h available under -nostdinc */
 #ifndef UIOX_ASSERT
 #  define UIOX_ASSERT(cond)  do { if (!(cond)) __builtin_trap(); } while (0)
 #endif
 
 /*
  * Freestanding-safe pointer <-> integer helpers via memcpy.
  * sizeof(uintptr_t) == sizeof(void *) on every target — no cast-size
  * warnings on arm32 or arm64.
  */
 #define PTR_TO_UINTPTR(dst, src)                        \
     do { const void *_q = (const void *)(src);          \
          memcpy(&(dst), &_q, sizeof(dst)); } while (0)
 
 #define UINTPTR_TO_PTR(dst, src)                        \
     do { uintptr_t _u = (src);                          \
          memcpy(&(dst), &_u, sizeof(dst)); } while (0)
 
 static uiox_hdmi_fb_t  s_fb_desc[UIOX_HDMI_FB_POOL_SIZE];
 static uint8_t         s_fb_mem [UIOX_HDMI_FB_POOL_SIZE]
                                  [UIOX_HDMI_FB_MAX_BYTES + UIOX_HDMI_FB_ALIGN];
 static uiox_hdmi_fb_t *s_fb_free = NULL;
 static uint8_t         s_fb_cnt  = 0;
 
 static uiox_hdmi_pkt_t  s_pkt_pool[UIOX_HDMI_PKT_POOL_SIZE];
 static uiox_hdmi_pkt_t *s_pkt_free = NULL;
 static uint16_t          s_pkt_cnt  = 0;
 
 static uintptr_t align_up(uintptr_t p, uintptr_t a)
 { return (p + a - 1u) & ~(a - 1u); }
 
 void uiox_hdmi_buf_init(uint16_t w, uint16_t h, uint32_t stride,
                          uiox_hdmi_colorspace_t cs, uint8_t bpc)
 {
     s_fb_free = NULL; s_fb_cnt = 0;
     for (int i = 0; i < UIOX_HDMI_FB_POOL_SIZE; i++) {
         uiox_hdmi_fb_t *fb = &s_fb_desc[i];
         memset(fb, 0, sizeof(*fb));
 
         /* line 34 fix: pointer → uintptr_t via memcpy, no cast */
         uintptr_t base;
         uint8_t  *mem_i = s_fb_mem[i];
         PTR_TO_UINTPTR(base, mem_i);
         uintptr_t al = align_up(base, UIOX_HDMI_FB_ALIGN);
 
         /* line 35 fix: uintptr_t → uint8_t * via memcpy, no cast */
         uint8_t *vaddr;
         UINTPTR_TO_PTR(vaddr, al);
         fb->vaddr    = vaddr;
         fb->paddr    = al;          /* uintptr_t = uintptr_t — no warning */
 
         fb->capacity = UIOX_HDMI_FB_MAX_BYTES;
         fb->width    = w;
         fb->height   = h;
         fb->stride   = stride;
         fb->cs       = cs;
         fb->bpc      = bpc;
         fb->state    = UIOX_HDMI_FB_FREE;
         fb->next     = s_fb_free;
         s_fb_free    = fb;
         s_fb_cnt++;
     }
 
     s_pkt_free = NULL; s_pkt_cnt = 0;
     for (int i = 0; i < UIOX_HDMI_PKT_POOL_SIZE; i++) {
         memset(&s_pkt_pool[i], 0, sizeof(s_pkt_pool[i]));
         s_pkt_pool[i].next = s_pkt_free;
         s_pkt_free = &s_pkt_pool[i]; s_pkt_cnt++;
     }
 }
 
 uiox_hdmi_fb_t *uiox_hdmi_buf_alloc_fb(void)
 {
     if (!s_fb_free) return NULL;
     uiox_hdmi_fb_t *fb = s_fb_free;
     s_fb_free = fb->next; s_fb_cnt--;
     fb->next = NULL; fb->in_use = 1; fb->state = UIOX_HDMI_FB_RENDERING;
     return fb;
 }
 
 void uiox_hdmi_buf_free_fb(uiox_hdmi_fb_t *fb)
 {
     if (!fb) return;
     UIOX_ASSERT(fb->in_use > 0);
     if (--fb->in_use == 0) {
         fb->state = UIOX_HDMI_FB_FREE;
         fb->next = s_fb_free; s_fb_free = fb; s_fb_cnt++;
     }
 }
 
 void uiox_hdmi_buf_ref_fb(uiox_hdmi_fb_t *fb) { if (fb) fb->in_use++; }
 uint8_t uiox_hdmi_buf_fb_free(void) { return s_fb_cnt; }
 
 uiox_hdmi_pkt_t *uiox_hdmi_buf_alloc_pkt(void)
 {
     if (!s_pkt_free) return NULL;
     uiox_hdmi_pkt_t *p = s_pkt_free;
     s_pkt_free = p->next; s_pkt_cnt--;
     p->next = NULL; p->in_use = 1;
     memset(p->payload, 0, sizeof(p->payload));
     return p;
 }
 
 void uiox_hdmi_buf_free_pkt(uiox_hdmi_pkt_t *pkt)
 {
     if (!pkt) return;
     UIOX_ASSERT(pkt->in_use > 0);
     if (--pkt->in_use == 0) {
         pkt->next = s_pkt_free; s_pkt_free = pkt; s_pkt_cnt++;
     }
 }
 
 uint16_t uiox_hdmi_buf_pkt_free(void) { return s_pkt_cnt; }
 
 void uiox_hdmi_buf_clear_fb(uiox_hdmi_fb_t *fb, uint32_t colour)
 {
     if (!fb || !fb->vaddr) return;
     uint32_t *px = (uint32_t *)fb->vaddr;
     uint32_t  n  = (fb->stride * fb->height) / 4u;
     for (uint32_t i = 0; i < n; i++) px[i] = colour;
 }
 