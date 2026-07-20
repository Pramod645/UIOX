#ifndef ARCH_RUNTIME_X86_64_H
#define ARCH_RUNTIME_X86_64_H

/*
 * 10_Arch/x86_64/include/arch_runtime.h
 * AMD64 runtime interfaces consumed by:
 *   33_ProcessControlSubsystem — context switch, atomic ops
 *   32_FileSystem              — MMU map/unmap (CR3, page tables)
 *   40_SystemCallInterface     — syscall entry/exit (SYSCALL instruction)
 */

#include "arch_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * CPU context — x86_64 System V ABI callee-saved registers
 *
 * Callee-saved: rbx, rbp, r12-r15, rsp.
 * We also save rip (via the call/ret mechanism) and rflags.
 * ====================================================================== */
typedef struct arch_context {
    unsigned long rbx;
    unsigned long rbp;
    unsigned long r12;
    unsigned long r13;
    unsigned long r14;
    unsigned long r15;
    unsigned long rsp;
    unsigned long rip;      /* saved return address                     */
    unsigned long rflags;
    unsigned long fs_base;  /* thread-local storage (FS.base MSR)       */
} arch_context_t;

/* =========================================================================
 * Context switch  (33_ProcessControlSubsystem)
 * ====================================================================== */
void arch_context_switch(arch_context_t *old_ctx,
                          arch_context_t *new_ctx);

void arch_context_init  (arch_context_t *ctx,
                          void          (*entry)(void *arg),
                          unsigned long   stack_top,
                          void           *arg);

/* =========================================================================
 * MMU / page table  (x86_64 four-level, 4KB pages, CR3)
 * ====================================================================== */
int  arch_mmu_map      (unsigned long virt,
                         unsigned long phys,
                         unsigned long flags);
void arch_mmu_unmap    (unsigned long virt);
void arch_tlb_flush    (unsigned long virt);
void arch_tlb_flush_all(void);
void arch_mmu_switch   (unsigned long pml4_phys);

/* MMU flags */
#define ARCH_MMU_R   (1u << 0)
#define ARCH_MMU_W   (1u << 1)
#define ARCH_MMU_X   (1u << 2)
#define ARCH_MMU_U   (1u << 3)
#define ARCH_MMU_NC  (1u << 4)

/* =========================================================================
 * System call  (SYSCALL/SYSRET instruction pair)
 * ====================================================================== */
long arch_syscall_dispatch(unsigned long nr,
                            unsigned long a0, unsigned long a1,
                            unsigned long a2, unsigned long a3,
                            unsigned long a4, unsigned long a5);

/* =========================================================================
 * Atomic operations  (LOCK prefix + CMPXCHG)
 * ====================================================================== */
long arch_atomic_add  (long *ptr, long val);
long arch_atomic_sub  (long *ptr, long val);
int  arch_atomic_cas  (long *ptr, long expected, long desired);
long arch_atomic_load (const long *ptr);
void arch_atomic_store(long *ptr, long val);

/* =========================================================================
 * Spinlock  (LOCK XCHG)
 * ====================================================================== */
typedef struct arch_spinlock { long lock; } arch_spinlock_t;
#define ARCH_SPINLOCK_INIT { 0L }

void arch_spin_lock   (arch_spinlock_t *sl);
void arch_spin_unlock (arch_spinlock_t *sl);
int  arch_spin_trylock(arch_spinlock_t *sl);

/* =========================================================================
 * IRQ save/restore  (RFLAGS.IF via CLI/STI)
 * ====================================================================== */
unsigned long arch_irq_save   (void);
void          arch_irq_restore(unsigned long flags);

#ifdef __cplusplus
}
#endif
#endif /* ARCH_RUNTIME_X86_64_H */
