#ifndef ARCH_RUNTIME_ARM32_H
#define ARCH_RUNTIME_ARM32_H

/*
 * 10_Arch/arm32/include/arch_runtime.h
 * ARMv7-A runtime interfaces consumed by:
 *   33_ProcessControlSubsystem — context switch, atomic ops
 *   32_FileSystem              — MMU map/unmap
 *   40_SystemCallInterface     — syscall entry/exit
 */

#include "arch_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * CPU context — ARMv7-A callee-saved register set
 *
 * ABI (AAPCS32): callee saves r4-r11, lr.
 * We also save sp, cpsr, and pc-equivalent (lr at call time).
 * ====================================================================== */
typedef struct arch_context {
    unsigned int r4, r5, r6, r7;   /* callee-saved GPRs               */
    unsigned int r8, r9, r10, r11; /* callee-saved GPRs (fp = r11)    */
    unsigned int lr;                /* return address / link register  */
    unsigned int sp;                /* stack pointer                   */
    unsigned int cpsr;              /* current program status register */
    unsigned int pc;                /* programme counter               */
} arch_context_t;

/* =========================================================================
 * Context switch  (33_ProcessControlSubsystem)
 * ====================================================================== */
void arch_context_switch(arch_context_t *old_ctx,
                          arch_context_t *new_ctx);

void arch_context_init  (arch_context_t *ctx,
                          void          (*entry)(void *arg),
                          unsigned int    stack_top,
                          void           *arg);

/* =========================================================================
 * MMU / page table  (ARMv7-A short-descriptor format, 4KB pages)
 * ====================================================================== */
int  arch_mmu_map      (unsigned int virt,
                         unsigned int phys,
                         unsigned int flags);
void arch_mmu_unmap    (unsigned int virt);
void arch_tlb_flush    (unsigned int virt);
void arch_tlb_flush_all(void);
void arch_mmu_switch   (unsigned int pgd_phys);

/* MMU flags */
#define ARCH_MMU_R   (1u << 0)
#define ARCH_MMU_W   (1u << 1)
#define ARCH_MMU_X   (1u << 2)
#define ARCH_MMU_U   (1u << 3)
#define ARCH_MMU_NC  (1u << 4)

/* =========================================================================
 * System call  (SVC instruction)
 * ====================================================================== */
long arch_syscall_dispatch(unsigned int nr,
                            unsigned int a0, unsigned int a1,
                            unsigned int a2, unsigned int a3,
                            unsigned int a4, unsigned int a5);

/* =========================================================================
 * Atomic operations  (ARMv7-A LDREX/STREX)
 * ====================================================================== */
int  arch_atomic_add  (int *ptr, int val);
int  arch_atomic_sub  (int *ptr, int val);
int  arch_atomic_cas  (int *ptr, int expected, int desired);
int  arch_atomic_load (const int *ptr);
void arch_atomic_store(int *ptr, int val);

/* =========================================================================
 * Spinlock  (LDREX/STREX + WFE/SEV)
 * ====================================================================== */
typedef struct arch_spinlock { int lock; } arch_spinlock_t;
#define ARCH_SPINLOCK_INIT { 0 }

void arch_spin_lock   (arch_spinlock_t *sl);
void arch_spin_unlock (arch_spinlock_t *sl);
int  arch_spin_trylock(arch_spinlock_t *sl);

/* =========================================================================
 * IRQ save/restore  (CPSR.I bit via CPS / MRS / MSR)
 * ====================================================================== */
unsigned int arch_irq_save   (void);
void         arch_irq_restore(unsigned int flags);

#ifdef __cplusplus
}
#endif
#endif /* ARCH_RUNTIME_ARM32_H */
