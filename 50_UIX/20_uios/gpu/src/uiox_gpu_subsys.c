/**
 * @file    uiox_gpu_subsys.c
 * @brief   UIOX GPU subsystem implementation.
 * @date    2026-06-01
 */

 #include "uiox_gpu_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_gpu_subsys_init(uiox_gpu_subsys_t *sys,
                           uiox_gpu_hw_t *hw,
                           uint64_t vram_phys, uint64_t vram_size)
 {
     if (!sys || !hw) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
 
     uiox_gpu_buf_pool_init();
 
     int rc = uiox_gpu_mem_init(&sys->mem, hw, vram_phys, vram_size);
     if (rc < 0) return rc;
 
     rc = uiox_gpu_if_init(&sys->gif, hw, &sys->mem);
     if (rc < 0) return rc;
 
     sys->cmd_buf = uiox_gpu_buf_alloc(UIOX_GPU_BUF_CMD);
     if (!sys->cmd_buf) return -ENOMEM;
 
     sys->state    = UIOX_GPU_SUBSYS_IDLE;
     sys->frame_id = 0;
     return 0;
 }
 
 void uiox_gpu_subsys_deinit(uiox_gpu_subsys_t *sys)
 {
     if (!sys) return;
     if (sys->cmd_buf) uiox_gpu_buf_free(sys->cmd_buf);
     sys->state = UIOX_GPU_SUBSYS_STOPPED;
 }
 
 int uiox_gpu_subsys_create_pso(uiox_gpu_subsys_t *sys,
                                 const uiox_gpu_pso_t *desc)
 {
     if (!sys || !desc) return -EINVAL;
     if (sys->pso_count >= UIOX_GPU_MAX_PSO) return -ENOSPC;
     uint8_t id = sys->pso_count++;
     memcpy(&sys->pso_table[id], desc, sizeof(*desc));
     sys->pso_table[id].id    = id;
     sys->pso_table[id].valid = true;
     return (int)id;
 }
 
 void uiox_gpu_subsys_destroy_pso(uiox_gpu_subsys_t *sys, uint8_t pso_id)
 {
     if (!sys || pso_id >= sys->pso_count) return;
     sys->pso_table[pso_id].valid = false;
 }
 
 int uiox_gpu_subsys_begin_frame(uiox_gpu_subsys_t *sys)
 {
     if (!sys || sys->state == UIOX_GPU_SUBSYS_RECORDING) return -EINVAL;
     sys->frame_id++;
     memset(&sys->frame_stats, 0, sizeof(sys->frame_stats));
     sys->frame_stats.frame_id = sys->frame_id;
     int rc = uiox_gpu_cmd_begin(&sys->cmd, sys->cmd_buf);
     if (rc == 0) sys->state = UIOX_GPU_SUBSYS_RECORDING;
     return rc;
 }
 
 int uiox_gpu_subsys_begin_pass(uiox_gpu_subsys_t *sys,
                                 const uiox_gpu_renderpass_t *pass)
 {
     if (!sys || !pass || sys->state != UIOX_GPU_SUBSYS_RECORDING)
         return -EINVAL;
     if (pass->fbo)
         uiox_gpu_cmd_bind_fbo(&sys->cmd, pass->fbo);
     if (pass->clear_colour || pass->clear_depth_flag)
         uiox_gpu_cmd_clear(&sys->cmd,
                             pass->clear_r, pass->clear_g,
                             pass->clear_b, pass->clear_a,
                             pass->clear_depth,
                             pass->clear_colour,
                             pass->clear_depth_flag);
     return 0;
 }
 
 void uiox_gpu_subsys_end_pass(uiox_gpu_subsys_t *sys)
 {
     if (!sys) return;
     uiox_gpu_cmd_barrier(&sys->cmd, 0xFFFFFFFFu);
 }
 
 int uiox_gpu_subsys_bind_pso(uiox_gpu_subsys_t *sys, uint8_t pso_id)
 {
     if (!sys || pso_id >= sys->pso_count) return -EINVAL;
     const uiox_gpu_pso_t *pso = &sys->pso_table[pso_id];
     if (!pso->valid) return -EINVAL;
 
     uiox_gpu_cmd_bind_shader(&sys->cmd, pso->vert_shader);
     uiox_gpu_cmd_set_blend(&sys->cmd, pso->blend);
     uiox_gpu_cmd_set_cull(&sys->cmd, pso->cull);
     uiox_gpu_cmd_set_depth(&sys->cmd,
                             pso->depth_test, pso->depth_write,
                             pso->depth_func);
     return 0;
 }
 
 int uiox_gpu_subsys_end_frame(uiox_gpu_subsys_t *sys,
                                uint32_t wait_timeout_ms)
 {
     if (!sys || sys->state != UIOX_GPU_SUBSYS_RECORDING) return -EINVAL;
 
     int rc = uiox_gpu_cmd_end(&sys->cmd);
     if (rc < 0) return rc;
 
     sys->state = UIOX_GPU_SUBSYS_SUBMITTED;
 
     uint32_t fence = uiox_gpu_if_submit(&sys->gif, sys->cmd_buf,
                                          UIOX_GPU_PRIO_NORMAL,
                                          NULL, NULL);
     sys->last_fence = fence;
 
     rc = uiox_gpu_if_sync(&sys->gif, fence, wait_timeout_ms);
     if (rc == 0) sys->state = UIOX_GPU_SUBSYS_IDLE;
 
     uiox_gpu_cmd_reset(&sys->cmd);
     return rc;
 }
 
 void uiox_gpu_subsys_frame_stats(const uiox_gpu_subsys_t *sys,
                                   uiox_gpu_frame_stats_t *out)
 {
     if (!sys || !out) return;
     memcpy(out, &sys->frame_stats, sizeof(*out));
 }
 