/**
 * @file    uiox_gpu_device.h
 * @brief   UIOX GPU top-level application-facing device API.
 * @date    2026-06-01
 */

 #ifndef UIOX_GPU_DEVICE_H
 #define UIOX_GPU_DEVICE_H
 
 #include "uiox_gpu_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_gpu_hw_t            *hw;
     const uiox_gpu_hw_ops_t  *hw_ops;
     uint64_t                  vram_phys;
     uint64_t                  vram_size;
     uint32_t                  target_freq_mhz;
 } uiox_gpu_open_params_t;
 
 typedef struct {
     uiox_gpu_subsys_t  subsys;
     uiox_gpu_hw_t     *hw;
     bool               open;
 } uiox_gpu_device_t;
 
 int  uiox_gpu_open           (uiox_gpu_device_t           *dev,
                                const uiox_gpu_open_params_t *p);
 int  uiox_gpu_start          (uiox_gpu_device_t *dev);
 void uiox_gpu_stop           (uiox_gpu_device_t *dev);
 void uiox_gpu_close          (uiox_gpu_device_t *dev);
 
 int  uiox_gpu_create_pso     (uiox_gpu_device_t *dev,
                                const uiox_gpu_pso_t *desc);
 void uiox_gpu_destroy_pso    (uiox_gpu_device_t *dev, uint8_t pso_id);
 
 int  uiox_gpu_begin_frame    (uiox_gpu_device_t *dev);
 int  uiox_gpu_begin_pass     (uiox_gpu_device_t *dev,
                                const uiox_gpu_renderpass_t *pass);
 void uiox_gpu_end_pass       (uiox_gpu_device_t *dev);
 int  uiox_gpu_bind_pso       (uiox_gpu_device_t *dev, uint8_t pso_id);
 int  uiox_gpu_end_frame      (uiox_gpu_device_t *dev,
                                uint32_t wait_timeout_ms);
 
 /* Direct command buffer access */
 uiox_gpu_cmd_t *uiox_gpu_cmd (uiox_gpu_device_t *dev);
 
 /* Buffer helpers */
 uiox_gpu_buf_t     *uiox_gpu_alloc_vbo(uiox_gpu_device_t *dev,
                                         const void *data, uint32_t bytes);
 uiox_gpu_buf_t     *uiox_gpu_alloc_ibo(uiox_gpu_device_t *dev,
                                         const void *data, uint32_t bytes);
 uiox_gpu_buf_t     *uiox_gpu_alloc_ubo(uiox_gpu_device_t *dev,
                                         const void *data, uint32_t bytes);
 uiox_gpu_texture_t *uiox_gpu_alloc_tex(uiox_gpu_device_t *dev,
                                         uint16_t w, uint16_t h,
                                         uiox_gpu_fmt_t fmt,
                                         const void *pixels);
 uiox_gpu_fbo_t     *uiox_gpu_alloc_fbo(uiox_gpu_device_t *dev,
                                         uint16_t w, uint16_t h,
                                         uiox_gpu_fmt_t colour_fmt,
                                         bool with_depth);
 
 int  uiox_gpu_load_shader    (uiox_gpu_device_t *dev,
                                const uint8_t *binary, uint32_t size,
                                uint32_t stage_flags,
                                uint32_t *shader_id_out);
 
 void uiox_gpu_get_frame_stats(uiox_gpu_device_t     *dev,
                                uiox_gpu_frame_stats_t *out);
 void uiox_gpu_get_mem_stats  (uiox_gpu_device_t *dev,
                                uint64_t *used, uint64_t *free_bytes);
 void uiox_gpu_print_info     (const uiox_gpu_device_t *dev);
 void uiox_gpu_print_stats    (uiox_gpu_device_t *dev);
 
 const char *uiox_gpu_state_name(uiox_gpu_subsys_state_t s);
 const char *uiox_gpu_fmt_name (uiox_gpu_fmt_t fmt);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_GPU_DEVICE_H */
 