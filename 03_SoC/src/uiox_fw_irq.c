/**
 * @file  uiox_fw_irq.c
 * @brief UIOX Firmware — IRQ manager implementation.
 * @date  2026-06-21
 */

 #include "uiox_fw.h"

 static uiox_fw_irq_desc_t s_irq[UIOX_FW_IRQ_MAX];
 
 uiox_fw_err_t uiox_fw_irq_init(void)
 {
     uiox_fw_hw_irq_global_dis();
     for (uint32_t i = 0u; i < UIOX_FW_IRQ_MAX; i++) {
         s_irq[i].handler = NULL;
         s_irq[i].priv    = NULL;
         s_irq[i].enabled = false;
         s_irq[i].count   = 0u;
     }
     uiox_fw_hw_irq_init();
     FW_LOG("IRQ", "manager init OK (%u vectors)", UIOX_FW_IRQ_MAX);
     return UIOX_FW_OK;
 }
 
 uiox_fw_err_t uiox_fw_irq_register(uint32_t irq,
                                      uiox_fw_irq_handler_t handler,
                                      void *priv)
 {
     if (irq >= UIOX_FW_IRQ_MAX || !handler) return UIOX_FW_ERR_INVAL;
     s_irq[irq].handler = handler;
     s_irq[irq].priv    = priv;
     s_irq[irq].count   = 0u;
     s_irq[irq].enabled = true;
     uiox_fw_hw_irq_enable(irq);
     return UIOX_FW_OK;
 }
 
 uiox_fw_err_t uiox_fw_irq_unregister(uint32_t irq)
 {
     if (irq >= UIOX_FW_IRQ_MAX) return UIOX_FW_ERR_INVAL;
     uiox_fw_hw_irq_disable(irq);
     s_irq[irq].handler = NULL;
     s_irq[irq].priv    = NULL;
     s_irq[irq].enabled = false;
     return UIOX_FW_OK;
 }
 
 void uiox_fw_irq_dispatch(void)
 {
     /* Platform-specific IRQ identification happens in arch layer;
      * this generic dispatch is called with a known IRQ number.        */
     for (uint32_t i = 0u; i < UIOX_FW_IRQ_MAX; i++) {
         if (!s_irq[i].enabled || !s_irq[i].handler) continue;
         /* In real use the arch layer calls uiox_fw_irq_fire(irq_num) */
     }
 }
 
 /** Called by arch ISR stub with the resolved IRQ number. */
 void uiox_fw_irq_fire(uint32_t irq)
 {
     if (irq >= UIOX_FW_IRQ_MAX) return;
     s_irq[irq].count++;
     if (s_irq[irq].handler)
         s_irq[irq].handler(irq, s_irq[irq].priv);
     uiox_fw_hw_irq_ack(irq);
 }
 
 void uiox_fw_irq_enable (uint32_t irq)
 {
     if (irq < UIOX_FW_IRQ_MAX) {
         s_irq[irq].enabled = true;
         uiox_fw_hw_irq_enable(irq);
     }
 }
 
 void uiox_fw_irq_disable(uint32_t irq)
 {
     if (irq < UIOX_FW_IRQ_MAX) {
         s_irq[irq].enabled = false;
         uiox_fw_hw_irq_disable(irq);
     }
 }
 
 void uiox_fw_irq_global_enable (void) { uiox_fw_hw_irq_global_en(); }
 void uiox_fw_irq_global_disable(void) { uiox_fw_hw_irq_global_dis(); }
 
 uint32_t uiox_fw_irq_count(uint32_t irq)
 { return irq < UIOX_FW_IRQ_MAX ? s_irq[irq].count : 0u; }
 
 void uiox_fw_irq_print(void)
 {
     uiox_fw_puts("[FW] IRQ table:\n");
     for (uint32_t i = 0u; i < UIOX_FW_IRQ_MAX; i++) {
         if (!s_irq[i].handler) continue;
         uiox_fw_printf("  IRQ %3u  count=%-8u  %s\n",
                         i, s_irq[i].count,
                         s_irq[i].enabled ? "enabled" : "masked");
     }
 }
 