/**
 * @file    uiox_gpu_mem.h
 * @brief   UIOX GPU VRAM memory manager.
 *
 * Manages allocation of VRAM for GPU resources:
 *   - Buddy-system allocator for VRAM
 *   - MMU page-table management
 *   - Buffer suballocation from larger VRAM heaps
 *   - Memory statistics
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_GPU_MEM_H
 #define UIOX_GPU_MEM_H
 
 #include "uiox_gpu_hw.h"
 #include <stdint.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_GPU_MEM_MIN_ALLOC   (4u * 1024u)      /**< 4 KB minimum      */
 #define UIOX_GPU_MEM_MAX_BLOCKS  256
 
 typedef struct {
     uint64_t  gpu_va;
     uint64_t  phys;
     uint64_t  size;
     bool      free;
 } uiox_gpu_mem_block_t;
 
 typedef struct {
     uiox_gpu_hw_t        *hw;
     uint64_t              heap_base_phys;
     uint64_t              heap_base_va;
     uint64_t              heap_size;
     uiox_gpu_mem_block_t  blocks[UIOX_GPU_MEM_MAX_BLOCKS];
     uint32_t              num_blocks;
     uint64_t              bytes_used;
     uint64_t              bytes_free;
 } uiox_gpu_mem_t;
 
 int      uiox_gpu_mem_init   (uiox_gpu_mem_t *mem, uiox_gpu_hw_t *hw,
                                uint64_t heap_phys, uint64_t heap_size);
 int      uiox_gpu_mem_alloc  (uiox_gpu_mem_t *mem, uint64_t size,
                                uint32_t align,
                                uint64_t *gpu_va_out, uint64_t *phys_out);
 void     uiox_gpu_mem_free   (uiox_gpu_mem_t *mem, uint64_t gpu_va);
 void     uiox_gpu_mem_stats  (const uiox_gpu_mem_t *mem,
                                uint64_t *used, uint64_t *free_bytes);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_GPU_MEM_H */
 