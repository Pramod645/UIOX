/*
 * boot.c  —  x86_64 C-level boot entry point.
 *
 * Called from boot.S after long mode is established.
 * Initialises all arch subsystems then calls kernel_main().
 *
 * Mirrors: 10_Arch/arm32/src/boot.c
 */

 #include "../include/arch.h"
 #include <stdint.h>
 
 /* declared in each subsystem's .c */
 extern void gdt_init(void);
 extern void idt_init(void);
 extern void cpu_init(void);
 extern void mmu_init(void);
 
 /* kernel main — defined outside the arch layer */
 extern void kernel_main(uint64_t multiboot_info);
 
 /*
  * boot_main()
  *
  * Called from boot.S long_mode_start with:
  *   rdi = multiboot2 info physical address
  *
  * Sequence:
  *   1. GDT — reload segment registers, set up TSS
  *   2. IDT — install exception + IRQ stubs
  *   3. CPU — enable SSE, NX, identify vendor
  *   4. MMU — enable PGE, set up kernel direct map
  *   5. kernel_main()
  */
 static void boot_main(uint64_t multiboot_info)
 {
     /* Step 1: GDT + TSS */
     gdt_init();
 
     /* Step 2: IDT + PIC remap */
     idt_init();
 
     /* Step 3: CPU features */
     cpu_init();
 
     /* Step 4: MMU extras (PGE, etc.) */
     mmu_init();
 
     /* Step 5: hand off to the portable kernel */
     kernel_main(multiboot_info);
 
     /* should never reach here */
     for (;;)
         arch_hlt();
 }
 