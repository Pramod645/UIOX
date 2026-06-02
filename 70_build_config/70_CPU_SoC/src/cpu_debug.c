/*
 * cpu_debug.c - CPU debug / breakpoint / watchpoint
 */
#include "../include/cpu_debug.h"
#include "../include/cpu_regs.h"
#include <string.h>
#include <stdio.h>

static cpu_debug_handler_t g_debug_handler = NULL;
static cpu_breakpoint_t    g_bps[CPU_MAX_BREAKPOINTS];
static cpu_breakpoint_t    g_wps[CPU_MAX_WATCHPOINTS];

int cpu_debug_init(void)
{
    memset(g_bps, 0, sizeof(g_bps));
    memset(g_wps, 0, sizeof(g_wps));
#if defined(UIOX_ARCH_ARM64)
    /* Enable debug monitor in MDSCR_EL1 */
    cpu_u64_t mdscr;
    CPU_MRS(MDSCR_EL1, mdscr);
    mdscr |= (1u << 15); /* MDE — monitor debug enable */
    CPU_MSR(MDSCR_EL1, mdscr);
    cpu_isb();
#elif defined(UIOX_ARCH_X86_64)
    /* Clear DR6 / DR7 */
    __asm__ volatile(
        "xorq %%rax, %%rax\n\t"
        "movq %%rax, %%dr6\n\t"
        "movq %%rax, %%dr7\n\t" ::: "rax");
#elif defined(UIOX_ARCH_RISCV64)
    /* Enable debug mode via dcsr if in debug ROM */
    (void)0;
#endif
    return CPU_OK;
}

int cpu_debug_set_bp(cpu_u32_t slot, cpu_addr_t addr)
{
    if (slot >= CPU_MAX_BREAKPOINTS) return CPU_ERR;
#if defined(UIOX_ARCH_ARM64)
    /* Write DBGBVR<n>_EL1 and DBGBCR<n>_EL1 */
    switch (slot) {
        case 0: CPU_MSR(DBGBVR0_EL1, addr);
                CPU_MSR(DBGBCR0_EL1, 0x000001E5ULL); break; /* E=1,PMC=11,BAS=1111 */
        case 1: CPU_MSR(DBGBVR1_EL1, addr);
                CPU_MSR(DBGBCR1_EL1, 0x000001E5ULL); break;
        case 2: CPU_MSR(DBGBVR2_EL1, addr);
                CPU_MSR(DBGBCR2_EL1, 0x000001E5ULL); break;
        case 3: CPU_MSR(DBGBVR3_EL1, addr);
                CPU_MSR(DBGBCR3_EL1, 0x000001E5ULL); break;
        default: return CPU_ENOSUP;
    }
    cpu_isb();
#elif defined(UIOX_ARCH_X86_64)
    if (slot >= 4) return CPU_ERR;
    switch (slot) {
        case 0: __asm__ volatile("movq %0, %%dr0" :: "r"(addr)); break;
        case 1: __asm__ volatile("movq %0, %%dr1" :: "r"(addr)); break;
        case 2: __asm__ volatile("movq %0, %%dr2" :: "r"(addr)); break;
        case 3: __asm__ volatile("movq %0, %%dr3" :: "r"(addr)); break;
    }
    /* Set DR7: local enable for slot */
    cpu_u64_t dr7;
    __asm__ volatile("movq %%dr7, %0" : "=r"(dr7));
    dr7 |= (1u << (slot * 2));
    __asm__ volatile("movq %0, %%dr7" :: "r"(dr7));
#elif defined(UIOX_ARCH_RISCV64)
    (void)addr;
    return CPU_ENOSUP;
#endif
    g_bps[slot].addr    = addr;
    g_bps[slot].type    = CPU_BP_EXEC;
    g_bps[slot].enabled = CPU_TRUE;
    return CPU_OK;
}

int cpu_debug_clear_bp(cpu_u32_t slot)
{
    if (slot >= CPU_MAX_BREAKPOINTS) return CPU_ERR;
#if defined(UIOX_ARCH_ARM64)
    switch (slot) {
        case 0: CPU_MSR(DBGBCR0_EL1, 0ULL); break;
        case 1: CPU_MSR(DBGBCR1_EL1, 0ULL); break;
        case 2: CPU_MSR(DBGBCR2_EL1, 0ULL); break;
        case 3: CPU_MSR(DBGBCR3_EL1, 0ULL); break;
        default: return CPU_ENOSUP;
    }
#elif defined(UIOX_ARCH_X86_64)
    if (slot < 4) {
        cpu_u64_t dr7;
        __asm__ volatile("movq %%dr7,%0" : "=r"(dr7));
        dr7 &= ~(3u << (slot * 2));
        __asm__ volatile("movq %0,%%dr7" :: "r"(dr7));
    }
#endif
    g_bps[slot].enabled = CPU_FALSE;
    return CPU_OK;
}

int cpu_debug_set_wp(cpu_u32_t slot, cpu_addr_t addr,
                      cpu_u32_t len, cpu_bp_type_t type)
{
    if (slot >= CPU_MAX_WATCHPOINTS) return CPU_ERR;
#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t ctrl = 0x1u; /* E=1 */
    ctrl |= ((cpu_u64_t)(type == CPU_BP_READ  ? 1u :
                          type == CPU_BP_WRITE ? 2u : 3u) << 3);
    ctrl |= ((cpu_u64_t)((1u << len) - 1u) << 5); /* BAS */
    switch (slot) {
        case 0: CPU_MSR(DBGWVR0_EL1, addr); CPU_MSR(DBGWCR0_EL1, ctrl); break;
        case 1: CPU_MSR(DBGWVR1_EL1, addr); CPU_MSR(DBGWCR1_EL1, ctrl); break;
        case 2: CPU_MSR(DBGWVR2_EL1, addr); CPU_MSR(DBGWCR2_EL1, ctrl); break;
        case 3: CPU_MSR(DBGWVR3_EL1, addr); CPU_MSR(DBGWCR3_EL1, ctrl); break;
        default: return CPU_ENOSUP;
    }
    cpu_isb();
#elif defined(UIOX_ARCH_X86_64)
    if (slot >= 4) return CPU_ERR;
    switch (slot) {
        case 0: __asm__ volatile("movq %0,%%dr0" :: "r"(addr)); break;
        case 1: __asm__ volatile("movq %0,%%dr1" :: "r"(addr)); break;
        case 2: __asm__ volatile("movq %0,%%dr2" :: "r"(addr)); break;
        case 3: __asm__ volatile("movq %0,%%dr3" :: "r"(addr)); break;
    }
    cpu_u64_t dr7;
    __asm__ volatile("movq %%dr7,%0" : "=r"(dr7));
    /* encode: LEN | R/W condition | local enable */
    cpu_u32_t cond = (type == CPU_BP_WRITE) ? 1u :
                     (type == CPU_BP_RW)    ? 3u : 2u;
    cpu_u32_t lbits = (len == 8) ? 2u : (len == 4) ? 3u :
                      (len == 2) ? 1u : 0u;
    dr7 &= ~(0xFu << (16 + slot * 4));
    dr7 |=  ((cpu_u64_t)(lbits << 2 | cond) << (16 + slot * 4));
    dr7 |=   (1u << (slot * 2));
    __asm__ volatile("movq %0,%%dr7" :: "r"(dr7));
    (void)len;
#else
    (void)addr; (void)len; (void)type;
    return CPU_ENOSUP;
#endif
    g_wps[slot].addr    = addr;
    g_wps[slot].type    = type;
    g_wps[slot].enabled = CPU_TRUE;
    return CPU_OK;
}

int cpu_debug_clear_wp(cpu_u32_t slot)
{
    if (slot >= CPU_MAX_WATCHPOINTS) return CPU_ERR;
    g_wps[slot].enabled = CPU_FALSE;
    return CPU_OK;
}

void cpu_debug_enable(void)
{
#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t mdscr;
    CPU_MRS(MDSCR_EL1, mdscr);
    mdscr |= (1u << 15);
    CPU_MSR(MDSCR_EL1, mdscr);
    cpu_isb();
#elif defined(UIOX_ARCH_X86_64)
    cpu_u64_t dr7;
    __asm__ volatile("movq %%dr7,%0" : "=r"(dr7));
    dr7 |= 0x400u; /* GE=1 */
    __asm__ volatile("movq %0,%%dr7" :: "r"(dr7));
#endif
}

void cpu_debug_disable(void)
{
#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t mdscr;
    CPU_MRS(MDSCR_EL1, mdscr);
    mdscr &= ~(1u << 15);
    CPU_MSR(MDSCR_EL1, mdscr);
    cpu_isb();
#elif defined(UIOX_ARCH_X86_64)
    __asm__ volatile(
        "xorq %%rax,%%rax\n\t"
        "movq %%rax,%%dr7\n\t" ::: "rax");
#endif
}

void cpu_debug_register_handler(cpu_debug_handler_t h)
{ g_debug_handler = h; }

cpu_u32_t cpu_debug_num_bp(void)
{
#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t dfr0;
    CPU_MRS(ID_AA64DFR0_EL1, dfr0);
    return (cpu_u32_t)((dfr0 >> 12) & 0xFu) + 1u;
#elif defined(UIOX_ARCH_X86_64)
    return 4u;
#else
    return 0u;
#endif
}

cpu_u32_t cpu_debug_num_wp(void)
{
#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t dfr0;
    CPU_MRS(ID_AA64DFR0_EL1, dfr0);
    return (cpu_u32_t)((dfr0 >> 20) & 0xFu) + 1u;
#elif defined(UIOX_ARCH_X86_64)
    return 4u;
#else
    return 0u;
#endif
}

void cpu_debug_print_state(void)
{
    printf("[debug] Breakpoints (%u HW slots):\n", cpu_debug_num_bp());
    for (cpu_u32_t i = 0; i < CPU_MAX_BREAKPOINTS; i++)
        if (g_bps[i].enabled)
            printf("  BP[%u] addr=0x%016llx\n",
                   i, (unsigned long long)g_bps[i].addr);
    printf("[debug] Watchpoints (%u HW slots):\n", cpu_debug_num_wp());
    for (cpu_u32_t i = 0; i < CPU_MAX_WATCHPOINTS; i++)
        if (g_wps[i].enabled)
            printf("  WP[%u] addr=0x%016llx type=%d\n",
                   i, (unsigned long long)g_wps[i].addr,
                   g_wps[i].type);
}
