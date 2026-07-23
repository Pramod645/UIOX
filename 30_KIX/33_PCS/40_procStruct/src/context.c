#include "../include/context.h"
#include "../include/proc_algo.h"
#include <string.h>
#include <stdio.h>

/* ── Globals ────────────────────────────────────────────────── */
intr_vector_t  intr_vector_table[NVEC];
sys_context_t *current_context = NULL;

/* ── intr_register ──────────────────────────────────────────
 * Register a handler in the interrupt vector table.
 */
void intr_register(int vec, intr_handler_t handler, const char *name)
{
    if (vec < 0 || vec >= NVEC) return;
    intr_vector_table[vec].iv_num     = vec;
    intr_vector_table[vec].iv_handler = handler;
    intr_vector_table[vec].iv_name    = name;
}

/* ── context_save ───────────────────────────────────────────
 * Push current register context onto the context layer stack.
 */
void context_save(sys_context_t *ctx, reg_context_t *regs)
{
    if (!ctx || !regs) return;
    if (ctx->sc_layer_top >= MAX_CONTEXT_LAYERS - 1) {
        fprintf(stderr, "[context_save] context stack overflow\n");
        return;
    }
    ctx->sc_layer_top++;
    memcpy(&ctx->sc_layers[ctx->sc_layer_top], regs,
           sizeof(reg_context_t));
    printf("[context_save] saved context layer %d "
           "pc=0x%lx\n", ctx->sc_layer_top,
           (unsigned long)regs->rc_pc);
}

/* ── context_restore ────────────────────────────────────────
 * Pop previous register context from the context layer stack.
 */
void context_restore(sys_context_t *ctx, reg_context_t *regs)
{
    if (!ctx || !regs) return;
    if (ctx->sc_layer_top < 0) {
        fprintf(stderr, "[context_restore] context stack underflow\n");
        return;
    }
    memcpy(regs, &ctx->sc_layers[ctx->sc_layer_top],
           sizeof(reg_context_t));
    ctx->sc_layer_top--;
    printf("[context_restore] restored context layer %d "
           "pc=0x%lx\n", ctx->sc_layer_top + 1,
           (unsigned long)regs->rc_pc);
}

/* ── context_switch ─────────────────────────────────────────
 * Full context switch between two processes.
 * Saves 'from' context, loads 'to' context.
 */
void context_switch(sys_context_t *from, sys_context_t *to)
{
    if (!from || !to) return;
    printf("[context_switch] switching context\n");

    /* Save current hardware state into 'from' */
    /* In real kernel: save CPU registers to from->sc_layers */

    /* Restore 'to' hardware state */
    /* In real kernel: restore CPU registers from to->sc_layers */

    current_context = to;
}

/* ─────────────────────────────────────────────────────────────
 * 1. Algorithm inthand
 *    input : interrupt vector number, saved register context
 *    output: none
 *
 *    Handles hardware and software interrupts.
 */
void inthand(int vec, reg_context_t *regs)
{
    extern u_area_t u;

    /* Save (push) current context layer */
    context_save(&u.u_sysctx, regs);

    /* Determine interrupt source */
    printf("[inthand] interrupt received: vector=%d\n", vec);

    /* Find interrupt vector */
    if (vec < 0 || vec >= NVEC) {
        fprintf(stderr, "[inthand] unknown interrupt vector %d\n",
                vec);
        context_restore(&u.u_sysctx, regs);
        return;
    }

    intr_vector_t *ivp = &intr_vector_table[vec];
    printf("[inthand] handler: %s\n",
           ivp->iv_name ? ivp->iv_name : "(unnamed)");

    /* Call interrupt handler */
    if (ivp->iv_handler)
        ivp->iv_handler(vec, regs);
    else
        fprintf(stderr, "[inthand] no handler for vector %d\n", vec);

    /* Restore (pop) previous context layer */
    context_restore(&u.u_sysctx, regs);
}
