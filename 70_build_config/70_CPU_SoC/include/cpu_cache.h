#ifndef CPU_CACHE_H
#define CPU_CACHE_H
/*
 * cpu_cache.h - Cache management interface (ARM/x86/RISC-V)
 */
#include "cpu_types.h"

/* -- Cache operations --------------------------------------- */
void cpu_cache_invalidate_all  (void);
void cpu_cache_clean_all       (void);
void cpu_cache_flush_all       (void);   /* clean + invalidate   */

void cpu_icache_invalidate     (cpu_addr_t start, cpu_u64_t size);
void cpu_dcache_clean          (cpu_addr_t start, cpu_u64_t size);
void cpu_dcache_invalidate     (cpu_addr_t start, cpu_u64_t size);
void cpu_dcache_flush          (cpu_addr_t start, cpu_u64_t size);

void cpu_cache_sync_for_dma    (cpu_addr_t start, cpu_u64_t size);
void cpu_cache_sync_after_dma  (cpu_addr_t start, cpu_u64_t size);

/* -- Cache info --------------------------------------------- */
typedef struct cpu_cache_info {
    cpu_u32_t l1i_size_kb;
    cpu_u32_t l1i_ways;
    cpu_u32_t l1i_line;
    cpu_u32_t l1d_size_kb;
    cpu_u32_t l1d_ways;
    cpu_u32_t l1d_line;
    cpu_u32_t l2_size_kb;
    cpu_u32_t l2_ways;
    cpu_u32_t l2_line;
    cpu_u32_t l3_size_kb;
    cpu_u32_t l3_ways;
    cpu_u32_t l3_line;
} cpu_cache_info_t;

int  cpu_cache_probe (cpu_cache_info_t *info);
void cpu_cache_print (const cpu_cache_info_t *info);

#endif /* CPU_CACHE_H */
