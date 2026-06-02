#ifndef CPU_REGS_H
#define CPU_REGS_H
/*
 * cpu_regs.h - Register access abstractions for all 3 architectures
 */
#include "cpu_types.h"

/* ============================================================
   ARM Cortex-A76 (AArch64) system register access
   ============================================================ */
#if defined(UIOX_ARCH_ARM64)

#define CPU_MRS(reg, val) \
    __asm__ volatile("mrs %0, " #reg : "=r"(val) :: "memory")
#define CPU_MSR(reg, val) \
    __asm__ volatile("msr " #reg ", %0" :: "r"(val) : "memory")

/* PSTATE / DAIF */
#define cpu_irq_disable()   __asm__ volatile("msr daifset, #0xF\n\t" ::: "memory")
#define cpu_irq_enable()    __asm__ volatile("msr daifclr, #0xF\n\t" ::: "memory")
#define cpu_irq_save(f)     __asm__ volatile("mrs %0, daif\n\t" : "=r"(f) :: "memory")
#define cpu_irq_restore(f)  __asm__ volatile("msr daif, %0\n\t" :: "r"(f) : "memory")

/* Barriers */
#define cpu_dsb()   __asm__ volatile("dsb sy\n\t" ::: "memory")
#define cpu_isb()   __asm__ volatile("isb\n\t"    ::: "memory")
#define cpu_dmb()   __asm__ volatile("dmb sy\n\t" ::: "memory")
#define cpu_nop()   __asm__ volatile("nop\n\t")
#define cpu_wfi()   __asm__ volatile("wfi\n\t")
#define cpu_wfe()   __asm__ volatile("wfe\n\t")
#define cpu_sev()   __asm__ volatile("sev\n\t")

/* System register shortcuts */
static inline cpu_u64_t cpu_read_midr(void)
{ cpu_u64_t v; CPU_MRS(MIDR_EL1, v); return v; }

static inline cpu_u64_t cpu_read_mpidr(void)
{ cpu_u64_t v; CPU_MRS(MPIDR_EL1, v); return v; }

static inline cpu_u64_t cpu_read_currentel(void)
{ cpu_u64_t v; CPU_MRS(CurrentEL, v); return (v >> 2) & 3; }

static inline cpu_u64_t cpu_read_cntfrq(void)
{ cpu_u64_t v; CPU_MRS(CNTFRQ_EL0, v); return v; }

static inline cpu_u64_t cpu_read_cntvct(void)
{ cpu_u64_t v; CPU_MRS(CNTVCT_EL0, v); return v; }

static inline cpu_u64_t cpu_read_sp(void)
{ cpu_u64_t v; __asm__ volatile("mov %0, sp" : "=r"(v)); return v; }

/* ============================================================
   x86-64 register access
   ============================================================ */
#elif defined(UIOX_ARCH_X86_64)

#define cpu_irq_disable()   __asm__ volatile("cli\n\t" ::: "memory")
#define cpu_irq_enable()    __asm__ volatile("sti\n\t" ::: "memory")
#define cpu_irq_save(f) \
    __asm__ volatile("pushfq\n\tpopq %0\n\t" : "=r"(f) :: "memory")
#define cpu_irq_restore(f) \
    __asm__ volatile("pushq %0\n\tpopfq\n\t" :: "r"(f) : "memory")

#define cpu_dsb()   __asm__ volatile("mfence\n\t" ::: "memory")
#define cpu_isb()   __asm__ volatile("" ::: "memory")
#define cpu_dmb()   __asm__ volatile("lfence\n\t" ::: "memory")
#define cpu_nop()   __asm__ volatile("nop\n\t")
#define cpu_wfi()   __asm__ volatile("hlt\n\t")
#define cpu_wfe()   __asm__ volatile("pause\n\t")
#define cpu_sev()   do {} while(0)

/* CPUID helper */
static inline void cpu_cpuid(cpu_u32_t leaf,
                               cpu_u32_t *eax, cpu_u32_t *ebx,
                               cpu_u32_t *ecx, cpu_u32_t *edx)
{
    __asm__ volatile("cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(0) );
}

/* MSR access */
static inline cpu_u64_t cpu_rdmsr(cpu_u32_t idx)
{
    cpu_u32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(idx));
    return ((cpu_u64_t)hi << 32) | lo;
}
static inline void cpu_wrmsr(cpu_u32_t idx, cpu_u64_t val)
{
    __asm__ volatile("wrmsr"
        :: "c"(idx), "a"((cpu_u32_t)val),
           "d"((cpu_u32_t)(val >> 32)));
}

/* Port I/O */
static inline void     cpu_outb(cpu_u16_t p, cpu_u8_t  v)
{ __asm__ volatile("outb %0,%1" :: "a"(v),  "Nd"(p)); }
static inline void     cpu_outw(cpu_u16_t p, cpu_u16_t v)
{ __asm__ volatile("outw %0,%1" :: "a"(v),  "Nd"(p)); }
static inline void     cpu_outl(cpu_u16_t p, cpu_u32_t v)
{ __asm__ volatile("outl %0,%1" :: "a"(v),  "Nd"(p)); }
static inline cpu_u8_t  cpu_inb(cpu_u16_t p)
{ cpu_u8_t  v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p)); return v; }
static inline cpu_u16_t cpu_inw(cpu_u16_t p)
{ cpu_u16_t v; __asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p)); return v; }
static inline cpu_u32_t cpu_inl(cpu_u16_t p)
{ cpu_u32_t v; __asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p)); return v; }

static inline cpu_u64_t cpu_read_rsp(void)
{ cpu_u64_t v; __asm__ volatile("movq %%rsp,%0":"=r"(v)); return v; }

/* ============================================================
   RISC-V RV64GC CSR access
   ============================================================ */
#elif defined(UIOX_ARCH_RISCV64)

#define CPU_CSR_READ(csr, val) \
    __asm__ volatile("csrr %0, " #csr : "=r"(val) :: "memory")
#define CPU_CSR_WRITE(csr, val) \
    __asm__ volatile("csrw " #csr ", %0" :: "r"(val) : "memory")
#define CPU_CSR_SET(csr, bits) \
    __asm__ volatile("csrs " #csr ", %0" :: "r"(bits) : "memory")
#define CPU_CSR_CLR(csr, bits) \
    __asm__ volatile("csrc " #csr ", %0" :: "r"(bits) : "memory")

/* sstatus.SIE bit */
#define SSTATUS_SIE  (1ULL << 1)

#define cpu_irq_disable()   CPU_CSR_CLR(sstatus, SSTATUS_SIE)
#define cpu_irq_enable()    CPU_CSR_SET(sstatus, SSTATUS_SIE)
#define cpu_irq_save(f) \
    __asm__ volatile("csrr %0, sstatus\n\t" \
                     "csrc sstatus, %1\n\t"  \
                     : "=r"(f) : "r"(SSTATUS_SIE) : "memory")
#define cpu_irq_restore(f) \
    CPU_CSR_WRITE(sstatus, f)

#define cpu_dsb()   __asm__ volatile("fence\n\t"     ::: "memory")
#define cpu_isb()   __asm__ volatile("fence.i\n\t"   ::: "memory")
#define cpu_dmb()   __asm__ volatile("fence r,r\n\t" ::: "memory")
#define cpu_nop()   __asm__ volatile("nop\n\t")
#define cpu_wfi()   __asm__ volatile("wfi\n\t")
#define cpu_wfe()   __asm__ volatile("wfi\n\t")
#define cpu_sev()   do {} while(0)

static inline cpu_u64_t cpu_read_mhartid(void)
{ cpu_u64_t v; CPU_CSR_READ(mhartid, v); return v; }

static inline cpu_u64_t cpu_read_time(void)
{ cpu_u64_t v; CPU_CSR_READ(time, v); return v; }

static inline cpu_u64_t cpu_read_cycle(void)
{ cpu_u64_t v; CPU_CSR_READ(cycle, v); return v; }

static inline cpu_u64_t cpu_read_sp(void)
{ cpu_u64_t v; __asm__ volatile("mv %0, sp" : "=r"(v)); return v; }

#else
#error "UIOX_ARCH_ARM64, UIOX_ARCH_X86_64, or UIOX_ARCH_RISCV64 must be defined"
#endif

/* -- Common MMIO accessors (all architectures) -------------- */
static inline void cpu_mmio_write32(cpu_addr_t addr, cpu_u32_t val)
{
    *((volatile cpu_u32_t *)(cpu_addr_t)(addr)) = val;
    cpu_dsb();
}
static inline cpu_u32_t cpu_mmio_read32(cpu_addr_t addr)
{
    cpu_u32_t v = *((volatile cpu_u32_t *)(cpu_addr_t)(addr));
    cpu_dsb();
    return v;
}
static inline void cpu_mmio_write64(cpu_addr_t addr, cpu_u64_t val)
{
    *((volatile cpu_u64_t *)(cpu_addr_t)(addr)) = val;
    cpu_dsb();
}
static inline cpu_u64_t cpu_mmio_read64(cpu_addr_t addr)
{
    cpu_u64_t v = *((volatile cpu_u64_t *)(cpu_addr_t)(addr));
    cpu_dsb();
    return v;
}
static inline void cpu_mmio_write8(cpu_addr_t addr, cpu_u8_t val)
{
    *((volatile cpu_u8_t *)(cpu_addr_t)(addr)) = val;
}
static inline cpu_u8_t cpu_mmio_read8(cpu_addr_t addr)
{
    return *((volatile cpu_u8_t *)(cpu_addr_t)(addr));
}

#endif /* CPU_REGS_H */
