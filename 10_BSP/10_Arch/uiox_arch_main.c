/*
 * 10_Arch/uiox_arch_main.c
 *
 * Unified architecture + SoC entry point.
 *
 * This file lives at the top of 10_Arch/ (parallel to the Makefile)
 * and is compiled into every architecture library:
 *
 *   libarch_arm64.a   — compiled with -DARCH_ARM64
 *   libarch_arm32.a   — compiled with -DARCH_ARM32
 *   libarch_x86_64.a  — compiled with -DARCH_X86_64
 *   libarch_riscv64.a — compiled with -DARCH_RISCV64
 *
 * The kernel calls uiox_arch_main(dtb_pa) exactly once.
 * This function calls:
 *   1. arch_init()      — ISA-level setup (supplied by arch_init.c)
 *   2. uiox_soc_init()  — SoC-level setup (supplied by 03_SoC)
 *
 * After uiox_arch_main() returns successfully the kernel may proceed
 * with process init, filesystem mount, and shell startup.
 */

 #include "uiox_arch_main.h"
 #include "../03_SoC/include/uiox_soc_stdio.h"  /* uiox_soc_puts() */
 
 /* ── arch_init() — provided by this arch's arch_init.c ─────────────── */
 extern int arch_init(void);
 
 /* ── uiox_soc_init() — provided by 03_SoC ──────────────────────────── */
 extern int uiox_soc_init(void);
 
 /* ── DTB physical address — weak so the kernel can override ─────────── */
 unsigned long uiox_arch_dtb_pa __attribute__((weak)) = 0UL;
 
 /* =========================================================================
  * uiox_arch_main
  * ====================================================================== */
 int uiox_arch_main(unsigned long dtb_pa)
 {
     int rc;
 
     /* ── Stage 1: Architecture ISA initialisation ──────────────────────
      *
      * arch_init() is resolved at link time to the correct implementation:
      *   -DARCH_ARM64   → arm64/src/arch_init.c
      *   -DARCH_ARM32   → arm32/src/arch_init.c
      *   -DARCH_X86_64  → x86_64/src/arch_init.c
      *   -DARCH_RISCV64 → riscv64/src/arch_init.c
      *
      * What it does (ISA-level only):
      *   • CPU identification (MIDR / CPUID / misa)
      *   • I/D cache enable
      *   • GIC-400 / APIC / PLIC distributor + CPU interface
      *   • Exception vector table install (VBAR_EL1 / IDT / stvec)
      *   • Generic timer / CLINT / HPET start at 100 Hz
      *   • IRQ handler registration
      *   • Global interrupt unmask
      * ────────────────────────────────────────────────────────────────── */
     rc = arch_init();
     if (rc != 0) {
         uiox_soc_puts("[arch_main] FATAL: arch_init() failed\n");
         return rc;
     }
 
     /* ── Stage 2: SoC initialisation ────────────────────────────────────
      *
      * uiox_soc_init() lives in 03_SoC and dispatches at runtime to the
      * correct chip backend using MIDR / mvendorid / CPUID detection:
      *   QEMU virt A64 / BCM2711 / BCM2712 / IMX8MP / RK3588
      *   QEMU virt A32 / BCM2836 / IMX6Q / OMAP4430
      *   QEMU Q35 / x86 generic
      *   QEMU virt RV64 / SiFive U74 / TH1520
      *
      * What it does (SoC peripherals):
      *   • UART baud rate programming
      *   • Clock PLL / CCM setup
      *   • Power domain enable
      *   • Memory map build
      *   • POST (Power-On Self Test)
      *   • Secure boot chain verification
      *   • DMA controller init
      *   • PCIe ECAM scan and BAR assignment
      * ────────────────────────────────────────────────────────────────── */
     rc = uiox_soc_init();
     if (rc != 0) {
         uiox_soc_puts("[arch_main] FATAL: uiox_soc_init() failed\n");
         return rc;
     }
 
     /* Store DTB PA for higher kernel layers */
     uiox_arch_dtb_pa = dtb_pa;
 
     uiox_soc_puts("[arch_main] arch + SoC init complete\n");
     return 0;
 }
 
 /* =========================================================================
  * uiox_arch_dtb_get — accessor for kernel layers above this one
  * ====================================================================== */
 unsigned long uiox_arch_dtb_get(void)
 {
     return uiox_arch_dtb_pa;
 }
 