/**
 * @file    uiox_gpu_mem.c
 * @brief   UIOX GPU VRAM memory manager implementation.
 * @date    2026-06-01
 */

 #include "uiox_gpu_mem.h"
 
 int uiox_gpu_mem_init(uiox_gpu_mem_t *mem, uiox_gpu_hw_t *hw,
                        uint64_t heap_phys, uint64_t heap_size)
 {
     if (!mem || !hw || !heap_size) return -EINVAL;
     memset(mem, 0, sizeof(*mem));
     mem->hw              = hw;
     mem->heap_base_phys  = heap_phys;
     mem->heap_base_va    = heap_phys;  /* identity for non-MMU */
     mem->heap_size       = heap_size;
     mem->bytes_free      = heap_size;
 
     /* One initial free block covering the whole heap */
     mem->blocks[0].gpu_va = heap_phys;
     mem->blocks[0].phys   = heap_phys;
     mem->blocks[0].size   = heap_size;
     mem->blocks[0].free   = true;
     mem->num_blocks       = 1;
     return 0;
 }
 
 int uiox_gpu_mem_alloc(uiox_gpu_mem_t *mem, uint64_t size,
                         uint32_t align,
                         uint64_t *gpu_va_out, uint64_t *phys_out)
 {
     if (!mem || !size || !gpu_va_out || !phys_out) return -EINVAL;
     if (align < UIOX_GPU_MEM_MIN_ALLOC) align = UIOX_GPU_MEM_MIN_ALLOC;
 
     /* Round up size to alignment */
     size = (size + align - 1u) & ~((uint64_t)align - 1u);
 
     for (uint32_t i = 0; i < mem->num_blocks; i++) {
         uiox_gpu_mem_block_t *b = &mem->blocks[i];
         if (!b->free || b->size < size) continue;
 
         /* Align start address */
         uint64_t aligned_start = (b->gpu_va + align - 1u) &
                                  ~((uint64_t)align - 1u);
         uint64_t padding = aligned_start - b->gpu_va;
         if (b->size < size + padding) continue;
 
         /* Split block if needed */
         if (b->size > size + padding && mem->num_blocks < UIOX_GPU_MEM_MAX_BLOCKS) {
             uiox_gpu_mem_block_t *rem = &mem->blocks[mem->num_blocks++];
             rem->gpu_va = aligned_start + size;
             rem->phys   = b->phys + padding + size;
             rem->size   = b->size - padding - size;
             rem->free   = true;
         }
 
         b->gpu_va     = aligned_start;
         b->phys       = b->phys + padding;
         b->size       = size;
         b->free       = false;
         mem->bytes_used += size;
         mem->bytes_free -= size;
 
         /* Map into GPU MMU */
         const uiox_gpu_hw_ops_t *ops =
             (const uiox_gpu_hw_ops_t *)mem->hw->priv;
         if (ops && ops->mmu_map)
             ops->mmu_map(mem->hw, b->gpu_va, b->phys, size, 0x3u);
 
         *gpu_va_out = b->gpu_va;
         *phys_out   = b->phys;
         return 0;
     }
     return -ENOMEM;
 }
 
 void uiox_gpu_mem_free(uiox_gpu_mem_t *mem, uint64_t gpu_va)
 {
     if (!mem) return;
     for (uint32_t i = 0; i < mem->num_blocks; i++) {
         uiox_gpu_mem_block_t *b = &mem->blocks[i];
         if (b->free || b->gpu_va != gpu_va) continue;
 
         /* Unmap from GPU MMU */
         const uiox_gpu_hw_ops_t *ops =
             (const uiox_gpu_hw_ops_t *)mem->hw->priv;
         if (ops && ops->mmu_unmap)
             ops->mmu_unmap(mem->hw, b->gpu_va, b->size);
 
         mem->bytes_used -= b->size;
         mem->bytes_free += b->size;
         b->free          = true;
 
         /* Merge with adjacent free blocks */
         for (uint32_t j = 0; j < mem->num_blocks; j++) {
             if (j == i) continue;
             uiox_gpu_mem_block_t *n = &mem->blocks[j];
             if (!n->free) continue;
             if (n->gpu_va + n->size == b->gpu_va) {
                 n->size += b->size; b->size = 0; break;
             }
             if (b->gpu_va + b->size == n->gpu_va) {
                 b->size += n->size; n->size = 0; break;
             }
         }
         return;
     }
 }
 
 void uiox_gpu_mem_stats(const uiox_gpu_mem_t *mem,
                          uint64_t *used, uint64_t *free_bytes)
 {
     if (!mem) return;
     if (used)       *used       = mem->bytes_used;
     if (free_bytes) *free_bytes = mem->bytes_free;
 }
 