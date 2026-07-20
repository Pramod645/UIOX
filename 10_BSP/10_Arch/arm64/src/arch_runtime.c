/*
 * 10_Arch/arm64/src/arch_runtime.c
 * ARMv8-A runtime services: IRQ dispatch, MMU, atomics, spinlocks.
 */

 #include "arch_defs.h"
 #include "arch_runtime.h"
 #include "../../../03_SoC/include/uiox_soc_stdio.h"
 
 /* ── External C dispatcher (20_DriverInterfaces) ─────────────────── */
 extern void irq_dispatch(unsigned int irq);
 
 /* Called from arch_irq.S after reading GIC IAR */
 void arch_irq_dispatch(unsigned int irq)
 {
     /* Spurious IRQ from GIC has value 1023 */
     if (irq >= 1023u) return;
     irq_dispatch(irq);
 }
 
 /* Called from arch_irq.S SVC handler */
 extern long syscall_dispatch(unsigned long nr,
                                unsigned long a0, unsigned long a1,
                                unsigned long a2, unsigned long a3,
                                unsigned long a4, unsigned long a5);
 
 long arch_syscall_dispatch(unsigned long nr,
                              unsigned long a0, unsigned long a1,
                              unsigned long a2, unsigned long a3,
                              unsigned long a4, unsigned long a5)
 {
     return syscall_dispatch(nr, a0, a1, a2, a3, a4, a5);
 }
 
 /* =========================================================================
  * IRQ save / restore  (ISA: MSR/MRS DAIF)
  * ====================================================================== */
 
 unsigned long arch_irq_save(void)
 {
     unsigned long flags;
     __asm__ volatile("mrs %0, daif" : "=r"(flags));
     __asm__ volatile("msr daifset, #2" ::: "memory");   /* mask IRQ */
     return flags;
 }
 
 void arch_irq_restore(unsigned long flags)
 {
     __asm__ volatile("msr daif, %0" :: "r"(flags) : "memory");
 }
 
 /* =========================================================================
  * MMU operations  (ISA: TTBR0_EL1, TLBI instructions)
  *
  * Simple two-level page table for UIOX kernel (4KB pages, 48-bit VA).
  * The kernel's physical page allocator (32_FileSystem mm layer) provides
  * phys_alloc_page() — declared here as extern.
  * ====================================================================== */
 
 extern unsigned long phys_alloc_page(void);  /* 32_FileSystem mm */
 
 /* PTE flags — ARMv8-A page table entry bits */
 #define PTE_VALID   (1UL << 0)
 #define PTE_TABLE   (1UL << 1)   /* 1 = table, 0 = block/page */
 #define PTE_PAGE    (3UL << 0)   /* valid + table for leaf      */
 #define PTE_USER    (1UL << 6)   /* AP[1]: EL0 access           */
 #define PTE_RO      (1UL << 7)   /* AP[2]: read-only            */
 #define PTE_SH_IS   (3UL << 8)   /* inner-shareable             */
 #define PTE_AF      (1UL << 10)  /* access flag (hw managed)    */
 #define PTE_nG      (1UL << 11)  /* not global                  */
 #define PTE_XN      (1UL << 54)  /* execute-never               */
 #define PTE_PXN     (1UL << 53)  /* privileged execute-never    */
 #define PTE_ATTR_NI (0UL << 2)   /* AttrIdx=0: normal, inner WB */
 
 static unsigned long arch_flags_to_pte(unsigned long flags)
 {
     unsigned long pte = PTE_PAGE | PTE_AF | PTE_SH_IS | PTE_ATTR_NI;
     if (flags & ARCH_MMU_U)  pte |= PTE_USER;
     if (!(flags & ARCH_MMU_W)) pte |= PTE_RO;
     if (!(flags & ARCH_MMU_X)) pte |= PTE_XN | PTE_PXN;
     return pte;
 }
 
 /* Walk / allocate page table levels for @virt, return leaf PTE pointer */
 static unsigned long *arch_pte_walk(unsigned long virt, int alloc)
 {
     unsigned long pgd_phys;
     __asm__ volatile("mrs %0, ttbr0_el1" : "=r"(pgd_phys));
 
     unsigned long *pgd = (unsigned long *)pgd_phys;
     unsigned int  l0   = (unsigned int)((virt >> 39u) & 0x1FFu);
     unsigned int  l1   = (unsigned int)((virt >> 30u) & 0x1FFu);
     unsigned int  l2   = (unsigned int)((virt >> 21u) & 0x1FFu);
     unsigned int  l3   = (unsigned int)((virt >> 12u) & 0x1FFu);
 
 #define NEXT_TABLE(parent, idx) ({                              \
     unsigned long _e = (parent)[(idx)];                         \
     unsigned long *_t;                                          \
     if (!(_e & PTE_VALID)) {                                    \
         if (!alloc) return (unsigned long *)0;                  \
         unsigned long _p = phys_alloc_page();                   \
         if (!_p) return (unsigned long *)0;                     \
         (parent)[(idx)] = _p | PTE_TABLE | PTE_VALID;          \
         _t = (unsigned long *)_p;                               \
     } else {                                                    \
         _t = (unsigned long *)(_e & ~0xFFFUL);                 \
     }                                                           \
     _t; })
 
     unsigned long *l1t = NEXT_TABLE(pgd, l0);
     if (!l1t) return (unsigned long *)0;
     unsigned long *l2t = NEXT_TABLE(l1t, l1);
     if (!l2t) return (unsigned long *)0;
     unsigned long *l3t = NEXT_TABLE(l2t, l2);
     if (!l3t) return (unsigned long *)0;
     return &l3t[l3];
 #undef NEXT_TABLE
 }
 
 int arch_mmu_map(unsigned long virt, unsigned long phys,
                   unsigned long flags)
 {
     unsigned long *pte = arch_pte_walk(virt, 1);
     if (!pte) return -1;
     *pte = (phys & ~0xFFFUL) | arch_flags_to_pte(flags);
     arch_tlb_flush(virt);
     return 0;
 }
 
 void arch_mmu_unmap(unsigned long virt)
 {
     unsigned long *pte = arch_pte_walk(virt, 0);
     if (pte) { *pte = 0UL; arch_tlb_flush(virt); }
 }
 
 void arch_tlb_flush(unsigned long virt)
 {
     arch_dsb_sy();
     arch_tlbi_vaae1is(virt >> 12u);
     arch_dsb_sy();
     arch_isb();
 }
 
 void arch_tlb_flush_all(void)
 {
     arch_dsb_sy();
     arch_tlbi_vmalle1is();
     arch_dsb_sy();
     arch_isb();
 }
 
 void arch_mmu_switch(unsigned long pgd_phys)
 {
     arch_dsb_sy();
     __asm__ volatile("msr ttbr0_el1, %0" :: "r"(pgd_phys) : "memory");
     arch_isb();
     arch_tlb_flush_all();
 }
 
 /* =========================================================================
  * Atomic operations  (ISA: LDXR/STXR — ARMv8.0-A)
  * ====================================================================== */
 
 long arch_atomic_add(long *ptr, long val)
 {
     long old, tmp;
     unsigned int fail;
     __asm__ volatile(
         "1: ldxr  %0,       [%3]    \n"
         "   add   %1, %0,   %4      \n"
         "   stxr  %w2, %1,  [%3]   \n"
         "   cbnz  %w2, 1b           \n"
         : "=&r"(old), "=&r"(tmp), "=&r"(fail)
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
     long actual;
     unsigned int fail;
     __asm__ volatile(
         "1: ldxr  %0,      [%3]     \n"
         "   cmp   %0,      %4       \n"
         "   b.ne  2f                \n"
         "   stxr  %w1, %5, [%3]    \n"
         "   cbnz  %w1, 1b           \n"
         "2:                         \n"
         : "=&r"(actual), "=&r"(fail)
         : "r"(ptr), "r"(ptr), "r"(expected), "r"(desired)
         : "memory", "cc");
     return (actual == expected) ? 1 : 0;
 }
 
 long arch_atomic_load(const long *ptr)
 {
     long v;
     __asm__ volatile("ldar %0, [%1]" : "=r"(v) : "r"(ptr) : "memory");
     return v;
 }
 
 void arch_atomic_store(long *ptr, long val)
 {
     __asm__ volatile("stlr %0, [%1]" :: "r"(val), "r"(ptr) : "memory");
 }
 
 /* =========================================================================
  * Spinlock  (ISA: LDAXR/STLXR + WFE/SEV)
  * ====================================================================== */
 
 void arch_spin_lock(arch_spinlock_t *sl)
 {
     long tmp;
     unsigned int fail;
     __asm__ volatile(
         "1: ldaxr  %w0,      [%2]   \n"
         "   cbnz   %w0,      2f     \n"
         "   stxr   %w1, %w3, [%2]  \n"
         "   cbnz   %w1,      1b     \n"
         "   b      3f                \n"
         "2: wfe                     \n"
         "   b      1b                \n"
         "3:                         \n"
         : "=&r"(tmp), "=&r"(fail)
         : "r"(&sl->lock), "r"(1L)
         : "memory");
 }
 
 void arch_spin_unlock(arch_spinlock_t *sl)
 {
     __asm__ volatile("stlr %w0, [%1]" :: "r"(0), "r"(&sl->lock) : "memory");
     __asm__ volatile("sev" ::: "memory");
 }
 
 int arch_spin_trylock(arch_spinlock_t *sl)
 {
     long old; unsigned int fail;
     __asm__ volatile(
         "ldaxr  %w0,      [%2]  \n"
         "cbnz   %w0,      1f    \n"
         "stxr   %w1, %w3, [%2] \n"
         "1:                     \n"
         : "=&r"(old), "=&r"(fail)
         : "r"(&sl->lock), "r"(1L)
         : "memory");
     return (old == 0 && fail == 0) ? 1 : 0;
 }
 