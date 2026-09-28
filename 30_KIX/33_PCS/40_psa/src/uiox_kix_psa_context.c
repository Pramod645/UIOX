/*
 * 30_KIX/33_PCS/40psa/src/uiox_kix_psa_context.c
 *
 * PSA Algorithm 1 — inthand, and the context bookkeeping around it.
 *
 * ── what a context switch is, and is not ────────────────────────────
 * The original printed a line and assigned current_context.  That is the
 * shape of a switch without the operation.  PSA says the process table
 * entry carries a field that lets "the kernel to locate the process and
 * its u area in main memory and this get used for context switch" — so
 * the bookkeeping is real work this layer can own: which context is
 * current, and how deep the system-level stack is.
 *
 * The register transfer itself is per-architecture and is NOT here.  That
 * is why uiox_kix_psa_swtch() is declared in the header and defined
 * nowhere in this layer — the dependency is visible rather than
 * papered over with a stub that appears to work.
 *
 * ── the bounded context stack ───────────────────────────────────────
 * inthand pushes a layer onto u_sysctx.  The push is checked against
 * MAX_CTX_LAYERS, because a recursive interrupt with no bound would walk
 * off the end of sc_layers[] and corrupt whatever follows it in the u
 * area.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_psa_context.h"
#include "../include/uiox_kix_psa_proc_algo.h"

/* ── Globals ───────────────────────────────────────────────────────── */
uiox_kix_psa_intr_vector_t
    uiox_kix_psa_intr_vector_table[UIOX_KIX_PSA_NVEC];

uiox_kix_psa_sys_context_t *uiox_kix_psa_current_context =
    (uiox_kix_psa_sys_context_t *)0;

/* ── uiox_kix_psa_intr_register ──────────────────────────────────────
 * Install a handler, a name, or both for an interrupt vector.
 *
 * The range check refuses rather than clamping: a vector outside the
 * table is a caller error, and silently redirecting it to the last slot
 * would deliver interrupts to a handler that never asked for them.
 *
 * Registering an already-registered vector replaces the handler and the
 * name, which is what makes re-initialisation idempotent.
 *
 * A NULL handler is accepted and means "known vector, deliberately
 * unhandled" — inthand treats it as a no-op rather than an error. */
void uiox_kix_psa_intr_register(int vec,
                                uiox_kix_psa_intr_handler_t handler,
                                const char *name)
{
    if (vec < 0 || vec >= UIOX_KIX_PSA_NVEC) return;

    uiox_kix_psa_intr_vector_table[vec].iv_num     = vec;
    uiox_kix_psa_intr_vector_table[vec].iv_handler = handler;
    uiox_kix_psa_intr_vector_table[vec].iv_name    = name;
}

/* ── uiox_kix_psa_context_save ───────────────────────────────────────
 * Push a register frame onto a context's layer stack.
 *
 * Refuses at MAX_CTX_LAYERS - 1 rather than overwriting the top: the top
 * frame belongs to the interrupt currently executing, and clobbering it
 * would lose the context this handler must return to.  Refusing loses an
 * interrupt instead of a process, which is the lesser failure.
 *
 * The whole frame is copied, so a handler may modify the saved registers
 * and thereby change where the interrupted process resumes. */
void uiox_kix_psa_context_save(uiox_kix_psa_sys_context_t *ctx,
                               uiox_kix_psa_reg_context_t *regs)
{
    if (!ctx || !regs) return;
    if (ctx->sc_layer_top >= UIOX_KIX_PSA_MAX_CTX_LAYERS - 1) return;

    ctx->sc_layer_top++;
    memcpy(&ctx->sc_layers[ctx->sc_layer_top], regs,
           sizeof(uiox_kix_psa_reg_context_t));
}

/* ── uiox_kix_psa_context_restore ────────────────────────────────────
 * Pop the top frame back into regs and lower the stack.
 *
 * Refuses on an empty stack.  Restoring from an underflow would copy
 * whatever happens to sit below sc_layers[0] — in a u area that is the
 * preceding field — so the check is what keeps a double-pop from
 * silently installing garbage as a return address.
 *
 * Pair with context_save: every save must be matched by exactly one
 * restore, and inthand's two exits do that deliberately. */
void uiox_kix_psa_context_restore(uiox_kix_psa_sys_context_t *ctx,
                                  uiox_kix_psa_reg_context_t *regs)
{
    if (!ctx || !regs) return;
    if (ctx->sc_layer_top < 0) return;

    memcpy(regs, &ctx->sc_layers[ctx->sc_layer_top],
           sizeof(uiox_kix_psa_reg_context_t));
    ctx->sc_layer_top--;
}

/* ── uiox_kix_psa_context_switch ─────────────────────────────────────
 * Make a context the current one.
 *
 * BOOKKEEPING ONLY.  The register transfer is uiox_kix_psa_swtch(), which
 * the arch layer supplies; this function records which context the kernel
 * should treat as current so that a later save lands in the right place.
 *
 * A NULL target is refused, so the current context is never cleared by an
 * accidental NULL — losing it would leave the interrupt path with nowhere
 * to push, and the next inthand would fault. */
void uiox_kix_psa_context_switch(uiox_kix_psa_sys_context_t *from,
                                 uiox_kix_psa_sys_context_t *to)
{
    if (!to) return;
    uiox_kix_psa_current_context = to;
    (void)from;
}

/* ── PSA Algorithm 1 — uiox_kix_psa_inthand ──────────────────────────
 * Handle an interrupt.
 *
 * The interrupted context is pushed FIRST, so a handler may itself sleep,
 * fault, or take a nested interrupt without losing where it came from.
 *
 * An out-of-range vector and a NULL handler are both no-ops that still
 * balance the push — an unhandled interrupt must not corrupt the context
 * stack, or the next legitimate interrupt inherits the damage.
 *
 * The handler receives the vector number and a pointer to the SAVED
 * frame, so modifications it makes are what the interrupted process
 * resumes with.  That is how a signal handler or a page-fault resolver
 * changes control flow. */
void uiox_kix_psa_inthand(int vec, uiox_kix_psa_reg_context_t *regs)
{
    uiox_kix_psa_intr_vector_t *ivp;

    uiox_kix_psa_context_save(&uiox_kix_psa_u.u_sysctx, regs);

    if (vec < 0 || vec >= UIOX_KIX_PSA_NVEC) {
        uiox_kix_psa_context_restore(&uiox_kix_psa_u.u_sysctx, regs);
        return;
    }

    ivp = &uiox_kix_psa_intr_vector_table[vec];

    if (ivp->iv_handler)
        ivp->iv_handler(vec, regs);

    uiox_kix_psa_context_restore(&uiox_kix_psa_u.u_sysctx, regs);
}
