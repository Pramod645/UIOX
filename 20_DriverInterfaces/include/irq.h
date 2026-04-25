#ifndef UIOX_IRQ_H
#define UIOX_IRQ_H

/*
 * irq.h
 *
 * Interrupt vector table, handler registration, and dispatch.
 *
 * When a device interrupt occurs:
 *   1. Hardware saves the current CPU context (registers, PC).
 *   2. The interrupt vector table maps IRQ number → handler.
 *   3. The handler runs in the context of the interrupted process.
 *   4. Hardware restores the saved context and resumes execution.
 *
 * Interrupts are NOT serviced by special processes — they are
 * special kernel functions called in the context of whatever
 * process happened to be running when the interrupt arrived.
 */

#include "hw_types.h"

/* =============================================================
 * IRQ handler function signature
 *
 * @irq    — IRQ line number (0 … IRQ_MAX-1)
 * @ctx    — pointer to the saved CPU context at interrupt entry
 * @dev_id — cookie registered with irq_request(); identifies
 *           the specific hardware unit (minor-number equivalent)
 * ============================================================= */
typedef void (*irq_handler_t)(int irq,
                               hw_context_t *ctx,
                               void         *dev_id);

/* =============================================================
 * IRQ descriptor — one entry in the vector table
 * ============================================================= */
typedef struct {
    irq_handler_t  id_handler;    /* registered handler or NULL    */
    void          *id_dev_id;     /* opaque cookie                 */
    const char    *id_name;       /* human-readable device name    */
    uint32_t       id_count;      /* interrupt count (statistics)  */
    int            id_enabled;    /* non-zero if IRQ is unmasked   */
} irq_desc_t;

/* =============================================================
 * IRQ subsystem API
 * ============================================================= */

/* Initialise the vector table (call once at startup) */
void irq_init(void);

/*
 * irq_request
 * Register an interrupt handler for IRQ line 'irq'.
 *
 * @irq    — IRQ number (0 … IRQ_MAX-1)
 * @handler — function to call when IRQ fires
 * @dev_id — cookie passed back to handler (identifies hw unit)
 * @name   — device name string (for /proc/interrupts equivalent)
 *
 * Returns HW_OK or HW_ERR_RANGE if irq is out of bounds.
 */
int  irq_request(int irq, irq_handler_t handler,
                 void *dev_id, const char *name);

/*
 * irq_free
 * Unregister the handler for IRQ line 'irq'.
 */
void irq_free(int irq);

/*
 * irq_enable  / irq_disable
 * Mask or unmask a single IRQ line at the interrupt controller.
 */
void irq_enable (int irq);
void irq_disable(int irq);

/*
 * irq_dispatch
 * Called by the low-level interrupt entry stub (assembly trampoline
 * on real hardware; simulated call in this build).
 *
 * Saves the current context into *ctx, looks up the handler in the
 * vector table, calls it, then marks the interrupt as serviced at
 * the interrupt controller (EOI).
 *
 * The interrupted process is resumed by the caller restoring *ctx.
 *
 * @irq — hardware IRQ number latched from the interrupt controller
 * @ctx — pointer to the hw_context_t filled in by the entry stub
 */
void irq_dispatch(int irq, hw_context_t *ctx);

/*
 * irq_spurious
 * Called when no registered handler exists for a fired IRQ.
 */
void irq_spurious(int irq);

/* =============================================================
 * Timer interrupt support
 * ============================================================= */

/* Initialise the platform timer and register its IRQ handler */
void timer_init(uint32_t hz);

/* Called from the timer IRQ handler; increments jiffies etc. */
void timer_tick(int irq, hw_context_t *ctx, void *dev_id);

/* Return simulated jiffies (ticks since timer_init) */
uint64_t timer_jiffies(void);

/* =============================================================
 * IRQ statistics / debug
 * ============================================================= */
void irq_print_table(void);

#endif /* UIOX_IRQ_H */
