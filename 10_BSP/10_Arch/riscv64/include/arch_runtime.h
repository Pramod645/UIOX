#ifndef ARCH_RUNTIME_RISCV64_H
#define ARCH_RUNTIME_RISCV64_H

/*
 * 10_Arch/riscv64/include/arch_runtime.h
 * RISC-V RV64 runtime interfaces for the UIOX kernel.
 */

#include "arch_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── CPU context for context switch ─────────────────────── */
typedef struct arch_context {
    /* Callee-saved regs: s0-s11 (x8-x9, x18-x27) */
    unsigned long s0,  s1,  s2,  s3,  s4,  s5;
    unsigned long s6,  s7,  s8,  s9,  s10, s11;
    unsigned long ra;       /* return address (x1)           */
    unsigned long sp;       /* stack pointer (x2)            */
    unsigned long tp;       /* thread pointer (x4)           */
    unsigned long sstatus;  /* supervisor status             */
    unsigned long sepc;     /* supervisor exception PC       */
} arch_context_t;

/* ── Context switch ──────────────────────────────────────── */
void arch_context_switch(arch_context_t *old_ctx,
                          arch_context_t *new_ctx);
void arch_context_init  (arch_context_t *ctx,
                          void          (*entry)(void *arg),
                          unsigned long   stack_top,
                          void           *arg);

/* ── MMU ─────────────────────────────────────────────────── */
int  arch_mmu_map      (unsigned long virt, unsigned long phys,
                          unsigned long flags);
void arch_mmu_unmap    (unsigned long virt);
void arch_tlb_flush    (unsigned long virt);
void arch_tlb_flush_all(void);
void arch_mmu_switch   (unsigned long pgd_phys);

#define ARCH_MMU_R   (1u << 0)
#define ARCH_MMU_W   (1u << 1)
#define ARCH_MMU_X   (1u << 2)
#define ARCH_MMU_U   (1u << 3)
#define ARCH_MMU_NC  (1u << 4)

/* ── Syscall dispatch ────────────────────────────────────── */
long arch_syscall_dispatch(unsigned long nr,
                             unsigned long a0, unsigned long a1,
                             unsigned long a2, unsigned long a3,
                             unsigned long a4, unsigned long a5);

/* ── Atomic operations ───────────────────────────────────── */
long arch_atomic_add  (long *ptr, long val);
long arch_atomic_sub  (long *ptr, long val);
int  arch_atomic_cas  (long *ptr, long expected, long desired);
long arch_atomic_load (const long *ptr);
void arch_atomic_store(long *ptr, long val);

/* ── Spinlock ─────────────────────────────────────────────── */
typedef struct arch_spinlock { long lock; } arch_spinlock_t;
#define ARCH_SPINLOCK_INIT { 0L }

void arch_spin_lock   (arch_spinlock_t *sl);
void arch_spin_unlock (arch_spinlock_t *sl);
int  arch_spin_trylock(arch_spinlock_t *sl);

/* ── IRQ save/restore ────────────────────────────────────── */
unsigned long arch_irq_save   (void);
void          arch_irq_restore(unsigned long flags);

/* ── PLIC claim/complete (runtime interrupt acknowledge) ─── */
unsigned int arch_plic_claim    (void);
void         arch_plic_complete (unsigned int irq);

#ifdef __cplusplus
}
#endif
#endif /* ARCH_RUNTIME_RISCV64_H */
