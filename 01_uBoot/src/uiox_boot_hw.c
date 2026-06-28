/**
 * @file  uiox_boot_hw.c
 * @brief UIOX Bootloader — HAL ops table registration and wrappers.
 * @date  2026-06-12
 */

 #include "uiox_boot.h"

 static const uiox_boot_hw_ops_t *s_ops = NULL;
 
 void uiox_boot_hw_register(const uiox_boot_hw_ops_t *ops)
 {
     s_ops = ops;
     if (ops && ops->init)
         ops->init();
 }
 
 const uiox_boot_hw_ops_t *uiox_boot_hw_ops(void) { return s_ops; }
 
 void uiox_boot_hw_init(void)
 {
     /* For architectures that call hw_register() directly from their
      * entry point, this is a no-op. For x86 the entry point calls
      * uiox_boot_hw_x86_register() which calls init() via register(). */
 }
 
 void uiox_boot_hw_uart_putc(char c)
 { if (s_ops && s_ops->uart_putc) s_ops->uart_putc(c); }
 
 void uiox_boot_hw_dcache_flush(uintptr_t start, size_t len)
 { if (s_ops && s_ops->dcache_flush) s_ops->dcache_flush(start, len); }
 
 void uiox_boot_hw_icache_inv(void)
 { if (s_ops && s_ops->icache_inv) s_ops->icache_inv(); }
 
 uint64_t uiox_boot_hw_get_ticks(void)
 { return s_ops && s_ops->get_ticks ? s_ops->get_ticks() : 0u; }
 
 void uiox_boot_hw_udelay(uint32_t us)
 { if (s_ops && s_ops->udelay) s_ops->udelay(us); }
 
 void __attribute__((noreturn)) uiox_boot_hw_reset(void)
 {
     if (s_ops && s_ops->reset)
         s_ops->reset();
     for (;;) ;
 }
 
 void uiox_boot_hw_barrier(void)
 { if (s_ops && s_ops->barrier) s_ops->barrier(); }
 