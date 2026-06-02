#ifndef RISCV64_H
#define RISCV64_H
/*
 * riscv64.h - RISC-V RV64GC specific definitions
 * Reference: RISC-V Privileged ISA Specification v1.12
 */
#include "../cpu_types.h"

/* -- CSR addresses ------------------------------------------ */
#define CSR_MSTATUS     0x300u
#define CSR_MISA        0x301u
#define CSR_MEDELEG     0x302u
#define CSR_MIDELEG     0x303u
#define CSR_MIE         0x304u
#define CSR_MTVEC       0x305u
#define CSR_MCOUNTEREN  0x306u
#define CSR_MSCRATCH    0x340u
#define CSR_MEPC        0x341u
#define CSR_MCAUSE      0x342u
#define CSR_MTVAL       0x343u
#define CSR_MIP         0x344u
#define CSR_MHARTID     0xF14u
#define CSR_SSTATUS     0x100u
#define CSR_SIE         0x104u
#define CSR_STVEC       0x105u
#define CSR_SSCRATCH    0x140u
#define CSR_SEPC        0x141u
#define CSR_SCAUSE      0x142u
#define CSR_STVAL       0x143u
#define CSR_SIP         0x144u
#define CSR_SATP        0x180u
#define CSR_TIME        0xC01u
#define CSR_CYCLE       0xC00u
#define CSR_INSTRET     0xC02u

/* -- MSTATUS bits ------------------------------------------- */
#define MSTATUS_SIE     (1ULL <<  1)
#define MSTATUS_MIE     (1ULL <<  3)
#define MSTATUS_SPIE    (1ULL <<  5)
#define MSTATUS_MPIE    (1ULL <<  7)
#define MSTATUS_SPP     (1ULL <<  8)   /* S-mode previous priv   */
#define MSTATUS_MPP_S   (1ULL << 11)   /* M-mode prev = S-mode   */
#define MSTATUS_MPP_M   (3ULL << 11)   /* M-mode prev = M-mode   */
#define MSTATUS_FS_INIT (1ULL << 13)   /* FP dirty/initial       */
#define MSTATUS_SUM     (1ULL << 18)   /* S-mode user mem access */
#define MSTATUS_MXR     (1ULL << 19)   /* Make eXecutable Readable*/

/* -- MIE / MIP interrupt bits ------------------------------- */
#define MIE_SSIE        (1ULL <<  1)   /* S-mode software IRQ    */
#define MIE_MSIE        (1ULL <<  3)   /* M-mode software IRQ    */
#define MIE_STIE        (1ULL <<  5)   /* S-mode timer IRQ       */
#define MIE_MTIE        (1ULL <<  7)   /* M-mode timer IRQ       */
#define MIE_SEIE        (1ULL <<  9)   /* S-mode external IRQ    */
#define MIE_MEIE        (1ULL << 11)   /* M-mode external IRQ    */

/* -- SATP modes --------------------------------------------- */
#define SATP_MODE_BARE  (0ULL << 60)
#define SATP_MODE_SV39  (8ULL << 60)
#define SATP_MODE_SV48  (9ULL << 60)
#define SATP_MODE_SV57  (10ULL<< 60)

/* -- MCAUSE exception codes --------------------------------- */
#define MCAUSE_INST_MISALIGN    0ULL
#define MCAUSE_INST_FAULT       1ULL
#define MCAUSE_ILLEGAL_INST     2ULL
#define MCAUSE_BREAKPOINT       3ULL
#define MCAUSE_LOAD_MISALIGN    4ULL
#define MCAUSE_LOAD_FAULT       5ULL
#define MCAUSE_STORE_MISALIGN   6ULL
#define MCAUSE_STORE_FAULT      7ULL
#define MCAUSE_ECALL_U          8ULL
#define MCAUSE_ECALL_S          9ULL
#define MCAUSE_ECALL_M          11ULL
#define MCAUSE_INST_PAGE_FAULT  12ULL
#define MCAUSE_LOAD_PAGE_FAULT  13ULL
#define MCAUSE_STORE_PAGE_FAULT 15ULL
#define MCAUSE_INTERRUPT_BIT    (1ULL << 63)

/* -- PLIC base (SiFive HiFive Unmatched / QEMU virt) -------- */
#define RISCV_PLIC_BASE         0x0C000000ULL
#define RISCV_PLIC_SIZE         0x04000000ULL
#define RISCV_MTIME_BASE        0x02000000ULL
#define RISCV_MTIMECMP_BASE     0x02004000ULL
#define RISCV_UART0_BASE        0x10000000ULL
#define RISCV_UART0_IRQ         10u

/* -- RISC-V specific functions ------------------------------ */
int  riscv64_init            (void);
void riscv64_trap_init       (cpu_addr_t tvec);
void riscv64_delegate_traps  (void);
void riscv64_paging_init     (cpu_addr_t root_ppn);
void riscv64_enable_fp       (void);
void riscv64_print_info      (void);
cpu_u64_t riscv64_get_hartid (void);

/* -- SV39 page-table entry ---------------------------------- */
#define PTE_V     (1ULL << 0)   /* valid                        */
#define PTE_R     (1ULL << 1)   /* readable                     */
#define PTE_W     (1ULL << 2)   /* writable                     */
#define PTE_X     (1ULL << 3)   /* executable                   */
#define PTE_U     (1ULL << 4)   /* user accessible              */
#define PTE_G     (1ULL << 5)   /* global                       */
#define PTE_A     (1ULL << 6)   /* accessed                     */
#define PTE_D     (1ULL << 7)   /* dirty                        */
#define PTE_PPN_SHIFT  10

static inline cpu_u64_t riscv_pte(cpu_addr_t phys, cpu_u64_t flags)
{
    return ((phys >> 12) << PTE_PPN_SHIFT) | flags | PTE_V;
}

#endif /* RISCV64_H */
