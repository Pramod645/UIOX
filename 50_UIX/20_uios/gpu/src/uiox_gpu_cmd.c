/**
 * @file    uiox_gpu_cmd.c
 * @brief   UIOX GPU command buffer encoder implementation.
 * @date    2026-06-01
 */

 #include "uiox_gpu_cmd.h"
 #include <string.h>
 #include <errno.h>
 
 /* -------------------------------------------------------------------------
  * Internal write helper — appends bytes to command buffer
  * ---------------------------------------------------------------------- */
 
 static int cmd_write(uiox_gpu_cmd_t *cmd, const void *data, uint32_t len)
 {
     if (!cmd->recording) return -EINVAL;
     if (cmd->size + len > cmd->buf->capacity) return -ENOSPC;
     memcpy(cmd->write, data, len);
     cmd->write += len;
     cmd->size  += len;
     cmd->buf->used = cmd->size;
     return 0;
 }
 
 #define CMD_WRITE(cmd, op, payload) do { \
     uint8_t _op = (uint8_t)(op);        \
     if (cmd_write((cmd), &_op, 1) < 0)  \
         return -ENOSPC;                  \
     if (sizeof(payload) > 1 ||           \
         (sizeof(payload) == 1 &&         \
          ((const uint8_t*)&(payload))[0])) \
         cmd_write((cmd), &(payload), sizeof(payload)); \
 } while(0)
 
 int uiox_gpu_cmd_begin(uiox_gpu_cmd_t *cmd, uiox_gpu_buf_t *buf)
 {
     if (!cmd || !buf || buf->type != UIOX_GPU_BUF_CMD) return -EINVAL;
     cmd->buf       = buf;
     cmd->write     = buf->cpu_addr;
     cmd->size      = 0;
     cmd->recording = true;
     cmd->finished  = false;
     buf->used      = 0;
     return 0;
 }
 
 int uiox_gpu_cmd_end(uiox_gpu_cmd_t *cmd)
 {
     if (!cmd || !cmd->recording) return -EINVAL;
     uint8_t op = UIOX_GPU_CMD_END;
     cmd_write(cmd, &op, 1);
     cmd->recording = false;
     cmd->finished  = true;
     return 0;
 }
 
 void uiox_gpu_cmd_reset(uiox_gpu_cmd_t *cmd)
 {
     if (!cmd) return;
     cmd->write     = cmd->buf ? cmd->buf->cpu_addr : NULL;
     cmd->size      = 0;
     cmd->recording = false;
     cmd->finished  = false;
     if (cmd->buf) cmd->buf->used = 0;
 }
 
 /* -------------------------------------------------------------------------
  * State commands
  * ---------------------------------------------------------------------- */
 
 int uiox_gpu_cmd_bind_shader(uiox_gpu_cmd_t *cmd, uint32_t shader_id)
 {
     if (!cmd || !cmd->recording) return -EINVAL;
     struct { uint32_t id; } p = { shader_id };
     uint8_t op = UIOX_GPU_CMD_BIND_SHADER;
     cmd_write(cmd, &op, 1);
     return cmd_write(cmd, &p, sizeof(p));
 }
 
 int uiox_gpu_cmd_bind_vbo(uiox_gpu_cmd_t *cmd,
                            const uiox_gpu_buf_t *vbo,
                            uint32_t stride, uint32_t offset)
 {
     if (!cmd || !cmd->recording || !vbo) return -EINVAL;
     struct { uint64_t gpu_va; uint32_t stride; uint32_t offset; }
         p = { vbo->gpu_va, stride, offset };
     uint8_t op = UIOX_GPU_CMD_BIND_VBO;
     cmd_write(cmd, &op, 1);
     return cmd_write(cmd, &p, sizeof(p));
 }
 
 int uiox_gpu_cmd_bind_ibo(uiox_gpu_cmd_t *cmd,
                            const uiox_gpu_buf_t *ibo,
                            bool use_16bit_indices)
 {
     if (!cmd || !cmd->recording || !ibo) return -EINVAL;
     struct { uint64_t gpu_va; uint8_t idx16; }
         p = { ibo->gpu_va, use_16bit_indices ? 1u : 0u };
     uint8_t op = UIOX_GPU_CMD_BIND_IBO;
     cmd_write(cmd, &op, 1);
     return cmd_write(cmd, &p, sizeof(p));
 }
 
 int uiox_gpu_cmd_bind_ubo(uiox_gpu_cmd_t *cmd,
                            const uiox_gpu_buf_t *ubo,
                            uint8_t binding, uint32_t offset)
 {
     if (!cmd || !cmd->recording || !ubo) return -EINVAL;
     struct { uint64_t gpu_va; uint32_t size; uint32_t offset; uint8_t binding; }
         p = { ubo->gpu_va, ubo->used, offset, binding };
     uint8_t op = UIOX_GPU_CMD_BIND_UBO;
     cmd_write(cmd, &op, 1);
     return cmd_write(cmd, &p, sizeof(p));
 }
 
 int uiox_gpu_cmd_bind_texture(uiox_gpu_cmd_t *cmd,
                                const uiox_gpu_texture_t *tex,
                                uint8_t unit)
 {
     if (!cmd || !cmd->recording || !tex) return -EINVAL;
     struct { uint64_t gpu_va; uint16_t w; uint16_t h;
              uint8_t mips; uint8_t fmt; uint8_t unit; }
         p = { tex->gpu_va, tex->width, tex->height,
               tex->mip_levels, (uint8_t)tex->fmt, unit };
     uint8_t op = UIOX_GPU_CMD_BIND_TEXTURE;
     cmd_write(cmd, &op, 1);
     return cmd_write(cmd, &p, sizeof(p));
 }
 
 int uiox_gpu_cmd_bind_fbo(uiox_gpu_cmd_t *cmd, const uiox_gpu_fbo_t *fbo)
 {
     if (!cmd || !cmd->recording || !fbo) return -EINVAL;
     struct { uint16_t w; uint16_t h; uint8_t msaa; uint8_t num_colour; }
         p = { fbo->width, fbo->height, fbo->msaa_samples, fbo->num_colour };
     uint8_t op = UIOX_GPU_CMD_BIND_FBO;
     cmd_write(cmd, &op, 1);
     return cmd_write(cmd, &p, sizeof(p));
 }
 
 int uiox_gpu_cmd_set_viewport(uiox_gpu_cmd_t *cmd,
                                float x, float y, float w, float h,
                                float near_z, float far_z)
 {
     if (!cmd || !cmd->recording) return -EINVAL;
     struct { float x, y, w, h, near_z, far_z; }
         p = { x, y, w, h, near_z, far_z };
     uint8_t op = UIOX_GPU_CMD_SET_VIEWPORT;
     cmd_write(cmd, &op, 1);
     return cmd_write(cmd, &p, sizeof(p));
 }
 
 int uiox_gpu_cmd_set_scissor(uiox_gpu_cmd_t *cmd,
                               uint16_t x, uint16_t y,
                               uint16_t w, uint16_t h)
 {
     if (!cmd || !cmd->recording) return -EINVAL;
     struct { uint16_t x, y, w, h; } p = { x, y, w, h };
     uint8_t op = UIOX_GPU_CMD_SET_SCISSOR;
     cmd_write(cmd, &op, 1);
     return cmd_write(cmd, &p, sizeof(p));
 }
 
 int uiox_gpu_cmd_set_depth(uiox_gpu_cmd_t *cmd,
                             bool test, bool write, uint8_t func)
 {
     if (!cmd || !cmd->recording) return -EINVAL;
     struct { uint8_t test; uint8_t write; uint8_t func; }
         p = { test ? 1u : 0u, write ? 1u : 0u, func };
     uint8_t op = UIOX_GPU_CMD_SET_DEPTH;
     cmd_write(cmd, &op, 1);
     return cmd_write(cmd, &p, sizeof(p));
 }
 
 int uiox_gpu_cmd_set_blend(uiox_gpu_cmd_t *cmd, uiox_gpu_blend_t mode)
 {
     if (!cmd || !cmd->recording) return -EINVAL;
     uint8_t op = UIOX_GPU_CMD_SET_BLEND;
     uint8_t m  = (uint8_t)mode;
     cmd_write(cmd, &op, 1);
     return cmd_write(cmd, &m, 1);
 }
 
 int uiox_gpu_cmd_set_cull(uiox_gpu_cmd_t *cmd, uiox_gpu_cull_t mode)
 {
     if (!cmd || !cmd->recording) return -EINVAL;
     uint8_t op = UIOX_GPU_CMD_SET_CULL;
     uint8_t m  = (uint8_t)mode;
     cmd_write(cmd, &op, 1);
     return cmd_write(cmd, &m, 1);
 }
 
 int uiox_gpu_cmd_clear(uiox_gpu_cmd_t *cmd,
                         float r, float g, float b, float a,
                         float depth, bool colour, bool do_depth)
 {
     if (!cmd || !cmd->recording) return -EINVAL;
     struct { float r,g,b,a,depth; uint8_t colour; uint8_t do_depth; }
         p = { r, g, b, a, depth,
               colour ? 1u : 0u, do_depth ? 1u : 0u };
     uint8_t op = UIOX_GPU_CMD_CLEAR;
     cmd_write(cmd, &op, 1);
     return cmd_write(cmd, &p, sizeof(p));
 }
 
 int uiox_gpu_cmd_push_constants(uiox_gpu_cmd_t *cmd,
                                  const void *data, uint16_t size,
                                  uint32_t stage_flags)
 {
     if (!cmd || !cmd->recording || !data || size > 256) return -EINVAL;
     struct { uint16_t size; uint32_t stages; } hdr = { size, stage_flags };
     uint8_t op = UIOX_GPU_CMD_PUSH_CONSTANTS;
     cmd_write(cmd, &op, 1);
     cmd_write(cmd, &hdr, sizeof(hdr));
     return cmd_write(cmd, data, size);
 }
 
 int uiox_gpu_cmd_draw(uiox_gpu_cmd_t *cmd,
                        uiox_gpu_topology_t topo,
                        uint32_t first_vertex, uint32_t count,
                        uint32_t instances)
 {
     if (!cmd || !cmd->recording) return -EINVAL;
     struct { uint8_t topo; uint32_t first; uint32_t count; uint32_t inst; }
         p = { (uint8_t)topo, first_vertex, count, instances };
     uint8_t op = UIOX_GPU_CMD_DRAW;
     cmd_write(cmd, &op, 1);
     return cmd_write(cmd, &p, sizeof(p));
 }
 
 int uiox_gpu_cmd_draw_indexed(uiox_gpu_cmd_t *cmd,
                                uiox_gpu_topology_t topo,
                                uint32_t index_count,
                                uint32_t first_index,
                                int32_t  vertex_offset,
                                uint32_t instances)
 {
     if (!cmd || !cmd->recording) return -EINVAL;
     struct { uint8_t topo; uint32_t idx_cnt; uint32_t first_idx;
              int32_t vtx_off; uint32_t inst; }
         p = { (uint8_t)topo, index_count, first_index,
               vertex_offset, instances };
     uint8_t op = UIOX_GPU_CMD_DRAW_INDEXED;
     cmd_write(cmd, &op, 1);
     return cmd_write(cmd, &p, sizeof(p));
 }
 
 int uiox_gpu_cmd_dispatch(uiox_gpu_cmd_t *cmd,
                            uint32_t gx, uint32_t gy, uint32_t gz)
 {
     if (!cmd || !cmd->recording) return -EINVAL;
     struct { uint32_t gx, gy, gz; } p = { gx, gy, gz };
     uint8_t op = UIOX_GPU_CMD_DISPATCH;
     cmd_write(cmd, &op, 1);
     return cmd_write(cmd, &p, sizeof(p));
 }
 
 int uiox_gpu_cmd_barrier(uiox_gpu_cmd_t *cmd, uint32_t flags)
 {
     if (!cmd || !cmd->recording) return -EINVAL;
     uint8_t op = UIOX_GPU_CMD_BARRIER;
     cmd_write(cmd, &op, 1);
     return cmd_write(cmd, &flags, sizeof(flags));
 }
 