/*
 * cpu_smp.c - SMP / multi-core support
 */
#include "../include/cpu_smp.h"
#include "../include/cpu_power.h"
#include "../include/cpu_regs.h"
#include <string.h>
#include <stdio.h>

cpu_core_info_t    g_cores[CPU_MAX_CORES];
volatile cpu_u32_t g_online_cores = 1;

int cpu_smp_init(void)
{
    memset(g_cores, 0, sizeof(g_cores));
    g_cores[0].id      = 0;
    g_cores[0].online  = CPU_TRUE;
    g_cores[0].primary = CPU_TRUE;
#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t mpidr;
    CPU_MRS(MPIDR_EL1, mpidr);
    g_cores[0].phys_id = (cpu_u32_t)(mpidr & 0xFF);
#elif defined(UIOX_ARCH_X86_64)
    cpu_u32_t eax, ebx, ecx, edx;
    cpu_cpuid(1, &eax, &ebx, &ecx, &edx);
    g_cores[0].phys_id = (ebx >> 24) & 0xFF;
#elif defined(UIOX_ARCH_RISCV64)
    cpu_u64_t hartid;
    CPU_CSR_READ(mhartid, hartid);
    g_cores[0].phys_id = (cpu_u32_t)hartid;
#endif
    return CPU_OK;
}

cpu_u32_t cpu_smp_core_id(void)
{
#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t mpidr;
    CPU_MRS(MPIDR_EL1, mpidr);
    return (cpu_u32_t)(mpidr & 0xFF);
#elif defined(UIOX_ARCH_X86_64)
    cpu_u32_t eax, ebx, ecx, edx;
    cpu_cpuid(1, &eax, &ebx, &ecx, &edx);
    return (ebx >> 24) & 0xFF;
#elif defined(UIOX_ARCH_RISCV64)
    cpu_u64_t v; CPU_CSR_READ(mhartid, v); return (cpu_u32_t)v;
#else
    return 0;
#endif
}

cpu_u32_t cpu_smp_phys_id(void)  { return cpu_smp_core_id(); }
cpu_u32_t cpu_smp_num_cores(void){ return g_online_cores; }

int cpu_smp_boot_core(cpu_u32_t core_id,
                       cpu_addr_t entry, cpu_addr_t stack)
{
    if (core_id >= CPU_MAX_CORES) return CPU_ERR;
    g_cores[core_id].stack_top = stack;
    int rc = cpu_core_on(core_id, entry, stack);
    if (rc == CPU_OK) {
        g_cores[core_id].id     = core_id;
        g_cores[core_id].online = CPU_TRUE;
        __atomic_fetch_add(&g_online_cores, 1, __ATOMIC_SEQ_CST);
    }
    return rc;
}

void cpu_smp_send_ipi(cpu_u32_t core_mask, cpu_u32_t ipi_id)
{
#if defined(UIOX_ARCH_ARM64)
    /* GIC SGI */
    cpu_u64_t sgir = ((cpu_u64_t)(core_mask & 0xFF) << 16)
                   | (ipi_id & 0xF);
    CPU_MSR(ICC_SGI1R_EL1, sgir);
    cpu_isb();
#elif defined(UIOX_ARCH_X86_64)
    (void)core_mask; (void)ipi_id;
    /* LAPIC ICR write — handled in apic driver */
#elif defined(UIOX_ARCH_RISCV64)
    /* SBI IPI */
    __asm__ volatile(
        "li a7, 0x735049\n\t"  /* SBI_EXT_IPI */
        "li a6, 0\n\t"
        "mv a0, %0\n\t"
        "li a1, 0\n\t"
        "ecall\n\t"
        :: "r"((cpu_u64_t)core_mask)
        : "a0","a1","a6","a7","memory");
#endif
    (void)core_mask; (void)ipi_id;
}

void cpu_smp_broadcast_ipi(cpu_u32_t ipi_id)
{
    cpu_smp_send_ipi(0xFFFFFFFF, ipi_id);
}

void cpu_smp_barrier(void)
{
    static volatile cpu_u32_t barrier_cnt = 0;
    __atomic_fetch_add(&barrier_cnt, 1, __ATOMIC_SEQ_CST);
    while (barrier_cnt < g_online_cores) cpu_nop();
}

void cpu_spin_lock(cpu_spinlock_t *lock)
{
    while (__atomic_test_and_set(lock, __ATOMIC_ACQUIRE))
        cpu_nop();
}

void cpu_spin_unlock(cpu_spinlock_t *lock)
{
    __atomic_clear(lock, __ATOMIC_RELEASE);
}

int cpu_spin_trylock(cpu_spinlock_t *lock)
{
    return !__atomic_test_and_set(lock, __ATOMIC_ACQUIRE);
}

void cpu_smp_print_info(void)
{
    printf("[smp] Online cores: %u\n", g_online_cores);
    for (cpu_u32_t i = 0; i < CPU_MAX_CORES; i++) {
        if (!g_cores[i].online) continue;
        printf("  Core[%u]  phys_id=%u  %s\n",
               g_cores[i].id,
               g_cores[i].phys_id,
               g_cores[i].primary ? "(primary)" : "");
    }
}
