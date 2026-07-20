#ifndef ARCH_RUNTIME_ARM64_H
#define ARCH_RUNTIME_ARM64_H

/*
 * 10_Arch/arm64/include/arch_runtime.h
 * ARMv8-A runtime interfaces consumed by:
 *   33_ProcessControlSubsystem — context switch, atomic ops
 *   32_FileSystem              — MMU map/unmap
 *   40_SystemCallInterface     — syscall entry/exit
 *
 * All declarations here are ISA-defined operations.
 * No SoC-specific content belongs here.
 */

#include "arch_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * CPU context — saved/restored on every context switch
 * ====================================================================== */
typedef struct arch_context {
    /* Callee-saved general-purpose registers (ABI: x19-x28) */
    unsigned long x19, x20, x21, x22, x23, x24, x25, x26, x27, x28;
    /* Frame pointer and link register */
    unsigned long x29;   /* fp */
    unsigned long x30;   /* lr */
    /* Stack pointer at time of switch */
    unsigned long sp;
    /* SPSR_EL1 and ELR_EL1 — needed when switching from exception */
    unsigned long spsr;
    unsigned long elr;
    /* TPIDR_EL0 — thread-local storage pointer */
    unsigned long tpidr;
} arch_context_t;

/* =========================================================================
 * Context switch  (called by 33_ProcessControlSubsystem scheduler)
 * ====================================================================== */

/** Save current context into @old_ctx, load @new_ctx and resume. */
void arch_context_switch(arch_context_t *old_ctx,
                          arch_context_t *new_ctx);

/** Initialise a fresh context for a new thread.
 *  @ctx       context struct in the new thread's TCB
 *  @entry     function the thread starts at
 *  @stack_top top of the new thread's stack
 *  @arg       first argument passed to @entry                          */
void arch_context_init(arch_context_t *ctx,
                        void          (*entry)(void *arg),
                        unsigned long   stack_top,
                        void           *arg);

/* =========================================================================
 * MMU / page table operations  (called by 32_FileSystem and mm layer)
 * ====================================================================== */

/** Map @phys → @virt with @flags (PROT_READ | PROT_WRITE | PROT_EXEC).
 *  Page size = UIOX_PAGE_SIZE from arch_defs.h.                        */
int  arch_mmu_map    (unsigned long virt,
                       unsigned long phys,
                       unsigned long flags);

/** Unmap the page at @virt. */
void arch_mmu_unmap  (unsigned long virt);

/** Flush the TLB entry for @virt (TLBI VAE1IS).                        */
void arch_tlb_flush  (unsigned long virt);

/** Flush entire TLB for current ASID (TLBI VMALLE1IS).                 */
void arch_tlb_flush_all(void);

/** Switch the active page table to @pgd_phys (write TTBR0_EL1).       */
void arch_mmu_switch (unsigned long pgd_phys);

/* MMU flags */
#define ARCH_MMU_R    (1u << 0)   /* readable             */
#define ARCH_MMU_W    (1u << 1)   /* writable             */
#define ARCH_MMU_X    (1u << 2)   /* executable           */
#define ARCH_MMU_U    (1u << 3)   /* user-accessible      */
#define ARCH_MMU_NC   (1u << 4)   /* non-cacheable        */

/* =========================================================================
 * System call entry/exit  (called from arch_syscall.S stubs)
 * ====================================================================== */

/** Kernel-side syscall dispatcher — called from SVC handler.
 *  @nr   syscall number (x8 in ARM64 ABI)
 *  @a0-5 arguments (x0-x5)
 *  Returns value placed in x0 on return to user.                       */
long arch_syscall_dispatch(unsigned long nr,
                            unsigned long a0, unsigned long a1,
                            unsigned long a2, unsigned long a3,
                            unsigned long a4, unsigned long a5);

/* =========================================================================
 * Atomic operations  (ISA: LDXR/STXR / LDADD instructions)
 * ====================================================================== */

/** Atomically add @val to *@ptr, return old value. */
long arch_atomic_add  (long *ptr, long val);

/** Atomically subtract @val from *@ptr, return old value. */
long arch_atomic_sub  (long *ptr, long val);

/** Atomically compare *@ptr with @expected; if equal write @desired.
 *  Returns 1 on success, 0 on failure.                                 */
int  arch_atomic_cas  (long *ptr, long expected, long desired);

/** Atomically load *@ptr with acquire semantics. */
long arch_atomic_load (const long *ptr);

/** Atomically store @val to *@ptr with release semantics. */
void arch_atomic_store(long *ptr, long val);

/* =========================================================================
 * Spinlock  (ISA: LDAXR/STLXR + WFE/SEV)
 * ====================================================================== */
typedef struct arch_spinlock { long lock; } arch_spinlock_t;
#define ARCH_SPINLOCK_INIT  { 0L }

void arch_spin_lock  (arch_spinlock_t *sl);
void arch_spin_unlock(arch_spinlock_t *sl);
int  arch_spin_trylock(arch_spinlock_t *sl);

/* =========================================================================
 * IRQ save/restore  (ISA: MSR DAIF)
 * ====================================================================== */

/** Save DAIF flags and disable IRQs. Returns saved flags. */
unsigned long arch_irq_save(void);

/** Restore previously saved DAIF flags. */
void arch_irq_restore(unsigned long flags);

#ifdef __cplusplus
}
#endif
#endif /* ARCH_RUNTIME_ARM64_H */
