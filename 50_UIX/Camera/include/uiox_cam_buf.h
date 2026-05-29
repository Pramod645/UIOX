/**
 * @file    uiox_cam_buf.h
 * @brief   Frame buffer pool for camera capture (zero-copy).
 */

 #ifndef UIOX_CAM_BUF_H
 #define UIOX_CAM_BUF_H
 
 #include <stdint.h>
 #include <stddef.h>
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_CAM_POOL_FRAMES     8
 #define UIOX_CAM_FRAME_MAX       (1920*1080*2) /* Enough for 1080p YUV422 */
 #define UIOX_CAM_FRAME_ALIGN     64
 
 typedef struct uiox_cam_frame {
     uint8_t   *vaddr;      /* virtual address */
     uintptr_t  paddr;      /* physical address (for DMA) */
     uint32_t   length;     /* total allocated size in bytes */
     uint16_t   width;      /* active capture width */
     uint16_t   height;     /* active capture height */
     uint32_t   stride;     /* bytes per line */
     uint64_t   ts_ns;      /* timestamp (filled on capture complete) */
     uint8_t    fmt;        /* uiox_cam_pixfmt_t */
     uint8_t    in_use;     /* reference count */
     struct uiox_cam_frame *next;
 } uiox_cam_frame_t;
 
 void  uiox_cam_buf_init(uint16_t width, uint16_t height,
                         uint32_t stride, uint8_t fmt);
 uiox_cam_frame_t *uiox_cam_buf_alloc(void);
 void  uiox_cam_buf_ref  (uiox_cam_frame_t *f);
 void  uiox_cam_buf_free (uiox_cam_frame_t *f);
 uint16_t uiox_cam_buf_free_count(void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif
 