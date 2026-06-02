/*
 * cpu_mmu.c - MMU / TLB management
 */
#include "../include/cpu_mmu.h"
#include "../include/cpu_regs.h"
#include <stdio.h>

int cpu_mmu_enable(void)
{
#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t sctlr;
    CPU_MRS(SCTLR_EL1, sctlr);
    sctlr |= (1u << 0) | (1u << 2) | (1u << 12); /* M + C + I */
    cpu_dsb(); cpu_isb();
    CPU_MSR(SCTLR_EL1, sctlr);
    cpu_isb();
#elif defined(UIOX_ARCH_X86_64)
    cpu_u64_t cr0;
    __asm__ volatile("movq %%cr0, %0" : "=r"(cr0));
    cr0 |= (1ULL << 31); /* PG bit */
    __asm__ volatile("movq %0, %%cr0" :: "r"(cr0));
#elif defined(UIOX_ARCH_RISCV64)
    /* SATP is set in cpu_mmu_set_page_table */
    cpu_isb();
#endif
    return CPU_OK;
}

void cpu_mmu_disable(void)
{
#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t sctlr;
    CPU_MRS(SCTLR_EL1, sctlr);
    sctlr &= ~(1u << 0);
    CPU_MSR(SCTLR_EL1, sctlr);
    cpu_isb();
#endif
}

void cpu_mmu_tlb_flush_all(void)
{
#if defined(UIOX_ARCH_ARM64)
    __asm__ volatile("tlbi vmalle1is\n\t" ::: "memory");
    cpu_dsb(); cpu_isb();
#elif defined(UIOX_ARCH_X86_64)
    cpu_u64_t cr3;
    __asm__ volatile("movq %%cr3,%0" : "=r"(cr3));
    __asm__ volatile("movq %0,%%cr3" :: "r"(cr3) : "memory");
#elif defined(UIOX_ARCH_RISCV64)
    __asm__ volatile("sfence.vma\n\t" ::: "memory");
#endif
}

void cpu_mmu_tlb_flush_va(cpu_addr_t virt)
{
#if defined(UIOX_ARCH_ARM64)
    __asm__ volatile("tlbi vae1is, %0\n\t"
                     :: "r"(virt >> 12) : "memory");
    cpu_dsb(); cpu_isb();
#elif defined(UIOX_ARCH_X86_64)
    __asm__ volatile("invlpg (%0)" :: "r"((void*)virt) : "memory");
#elif defined(UIOX_ARCH_RISCV64)
    __asm__ volatile("sfence.vma %0, zero\n\t" :: "r"(virt) : "memory");
#endif
}

void cpu_mmu_set_page_table(cpu_addr_t pt_phys)
{
#if defined(UIOX_ARCH_ARM64)
    CPU_MSR(TTBR0_EL1, pt_phys);
    cpu_isb();
#elif defined(UIOX_ARCH_X86_64)
    __asm__ volatile("movq %0, %%cr3" :: "r"(pt_phys) : "memory");
#elif defined(UIOX_ARCH_RISCV64)
    /* SV39 mode: SATP = (8<<60) | (asid<<44) | (ppn) */
    cpu_u64_t satp = (8ULL << 60) | (pt_phys >> 12);
    CPU_CSR_WRITE(satp, satp);
    __asm__ volatile("sfence.vma\n\t" ::: "memory");
#endif
}

cpu_addr_t cpu_mmu_get_page_table(void)
{
#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t v; CPU_MRS(TTBR0_EL1, v); return v;
#elif defined(UIOX_ARCH_X86_64)
    cpu_u64_t v;
    __asm__ volatile("movq %%cr3,%0" : "=r"(v));
    return v & ~0xFFFULL;
#elif defined(UIOX_ARCH_RISCV64)
    cpu_u64_t v; CPU_CSR_READ(satp, v);
    return (v & 0x00000FFFFFFFFFFFULL) << 12;
#else
    return 0;
#endif
}

/* Simplified: map/unmap use identity mapping stubs */
int cpu_mmu_map(cpu_addr_t virt, cpu_addr_t phys,
                 cpu_u64_t size, cpu_u32_t attrs)
{
    (void)virt; (void)phys; (void)size; (void)attrs;
    return CPU_OK;
}

int cpu_mmu_unmap(cpu_addr_t virt, cpu_u64_t size)
{
    (void)virt; (void)size;
    cpu_mmu_tlb_flush_all();
    return CPU_OK;
}

cpu_addr_t cpu_mmu_virt_to_phys(cpu_addr_t virt)
{
#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t par;
    __asm__ volatile("at s1e1r, %0\n\t" :: "r"(virt) : "memory");
    cpu_isb();
    CPU_MRS(PAR_EL1, par);
    if (par & 1u) return (cpu_addr_t)-1; /* translation failed */
    return (par & 0x000FFFFFFFFFF000ULL) | (virt & 0xFFFULL);
#elif defined(UIOX_ARCH_X86_64)
    return virt; /* identity mapped in bare-metal kernel         */
#elif defined(UIOX_ARCH_RISCV64)
    return virt; /* identity mapped                              */
#else
    return virt;
#endif
}

int cpu_mmu_set_attrs(cpu_addr_t virt, cpu_u64_t size, cpu_u32_t attrs)
{
    (void)virt; (void)size; (void)attrs;
    cpu_mmu_tlb_flush_all();
    return CPU_OK;
}

void cpu_mmu_set_asid(cpu_asid_t asid)
{
#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t ttbr0;
    CPU_MRS(TTBR0_EL1, ttbr0);
    ttbr0 = (ttbr0 & 0x0000FFFFFFFFFFFFULL) |
            ((cpu_u64_t)asid << 48);
    CPU_MSR(TTBR0_EL1, ttbr0);
    cpu_isb();
#elif defined(UIOX_ARCH_RISCV64)
    cpu_u64_t satp;
    CPU_CSR_READ(satp, satp);
    satp = (satp & ~(0xFFFFULL << 44)) | ((cpu_u64_t)asid << 44);
    CPU_CSR_WRITE(satp, satp);
    __asm__ volatile("sfence.vma\n\t" ::: "memory");
#else
    (void)asid;
#endif
}

cpu_asid_t cpu_mmu_get_asid(void)
{
#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t ttbr0;
    CPU_MRS(TTBR0_EL1, ttbr0);
    return (cpu_asid_t)((ttbr0 >> 48) & 0xFFFF);
#elif defined(UIOX_ARCH_RISCV64)
    cpu_u64_t satp;
    CPU_CSR_READ(satp, satp);
    return (cpu_asid_t)((satp >> 44) & 0xFFFF);
#else
    return 0;
#endif
}
