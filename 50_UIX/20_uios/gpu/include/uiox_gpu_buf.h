/**
 * @file    uiox_gpu_buf.h
 * @brief   UIOX GPU buffer objects (VBO, IBO, UBO, FBO, texture).
 *
 * Manages static pools of GPU buffer descriptors:
 *   VBO — vertex buffer objects
 *   IBO — index buffer objects
 *   UBO — uniform buffer objects
 *   FBO — framebuffer objects
 *   TEX — texture objects
 *   CMD — command buffer objects
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_GPU_BUF_H
 #define UIOX_GPU_BUF_H
 
 #include "uiox_gpu_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Buffer pool sizes
  * ====================================================================== */
 
 #define UIOX_GPU_VBO_POOL_SIZE   16
 #define UIOX_GPU_IBO_POOL_SIZE   16
 #define UIOX_GPU_UBO_POOL_SIZE   32
 #define UIOX_GPU_FBO_POOL_SIZE   4
 #define UIOX_GPU_TEX_POOL_SIZE   32
 #define UIOX_GPU_CMD_POOL_SIZE   8
 
 /* Max sizes per buffer type */
 #define UIOX_GPU_VBO_MAX_BYTES   (4u * 1024u * 1024u)  /* 4 MB  */
 #define UIOX_GPU_IBO_MAX_BYTES   (1u * 1024u * 1024u)  /* 1 MB  */
 #define UIOX_GPU_UBO_MAX_BYTES   (64u * 1024u)          /* 64 KB */
 #define UIOX_GPU_CMD_MAX_BYTES   (256u * 1024u)         /* 256 KB*/
 #define UIOX_GPU_BUF_ALIGN       256
 
 /* =========================================================================
  * Buffer object types
  * ====================================================================== */
 
 typedef enum {
     UIOX_GPU_BUF_VBO = 0,
     UIOX_GPU_BUF_IBO,
     UIOX_GPU_BUF_UBO,
     UIOX_GPU_BUF_CMD,
 } uiox_gpu_buf_type_t;
 
 /* =========================================================================
  * Generic GPU buffer object
  * ====================================================================== */
 
 typedef struct uiox_gpu_buf {
     uint8_t    *cpu_addr;     /**< CPU virtual address                    */
     uint64_t    gpu_va;       /**< GPU virtual address (after MMU map)    */
     uintptr_t   paddr;        /**< Physical address (for DMA)             */
     uint32_t    capacity;     /**< Allocated bytes                        */
     uint32_t    used;         /**< Bytes of valid data written            */
     uiox_gpu_buf_type_t type;
     uint8_t     in_use;
     struct uiox_gpu_buf *next;
 } uiox_gpu_buf_t;
 
 /* =========================================================================
  * Texture descriptor
  * ====================================================================== */
 
 typedef struct uiox_gpu_texture {
     uint8_t    *cpu_addr;
     uint64_t    gpu_va;
     uintptr_t   paddr;
     uint32_t    capacity;
     uint16_t    width, height;
     uint8_t     mip_levels;
     uiox_gpu_fmt_t fmt;
     uint8_t     in_use;
     struct uiox_gpu_texture *next;
 } uiox_gpu_texture_t;
 
 /* =========================================================================
  * Framebuffer object
  * ====================================================================== */
 
 typedef struct uiox_gpu_fbo {
     uiox_gpu_texture_t *colour[4]; /**< Up to 4 colour attachments        */
     uiox_gpu_texture_t *depth;
     uiox_gpu_texture_t *stencil;
     uint16_t            width, height;
     uint8_t             msaa_samples;
     uint8_t             num_colour;
     uint8_t             in_use;
     struct uiox_gpu_fbo *next;
 } uiox_gpu_fbo_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void             uiox_gpu_buf_pool_init(void);
 
 uiox_gpu_buf_t  *uiox_gpu_buf_alloc   (uiox_gpu_buf_type_t type);
 void             uiox_gpu_buf_free     (uiox_gpu_buf_t *buf);
 void             uiox_gpu_buf_ref      (uiox_gpu_buf_t *buf);
 
 uiox_gpu_texture_t *uiox_gpu_tex_alloc(uint16_t w, uint16_t h,
                                         uint8_t mips, uiox_gpu_fmt_t fmt);
 void             uiox_gpu_tex_free     (uiox_gpu_texture_t *tex);
 
 uiox_gpu_fbo_t  *uiox_gpu_fbo_alloc   (uint16_t w, uint16_t h,
                                         uint8_t msaa);
 void             uiox_gpu_fbo_free     (uiox_gpu_fbo_t *fbo);
 int              uiox_gpu_fbo_attach   (uiox_gpu_fbo_t *fbo,
                                         uiox_gpu_texture_t *tex,
                                         bool is_depth);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_GPU_BUF_H */
 