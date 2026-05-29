#ifndef UIOX_CODEGEN_H
#define UIOX_CODEGEN_H
/*
 * uiox_codegen.h - UIOX code generator (AST -> IR -> machine code)
 */
#include "uiox_ast.h"
#include "uiox_ir.h"
#include "uiox_symtab.h"
#include "uiox_emit.h"
#include "uiox_regalloc.h"
#include "uiox_error.h"

typedef struct uiox_codegen {
    uiox_ir_module_t   ir;
    uiox_symtab_t      symtab;
    uiox_emit_ctx_t    emitter;
    uiox_regalloc_t    ra;
    uiox_diag_ctx_t   *diag;
    uiox_target_arch_t arch;
    int                temp_id;
    int                label_id;
    char               break_label[64];
    char               cont_label[64];
} uiox_codegen_t;

void             uiox_codegen_init    (uiox_codegen_t *cg,
                                        uiox_target_arch_t arch,
                                        uiox_diag_ctx_t *diag);
void             uiox_codegen_free    (uiox_codegen_t *cg);
int              uiox_codegen_run     (uiox_codegen_t *cg,
                                        uiox_ast_node_t *tu,
                                        uiox_object_t *out);

/* -- AST -> IR lowering ------------------------------------- */
void             uiox_cg_lower_unit   (uiox_codegen_t *cg,
                                        uiox_ast_node_t *tu);
uiox_ir_func_t  *uiox_cg_lower_func  (uiox_codegen_t *cg,
                                        uiox_ast_node_t *fn);
void             uiox_cg_lower_stmt   (uiox_codegen_t *cg,
                                        uiox_ir_func_t *f,
                                        uiox_ir_block_t **b,
                                        uiox_ast_node_t *stmt);
uiox_ir_operand_t uiox_cg_lower_expr (uiox_codegen_t *cg,
                                        uiox_ir_func_t *f,
                                        uiox_ir_block_t **b,
                                        uiox_ast_node_t *expr);

/* -- Helpers ------------------------------------------------ */
uiox_ir_operand_t uiox_cg_new_temp   (uiox_codegen_t *cg,
                                        uiox_ir_func_t *f, int sz);
char             *uiox_cg_new_label  (uiox_codegen_t *cg, const char *prefix);

#endif /* UIOX_CODEGEN_H */
