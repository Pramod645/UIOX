/**
 * @file    uiox_gpu_subsys.h
 * @brief   UIOX GPU subsystem — render pipeline, passes, shaders.
 *
 * Manages:
 *   - Graphics pipeline state objects (PSO)
 *   - Render pass begin/end (with clear and load/store ops)
 *   - Shader program linking (vertex + fragment)
 *   - Frame lifecycle (begin/end frame, present)
 *   - GPU timeline and per-frame statistics
 *   - Compute pipeline
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_GPU_SUBSYS_H
 #define UIOX_GPU_SUBSYS_H
 
 #include "uiox_gpu_if.h"
 #include "uiox_gpu_cmd.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Pipeline State Object (PSO)
  * ====================================================================== */
 
 #define UIOX_GPU_MAX_PSO     16
 
 typedef struct {
     uint32_t          vert_shader;
     uint32_t          frag_shader;
     uint32_t          comp_shader;  /**< 0 = graphics pipeline            */
     uiox_gpu_topology_t topology;
     uiox_gpu_blend_t  blend;
     uiox_gpu_cull_t   cull;
     bool              depth_test;
     bool              depth_write;
     uint8_t           depth_func;
     uiox_gpu_fmt_t    colour_fmt;
     uiox_gpu_fmt_t    depth_fmt;
     bool              valid;
     uint8_t           id;
 } uiox_gpu_pso_t;
 
 /* =========================================================================
  * Render pass descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_gpu_fbo_t  *fbo;
     float            clear_r, clear_g, clear_b, clear_a;
     float            clear_depth;
     bool             clear_colour;
     bool             clear_depth_flag;
 } uiox_gpu_renderpass_t;
 
 /* =========================================================================
  * Frame statistics
  * ====================================================================== */
 
 typedef struct {
     uint32_t  draw_calls;
     uint32_t  triangles;
     uint32_t  vertices;
     uint32_t  dispatch_calls;
     uint64_t  gpu_time_ns;
     uint32_t  frame_id;
 } uiox_gpu_frame_stats_t;
 
 /* =========================================================================
  * Subsystem state
  * ====================================================================== */
 
 typedef enum {
     UIOX_GPU_SUBSYS_STOPPED = 0,
     UIOX_GPU_SUBSYS_IDLE,
     UIOX_GPU_SUBSYS_RECORDING,
     UIOX_GPU_SUBSYS_SUBMITTED,
 } uiox_gpu_subsys_state_t;
 
 /* =========================================================================
  * Subsystem descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_gpu_if_t           gif;
     uiox_gpu_mem_t           mem;
     uiox_gpu_cmd_t           cmd;
     uiox_gpu_buf_t          *cmd_buf;
     uiox_gpu_pso_t           pso_table[UIOX_GPU_MAX_PSO];
     uint8_t                  pso_count;
     uiox_gpu_subsys_state_t  state;
     uiox_gpu_frame_stats_t   frame_stats;
     uint32_t                 frame_id;
     uint32_t                 last_fence;
 } uiox_gpu_subsys_t;
 
 /* =========================================================================
  * Subsystem API
  * ====================================================================== */
 
 int  uiox_gpu_subsys_init    (uiox_gpu_subsys_t *sys,
                                uiox_gpu_hw_t *hw,
                                uint64_t vram_phys, uint64_t vram_size);
 void uiox_gpu_subsys_deinit  (uiox_gpu_subsys_t *sys);
 
 /** Create a pipeline state object. Returns PSO id (≥0) or negative errno. */
 int  uiox_gpu_subsys_create_pso(uiox_gpu_subsys_t *sys,
                                  const uiox_gpu_pso_t *desc);
 
 /** Destroy a PSO by id. */
 void uiox_gpu_subsys_destroy_pso(uiox_gpu_subsys_t *sys, uint8_t pso_id);
 
 /** Begin recording a new frame. */
 int  uiox_gpu_subsys_begin_frame(uiox_gpu_subsys_t *sys);
 
 /** Begin a render pass (binds FBO, emits clear). */
 int  uiox_gpu_subsys_begin_pass(uiox_gpu_subsys_t *sys,
                                  const uiox_gpu_renderpass_t *pass);
 
 /** End the current render pass. */
 void uiox_gpu_subsys_end_pass  (uiox_gpu_subsys_t *sys);
 
 /** Bind a PSO for subsequent draw calls. */
 int  uiox_gpu_subsys_bind_pso  (uiox_gpu_subsys_t *sys, uint8_t pso_id);
 
 /** Submit the recorded frame to the GPU and wait for completion. */
 int  uiox_gpu_subsys_end_frame (uiox_gpu_subsys_t *sys,
                                  uint32_t wait_timeout_ms);
 
 /** Snapshot frame statistics. */
 void uiox_gpu_subsys_frame_stats(const uiox_gpu_subsys_t *sys,
                                   uiox_gpu_frame_stats_t *out);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_GPU_SUBSYS_H */
 