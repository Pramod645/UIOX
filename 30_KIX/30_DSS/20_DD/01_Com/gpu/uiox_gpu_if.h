/**
 * @file    uiox_gpu_if.h
 * @brief   UIOX GPU interface driver (submit, fence, sync).
 *
 * Manages:
 *   - Command buffer submission queue
 *   - Fence signalling and wait
 *   - GPU/CPU synchronisation primitives
 *   - Job priority scheduling (high/normal/low)
 *   - Interface statistics
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_GPU_IF_H
 #define UIOX_GPU_IF_H
 
 #include "uiox_gpu_hw.h"
 #include "uiox_gpu_buf.h"
 #include "uiox_gpu_mem.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_GPU_SUBMIT_QUEUE_DEPTH   16
 
 typedef enum {
     UIOX_GPU_PRIO_HIGH   = 0,
     UIOX_GPU_PRIO_NORMAL = 1,
     UIOX_GPU_PRIO_LOW    = 2,
 } uiox_gpu_priority_t;
 
 typedef struct {
     uiox_gpu_buf_t     *cmd_buf;
     uint32_t            fence_val;
     uiox_gpu_priority_t priority;
     void (*on_complete)(uint32_t fence_val, void *ctx);
     void               *ctx;
 } uiox_gpu_submit_t;
 
 typedef struct {
     uint64_t  jobs_submitted;
     uint64_t  jobs_completed;
     uint64_t  fence_waits;
     uint64_t  gpu_faults;
     uint64_t  bytes_submitted;
 } uiox_gpu_if_stats_t;
 
 typedef struct {
     uiox_gpu_hw_t      *hw;
     uiox_gpu_mem_t     *mem;
     uiox_gpu_submit_t   queue[UIOX_GPU_SUBMIT_QUEUE_DEPTH];
     uint8_t             q_head, q_tail, q_count;
     uint32_t            next_fence;
     uiox_gpu_if_stats_t stats;
     bool                primed;
 } uiox_gpu_if_t;
 
 int  uiox_gpu_if_init    (uiox_gpu_if_t *gif,
                            uiox_gpu_hw_t *hw,
                            uiox_gpu_mem_t *mem);
 
 uint32_t uiox_gpu_if_submit(uiox_gpu_if_t    *gif,
                              uiox_gpu_buf_t   *cmd_buf,
                              uiox_gpu_priority_t prio,
                              void (*on_complete)(uint32_t, void *),
                              void *ctx);
 
 int  uiox_gpu_if_flush   (uiox_gpu_if_t *gif);
 int  uiox_gpu_if_sync    (uiox_gpu_if_t *gif, uint32_t fence_val,
                            uint32_t timeout_ms);
 
 void uiox_gpu_if_stats_get  (const uiox_gpu_if_t *gif,
                               uiox_gpu_if_stats_t *out);
 void uiox_gpu_if_stats_reset(uiox_gpu_if_t *gif);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_GPU_IF_H */
 