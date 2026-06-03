/**
 * @file    uiox_gpu_if.c
 * @brief   UIOX GPU interface driver implementation.
 * @date    2026-06-01
 */

 #include "uiox_gpu_if.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_gpu_if_init(uiox_gpu_if_t *gif,
                       uiox_gpu_hw_t *hw,
                       uiox_gpu_mem_t *mem)
 {
     if (!gif || !hw || !mem) return -EINVAL;
     memset(gif, 0, sizeof(*gif));
     gif->hw         = hw;
     gif->mem        = mem;
     gif->next_fence = 1u;
     gif->primed     = true;
     return 0;
 }
 
 uint32_t uiox_gpu_if_submit(uiox_gpu_if_t    *gif,
                               uiox_gpu_buf_t   *cmd_buf,
                               uiox_gpu_priority_t prio,
                               void (*on_complete)(uint32_t, void *),
                               void *ctx)
 {
     if (!gif || !cmd_buf || gif->q_count >= UIOX_GPU_SUBMIT_QUEUE_DEPTH)
         return 0;
 
     uint32_t fence = gif->next_fence++;
     uiox_gpu_submit_t *s = &gif->queue[gif->q_tail % UIOX_GPU_SUBMIT_QUEUE_DEPTH];
     s->cmd_buf     = cmd_buf;
     s->fence_val   = fence;
     s->priority    = prio;
     s->on_complete = on_complete;
     s->ctx         = ctx;
     gif->q_tail++;
     gif->q_count++;
 
     gif->stats.jobs_submitted++;
     gif->stats.bytes_submitted += cmd_buf->used;
 
     /* Immediately flush to hardware */
     uiox_gpu_if_flush(gif);
     return fence;
 }
 
 int uiox_gpu_if_flush(uiox_gpu_if_t *gif)
 {
     if (!gif || !gif->hw) return -EINVAL;
     while (gif->q_count > 0) {
         uiox_gpu_submit_t *s =
             &gif->queue[gif->q_head % UIOX_GPU_SUBMIT_QUEUE_DEPTH];
         int rc = uiox_gpu_hw_cmd_submit(gif->hw,
                                          (uintptr_t)s->cmd_buf->paddr,
                                          s->cmd_buf->used,
                                          s->fence_val);
         if (rc < 0) return rc;
         gif->q_head++;
         gif->q_count--;
     }
     return 0;
 }
 
 int uiox_gpu_if_sync(uiox_gpu_if_t *gif,
                       uint32_t fence_val, uint32_t timeout_ms)
 {
     if (!gif) return -EINVAL;
     gif->stats.fence_waits++;
     int rc = uiox_gpu_hw_fence_wait(gif->hw, fence_val, timeout_ms);
     if (rc == 0) gif->stats.jobs_completed++;
     return rc;
 }
 
 void uiox_gpu_if_stats_get(const uiox_gpu_if_t *gif,
                              uiox_gpu_if_stats_t *out)
 {
     if (!gif || !out) return;
     memcpy(out, &gif->stats, sizeof(*out));
 }
 
 void uiox_gpu_if_stats_reset(uiox_gpu_if_t *gif)
 {
     if (!gif) return;
     memset(&gif->stats, 0, sizeof(gif->stats));
 }
 