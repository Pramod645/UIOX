/*
 * cpu_cache.c - Cache management
 */
#include "../include/cpu_cache.h"
#include "../include/cpu_regs.h"
#include <stdio.h>

#if defined(UIOX_ARCH_ARM64)

void cpu_cache_flush_all(void)
{
    /* clean + invalidate all D-cache levels by set/way */
    cpu_u64_t clidr;
    CPU_MRS(CLIDR_EL1, clidr);
    cpu_u32_t loc = (cpu_u32_t)((clidr >> 24) & 0x7u);
    for (cpu_u32_t level = 0; level < loc; level++) {
        cpu_u32_t ctype = (cpu_u32_t)((clidr >> (level * 3)) & 0x7u);
        if (ctype < 2) continue;  /* no D-cache at this level */
        cpu_u64_t csselr = (cpu_u64_t)(level << 1);
        CPU_MSR(CSSELR_EL1, csselr);
        cpu_isb();
        cpu_u64_t ccsidr;
        CPU_MRS(CCSIDR_EL1, ccsidr);
        cpu_u32_t sets  = (cpu_u32_t)((ccsidr >> 13) & 0x7FFFu) + 1;
        cpu_u32_t ways  = (cpu_u32_t)((ccsidr >>  3) & 0x3FFu)  + 1;
        cpu_u32_t lsize = (cpu_u32_t)(ccsidr & 0x7u) + 4;
        for (cpu_u32_t way = 0; way < ways; way++) {
            for (cpu_u32_t set = 0; set < sets; set++) {
                cpu_u64_t val = ((cpu_u64_t)way << (32u - __builtin_clz(ways - 1)))
                              | ((cpu_u64_t)set << lsize)
                              | ((cpu_u64_t)level << 1);
                __asm__ volatile("dc cisw, %0" :: "r"(val) : "memory");
            }
        }
    }
    cpu_dsb();
    cpu_isb();
}

void cpu_cache_clean_all(void)
{
    __asm__ volatile("" ::: "memory");
    cpu_dsb();
}

void cpu_cache_invalidate_all(void)
{
    __asm__ volatile("" ::: "memory");
    cpu_isb();
}

void cpu_icache_invalidate(cpu_addr_t start, cpu_u64_t size)
{
    cpu_addr_t end = start + size;
    cpu_addr_t a   = start & ~((cpu_addr_t)63);
    while (a < end) {
        __asm__ volatile("ic ivau, %0" :: "r"(a) : "memory");
        a += 64;
    }
    cpu_dsb(); cpu_isb();
}

void cpu_dcache_clean(cpu_addr_t start, cpu_u64_t size)
{
    cpu_addr_t end = start + size;
    cpu_addr_t a   = start & ~((cpu_addr_t)63);
    while (a < end) {
        __asm__ volatile("dc cvac, %0" :: "r"(a) : "memory");
        a += 64;
    }
    cpu_dsb();
}

void cpu_dcache_invalidate(cpu_addr_t start, cpu_u64_t size)
{
    cpu_addr_t end = start + size;
    cpu_addr_t a   = start & ~((cpu_addr_t)63);
    while (a < end) {
        __asm__ volatile("dc ivac, %0" :: "r"(a) : "memory");
        a += 64;
    }
    cpu_dsb();
}

void cpu_dcache_flush(cpu_addr_t start, cpu_u64_t size)
{
    cpu_addr_t end = start + size;
    cpu_addr_t a   = start & ~((cpu_addr_t)63);
    while (a < end) {
        __asm__ volatile("dc civac, %0" :: "r"(a) : "memory");
        a += 64;
    }
    cpu_dsb();
}

#elif defined(UIOX_ARCH_X86_64)

void cpu_cache_flush_all(void)
{ __asm__ volatile("wbinvd\n\t" ::: "memory"); }

void cpu_cache_clean_all(void)
{ __asm__ volatile("mfence\n\t" ::: "memory"); }

void cpu_cache_invalidate_all(void)
{ __asm__ volatile("invd\n\t" ::: "memory"); }

void cpu_icache_invalidate(cpu_addr_t start, cpu_u64_t size)
{ (void)start; (void)size; /* x86 coherent I/D cache */ }

void cpu_dcache_clean(cpu_addr_t start, cpu_u64_t size)
{
    cpu_addr_t a = start & ~((cpu_addr_t)63);
    cpu_addr_t e = start + size;
    while (a < e) {
        __asm__ volatile("clflushopt (%0)" :: "r"((void*)a) : "memory");
        a += 64;
    }
    __asm__ volatile("mfence\n\t" ::: "memory");
}

void cpu_dcache_invalidate(cpu_addr_t s, cpu_u64_t sz)
{ cpu_dcache_clean(s, sz); }

void cpu_dcache_flush(cpu_addr_t s, cpu_u64_t sz)
{ cpu_dcache_clean(s, sz); }

#elif defined(UIOX_ARCH_RISCV64)

void cpu_cache_flush_all(void)
{ __asm__ volatile("fence\n\tfence.i\n\t" ::: "memory"); }

void cpu_cache_clean_all(void)
{ __asm__ volatile("fence\n\t" ::: "memory"); }

void cpu_cache_invalidate_all(void)
{ __asm__ volatile("fence.i\n\t" ::: "memory"); }

void cpu_icache_invalidate(cpu_addr_t s, cpu_u64_t sz)
{ (void)s; (void)sz; __asm__ volatile("fence.i\n\t" ::: "memory"); }

void cpu_dcache_clean(cpu_addr_t s, cpu_u64_t sz)
{ (void)s; (void)sz; __asm__ volatile("fence\n\t" ::: "memory"); }

void cpu_dcache_invalidate(cpu_addr_t s, cpu_u64_t sz)
{ (void)s; (void)sz; __asm__ volatile("fence\n\t" ::: "memory"); }

void cpu_dcache_flush(cpu_addr_t s, cpu_u64_t sz)
{ (void)s; (void)sz; __asm__ volatile("fence\n\tfence.i\n\t" ::: "memory"); }
#endif

void cpu_cache_sync_for_dma (cpu_addr_t s, cpu_u64_t sz) { cpu_dcache_clean(s, sz); }
void cpu_cache_sync_after_dma(cpu_addr_t s, cpu_u64_t sz){ cpu_dcache_invalidate(s, sz); }

int cpu_cache_probe(cpu_cache_info_t *info)
{
    if (!info) return CPU_ERR;
    info->l1i_size_kb = g_cpu_id.l1i_size_kb;
    info->l1d_size_kb = g_cpu_id.l1d_size_kb;
    info->l2_size_kb  = g_cpu_id.l2_size_kb;
    info->l3_size_kb  = g_cpu_id.l3_size_kb;
    info->l1i_line    = g_cpu_id.cache_line;
    info->l1d_line    = g_cpu_id.cache_line;
    info->l2_line     = g_cpu_id.cache_line;
    info->l3_line     = g_cpu_id.cache_line;
    info->l1i_ways    = 4; info->l1d_ways = 4;
    info->l2_ways     = 8; info->l3_ways  = 16;
    return CPU_OK;
}

void cpu_cache_print(const cpu_cache_info_t *info)
{
    printf("[cache] L1i=%uKB/%u-way/%uB  L1d=%uKB/%u-way/%uB"
           "  L2=%uKB/%u-way  L3=%uKB/%u-way\n",
           info->l1i_size_kb, info->l1i_ways, info->l1i_line,
           info->l1d_size_kb, info->l1d_ways, info->l1d_line,
           info->l2_size_kb,  info->l2_ways,
           info->l3_size_kb,  info->l3_ways);
}
