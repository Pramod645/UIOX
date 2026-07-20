#ifndef UIOX_ARCH_MAIN_H
#define UIOX_ARCH_MAIN_H

/*
 * 10_Arch/uiox_arch_main.h
 *
 * Single header the kernel includes to call the unified arch entry point.
 *
 * File location:
 *   10_Arch/uiox_arch_main.h    ← this file
 *   10_Arch/uiox_arch_main.c    ← implementation
 *   10_Arch/Makefile             ← builds it into every arch library
 *
 * Kernel usage:
 *
 *   #include "../../10_Arch/uiox_arch_main.h"
 *
 *   void uiox_kernel_main(unsigned long dtb_pa)
 *   {
 *       if (uiox_arch_main(dtb_pa) != 0)
 *           for (;;) ;           // fatal
 *       uiox_ks_boot_entry();    // 12_ksign
 *       uiox_proc_init();        // 33_PCS
 *       uiox_shell_start();      // 50_UIX/01_shell
 *   }
 *
 * Kernel call chain wired inside uiox_arch_main():
 *   uiox_arch_main(dtb_pa)
 *       ├── arch_init()       ← 10_Arch/<arch>/src/arch_init.c
 *       │       (GIC/APIC/PLIC, cache, VBAR/stvec/IDT, generic timer)
 *       └── uiox_soc_init()   ← 03_SoC/src/uiox_soc_<arch>.c
 *               (UART baud, clocks, power, PCIe, POST, secboot)
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Unified arch + SoC entry point for the kernel.
 *
 *         Calls arch_init() then uiox_soc_init() in the correct order.
 *         Must be the first call in uiox_kernel_main() after early
 *         stack / BSS setup.
 *
 * @param  dtb_pa  Physical address of the Device Tree Blob.
 *                 Pass 0 if no DTB is available.
 * @return 0 on success, negative value on fatal failure.
 */
int uiox_arch_main(unsigned long dtb_pa);

/**
 * @brief  Return the DTB physical address stored during uiox_arch_main().
 *         Safe to call from any kernel layer after uiox_arch_main() returns.
 */
unsigned long uiox_arch_dtb_get(void);

/**
 * Weak global — readable directly by any translation unit that includes
 * this header.  The kernel may override it with a strong definition.
 */
extern unsigned long uiox_arch_dtb_pa;

#ifdef __cplusplus
}
#endif
#endif /* UIOX_ARCH_MAIN_H */
