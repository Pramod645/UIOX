/*
 * 30_KIX/33_PCS/40psa/include/uiox_kix_psa_context.h
 *
 * Context: what a context switch moves, and the interrupt vector table
 * the PSA inthand algorithm dispatches through.
 *
 * ── scope: interrupts only ──────────────────────────────────────────
 * PSA numbers two algorithms here — inthand (1) and syscall (2) — but
 * only inthand is implemented in this layer.  System calls belong to
 * 50_scps, which owns the dispatch mechanism and the syscall numbers.
 * A second dispatcher in this header would mean two places decide what
 * syscall number 9 does, which is a class of bug this tree already has
 * between its two process models.
 *
 * ── the system-level context stack ──────────────────────────────────
 * PSA's fork algorithm says "push dummy system level context layer onto
 * child system level context".  sys_context_t is that layer, sc_layers[]
 * is the stack, and sc_layer_top is its height.  The stack is bounded by
 * MAX_CTX_LAYERS and the push is checked, because a recursive interrupt
 * with no bound would walk off the end of the array.
 *
 * ── what this layer cannot do ───────────────────────────────────────
 * The actual register save and restore is per-architecture.  The
 * functions here do the bookkeeping — which context is current, how deep
 * the stack is — and the arch layer supplies the instruction sequence.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#ifndef UIOX_KIX_PSA_CONTEXT_H
#define UIOX_KIX_PSA_CONTEXT_H

#include "uiox_klibc.h"

/* ── Sizes ─────────────────────────────────────────────────────────────
 * NGPR is the general-purpose register count a saved context carries.
 * MAX_CTX_LAYERS bounds the system-level context stack depth.
 * NVEC sizes the interrupt vector table. */
#define UIOX_KIX_PSA_NGPR            16
#define UIOX_KIX_PSA_KERNEL_STACK_SZ 4096
#define UIOX_KIX_PSA_MAX_CTX_LAYERS  8
#define UIOX_KIX_PSA_NVEC            256

/* ── Saved register context ──────────────────────────────────────────
 * One frame of machine state.  rc_r0/rc_r1 carry a syscall's return
 * value across the switch and rc_carry is the error flag: a set carry
 * with rc_r0 holding the error number is how 50_scps reports failure. */
typedef struct uiox_kix_psa_reg_context {
    uintptr_t rc_pc;                        /* program counter          */
    uintptr_t rc_sp;                        /* stack pointer            */
    uint32_t  rc_sr;                        /* status register          */
    uint32_t  rc_carry;                     /* error indicator          */
    uintptr_t rc_gpr[UIOX_KIX_PSA_NGPR];    /* general-purpose regs     */
    uint32_t  rc_r0;                        /* syscall return, low      */
    uint32_t  rc_r1;                        /* syscall return, high     */
} uiox_kix_psa_reg_context_t;

/* ── The user address space map ──────────────────────────────────────
 * The MAP, not the regions: which address ranges this process owns.
 * The regions themselves are described in uiox_kix_psa_region.h. */
typedef struct uiox_kix_psa_user_addr_space {
    uintptr_t uas_text_base;  uint32_t uas_text_size;
    uintptr_t uas_data_base;  uint32_t uas_data_size;
    uintptr_t uas_stack_base; uint32_t uas_stack_size;
    uintptr_t uas_shmem_base; uint32_t uas_shmem_size;
} uiox_kix_psa_user_addr_space_t;

/* ── System-level context layer ──────────────────────────────────────
 * One entry per nesting depth.  sc_uarea and sc_pregion locate the
 * process's u area and region table so the kernel can find them after a
 * switch.  sc_prev chains to the previous context. */
typedef struct uiox_kix_psa_sys_context {
    void                              *sc_proc_entry; /* process table slot */
    void                              *sc_uarea;      /* u area location     */
    void                              *sc_pregion;    /* pregion table       */
    uint8_t                            sc_kstack[UIOX_KIX_PSA_KERNEL_STACK_SZ];
    int                                sc_layer_top;  /* stack height        */
    uiox_kix_psa_reg_context_t         sc_layers[UIOX_KIX_PSA_MAX_CTX_LAYERS];
    struct uiox_kix_psa_sys_context   *sc_prev;
} uiox_kix_psa_sys_context_t;

/* ── A process's context is one of these three views ─────────────────
 * A union because the same memory is read as registers by the arch layer,
 * as an address map by the memory manager, and as a context stack by the
 * interrupt path. */
typedef union uiox_kix_psa_proc_context {
    uiox_kix_psa_user_addr_space_t pc_user;
    uiox_kix_psa_reg_context_t     pc_regs;
    uiox_kix_psa_sys_context_t     pc_sys;
} uiox_kix_psa_proc_context_t;

/* ── Interrupt vectors ───────────────────────────────────────────────
 * A handler receives the vector number and the interrupted register
 * context, and may modify that context — which is how a handler changes
 * where the interrupted process resumes. */
typedef void (*uiox_kix_psa_intr_handler_t)(int vector,
                                            uiox_kix_psa_reg_context_t *ctx);
typedef struct uiox_kix_psa_intr_vector {
    int                        iv_num;      /* which vector this is     */
    uiox_kix_psa_intr_handler_t iv_handler; /* NULL = unhandled         */
    const char                *iv_name;     /* diagnostic label         */
} uiox_kix_psa_intr_vector_t;

/* ── Globals ────────────────────────────────────────────────────────── */
extern uiox_kix_psa_intr_vector_t
       uiox_kix_psa_intr_vector_table[UIOX_KIX_PSA_NVEC];
extern uiox_kix_psa_sys_context_t *uiox_kix_psa_current_context;

/* ── Context API ───────────────────────────────────────────────────── */

/* Install a handler for a vector.  Out-of-range vectors are ignored
 * rather than allowed to write past the table.  Registering the same
 * vector twice replaces the earlier handler. */
void uiox_kix_psa_intr_register(int vec,
                                uiox_kix_psa_intr_handler_t handler,
                                const char *name);

/* Push a register frame onto a context's layer stack.
 * Refuses when the stack is full (MAX_CTX_LAYERS) rather than
 * overwriting the top — the check is what makes the bound real. */
void uiox_kix_psa_context_save(uiox_kix_psa_sys_context_t *ctx,
                               uiox_kix_psa_reg_context_t *regs);

/* Pop the top frame back into regs and lower the stack.
 * Refuses on an empty stack; restoring from an underflow would copy
 * whatever happens to sit below sc_layers[0]. */
void uiox_kix_psa_context_restore(uiox_kix_psa_sys_context_t *ctx,
                                  uiox_kix_psa_reg_context_t *regs);

/* Make a context the current one.  This performs the BOOKKEEPING only;
 * the register transfer itself is the arch layer's switch primitive.
 * Refuses a NULL target so the current context is never cleared by an
 * accidental NULL. */
void uiox_kix_psa_context_switch(uiox_kix_psa_sys_context_t *from,
                                 uiox_kix_psa_sys_context_t *to);

/* PSA Algorithm 1 — handle an interrupt.
 * Pushes the interrupted context, looks up the vector, calls the handler
 * if one is installed, then pops the context.  An unknown or unhandled
 * vector is a no-op that still balances the push. */
void uiox_kix_psa_inthand(int vec, uiox_kix_psa_reg_context_t *regs);

#endif /* UIOX_KIX_PSA_CONTEXT_H */
