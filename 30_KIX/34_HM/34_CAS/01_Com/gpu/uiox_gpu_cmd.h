/**
 * @file    uiox_gpu_cmd.h
 * @brief   UIOX GPU command buffer — draw calls, state, dispatch.
 *
 * Encodes GPU commands into a binary command buffer for submission.
 * Commands include:
 *   - Bind shader program
 *   - Bind vertex / index buffer
 *   - Bind uniform buffer
 *   - Set viewport / scissor / depth
 *   - Draw (indexed / non-indexed)
 *   - Dispatch compute
 *   - Pipeline barriers
 *   - Clear colour / depth
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_GPU_CMD_H
 #define UIOX_GPU_CMD_H
 
 #include "uiox_gpu_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Command opcodes
  * ====================================================================== */
 
 typedef enum {
     UIOX_GPU_CMD_NOP            = 0x00u,
     UIOX_GPU_CMD_BIND_SHADER    = 0x01u,
     UIOX_GPU_CMD_BIND_VBO       = 0x02u,
     UIOX_GPU_CMD_BIND_IBO       = 0x03u,
     UIOX_GPU_CMD_BIND_UBO       = 0x04u,
     UIOX_GPU_CMD_BIND_TEXTURE   = 0x05u,
     UIOX_GPU_CMD_BIND_FBO       = 0x06u,
     UIOX_GPU_CMD_SET_VIEWPORT   = 0x07u,
     UIOX_GPU_CMD_SET_SCISSOR    = 0x08u,
     UIOX_GPU_CMD_SET_DEPTH      = 0x09u,
     UIOX_GPU_CMD_SET_BLEND      = 0x0Au,
     UIOX_GPU_CMD_SET_CULL       = 0x0Bu,
     UIOX_GPU_CMD_CLEAR          = 0x0Cu,
     UIOX_GPU_CMD_DRAW           = 0x0Du,
     UIOX_GPU_CMD_DRAW_INDEXED   = 0x0Eu,
     UIOX_GPU_CMD_DISPATCH       = 0x0Fu,
     UIOX_GPU_CMD_BARRIER        = 0x10u,
     UIOX_GPU_CMD_PUSH_CONSTANTS = 0x11u,
     UIOX_GPU_CMD_END            = 0xFFu,
 } uiox_gpu_cmd_op_t;
 
 /* =========================================================================
  * Blend modes
  * ====================================================================== */
 
 typedef enum {
     UIOX_GPU_BLEND_NONE = 0,
     UIOX_GPU_BLEND_ALPHA,
     UIOX_GPU_BLEND_ADDITIVE,
     UIOX_GPU_BLEND_PREMULT_ALPHA,
 } uiox_gpu_blend_t;
 
 /* =========================================================================
  * Cull mode
  * ====================================================================== */
 
 typedef enum {
     UIOX_GPU_CULL_NONE = 0,
     UIOX_GPU_CULL_FRONT,
     UIOX_GPU_CULL_BACK,
 } uiox_gpu_cull_t;
 
 /* =========================================================================
  * Command buffer context
  * ====================================================================== */
 
 typedef struct {
     uiox_gpu_buf_t *buf;     /**< Backing CMD buffer object               */
     uint8_t        *write;   /**< Current write pointer                   */
     uint32_t        size;    /**< Total bytes written                     */
     bool            recording;
     bool            finished;
 } uiox_gpu_cmd_t;
 
 /* =========================================================================
  * Command buffer API
  * ====================================================================== */
 
 int  uiox_gpu_cmd_begin   (uiox_gpu_cmd_t *cmd, uiox_gpu_buf_t *buf);
 int  uiox_gpu_cmd_end     (uiox_gpu_cmd_t *cmd);
 void uiox_gpu_cmd_reset   (uiox_gpu_cmd_t *cmd);
 
 /* State commands */
 int  uiox_gpu_cmd_bind_shader  (uiox_gpu_cmd_t *cmd, uint32_t shader_id);
 int  uiox_gpu_cmd_bind_vbo     (uiox_gpu_cmd_t *cmd,
                                  const uiox_gpu_buf_t *vbo,
                                  uint32_t stride, uint32_t offset);
 int  uiox_gpu_cmd_bind_ibo     (uiox_gpu_cmd_t *cmd,
                                  const uiox_gpu_buf_t *ibo,
                                  bool use_16bit_indices);
 int  uiox_gpu_cmd_bind_ubo     (uiox_gpu_cmd_t *cmd,
                                  const uiox_gpu_buf_t *ubo,
                                  uint8_t binding, uint32_t offset);
 int  uiox_gpu_cmd_bind_texture (uiox_gpu_cmd_t *cmd,
                                  const uiox_gpu_texture_t *tex,
                                  uint8_t unit);
 int  uiox_gpu_cmd_bind_fbo     (uiox_gpu_cmd_t *cmd,
                                  const uiox_gpu_fbo_t *fbo);
 int  uiox_gpu_cmd_set_viewport (uiox_gpu_cmd_t *cmd,
                                  float x, float y, float w, float h,
                                  float near, float far);
 int  uiox_gpu_cmd_set_scissor  (uiox_gpu_cmd_t *cmd,
                                  uint16_t x, uint16_t y,
                                  uint16_t w, uint16_t h);
 int  uiox_gpu_cmd_set_depth    (uiox_gpu_cmd_t *cmd,
                                  bool test, bool write, uint8_t func);
 int  uiox_gpu_cmd_set_blend    (uiox_gpu_cmd_t *cmd, uiox_gpu_blend_t mode);
 int  uiox_gpu_cmd_set_cull     (uiox_gpu_cmd_t *cmd, uiox_gpu_cull_t mode);
 int  uiox_gpu_cmd_clear        (uiox_gpu_cmd_t *cmd,
                                  float r, float g, float b, float a,
                                  float depth, bool colour, bool do_depth);
 int  uiox_gpu_cmd_push_constants(uiox_gpu_cmd_t *cmd,
                                   const void *data, uint16_t size,
                                   uint32_t stage_flags);
 
 /* Draw commands */
 int  uiox_gpu_cmd_draw         (uiox_gpu_cmd_t *cmd,
                                  uiox_gpu_topology_t topo,
                                  uint32_t first_vertex, uint32_t count,
                                  uint32_t instances);
 int  uiox_gpu_cmd_draw_indexed (uiox_gpu_cmd_t *cmd,
                                  uiox_gpu_topology_t topo,
                                  uint32_t index_count,
                                  uint32_t first_index,
                                  int32_t  vertex_offset,
                                  uint32_t instances);
 int  uiox_gpu_cmd_dispatch     (uiox_gpu_cmd_t *cmd,
                                  uint32_t gx, uint32_t gy, uint32_t gz);
 int  uiox_gpu_cmd_barrier      (uiox_gpu_cmd_t *cmd, uint32_t flags);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_GPU_CMD_H */
 