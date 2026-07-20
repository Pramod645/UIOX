/*
 * 10_Arch/riscv64/src/arch_runtime.c
 * RISC-V RV64 runtime services: IRQ dispatch, MMU (Sv39),
 * atomics (A extension), spinlocks, IRQ save/restore.
 */

 #include "arch_defs.h"
 #include "arch_runtime.h"
 #include "../../../03_SoC/include/uiox_soc_stdio.h"
 
 /* ── External C dispatchers ──────────────────────────────────────── */
 extern void irq_dispatch  (unsigned int irq);
 extern long syscall_dispatch(unsigned long nr,
                                unsigned long a0, unsigned long a1,
                                unsigned long a2, unsigned long a3,
                                unsigned long a4, unsigned long a5);
 extern void exception_dispatch(unsigned long cause, unsigned long tval,
                                 void *frame);
 
 /* =========================================================================
  * IRQ dispatch  — called from arch_irq.S after reading scause
  * ====================================================================== */
 void riscv_irq_dispatch(unsigned long cause)
 {
     /*
      * Supervisor-mode async interrupt causes (scause bit 63 stripped):
      *   1 = Supervisor software interrupt (SSIP)
      *   5 = Supervisor timer interrupt    (STIP)
      *   9 = Supervisor external interrupt (SEIP — from PLIC)
      */
     if (cause == 9u) {
         /* S-mode external interrupt: claim from PLIC */
         unsigned int irq = arch_plic_claim();
         if (irq) {
             irq_dispatch(irq);
             arch_plic_complete(irq);
         }
     } else if (cause == 5u) {
         /* Timer: reload MTIMECMP via SBI TIME extension */
         irq_dispatch(0u);   /* IRQ 0 = timer by convention */
     }
 }
 
 void riscv_exception_handler(unsigned long cause, unsigned long tval,
                                void *frame)
 {
     exception_dispatch(cause, tval, frame);
 }
 
 /* syscall entry from arch_irq.S */
 long arch_syscall_dispatch(unsigned long nr,
                              unsigned long a0, unsigned long a1,
                              unsigned long a2, unsigned long a3,
                              unsigned long a4, unsigned long a5)
 {
     return syscall_dispatch(nr, a0, a1, a2, a3, a4, a5);
 }
 
 /* =========================================================================
  * PLIC claim / complete  (RISC-V arch-defined register layout)
  * ====================================================================== */
 unsigned int arch_plic_claim(void)
 {
     /* Hart 0 S-mode context = 1, PLIC_CLAIM at known offset */
     unsigned int ctx = 1u;   /* PLIC_CTX_SMODE(0) */
     volatile unsigned int *claim =
         (volatile unsigned int *)(unsigned long)PLIC_CLAIM(ctx);
     return *claim;
 }
 
 void arch_plic_complete(unsigned int irq)
 {
     unsigned int ctx = 1u;
     volatile unsigned int *complete =
         (volatile unsigned int *)(unsigned long)PLIC_CLAIM(ctx);
     *complete = irq;
 }
 
 /* =========================================================================
  * IRQ save / restore  (ISA: csrrci / csrsi sstatus.SIE)
  * ====================================================================== */
 unsigned long arch_irq_save(void)
 {
     unsigned long flags = arch_csrr_mstatus() & MSTATUS_SIE;
     arch_csrc_sstatus(MSTATUS_SIE);   /* disable S-mode IRQs */
     return flags;
 }
 
 void arch_irq_restore(unsigned long flags)
 {
     if (flags & MSTATUS_SIE)
         arch_csrs_sstatus(MSTATUS_SIE);
     else
         arch_csrc_sstatus(MSTATUS_SIE);
 }
 
 /* =========================================================================
  * MMU — Sv39 (39-bit virtual address, 3-level page table)
  *
  * Sv39 PTE layout (64-bit):
  *  [63:54] reserved
  *  [53:10] PPN (44 bits, split across levels)
  *  [9:8]   RSW (reserved for software)
  *  [7]     D (dirty)   [6] A (accessed)
  *  [5]     G (global)  [4] U (user)
  *  [3]     X (execute) [2] W (write)  [1] R (read)  [0] V (valid)
  * ====================================================================== */
 
 #define PTE_V    (1UL << 0)
 #define PTE_R    (1UL << 1)
 #define PTE_W    (1UL << 2)
 #define PTE_X    (1UL << 3)
 #define PTE_U    (1UL << 4)
 #define PTE_G    (1UL << 5)
 #define PTE_A    (1UL << 6)
 #define PTE_D    (1UL << 7)
 #define PTE_PPN_SHIFT  10
 
 extern unsigned long phys_alloc_page(void);   /* 32_FileSystem mm */
 
 static unsigned long arch_flags_to_pte(unsigned long flags)
 {
     unsigned long pte = PTE_V | PTE_A | PTE_D;
     if (flags & ARCH_MMU_R) pte |= PTE_R;
     if (flags & ARCH_MMU_W) pte |= PTE_W;
     if (flags & ARCH_MMU_X) pte |= PTE_X;
     if (flags & ARCH_MMU_U) pte |= PTE_U;
     return pte;
 }
 
 /* Walk / allocate Sv39 page table, return pointer to leaf PTE */
 static unsigned long *sv39_walk(unsigned long virt, int alloc)
 {
     /* Read satp: bits [43:0] = PPN of root page table */
     unsigned long satp;
     __asm__ volatile("csrr %0, satp" : "=r"(satp));
     unsigned long root_phys = (satp & ((1UL << 44) - 1UL)) << 12u;
 
     unsigned long *root = (unsigned long *)root_phys;
     unsigned int vpn2   = (unsigned int)((virt >> 30u) & 0x1FFu);
     unsigned int vpn1   = (unsigned int)((virt >> 21u) & 0x1FFu);
     unsigned int vpn0   = (unsigned int)((virt >> 12u) & 0x1FFu);
 
 #define DESCEND(tbl, idx) ({                                        \
     unsigned long _e = (tbl)[(idx)];                                \
     unsigned long *_next;                                            \
     if (!(_e & PTE_V)) {                                            \
         if (!alloc) return (unsigned long *)0;                      \
         unsigned long _p = phys_alloc_page();                       \
         if (!_p) return (unsigned long *)0;                         \
         /* Internal node PTE: PPN only, no R/W/X bits */            \
         (tbl)[(idx)] = (_p >> 2u) | PTE_V;                         \
         _next = (unsigned long *)_p;                                \
     } else {                                                        \
         _next = (unsigned long *)((_e >> PTE_PPN_SHIFT) << 12u);   \
     }                                                               \
     _next; })
 
     unsigned long *l1 = DESCEND(root,  vpn2);
     if (!l1) return (unsigned long *)0;
     unsigned long *l0 = DESCEND(l1, vpn1);
     if (!l0) return (unsigned long *)0;
     return &l0[vpn0];
 #undef DESCEND
 }
 
 int arch_mmu_map(unsigned long virt, unsigned long phys,
                   unsigned long flags)
 {
     unsigned long *pte = sv39_walk(virt, 1);
     if (!pte) return -1;
     *pte = ((phys >> 12u) << PTE_PPN_SHIFT) | arch_flags_to_pte(flags);
     arch_tlb_flush(virt);
     return 0;
 }
 
 void arch_mmu_unmap(unsigned long virt)
 {
     unsigned long *pte = sv39_walk(virt, 0);
     if (pte) { *pte = 0UL; arch_tlb_flush(virt); }
 }
 
 void arch_tlb_flush(unsigned long virt)
 {
     arch_tlbi_mva(virt);
 }
 
 void arch_tlb_flush_all(void)
 {
     arch_tlbi_all();
 }
 
 void arch_mmu_switch(unsigned long pgd_phys)
 {
     /*
      * satp = MODE(Sv39=8) | ASID(0) | PPN
      * MODE field is bits [63:60], Sv39 = 8
      */
     unsigned long satp = SATP_MODE_SV39 | (pgd_phys >> 12u);
     arch_csrw_satp(satp);
     arch_isb();
     arch_tlb_flush_all();
 }
 
 /* =========================================================================
  * Atomic operations  (RISC-V A extension: AMO instructions)
  * ====================================================================== */
 
 long arch_atomic_add(long *ptr, long val)
 {
     long old;
     __asm__ volatile("amoadd.d.aqrl %0, %2, (%1)"
                      : "=r"(old)
                      : "r"(ptr), "r"(val)
                      : "memory");
     return old;
 }
 
 long arch_atomic_sub(long *ptr, long val)
 {
     return arch_atomic_add(ptr, -val);
 }
 
 int arch_atomic_cas(long *ptr, long expected, long desired)
 {
     long old; int ok;
     __asm__ volatile(
         "1: lr.d.aq   %0,     (%2)    \n"
         "   bne       %0, %3, 2f      \n"
         "   sc.d.rl   %1, %4, (%2)   \n"
         "   bnez      %1,     1b      \n"
         "2:                           \n"
         : "=&r"(old), "=&r"(ok)
         : "r"(ptr), "r"(expected), "r"(desired)
         : "memory");
     return (old == expected && ok == 0) ? 1 : 0;
 }
 
 long arch_atomic_load(const long *ptr)
 {
     long v;
     __asm__ volatile("ld %0, (%1)" : "=r"(v) : "r"(ptr) : "memory");
     __asm__ volatile("fence r,r" ::: "memory");
     return v;
 }
 
 void arch_atomic_store(long *ptr, long val)
 {
     __asm__ volatile("fence w,w" ::: "memory");
     __asm__ volatile("sd %0, (%1)" :: "r"(val), "r"(ptr) : "memory");
 }
 
 /* =========================================================================
  * Spinlock  (RISC-V: LR.D / SC.D with fence)
  * ====================================================================== */
 
 void arch_spin_lock(arch_spinlock_t *sl)
 {
     long tmp; int fail;
     __asm__ volatile(
         "1: lr.d.aq  %0,     (%2)   \n"
         "   bnez     %0,     1b     \n"  /* spin if already locked */
         "   sc.d.rl  %1, %3, (%2)  \n"
         "   bnez     %1,     1b     \n"  /* retry if SC failed      */
         : "=&r"(tmp), "=&r"(fail)
         : "r"(&sl->lock), "r"(1L)
         : "memory");
 }
 
 void arch_spin_unlock(arch_spinlock_t *sl)
 {
     __asm__ volatile("fence rw,w" ::: "memory");
     __asm__ volatile("sd zero, (%0)" :: "r"(&sl->lock) : "memory");
 }
 
 int arch_spin_trylock(arch_spinlock_t *sl)
 {
     long old; int fail;
     __asm__ volatile(
         "lr.d.aq  %0,     (%2)  \n"
         "bnez     %0,     1f    \n"
         "sc.d.rl  %1, %3, (%2) \n"
         "1:                     \n"
         : "=&r"(old), "=&r"(fail)
         : "r"(&sl->lock), "r"(1L)
         : "memory");
     return (old == 0 && fail == 0) ? 1 : 0;
 }
 