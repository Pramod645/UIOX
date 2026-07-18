/**
 * @file    uiox_soc_hw.c
 * @brief   UIOX SoC — HW HAL ops registration and wrappers.
 * @date    2026-06-21
 */

 #include "uiox_soc_hw.h"

 static const uiox_soc_hw_ops_t *s_ops  = NULL;
 static uiox_soc_platform_t     *s_plat = NULL;
 
 void uiox_soc_hw_register(const uiox_soc_hw_ops_t *ops,
                             uiox_soc_platform_t    *plat)
 {
     s_ops  = ops;
     s_plat = plat;
 }
 
 void uiox_soc_hw_barrier(void)
 {
     if (s_ops && s_ops->barrier_dsb) s_ops->barrier_dsb();
     if (s_ops && s_ops->barrier_isb) s_ops->barrier_isb();
 }
 
 const uiox_soc_hw_ops_t *uiox_soc_hw_ops(void)      { return s_ops;  }
 uiox_soc_platform_t     *uiox_soc_hw_platform(void) { return s_plat; }
 
 uiox_soc_err_t uiox_soc_hw_init(void)
 {
     if (!s_ops || !s_ops->init) return UIOX_SOC_ERR_NODEV;
     return s_ops->init(s_plat);
 }
 
 void uiox_soc_hw_irq_init(void)
 { if (s_ops && s_ops->irq_init)      s_ops->irq_init(s_plat); }
 
 void uiox_soc_hw_irq_enable(uint32_t irq)
 { if (s_ops && s_ops->irq_enable)    s_ops->irq_enable(irq); }
 
 void uiox_soc_hw_irq_disable(uint32_t irq)
 { if (s_ops && s_ops->irq_disable)   s_ops->irq_disable(irq); }
 
 void uiox_soc_hw_irq_ack(uint32_t irq)
 { if (s_ops && s_ops->irq_ack)       s_ops->irq_ack(irq); }
 
 void uiox_soc_hw_irq_global_en(void)
 { if (s_ops && s_ops->irq_global_en) s_ops->irq_global_en(); }
 
 void uiox_soc_hw_irq_global_dis(void)
 { if (s_ops && s_ops->irq_global_dis) s_ops->irq_global_dis(); }
 
 void uiox_soc_hw_uart_putc(char c)
 { if (s_ops && s_ops->uart_putc)     s_ops->uart_putc(c); }
 
 void uiox_soc_hw_cache_enable(void)
 { if (s_ops && s_ops->cache_enable)  s_ops->cache_enable(); }
 
 void uiox_soc_hw_cache_disable(void)
 { if (s_ops && s_ops->cache_disable) s_ops->cache_disable(); }
 
 void uiox_soc_hw_tlb_flush(void)
 { if (s_ops && s_ops->tlb_flush)     s_ops->tlb_flush(); }
 
 void uiox_soc_hw_dsb(void)
 { if (s_ops && s_ops->barrier_dsb)   s_ops->barrier_dsb(); }
 
 void uiox_soc_hw_isb(void)
 { if (s_ops && s_ops->barrier_isb)   s_ops->barrier_isb(); }
 
 uint64_t uiox_soc_hw_tick(void)
 { return s_ops && s_ops->timer_tick ? s_ops->timer_tick() : 0u; }
 
 void __attribute__((noreturn)) uiox_soc_hw_reset(void)
 {
     if (s_ops && s_ops->reset) s_ops->reset();
     for (;;) ;
 }
 
 void __attribute__((noreturn)) uiox_soc_hw_shutdown(void)
 {
     if (s_ops && s_ops->shutdown) s_ops->shutdown();
     for (;;) ;
 }
 