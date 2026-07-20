10_Arch/
├── Makefile                  ← build all four targets
├── uiox_arch_main.c          ← single kernel entry (parallel to Makefile)
├── uiox_arch_main.h          ← header (parallel to Makefile)
├── arm64/
│   ├── include/arch_defs.h
│   ├── include/arch_runtime.h
│   └── src/
│       ├── arch_init.c
│       ├── arch_runtime.c
│       ├── arch_context.S
│       └── arch_irq.S
├── arm32/   (same structure)
├── x86_64/  (same structure)
└── riscv64/ (same structure)
-----------------
/* 50_UIX/kernel/uiox_kernel_main.c */

#include "../../10_Arch/src/uiox_arch_main.h"
#include "../../33_ProcessControlSubsystem/include/proc.h"
#include "../../50_UIX/01_shell/shell.h"

void uiox_kernel_main(unsigned long dtb_pa)
{
    /* Step 1: arch_init() + uiox_soc_init() — single call */
    if (uiox_arch_main(dtb_pa) != 0)
        for (;;) ;   /* fatal — no recovery possible */

    /* Step 2: kernel subsystems */
    uiox_ks_boot_entry();   /* 12_ksign  */
    uiox_proc_init();        /* 33_PCS   */
    uiox_shell_start();      /* 50_UIX   */
}
