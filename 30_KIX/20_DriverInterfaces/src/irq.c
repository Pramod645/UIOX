/*
 * src/irq.c
 *
 * Interrupt vector table, handler registration, and dispatch.
 *
 * When an interrupt occurs the hardware:
 *   1. Finishes the current instruction.
 *   2. Saves PC and CPU flags / PSTATE into the link register
 *      (ARM) or onto the kernel stack (x86).
 *   3. Loads the handler address from the vector table.
 *   4. The handler runs in the context of the interrupted process
 *      (not a special process — this is critical Unix semantics).
 *   5. On return the hardware restores the saved state.
 *
 * irq_dispatch() is the C-level entry point called from the
 * low-level assembly trampoline after it has saved registers
 * into hw_context_t.
 */

 #include "irq.h"
 #include "cpu.h"
 #include "mmio.h"
 #include <stdio.h>
 #include <string.h>
 
 /* =============================================================
  * Interrupt vector table
  * ============================================================= */
 static irq_desc_t  ivt[IRQ_MAX];
 static uint64_t    jiffies_count = 0;
 
 /* =============================================================
  * irq_init
  * ============================================================= */
 void irq_init(void)
 {
     memset(ivt, 0, sizeof ivt);
     jiffies_count = 0;
     printf("[irq] vector table init: %d slots  arch=%s\n",
            IRQ_MAX, UIOX_ARCH_NAME);
 }
 
 /* =============================================================
  * irq_request — register a handler
  * ============================================================= */
 int irq_request(int irq, irq_handler_t handler,
                 void *dev_id, const char *name)
 {
     if (irq < 0 || irq >= IRQ_MAX) {
         fprintf(stderr, "[irq_request] irq %d out of range\n", irq);
         return HW_ERR_RANGE;
     }
 
     uint64_t flags = cpu_irq_disable();
 
     ivt[irq].id_handler = handler;
     ivt[irq].id_dev_id  = dev_id;
     ivt[irq].id_name    = name ? name : "unknown";
     ivt[irq].id_count   = 0;
     ivt[irq].id_enabled = 1;
 
     cpu_irq_restore(flags);
 
     printf("[irq_request] IRQ%-2d → '%s'\n", irq, ivt[irq].id_name);
     return HW_OK;
 }
 
 /* =============================================================
  * irq_free — unregister a handler
  * ============================================================= */
 void irq_free(int irq)
 {
     if (irq < 0 || irq >= IRQ_MAX) return;
 
     uint64_t flags = cpu_irq_disable();
     memset(&ivt[irq], 0, sizeof ivt[irq]);
     cpu_irq_restore(flags);
 
     printf("[irq_free] IRQ%d unregistered\n", irq);
 }
 
 /* =============================================================
  * irq_enable / irq_disable — mask at the interrupt controller
  * ============================================================= */
 void irq_enable(int irq)
 {
     if (irq < 0 || irq >= IRQ_MAX) return;
 
     /*
      * On a real GIC (ARM):  write irq bit to GICD_ISENABLER
      * On a real APIC (x86): write to IOAPIC redirection table
      * Simulated: set the enabled flag and write to INTC MMIO.
      */
     ivt[irq].id_enabled = 1;
     mmio_write32(MMIO_INTC_BASE + 0x100 + (uint32_t)(irq * 4), 1u);
     printf("[irq] IRQ%d enabled\n", irq);
 }
 
 void irq_disable(int irq)
 {
     if (irq < 0 || irq >= IRQ_MAX) return;
 
     ivt[irq].id_enabled = 0;
     mmio_write32(MMIO_INTC_BASE + 0x100 + (uint32_t)(irq * 4), 0u);
     printf("[irq] IRQ%d disabled\n", irq);
 }
 
 /* =============================================================
  * irq_dispatch — C-level interrupt entry point
  *
  * Called by the low-level assembly trampoline after it has:
  *   1. Saved all general-purpose registers into *ctx.
  *   2. Determined the IRQ number from the interrupt controller
  *      (GIC IAR on ARM; APIC vector on x86).
  *
  * This function:
  *   1. Increments the interrupt depth counter.
  *   2. Looks up the handler in the vector table.
  *   3. Calls the handler — which runs in the context of the
  *      interrupted process (not a separate process).
  *   4. Sends an End-Of-Interrupt (EOI) to the controller.
  *   5. Decrements the interrupt depth counter.
  *
  * The assembly trampoline restores *ctx after this returns,
  * resuming the interrupted process transparently.
  * ============================================================= */
 void irq_dispatch(int irq, hw_context_t *ctx)
 {
     if (irq < 0 || irq >= IRQ_MAX) {
         irq_spurious(irq);
         return;
     }
 
     /* Save IRQ number and increment nesting depth in context */
     ctx->irq_num = (uint32_t)irq;
     ctx->irq_depth++;
 
     printf("[irq_dispatch] IRQ%d '%s'  depth=%u  pc=0x%llx\n",
            irq,
            ivt[irq].id_name ? ivt[irq].id_name : "?",
            ctx->irq_depth,
            (unsigned long long)ctx->pc);
 
     if (!ivt[irq].id_enabled || !ivt[irq].id_handler) {
         irq_spurious(irq);
         goto eoi;
     }
 
     ivt[irq].id_count++;
 
     /*
      * Call the registered handler.
      *
      * The handler runs in the context of the interrupted process.
      * It must not sleep or call schedule() — it can only:
      *   - Read/write hardware registers (MMIO)
      *   - Wake sleeping processes (post to wait queues)
      *   - Set flags for deferred work (bottom-half / tasklet)
      */
     ivt[irq].id_handler(irq, ctx, ivt[irq].id_dev_id);
 
 eoi:
     /*
      * Send End-Of-Interrupt to the interrupt controller so it can
      * assert the next pending interrupt.
      *
      * GIC (ARM):  write irq to GICC_EOIR
      * APIC (x86): write 0 to APIC_EOI register (0xFEE000B0)
      * Simulated:  write to INTC EOI MMIO register
      */
     mmio_write32(MMIO_INTC_BASE + 0x200, (uint32_t)irq);
     ctx->irq_depth--;
 }
 
 /* =============================================================
  * irq_spurious — called for unregistered / masked IRQs
  * ============================================================= */
 void irq_spurious(int irq)
 {
     printf("[irq] SPURIOUS IRQ%d — no handler registered\n", irq);
 
     /*
      * On x86 APIC: a spurious vector (0xFF) must be acknowledged
      * without sending EOI (the APIC clears it automatically).
      * On GIC: read IAR, write EOIR to acknowledge.
      * Here we simply log and return.
      */
 }
 
 /* =============================================================
  * Timer support
  * ============================================================= */
 static uint32_t timer_hz = 0;
 
 void timer_init(uint32_t hz)
 {
     timer_hz = hz;
 
     /*
      * Programme the hardware timer to fire at 'hz' interrupts/sec.
      *
      * ARM SP804 / ARM64 generic timer:
      *   Write countdown value to MMIO_TIMER_BASE + LOAD register.
      *   Set IrqEnable + TimerEnable bits in control register.
      *
      * x86 PIT (8254):
      *   outb(0x43, 0x36);                // mode 3, binary
      *   outb(0x40, divisor & 0xFF);
      *   outb(0x40, divisor >> 8);
      *
      * Simulated: configure MMIO registers and register IRQ handler.
      */
     uint32_t load_val = 1000000u / hz;   /* microsecond units (sim) */
     mmio_write32(MMIO_TIMER_BASE + 0x00, load_val);   /* LOAD     */
     mmio_write32(MMIO_TIMER_BASE + 0x08, 0x83u);      /* CONTROL  */
 
     irq_request(IRQ_TIMER, timer_tick, NULL, "timer");
     irq_enable(IRQ_TIMER);
 
     printf("[timer] init: %u Hz  load=%u\n", hz, load_val);
 }
 
 void timer_tick(int irq, hw_context_t *ctx, void *dev_id)
 {
     (void)irq; (void)ctx; (void)dev_id;
     jiffies_count++;
 
     /* Acknowledge the timer by reading / writing the status register */
     mmio_write32(MMIO_TIMER_BASE + 0x0C, 1u);   /* clear interrupt */
 
     if (jiffies_count % 100 == 0)
         printf("[timer] tick: jiffies=%llu\n",
                (unsigned long long)jiffies_count);
 }
 
 uint64_t timer_jiffies(void) { return jiffies_count; }
 
 /* =============================================================
  * irq_print_table — debug dump
  * ============================================================= */
 void irq_print_table(void)
 {
     int i;
     printf("[irq] vector table:\n");
     printf("  IRQ  name             count   enabled\n");
     for (i = 0; i < IRQ_MAX; i++) {
         if (ivt[i].id_handler)
             printf("  %-4d %-16s %-7u %s\n",
                    i,
                    ivt[i].id_name    ? ivt[i].id_name : "?",
                    ivt[i].id_count,
                    ivt[i].id_enabled ? "yes" : "no");
     }
 }
 