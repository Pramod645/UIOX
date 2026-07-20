/*
 * 10_Arch/arm32/src/arch_runtime.c
 * ARMv7-A runtime services: IRQ dispatch, MMU (short-descriptor),
 * atomics (LDREX/STREX), spinlocks, IRQ save/restore.
 */

 #include "arch_defs.h"
 #include "arch_runtime.h"
 #include "../../../03_SoC/include/uiox_soc_stdio.h"
 
 /* ── External C dispatchers ──────────────────────────────────────── */
 extern void irq_dispatch      (unsigned int irq);
 extern long syscall_dispatch  (unsigned int nr,
                                 unsigned int a0, unsigned int a1,
                                 unsigned int a2, unsigned int a3,
                                 unsigned int a4, unsigned int a5);
 extern void exception_dispatch(unsigned int cause, unsigned int addr);
 
 void arch_irq_dispatch(unsigned int irq)
 {
     if (irq >= 1020u) return;   /* GIC spurious range               */
     irq_dispatch(irq);
 }
 
 void arm32_exception_dispatch(unsigned int cause, unsigned int addr)
 {
     exception_dispatch(cause, addr);
 }
 
 long arch_syscall_dispatch(unsigned int nr,
                              unsigned int a0, unsigned int a1,
                              unsigned int a2, unsigned int a3,
                              unsigned int a4, unsigned int a5)
 {
     return syscall_dispatch(nr, a0, a1, a2, a3, a4, a5);
 }
 
 /* =========================================================================
  * IRQ save / restore  (CPSR.I via MRS / MSR)
  * ====================================================================== */
 unsigned int arch_irq_save(void)
 {
     unsigned int flags;
     __asm__ volatile("mrs %0, cpsr" : "=r"(flags));
     __asm__ volatile("cpsid i" ::: "memory");
     return flags;
 }
 
 void arch_irq_restore(unsigned int flags)
 {
     __asm__ volatile("msr cpsr_c, %0" :: "r"(flags) : "memory");
 }
 
 /* =========================================================================
  * MMU — ARMv7-A short-descriptor (two-level, 4KB pages)
  *
  * Level-1 table: 4096 × 4-byte entries, covers 4 GB at 1 MB/entry
  * Level-2 table:  256 × 4-byte entries, covers 1 MB at 4 KB/entry
  *
  * We only map individual 4KB pages (fine-grain mapping).
  * ====================================================================== */
 extern unsigned int phys_alloc_page(void);   /* 32_FileSystem mm      */
 
 /* Short-descriptor PTE flags */
 #define L1_FAULT    0x0u
 #define L1_COARSE   0x1u                     /* points to L2 table    */
 #define L2_FAULT    0x0u
 #define L2_SMALL    0x2u                     /* small page (4KB)      */
 #define L2_AP_RW_S  (0x1u << 4)             /* SVC r/w, usr no access */
 #define L2_AP_RW_U  (0x3u << 4)             /* SVC r/w, usr r/w       */
 #define L2_AP_RO_S  (0x2u << 4)             /* SVC r/o                */
 #define L2_XN       (1u << 0)               /* execute-never (b0 of SP)*/
 #define L2_B        (1u << 2)               /* bufferable             */
 #define L2_C        (1u << 3)               /* cacheable              */
 #define L2_TEX0     (1u << 6)               /* TEX[0]                 */
 
 static unsigned int arm32_flags_to_pte(unsigned int flags)
 {
     unsigned int pte = L2_SMALL | L2_B | L2_C | L2_TEX0;  /* WB alloc */
     if (flags & ARCH_MMU_U)
         pte |= L2_AP_RW_U;
     else
         pte |= L2_AP_RW_S;
     if (!(flags & ARCH_MMU_W))
         pte = (pte & ~(3u << 4)) | L2_AP_RO_S;
     if (!(flags & ARCH_MMU_X))
         pte |= L2_XN;
     return pte;
 }
 
 /* Get or allocate the L2 table for @virt's 1 MB section */
 static unsigned int *arm32_l2_get(unsigned int virt, int alloc)
 {
     /* L1 base from CP15 TTBR0 */
     unsigned int ttbr0;
     __asm__ volatile("mrc p15,0,%0,c2,c0,0" : "=r"(ttbr0));
     unsigned int *l1 = (unsigned int *)(ttbr0 & ~0x3FFFu);
 
     unsigned int l1_idx = virt >> 20u;
     unsigned int l1e    = l1[l1_idx];
 
     if ((l1e & 0x3u) != L1_COARSE) {
         if (!alloc) return (unsigned int *)0;
         unsigned int l2_phys = phys_alloc_page();
         if (!l2_phys) return (unsigned int *)0;
         /* Zero the 1KB L2 table (256 entries × 4 bytes) */
         unsigned int *p = (unsigned int *)l2_phys;
         for (int i = 0; i < 256; i++) p[i] = 0u;
         l1[l1_idx] = (l2_phys & ~0x3FFu) | L1_COARSE;
         arch_dsb_sy();
         return p;
     }
     return (unsigned int *)(l1e & ~0x3FFu);
 }
 
 int arch_mmu_map(unsigned int virt, unsigned int phys, unsigned int flags)
 {
     unsigned int *l2 = arm32_l2_get(virt, 1);
     if (!l2) return -1;
     unsigned int l2_idx = (virt >> 12u) & 0xFFu;
     l2[l2_idx] = (phys & ~0xFFFu) | arm32_flags_to_pte(flags);
     arch_dsb_sy();
     arch_tlb_flush(virt);
     return 0;
 }
 
 void arch_mmu_unmap(unsigned int virt)
 {
     unsigned int *l2 = arm32_l2_get(virt, 0);
     if (!l2) return;
     unsigned int l2_idx = (virt >> 12u) & 0xFFu;
     l2[l2_idx] = L2_FAULT;
     arch_dsb_sy();
     arch_tlb_flush(virt);
 }
 
 void arch_tlb_flush(unsigned int virt)
 {
     arch_dsb_sy();
     arch_tlbi_mva(virt);
     arch_dsb_sy();
     arch_isb();
 }
 
 void arch_tlb_flush_all(void)
 {
     arch_dsb_sy();
     arch_tlbi_all();
     arch_dsb_sy();
     arch_isb();
 }
 
 void arch_mmu_switch(unsigned int pgd_phys)
 {
     arch_dsb_sy();
     /* Write TTBR0 */
     __asm__ volatile("mcr p15,0,%0,c2,c0,0" :: "r"(pgd_phys) : "memory");
     arch_isb();
     arch_tlb_flush_all();
 }
 
 /* =========================================================================
  * Atomic operations  (ARMv7-A LDREX/STREX)
  * ====================================================================== */
 int arch_atomic_add(int *ptr, int val)
 {
     int old, tmp; unsigned int fail;
     __asm__ volatile(
         "1: ldrex  %0,        [%3]   \n"
         "   add    %1, %0,    %4     \n"
         "   strex  %2, %1,    [%3]  \n"
         "   cmp    %2, #0            \n"
         "   bne    1b                \n"
         : "=&r"(old), "=&r"(tmp), "=&r"(fail)
         : "r"(ptr), "r"(val)
         : "memory", "cc");
     return old;
 }
 
 int arch_atomic_sub(int *ptr, int val) { return arch_atomic_add(ptr, -val); }
 
 int arch_atomic_cas(int *ptr, int expected, int desired)
 {
     int actual; unsigned int fail;
     __asm__ volatile(
         "1: ldrex  %0,     [%3]     \n"
         "   cmp    %0,     %4       \n"
         "   bne    2f               \n"
         "   strex  %1, %5, [%3]    \n"
         "   cmp    %1, #0           \n"
         "   bne    1b               \n"
         "2:                         \n"
         : "=&r"(actual), "=&r"(fail)
         : "r"(ptr), "r"(ptr), "r"(expected), "r"(desired)
         : "memory", "cc");
     return (actual == expected) ? 1 : 0;
 }
 
 int arch_atomic_load(const int *ptr)
 {
     int v;
     __asm__ volatile("ldr %0, [%1]" : "=r"(v) : "r"(ptr) : "memory");
     __asm__ volatile("dmb" ::: "memory");
     return v;
 }
 
 void arch_atomic_store(int *ptr, int val)
 {
     __asm__ volatile("dmb" ::: "memory");
     __asm__ volatile("str %0, [%1]" :: "r"(val), "r"(ptr) : "memory");
 }
 
 /* =========================================================================
  * Spinlock  (LDREX/STREX + WFE/SEV)
  * ====================================================================== */
 void arch_spin_lock(arch_spinlock_t *sl)
 {
     int tmp; unsigned int fail;
     __asm__ volatile(
         "1: ldrex  %0,     [%2]     \n"
         "   cmp    %0,     #0       \n"
         "   wfene                   \n"
         "   bne    1b               \n"
         "   strex  %1, %3, [%2]    \n"
         "   cmp    %1, #0           \n"
         "   bne    1b               \n"
         "   dmb                     \n"
         : "=&r"(tmp), "=&r"(fail)
         : "r"(&sl->lock), "r"(1)
         : "memory", "cc");
 }
 
 void arch_spin_unlock(arch_spinlock_t *sl)
 {
     __asm__ volatile("dmb" ::: "memory");
     __asm__ volatile("str %0, [%1]" :: "r"(0), "r"(&sl->lock) : "memory");
     __asm__ volatile("sev" ::: "memory");
 }
 
 int arch_spin_trylock(arch_spinlock_t *sl)
 {
     int old; unsigned int fail;
     __asm__ volatile(
         "ldrex  %0,     [%2]    \n"
         "cmp    %0,     #0      \n"
         "bne    1f              \n"
         "strex  %1, %3, [%2]   \n"
         "dmb                    \n"
         "1:                     \n"
         : "=&r"(old), "=&r"(fail)
         : "r"(&sl->lock), "r"(1)
         : "memory", "cc");
         return (old == 0 && fail == 0) ? 1 : 0;
        }
        
 