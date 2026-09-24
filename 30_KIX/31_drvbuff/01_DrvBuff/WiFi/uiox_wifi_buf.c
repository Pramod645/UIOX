/**
 * @file    uiox_wifi_buf.c
 * @brief   UIOX WiFi frame buffer pool implementation.
 * @date    2026-05-28
 */

 #include "uiox_wifi_buf.h"

 /* Freestanding assert — no libc assert.h available under -nostdinc */
 #ifndef UIOX_ASSERT
 #  define UIOX_ASSERT(cond)  do { if (!(cond)) __builtin_trap(); } while (0)
 #endif
 
 /*
  * Freestanding-safe pointer <-> integer helpers via memcpy.
  * sizeof(uintptr_t) == sizeof(void *) on every target, so these are
  * always correct and emit no -Werror=pointer-to-int-cast warnings on
  * arm32 or arm64.
  *
  * PTR_TO_UINTPTR : any pointer  → uintptr_t  (for alignment arithmetic)
  * UINTPTR_TO_PTR : uintptr_t    → uint8_t *  (after alignment)
  * PTR_TO_PADDR   : any pointer  → uint64_t   (store into f->paddr)
  */
 #define PTR_TO_UINTPTR(dst, src)                             \
     do { const void *_q = (const void *)(src);               \
          memcpy(&(dst), &_q, sizeof(dst)); } while (0)
 
 #define UINTPTR_TO_PTR(dst, src)                             \
     do { uintptr_t _u = (src);                               \
          memcpy(&(dst), &_u, sizeof(dst)); } while (0)
 
 #define PTR_TO_PADDR(dst, src)                               \
     do { const void *_p = (const void *)(src); (dst) = 0u;   \
          memcpy(&(dst), &_p, sizeof(_p)); } while (0)
 
 static uiox_wifi_frame_t s_tx_desc[UIOX_WIFI_TX_POOL_SIZE];
 static uiox_wifi_frame_t s_rx_desc[UIOX_WIFI_RX_POOL_SIZE];
 
 #define _UIOX_WIFI_BUF_STRIDE  (UIOX_WIFI_HEADROOM + UIOX_WIFI_FRAME_MAX + UIOX_WIFI_BUF_ALIGN)
 
 static uint8_t s_tx_mem[UIOX_WIFI_TX_POOL_SIZE][_UIOX_WIFI_BUF_STRIDE];
 static uint8_t s_rx_mem[UIOX_WIFI_RX_POOL_SIZE][_UIOX_WIFI_BUF_STRIDE];
 
 static uiox_wifi_frame_t *s_tx_free = NULL;
 static uiox_wifi_frame_t *s_rx_free = NULL;
 static uint16_t           s_tx_cnt  = 0;
 static uint16_t           s_rx_cnt  = 0;
 
 static uintptr_t align_up(uintptr_t p, uintptr_t a)
 { return (p + a - 1u) & ~(a - 1u); }
 
 static void build_pool(uiox_wifi_frame_t *descs, int n,
                        uint8_t (*mem)[_UIOX_WIFI_BUF_STRIDE],
                        uint16_t *cnt,
                        uiox_wifi_frame_t **list)
 {
     *list = NULL; *cnt = 0;
     for (int i = 0; i < n; i++) {
         uiox_wifi_frame_t *f = &descs[i];
         memset(f, 0, sizeof(*f));
 
         /* line 45 fix: pointer → uintptr_t via memcpy, no cast */
         uintptr_t base;
         uint8_t  *mem_i = mem[i];
         PTR_TO_UINTPTR(base, mem_i);
 
         uintptr_t aligned = align_up(base, UIOX_WIFI_BUF_ALIGN);
 
         /* line 47 fix: uintptr_t → uint8_t * via memcpy, no cast */
         uint8_t *aligned_ptr;
         UINTPTR_TO_PTR(aligned_ptr, aligned);
 
         f->buf_start = aligned_ptr;
         f->buf_end   = aligned_ptr + UIOX_WIFI_HEADROOM + UIOX_WIFI_FRAME_MAX;
         f->data      = aligned_ptr + UIOX_WIFI_HEADROOM;
 
         /* f->paddr is uint64_t; store as pointer → uint64_t via memcpy */
         PTR_TO_PADDR(f->paddr, f->data);
 
         f->len    = 0;
         f->in_use = 0;
         f->next   = *list;
         *list     = f;
         (*cnt)++;
     }
 }
 
 void uiox_wifi_buf_init(void)
 {
     build_pool(s_tx_desc, UIOX_WIFI_TX_POOL_SIZE,
                s_tx_mem, &s_tx_cnt, &s_tx_free);
     build_pool(s_rx_desc, UIOX_WIFI_RX_POOL_SIZE,
                s_rx_mem, &s_rx_cnt, &s_rx_free);
 }
 
 static uiox_wifi_frame_t *pool_alloc(uiox_wifi_frame_t **list,
                                       uint16_t *cnt)
 {
     if (!*list) return NULL;
     uiox_wifi_frame_t *f = *list;
     *list   = f->next;
     (*cnt)--;
     f->next   = NULL;
     f->in_use = 1;
     f->len    = 0;
     f->data   = f->buf_start + UIOX_WIFI_HEADROOM;
     /* line 78 fix: pointer → uint64_t via memcpy, no cast */
     PTR_TO_PADDR(f->paddr, f->data);
     return f;
 }
 
 uiox_wifi_frame_t *uiox_wifi_buf_alloc_tx(void)
 {
     uiox_wifi_frame_t *f = pool_alloc(&s_tx_free, &s_tx_cnt);
     if (f) f->type = UIOX_WIFI_FRAME_DATA;
     return f;
 }
 
 uiox_wifi_frame_t *uiox_wifi_buf_alloc_rx(void)
 {
     uiox_wifi_frame_t *f = pool_alloc(&s_rx_free, &s_rx_cnt);
     if (f) f->type = UIOX_WIFI_FRAME_DATA;
     return f;
 }
 
 void uiox_wifi_buf_ref(uiox_wifi_frame_t *f)
 { if (f) f->in_use++; }
 
 void uiox_wifi_buf_free(uiox_wifi_frame_t *f)
 {
     if (!f) return;
     UIOX_ASSERT(f->in_use > 0);
     if (--f->in_use == 0) {
         bool is_tx = (f >= s_tx_desc &&
                       f <  s_tx_desc + UIOX_WIFI_TX_POOL_SIZE);
         if (is_tx) { f->next = s_tx_free; s_tx_free = f; s_tx_cnt++; }
         else       { f->next = s_rx_free; s_rx_free = f; s_rx_cnt++; }
     }
 }
 
 void *uiox_wifi_buf_push(uiox_wifi_frame_t *f, uint16_t len)
 {
     if (!f || f->data - len < f->buf_start) return NULL;
     f->data -= len;
     f->len  += len;
     /* line 117 fix */
     PTR_TO_PADDR(f->paddr, f->data);
     return f->data;
 }
 
 void *uiox_wifi_buf_pull(uiox_wifi_frame_t *f, uint16_t len)
 {
     if (!f || len > f->len) return NULL;
     f->data += len;
     f->len  -= len;
     /* line 126 fix */
     PTR_TO_PADDR(f->paddr, f->data);
     return f->data;
 }
 
 void *uiox_wifi_buf_put(uiox_wifi_frame_t *f, uint16_t len)
 {
     if (!f) return NULL;
     uint8_t *tail = f->data + f->len;
     if (tail + len > f->buf_end) return NULL;
     f->len += len;
     return tail;
 }
 
 uint16_t uiox_wifi_buf_tx_free(void) { return s_tx_cnt; }
 uint16_t uiox_wifi_buf_rx_free(void) { return s_rx_cnt; }
 