/**
 * @file    uiox_mon_buf.h
 * @brief   UIOX Monitor framebuffer pool — zero-copy page-flip.
 *
 * Manages a static pool of framebuffers sized for the target resolution.
 * Supports double/triple buffering with hardware page-flip handoff.
 *
 * Layout per framebuffer:
 *   [ stride × height bytes of pixel data ]
 *   Aligned to UIOX_MON_BUF_ALIGN for DMA engine requirements.
 *
 * @date    2026-05-27
 */
//Layer 1.5 — Buffer Manager
 #ifndef UIOX_MON_BUF_H
 #define UIOX_MON_BUF_H
 
 #include "uiox_mon_hw.h"
#include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Pool configuration
  * FHD (1920×1080) × 4 bytes (XRGB8888) × 3 buffers = 23.7 MB
  * ====================================================================== */
 
 #define UIOX_MON_BUF_POOL_SIZE   3       /**< Triple buffering depth       */
 #define UIOX_MON_BUF_MAX_PIXELS  (1920u * 1080u)
 #define UIOX_MON_BUF_MAX_BPP     4       /**< Max bytes per pixel          */
 #define UIOX_MON_BUF_MAX_BYTES   (UIOX_MON_BUF_MAX_PIXELS * UIOX_MON_BUF_MAX_BPP)
 #define UIOX_MON_BUF_ALIGN       4096    /**< Page-aligned for DMA         */
 
 typedef enum {
     UIOX_MON_BUF_FREE = 0,
     UIOX_MON_BUF_RENDERING,    /**< CPU is drawing into this buffer        */
     UIOX_MON_BUF_PENDING,      /**< Flip submitted, waiting for VBlank     */
     UIOX_MON_BUF_DISPLAYED,    /**< Currently on screen                    */
 } uiox_mon_buf_state_t;
 
 /* =========================================================================
  * Framebuffer descriptor
  * ====================================================================== */
 
 typedef struct uiox_mon_fb {
     uint8_t    *vaddr;          /**< Virtual address of pixel data         */
     uintptr_t   paddr;          /**< Physical address (for DMA scanout)    */
     uint32_t    capacity;       /**< Allocated bytes                       */
     uint16_t    width;          /**< Active width (pixels)                 */
     uint16_t    height;         /**< Active height (lines)                 */
     uint32_t    stride;         /**< Bytes per line (may include padding)  */
     uint8_t     bpp;            /**< Bytes per pixel                       */
     uiox_mon_pixfmt_t   fmt;
     uiox_mon_buf_state_t state;
     uint32_t    frame_id;       /**< Monotonic frame counter               */
     uint64_t    ts_ns;          /**< Timestamp of last flip (ns)           */
     uint8_t     in_use;         /**< Reference count                       */
     struct uiox_mon_fb *next;   /**< Free-list linkage                     */
 } uiox_mon_fb_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void         uiox_mon_buf_init    (uint16_t width, uint16_t height,
                                     uint32_t stride, uiox_mon_pixfmt_t fmt);
 uiox_mon_fb_t *uiox_mon_buf_alloc (void);
 void         uiox_mon_buf_ref     (uiox_mon_fb_t *fb);
 void         uiox_mon_buf_free    (uiox_mon_fb_t *fb);
 uint8_t      uiox_mon_buf_free_count(void);
 
 /** Clear framebuffer to a solid colour (XRGB8888 format). */
 void         uiox_mon_buf_clear   (uiox_mon_fb_t *fb, uint32_t colour);
 
 /** Copy src framebuffer pixels into dst. */
 void         uiox_mon_buf_copy    (uiox_mon_fb_t *dst,
                                     const uiox_mon_fb_t *src);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MON_BUF_H */
 