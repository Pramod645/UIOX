#ifndef UIOX_EMIT_H
#define UIOX_EMIT_H
/*
 * uiox_emit.h - UIOX machine code emitter interface
 * One emitter per target architecture.
 */
#include "uiox_ir.h"
#include "uiox_regalloc.h"
#include "uiox_object.h"

typedef enum uiox_target_arch {
    UIOX_TARGET_X86_64,
    UIOX_TARGET_ARM64,
    UIOX_TARGET_ARM32,
} uiox_target_arch_t;

typedef struct uiox_emit_ctx {
    uiox_target_arch_t arch;
    uiox_object_t     *obj;        /* output object file         */
    uiox_section_t    *text;       /* .text section              */
    uiox_section_t    *rodata;     /* .rodata section            */
    uiox_section_t    *data;       /* .data section              */
    uiox_section_t    *bss;        /* .bss section               */
    uiox_regalloc_t   *ra;         /* register allocation result */
    int                frame_size; /* current function frame     */
    char               cur_func[128];
} uiox_emit_ctx_t;

void uiox_emit_init       (uiox_emit_ctx_t *ctx, uiox_target_arch_t arch,
                            uiox_object_t *obj, uiox_regalloc_t *ra);
void uiox_emit_func       (uiox_emit_ctx_t *ctx, uiox_ir_func_t *f);
void uiox_emit_module     (uiox_emit_ctx_t *ctx, uiox_ir_module_t *m);

/* -- x86_64 emitter ---------------------------------------- */
void uiox_emit_x86_prologue (uiox_emit_ctx_t *ctx, int frame_size);
void uiox_emit_x86_epilogue (uiox_emit_ctx_t *ctx);
void uiox_emit_x86_instr    (uiox_emit_ctx_t *ctx, uiox_ir_instr_t *ins);
void uiox_emit_x86_mov_rr   (uiox_emit_ctx_t *ctx, int dst, int src, int sz);
void uiox_emit_x86_mov_ri   (uiox_emit_ctx_t *ctx, int dst, long long imm, int sz);
void uiox_emit_x86_alu_rr   (uiox_emit_ctx_t *ctx, unsigned char opc,
                              int dst, int src, int sz);
void uiox_emit_x86_push     (uiox_emit_ctx_t *ctx, int reg);
void uiox_emit_x86_pop      (uiox_emit_ctx_t *ctx, int reg);
void uiox_emit_x86_call     (uiox_emit_ctx_t *ctx, const char *sym);
void uiox_emit_x86_ret      (uiox_emit_ctx_t *ctx);
void uiox_emit_x86_jmp      (uiox_emit_ctx_t *ctx, const char *label);
void uiox_emit_x86_jcc      (uiox_emit_ctx_t *ctx, unsigned char cc,
                              const char *label);
void uiox_emit_x86_label    (uiox_emit_ctx_t *ctx, const char *label);
void uiox_emit_x86_load     (uiox_emit_ctx_t *ctx, int dst, int base,
                              int disp, int sz);
void uiox_emit_x86_store    (uiox_emit_ctx_t *ctx, int src, int base,
                              int disp, int sz);

/* -- ARM64 emitter ----------------------------------------- */
void uiox_emit_a64_prologue (uiox_emit_ctx_t *ctx, int frame_size);
void uiox_emit_a64_epilogue (uiox_emit_ctx_t *ctx);
void uiox_emit_a64_instr    (uiox_emit_ctx_t *ctx, uiox_ir_instr_t *ins);
void uiox_emit_a64_mov_rr   (uiox_emit_ctx_t *ctx, int dst, int src);
void uiox_emit_a64_mov_imm  (uiox_emit_ctx_t *ctx, int dst, long long imm);
void uiox_emit_a64_alu      (uiox_emit_ctx_t *ctx, unsigned int opc,
                              int dst, int src1, int src2);
void uiox_emit_a64_ldr      (uiox_emit_ctx_t *ctx, int dst, int base, int off);
void uiox_emit_a64_str      (uiox_emit_ctx_t *ctx, int src, int base, int off);
void uiox_emit_a64_bl       (uiox_emit_ctx_t *ctx, const char *sym);
void uiox_emit_a64_ret      (uiox_emit_ctx_t *ctx);
void uiox_emit_a64_b        (uiox_emit_ctx_t *ctx, const char *label);
void uiox_emit_a64_bcond    (uiox_emit_ctx_t *ctx, unsigned int cc,
                              const char *label);

/* -- ARM32 emitter ----------------------------------------- */
void uiox_emit_a32_prologue (uiox_emit_ctx_t *ctx, int frame_size);
void uiox_emit_a32_epilogue (uiox_emit_ctx_t *ctx);
void uiox_emit_a32_instr    (uiox_emit_ctx_t *ctx, uiox_ir_instr_t *ins);
void uiox_emit_a32_mov_rr   (uiox_emit_ctx_t *ctx, int dst, int src);
void uiox_emit_a32_mov_imm  (uiox_emit_ctx_t *ctx, int dst, unsigned int imm);
void uiox_emit_a32_alu      (uiox_emit_ctx_t *ctx, unsigned int opc,
                              int dst, int rn, int rm);
void uiox_emit_a32_ldr      (uiox_emit_ctx_t *ctx, int dst, int base, int off);
void uiox_emit_a32_str      (uiox_emit_ctx_t *ctx, int src, int base, int off);
void uiox_emit_a32_bl       (uiox_emit_ctx_t *ctx, const char *sym);
void uiox_emit_a32_bx_lr    (uiox_emit_ctx_t *ctx);

#endif /* UIOX_EMIT_H */
