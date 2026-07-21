/*
 * 10_BSP/uiox_arch_main.c
 *
 * Single entry point — identical in both static and dynamic builds.
 * The static/dynamic switch is entirely inside uiox_soc_main.c Stage 8.
 */

 #include "uiox_arch_main.h"
 #include "./soc/include/uiox_soc_stdio.h"
 
 /* ── Provided by this arch's arch_init.c ─────────────────────────── */
 extern int arch_init(void);
 
 /* ── Provided by 10_BSP/soc/src/uiox_soc_main.c ─────────────────── */
 extern int uiox_soc_init(void);
 
 /* ── DTB physical address ────────────────────────────────────────── */
 unsigned long uiox_arch_dtb_pa __attribute__((weak)) = 0UL;
 
 /* ================================================================== */
 int uiox_arch_main(unsigned long dtb_pa)
 {
     int rc;
 
     /* Stage 1 — ISA init (cache, GIC/APIC/PLIC, VBAR/stvec/IDT) */
     rc = arch_init();
     if (rc != 0) {
         uiox_soc_puts("[arch_main] FATAL: arch_init failed\n");
         return rc;
     }
 
     /*
      * Stage 2 — SoC pipeline (Stages 0a–8).
      *
      * Stage 8 behaviour depends on the compile-time flag:
      *   Static  → calls extern uiox_kernel_main(dtb_pa)  — returns
      *   Dynamic → calls uiox_kernel_load/verify/jump()   — never returns
      *
      * The switch is inside uiox_soc_main.c, not here.
      * This function is identical in both builds.
      */
     rc = uiox_soc_init();
     if (rc != 0) {
         uiox_soc_puts("[arch_main] FATAL: uiox_soc_init failed\n");
         return rc;
     }
 
     uiox_arch_dtb_pa = dtb_pa;
     uiox_soc_puts("[arch_main] complete\n");
     return 0;
 }
 
 unsigned long uiox_arch_dtb_get(void)
 {
     return uiox_arch_dtb_pa;
 }
 
 /* ── Weak stubs — only needed in dynamic builds ──────────────────── */
 #if defined(UIOX_DYNAMIC_KERNEL_LOAD)
 
 void __attribute__((weak, noreturn))
 uiox_kernel_main(unsigned long dtb_pa)
 {
     (void)dtb_pa;
     for (;;) __asm__ volatile("" ::: "memory");
 }
 
 long __attribute__((weak))
 syscall_dispatch(unsigned long nr,
                   unsigned long a0, unsigned long a1,
                   unsigned long a2, unsigned long a3,
                   unsigned long a4, unsigned long a5)
 {
     (void)nr;(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;
     return -1L;
 }
 
 void __attribute__((weak))
 exception_dispatch(unsigned long cause,
                     unsigned long tval, void *frame)
 {
     (void)cause;(void)tval;(void)frame;
 }
 
 unsigned long __attribute__((weak)) phys_alloc_page(void) { return 0UL; }
 
 #endif /* UIOX_DYNAMIC_KERNEL_LOAD */
 