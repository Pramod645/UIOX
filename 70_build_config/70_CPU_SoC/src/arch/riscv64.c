/*
 * riscv64.c - RISC-V RV64GC specific initialisation
 */
#include "../../include/arch/riscv64.h"
#include "../../include/cpu_regs.h"
#include <stdio.h>

int riscv64_init(void)
{
    riscv64_delegate_traps();
    riscv64_enable_fp();
    printf("[rv64] RISC-V RV64GC initialised  hartid=%llu\n",
           (unsigned long long)riscv64_get_hartid());
    return CPU_OK;
}

void riscv64_trap_init(cpu_addr_t tvec)
{
    /* direct mode: tvec[1:0]=0, base=tvec */
    cpu_u64_t val = (cpu_u64_t)tvec & ~0x3ULL;
    CPU_CSR_WRITE(stvec, val);

    /* clear pending interrupts */
    CPU_CSR_WRITE(sip, 0ULL);

    /* enable S-mode external, timer, software IRQs */
    cpu_u64_t sie_val = MIE_SEIE | MIE_STIE | MIE_SSIE;
    CPU_CSR_WRITE(sie, sie_val);
}

void riscv64_delegate_traps(void)
{
    /* delegate all synchronous exceptions to S-mode */
    cpu_u64_t medeleg =
        (1ULL << MCAUSE_INST_MISALIGN)   |
        (1ULL << MCAUSE_ILLEGAL_INST)     |
        (1ULL << MCAUSE_BREAKPOINT)       |
        (1ULL << MCAUSE_LOAD_MISALIGN)    |
        (1ULL << MCAUSE_STORE_MISALIGN)   |
        (1ULL << MCAUSE_ECALL_U)          |
        (1ULL << MCAUSE_INST_PAGE_FAULT)  |
        (1ULL << MCAUSE_LOAD_PAGE_FAULT)  |
        (1ULL << MCAUSE_STORE_PAGE_FAULT);
    CPU_CSR_WRITE(medeleg, medeleg);

    /* delegate timer, software, external IRQs to S-mode */
    cpu_u64_t mideleg = MIE_SSIE | MIE_STIE | MIE_SEIE;
    CPU_CSR_WRITE(mideleg, mideleg);
}

void riscv64_paging_init(cpu_addr_t root_ppn)
{
    /* flush TLB */
    __asm__ volatile("sfence.vma\n\t" ::: "memory");

    /* set SATP: SV39 mode + root page table PPN */
    cpu_u64_t satp = SATP_MODE_SV39 | (root_ppn >> 12);
    CPU_CSR_WRITE(satp, satp);

    __asm__ volatile("sfence.vma\n\t" ::: "memory");
}

void riscv64_enable_fp(void)
{
    /* set FS = Initial (01) in sstatus */
    cpu_u64_t status;
    CPU_CSR_READ(sstatus, status);
    status = (status & ~(3ULL << 13)) | (1ULL << 13);
    CPU_CSR_WRITE(sstatus, status);

    /* zero all FP registers */
    __asm__ volatile(
        "fmv.d.x f0,  zero\n\t" "fmv.d.x f1,  zero\n\t"
        "fmv.d.x f2,  zero\n\t" "fmv.d.x f3,  zero\n\t"
        "fmv.d.x f4,  zero\n\t" "fmv.d.x f5,  zero\n\t"
        "fmv.d.x f6,  zero\n\t" "fmv.d.x f7,  zero\n\t"
        "fmv.d.x f8,  zero\n\t" "fmv.d.x f9,  zero\n\t"
        "fmv.d.x f10, zero\n\t" "fmv.d.x f11, zero\n\t"
        "fmv.d.x f12, zero\n\t" "fmv.d.x f13, zero\n\t"
        "fmv.d.x f14, zero\n\t" "fmv.d.x f15, zero\n\t"
        "fmv.d.x f16, zero\n\t" "fmv.d.x f17, zero\n\t"
        "fmv.d.x f18, zero\n\t" "fmv.d.x f19, zero\n\t"
        "fmv.d.x f20, zero\n\t" "fmv.d.x f21, zero\n\t"
        "fmv.d.x f22, zero\n\t" "fmv.d.x f23, zero\n\t"
        "fmv.d.x f24, zero\n\t" "fmv.d.x f25, zero\n\t"
        "fmv.d.x f26, zero\n\t" "fmv.d.x f27, zero\n\t"
        "fmv.d.x f28, zero\n\t" "fmv.d.x f29, zero\n\t"
        "fmv.d.x f30, zero\n\t" "fmv.d.x f31, zero\n\t"
        "fsflags zero\n\t" ::: "memory");
}

cpu_u64_t riscv64_get_hartid(void)
{
    cpu_u64_t id;
    CPU_CSR_READ(mhartid, id);
    return id;
}

void riscv64_print_info(void)
{
    cpu_u64_t misa, mhartid, mstatus;
    CPU_CSR_READ(misa,    misa);
    CPU_CSR_READ(mhartid, mhartid);
    CPU_CSR_READ(mstatus, mstatus);
    printf("[rv64] hartid=%llu  misa=0x%016llx  mstatus=0x%016llx\n",
           (unsigned long long)mhartid,
           (unsigned long long)misa,
           (unsigned long long)mstatus);

    printf("[rv64] ISA extensions: RV64");
    if (misa & (1u << ('I'-'A'))) printf("I");
    if (misa & (1u << ('M'-'A'))) printf("M");
    if (misa & (1u << ('A'-'A'))) printf("A");
    if (misa & (1u << ('F'-'A'))) printf("F");
    if (misa & (1u << ('D'-'A'))) printf("D");
    if (misa & (1u << ('C'-'A'))) printf("C");
    if (misa & (1u << ('V'-'A'))) printf("V");
    if (misa & (1u << ('H'-'A'))) printf("H");
    if (misa & (1u << ('S'-'A'))) printf("S");
    if (misa & (1u << ('U'-'A'))) printf("U");
    printf("\n");
}
