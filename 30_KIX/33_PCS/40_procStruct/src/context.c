/*
 * 30_KIX/33_PCS/40_procStruct/src/context.c
 *
 * Freestanding fixes (v2.0)
 * ─────────────────────────
 *   REMOVED: #include <string.h>  #include <stdio.h>
 *            Both provided through context.h → proc_algo.h → uiox_klibc.h
 *
 *   FIXED: fprintf(stderr, "[context_save] context stack overflow\n")
 *       → printf("[context_save] ERROR: context stack overflow\n")
 *
 *   FIXED: fprintf(stderr, "[context_restore] context stack underflow\n")
 *       → printf("[context_restore] ERROR: context stack underflow\n")
 *
 *   FIXED: fprintf(stderr, "[inthand] unknown interrupt vector %d\n", vec)
 *       → printf("[inthand] ERROR: unknown interrupt vector %d\n", vec)
 *
 *   FIXED: fprintf(stderr, "[inthand] no handler for vector %d\n", vec)
 *       → printf("[inthand] ERROR: no handler for vector %d\n", vec)
 *
 * No algorithm changes.
 *
 * @version 2.0.0  @date 2026-07-23
 */

 #include "../include/context.h"
 #include "../include/proc_algo.h"
 /* string.h / stdio.h removed — provided transitively via
    proc_algo.h → uiox_klibc.h                              */
 
 /* ── Globals ──────────────────────────────────────────────────── */
 intr_vector_t  intr_vector_table[NVEC];
 sys_context_t *current_context = (sys_context_t *)0;
 
 /* ── intr_register ────────────────────────────────────────────── */
 void intr_register(int vec, intr_handler_t handler, const char *name)
 {
     if (vec < 0 || vec >= NVEC) return;
     intr_vector_table[vec].iv_num     = vec;
     intr_vector_table[vec].iv_handler = handler;
     intr_vector_table[vec].iv_name    = name;
 }
 
 /* ── context_save ─────────────────────────────────────────────── */
 void context_save(sys_context_t *ctx, reg_context_t *regs)
 {
     if (!ctx || !regs) return;
     if (ctx->sc_layer_top >= MAX_CONTEXT_LAYERS - 1) {
         printf("[context_save] ERROR: context stack overflow\n"); /* was: fprintf(stderr,...) */
         return;
     }
     ctx->sc_layer_top++;
     memcpy(&ctx->sc_layers[ctx->sc_layer_top], regs,
            sizeof(reg_context_t));
     printf("[context_save] saved context layer %d  pc=0x%lx\n",
            ctx->sc_layer_top,
            (unsigned long)regs->rc_pc);
 }
 
 /* ── context_restore ──────────────────────────────────────────── */
 void context_restore(sys_context_t *ctx, reg_context_t *regs)
 {
     if (!ctx || !regs) return;
     if (ctx->sc_layer_top < 0) {
         printf("[context_restore] ERROR: context stack underflow\n"); /* was: fprintf(stderr,...) */
         return;
     }
     memcpy(regs, &ctx->sc_layers[ctx->sc_layer_top],
            sizeof(reg_context_t));
     ctx->sc_layer_top--;
     printf("[context_restore] restored context layer %d  pc=0x%lx\n",
            ctx->sc_layer_top + 1,
            (unsigned long)regs->rc_pc);
 }
 
 /* ── context_switch ───────────────────────────────────────────── */
 void context_switch(sys_context_t *from, sys_context_t *to)
 {
     if (!from || !to) return;
     printf("[context_switch] switching context\n");
     /* Save current hardware state into 'from'  (arch-layer fills this) */
     /* Restore 'to' hardware state              (arch-layer fills this) */
     current_context = to;
 }
 
 /* ── Algorithm inthand ────────────────────────────────────────── */
 void inthand(int vec, reg_context_t *regs)
 {
     intr_vector_t *ivp;
     extern u_area_t u;
 
     /* Push current context layer */
     context_save(&u.u_sysctx, regs);
 
     printf("[inthand] interrupt received: vector=%d\n", vec);
 
     if (vec < 0 || vec >= NVEC) {
         printf("[inthand] ERROR: unknown interrupt vector %d\n", vec); /* was: fprintf(stderr,...) */
         context_restore(&u.u_sysctx, regs);
         return;
     }
 
     ivp = &intr_vector_table[vec];
     printf("[inthand] handler: %s\n",
            ivp->iv_name ? ivp->iv_name : "(unnamed)");
 
     if (ivp->iv_handler)
         ivp->iv_handler(vec, regs);
     else
         printf("[inthand] ERROR: no handler for vector %d\n", vec); /* was: fprintf(stderr,...) */
 
     /* Pop previous context layer */
     context_restore(&u.u_sysctx, regs);
 }
 