/*
 * 10_Arch/x86_64/src/arch_runtime.c
 * AMD64 runtime services: IRQ/exception dispatch, MMU (4-level paging),
 * atomics (LOCK prefix), spinlocks (LOCK XCHG), IRQ save/restore.
 */

 #include "arch_defs.h"
 #include "arch_runtime.h"
 #include "../../../03_SoC/include/uiox_soc_stdio.h"
 
 /* ── External C dispatchers ──────────────────────────────────────── */
 extern void irq_dispatch      (unsigned int irq);
 extern long syscall_dispatch  (unsigned long nr,
                                 unsigned long a0, unsigned long a1,
                                 unsigned long a2, unsigned long a3,
                                 unsigned long a4, unsigned long a5);
 extern void exception_dispatch(unsigned long vector,
                                 unsigned long error,
                                 void *frame);
 
 /* Called from arch_irq.S common handler */
 void x86_irq_dispatch(unsigned long vector,
                         unsigned long error,
                         void *frame)
 {
     (void)error; (void)frame;
     /* Hardware IRQ: vector 32-47 = PIC-remapped IRQ 0-15 */
     irq_dispatch((unsigned int)(vector - 32u));
     /* Send EOI to LAPIC */
     mmio_write32((unsigned long)(LAPIC_BASE + 0x0B0u), 0u);
 }
 
 void x86_exception_dispatch(unsigned long vector,
                               unsigned long error,
                               void *frame)
 {
     exception_dispatch(vector, error, frame);
 }
 
 long arch_syscall_dispatch(unsigned long nr,
                              unsigned long a0, unsigned long a1,
                              unsigned long a2, unsigned long a3,
                              unsigned long a4, unsigned long a5)
 {
     return syscall_dispatch(nr, a0, a1, a2, a3, a4, a5);
 }
 
 /* =========================================================================
  * IRQ save / restore  (RFLAGS.IF via CLI/STI)
  * ====================================================================== */
 unsigned long arch_irq_save(void)
 {
     unsigned long flags;
     __asm__ volatile("pushfq; popq %0" : "=r"(flags));
     __asm__ volatile("cli" ::: "memory");
     return flags;
 }
 
 void arch_irq_restore(unsigned long flags)
 {
     __asm__ volatile("pushq %0; popfq" :: "r"(flags) : "memory", "cc");
 }
 
 /* =========================================================================
  * MMU — x86_64 four-level paging (PML4 → PDPT → PD → PT, 4KB pages)
  *
  * Page table entry flags (Intel SDM Vol. 3A, §4.5)
  * ====================================================================== */
 extern unsigned long phys_alloc_page(void);   /* 32_FileSystem mm      */
 
 #define PTE_P     (1UL << 0)   /* Present                              */
 #define PTE_RW    (1UL << 1)   /* Read/Write                           */
 #define PTE_US    (1UL << 2)   /* User/Supervisor                      */
 #define PTE_PWT   (1UL << 3)   /* Page Write-Through                   */
 #define PTE_PCD   (1UL << 4)   /* Page Cache Disable                   */
 #define PTE_A     (1UL << 5)   /* Accessed                             */
 #define PTE_D     (1UL << 6)   /* Dirty (leaf only)                    */
 #define PTE_XD    (1UL << 63)  /* Execute-Disable (NX)                 */
 #define PTE_ADDR  0x000FFFFFFFFFF000UL
 
 static unsigned long x86_flags_to_pte(unsigned long flags)
 {
     unsigned long pte = PTE_P | PTE_A;
     if (flags & ARCH_MMU_W) pte |= PTE_RW;
     if (flags & ARCH_MMU_U) pte |= PTE_US;
     if (!(flags & ARCH_MMU_X)) pte |= PTE_XD;
     if (flags & ARCH_MMU_NC) pte |= PTE_PWT | PTE_PCD;
     return pte;
 }
 
 /* Walk (and optionally allocate) the four page table levels            */
 static unsigned long *x86_pte_walk(unsigned long virt, int alloc)
 {
     /* PML4 base from CR3 */
     unsigned long cr3;
     __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
     unsigned long *pml4 = (unsigned long *)(cr3 & PTE_ADDR);
 
     unsigned int i3 = (unsigned int)((virt >> 39u) & 0x1FFu); /* PML4  */
     unsigned int i2 = (unsigned int)((virt >> 30u) & 0x1FFu); /* PDPT  */
     unsigned int i1 = (unsigned int)((virt >> 21u) & 0x1FFu); /* PD    */
     unsigned int i0 = (unsigned int)((virt >> 12u) & 0x1FFu); /* PT    */
 
 #define DESCEND(tbl, idx) ({                                        \
     unsigned long _e = (tbl)[(idx)];                                \
     unsigned long *_next;                                            \
     if (!(_e & PTE_P)) {                                            \
         if (!alloc) return (unsigned long *)0;                      \
         unsigned long _p = phys_alloc_page();                       \
         if (!_p) return (unsigned long *)0;                         \
         (tbl)[(idx)] = _p | PTE_P | PTE_RW | PTE_US;               \
         _next = (unsigned long *)_p;                                \
     } else {                                                        \
         _next = (unsigned long *)(_e & PTE_ADDR);                   \
     }                                                               \
     _next; })
 
     unsigned long *pdpt = DESCEND(pml4, i3);
     if (!pdpt) return (unsigned long *)0;
     unsigned long *pd   = DESCEND(pdpt, i2);
     if (!pd)   return (unsigned long *)0;
     unsigned long *pt   = DESCEND(pd,   i1);
     if (!pt)   return (unsigned long *)0;
     return &pt[i0];
 #undef DESCEND
 }
 
 int arch_mmu_map(unsigned long virt, unsigned long phys,
                   unsigned long flags)
 {
     unsigned long *pte = x86_pte_walk(virt, 1);
     if (!pte) return -1;
     *pte = (phys & PTE_ADDR) | x86_flags_to_pte(flags) | PTE_D;
     arch_tlb_flush(virt);
     return 0;
 }
 
 void arch_mmu_unmap(unsigned long virt)
 {
     unsigned long *pte = x86_pte_walk(virt, 0);
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
 
 void arch_mmu_switch(unsigned long pml4_phys)
 {
     __asm__ volatile("mov %0, %%cr3" :: "r"(pml4_phys) : "memory");
 }
 
 /* =========================================================================
  * Atomic operations  (LOCK prefix — x86_64 native)
  * ====================================================================== */
 long arch_atomic_add(long *ptr, long val)
 {
     long old = val;
     __asm__ volatile(
         "lock xaddq %0, (%1)"
         : "+r"(old)
         : "r"(ptr)
         : "memory");
     return old;
 }
 
 long arch_atomic_sub(long *ptr, long val)
 {
     return arch_atomic_add(ptr, -val);
 }
 
 int arch_atomic_cas(long *ptr, long expected, long desired)
 {
     long prev;
     __asm__ volatile(
         "lock cmpxchgq %2, (%3)"
         : "=a"(prev)
         : "a"(expected), "r"(desired), "r"(ptr)
         : "memory");
     return (prev == expected) ? 1 : 0;
 }
 
 long arch_atomic_load(const long *ptr)
 {
     long v;
     __asm__ volatile("movq (%1), %0" : "=r"(v) : "r"(ptr) : "memory");
     return v;
 }
 
 void arch_atomic_store(long *ptr, long val)
 {
     __asm__ volatile(
         "lock xchgq %0, (%1)"
         :: "r"(val), "r"(ptr)
         : "memory");
 }
/* =========================================================================
 * Spinlock  (LOCK XCHG — always has implicit LOCK prefix on x86)
 * ====================================================================== */

 void arch_spin_lock(arch_spinlock_t *sl)
 {
     long val = 1L;
     __asm__ volatile(
         "1: lock xchgq %0, (%1)     \n"
         "   test   %0,   %0         \n"
         "   jz     2f               \n"
         "   pause                   \n"
         "   jmp    1b               \n"
         "2:                         \n"
         : "+r"(val)
         : "r"(&sl->lock)
         : "memory");
 }
 
 void arch_spin_unlock(arch_spinlock_t *sl)
 {
     __asm__ volatile(
         "movq $0, (%0)"
         :: "r"(&sl->lock)
         : "memory");
 }
 
 int arch_spin_trylock(arch_spinlock_t *sl)
 {
     long old = 1L;
     __asm__ volatile(
         "lock xchgq %0, (%1)"
         : "+r"(old)
         : "r"(&sl->lock)
         : "memory");
     return (old == 0L) ? 1 : 0;
 }
  