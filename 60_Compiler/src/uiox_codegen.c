/*
 * uiox_codegen.c - UIOX AST -> IR lowering
 */
#include "../include/uiox_codegen.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void uiox_codegen_init(uiox_codegen_t *cg,
                        uiox_target_arch_t arch,
                        uiox_diag_ctx_t *diag)
{
    memset(cg, 0, sizeof(*cg));
    cg->arch     = arch;
    cg->diag     = diag;
    cg->temp_id  = 0;
    cg->label_id = 0;
    uiox_ir_module_init(&cg->ir);
    uiox_symtab_init(&cg->symtab);
}

void uiox_codegen_free(uiox_codegen_t *cg)
{
    uiox_ir_module_free(&cg->ir);
    uiox_symtab_free(&cg->symtab);
    uiox_regalloc_free(&cg->ra);
}

uiox_ir_operand_t uiox_cg_new_temp(uiox_codegen_t *cg,
                                     uiox_ir_func_t *f, int sz)
{
    (void)cg;
    return uiox_ir_new_temp(f, sz);
}

char *uiox_cg_new_label(uiox_codegen_t *cg, const char *prefix)
{
    static char buf[64];
    snprintf(buf, sizeof(buf), "%s_%d", prefix, cg->label_id++);
    return buf;
}

/* ── Expression lowering ─────────────────────────────────── */
uiox_ir_operand_t uiox_cg_lower_expr(uiox_codegen_t *cg,
                                       uiox_ir_func_t *f,
                                       uiox_ir_block_t **b,
                                       uiox_ast_node_t *e)
{
    if (!e) return uiox_ir_none();

    switch (e->kind) {
        case AST_INT_LIT:
            return uiox_ir_imm(e->ival.val, 8);

        case AST_IDENT_EXPR: {
            uiox_symbol_t *sym = uiox_symtab_lookup(&cg->symtab,
                                                      e->ident.name);
            uiox_ir_operand_t dst = uiox_cg_new_temp(cg, f, 8);
            if (sym && sym->offset) {
                /* local var: load from stack */
                uiox_ir_operand_t base = uiox_ir_none();
                uiox_ir_instr_t *ins =
                    uiox_ir_emit(*b, IR_LOAD, dst, base,
                                 uiox_ir_imm(sym->offset, 4));
                ins->offset = sym->offset;
            } else {
                /* global */
                uiox_ir_operand_t gop;
                memset(&gop, 0, sizeof(gop));
                gop.kind = IR_OP_GLOBAL;
                strncpy(gop.name, e->ident.name, UIOX_IR_NAME_MAX-1);
                uiox_ir_emit(*b, IR_LEA, dst, gop, uiox_ir_none());
            }
            return dst;
        }

        case AST_BINOP_EXPR:
        case AST_ASSIGN_EXPR: {
            uiox_ir_operand_t lhs =
                uiox_cg_lower_expr(cg, f, b, e->binop.left);
            uiox_ir_operand_t rhs =
                uiox_cg_lower_expr(cg, f, b, e->binop.right);
            uiox_ir_operand_t dst = uiox_cg_new_temp(cg, f, 8);

            uiox_ir_opcode_t op = IR_NOP;
            switch (e->binop.op) {
                case TOK_PLUS:     op = IR_ADD; break;
                case TOK_MINUS:    op = IR_SUB; break;
                case TOK_STAR:     op = IR_MUL; break;
                case TOK_SLASH:    op = IR_DIV; break;
                case TOK_PERCENT:  op = IR_MOD; break;
                case TOK_AMP:      op = IR_AND; break;
                case TOK_PIPE:     op = IR_OR;  break;
                case TOK_CARET:    op = IR_XOR; break;
                case TOK_LSHIFT:   op = IR_SHL; break;
                case TOK_RSHIFT:   op = IR_SHR; break;
                case TOK_EQ_EQ:    op = IR_EQ;  break;
                case TOK_BANG_EQ:  op = IR_NE;  break;
                case TOK_LT:       op = IR_LT;  break;
                case TOK_LT_EQ:    op = IR_LE;  break;
                case TOK_GT:       op = IR_GT;  break;
                case TOK_GT_EQ:    op = IR_GE;  break;
                case TOK_EQ:       op = IR_MOV; break;
                default:           op = IR_ADD; break;
            }
            uiox_ir_emit(*b, op, dst, lhs, rhs);
            return dst;
        }

        case AST_UNOP_EXPR: {
            uiox_ir_operand_t src =
                uiox_cg_lower_expr(cg, f, b, e->unop.operand);
            uiox_ir_operand_t dst = uiox_cg_new_temp(cg, f, 8);
            switch (e->unop.op) {
                case TOK_MINUS:
                    uiox_ir_emit(*b, IR_NEG, dst, src, uiox_ir_none());
                    break;
                case TOK_TILDE:
                    uiox_ir_emit(*b, IR_NOT, dst, src, uiox_ir_none());
                    break;
                case TOK_BANG: {
                    uiox_ir_operand_t z = uiox_ir_imm(0, 8);
                    uiox_ir_emit(*b, IR_EQ, dst, src, z);
                    break;
                }
                case TOK_STAR:
                    uiox_ir_emit(*b, IR_DEREF, dst, src, uiox_ir_none());
                    break;
                case TOK_AMP:
                    uiox_ir_emit(*b, IR_ADDR, dst, src, uiox_ir_none());
                    break;
                default:
                    uiox_ir_emit(*b, IR_MOV, dst, src, uiox_ir_none());
                    break;
            }
            return dst;
        }

        case AST_CALL_EXPR: {
            /* emit args first */
            for (int i = 0; i < e->call.args.count; i++) {
                uiox_ir_operand_t av =
                    uiox_cg_lower_expr(cg, f, b, e->call.args.items[i]);
                uiox_ir_operand_t slot = uiox_ir_imm(i, 4);
                uiox_ir_emit(*b, IR_ARG, slot, av, uiox_ir_none());
            }
            uiox_ir_operand_t dst = uiox_cg_new_temp(cg, f, 8);
            uiox_ir_operand_t fn_op;
            memset(&fn_op, 0, sizeof(fn_op));
            fn_op.kind = IR_OP_LABEL;
            if (e->call.callee->kind == AST_IDENT_EXPR)
                strncpy(fn_op.name, e->call.callee->ident.name,
                        UIOX_IR_NAME_MAX - 1);
            uiox_ir_instr_t *ci =
                uiox_ir_emit(*b, IR_CALL, dst, fn_op, uiox_ir_none());
            strncpy(ci->label, fn_op.name, UIOX_IR_NAME_MAX - 1);
            return dst;
        }

        case AST_TERNARY_EXPR: {
            char *lt = uiox_cg_new_label(cg, "tern_t");
            char *lf = uiox_cg_new_label(cg, "tern_f");
            char *le = uiox_cg_new_label(cg, "tern_e");
            uiox_ir_operand_t cond =
                uiox_cg_lower_expr(cg, f, b, e->ternary.cond);
            uiox_ir_instr_t *jz =
                uiox_ir_emit(*b, IR_JZ, uiox_ir_none(), cond,
                             uiox_ir_label_op(lf));
            strncpy(jz->label, lf, UIOX_IR_NAME_MAX - 1);

            uiox_ir_block_t *bt = uiox_ir_add_block(f, lt);
            uiox_ir_operand_t vt = uiox_cg_lower_expr(cg, f, &bt, e->ternary.then_e);
            uiox_ir_instr_t *je =
                uiox_ir_emit(bt, IR_JMP, uiox_ir_none(),
                             uiox_ir_label_op(le), uiox_ir_none());
            strncpy(je->label, le, UIOX_IR_NAME_MAX - 1);

            uiox_ir_block_t *bf = uiox_ir_add_block(f, lf);
            uiox_ir_operand_t vf = uiox_cg_lower_expr(cg, f, &bf, e->ternary.else_e);

            uiox_ir_block_t *bend = uiox_ir_add_block(f, le);
            uiox_ir_operand_t dst = uiox_cg_new_temp(cg, f, 8);
            uiox_ir_emit(bend, IR_PHI, dst, vt, vf);
            *b = bend;
            return dst;
        }

        default:
            return uiox_ir_imm(0, 8);
    }
}

/* ── Statement lowering ──────────────────────────────────── */
void uiox_cg_lower_stmt(uiox_codegen_t *cg,
                          uiox_ir_func_t *f,
                          uiox_ir_block_t **b,
                          uiox_ast_node_t *s)
{
    if (!s) return;

    switch (s->kind) {
        case AST_COMPOUND_STMT:
            uiox_symtab_push(&cg->symtab);
            for (int i = 0; i < s->compound.stmts.count; i++)
                uiox_cg_lower_stmt(cg, f, b, s->compound.stmts.items[i]);
            uiox_symtab_pop(&cg->symtab);
            break;

        case AST_EXPR_STMT:
            uiox_cg_lower_expr(cg, f, b, s);
            break;

        case AST_VAR_DECL: {
            uiox_symbol_t *sym = uiox_symtab_insert(&cg->symtab,
                                                     s->var.name,
                                                     SYM_VAR);
            f->frame_size += 8;
            sym->offset    = f->frame_size;
            if (s->var.init) {
                uiox_ir_operand_t val =
                    uiox_cg_lower_expr(cg, f, b, s->var.init);
                uiox_ir_operand_t dst = uiox_ir_none();
                uiox_ir_instr_t *st =
                    uiox_ir_emit(*b, IR_STORE, dst, val,
                                 uiox_ir_imm(sym->offset, 4));
                st->offset = sym->offset;
            }
            break;
        }

        case AST_RETURN_STMT: {
            if (s->ret.expr) {
                uiox_ir_operand_t rv =
                    uiox_cg_lower_expr(cg, f, b, s->ret.expr);
                uiox_ir_emit(*b, IR_RET, uiox_ir_none(),
                             rv, uiox_ir_none());
            } else {
                uiox_ir_emit(*b, IR_RET, uiox_ir_none(),
                             uiox_ir_none(), uiox_ir_none());
            }
            break;
        }

        case AST_IF_STMT: {
            char *lt = uiox_cg_new_label(cg, "if_t");
            char *lf = uiox_cg_new_label(cg, "if_f");
            char *le = uiox_cg_new_label(cg, "if_e");
            uiox_ir_operand_t cond =
                uiox_cg_lower_expr(cg, f, b, s->if_stmt.cond);
            uiox_ir_instr_t *jz =
                uiox_ir_emit(*b, IR_JZ, uiox_ir_none(), cond,
                             uiox_ir_label_op(s->if_stmt.else_s ? lf : le));
            strncpy(jz->label,
                    s->if_stmt.else_s ? lf : le,
                    UIOX_IR_NAME_MAX - 1);
            uiox_ir_block_t *bt = uiox_ir_add_block(f, lt);
            uiox_cg_lower_stmt(cg, f, &bt, s->if_stmt.then_s);
            uiox_ir_instr_t *jmp =
                uiox_ir_emit(bt, IR_JMP, uiox_ir_none(),
                             uiox_ir_label_op(le), uiox_ir_none());
            strncpy(jmp->label, le, UIOX_IR_NAME_MAX - 1);
            if (s->if_stmt.else_s) {
                uiox_ir_block_t *bf = uiox_ir_add_block(f, lf);
                uiox_cg_lower_stmt(cg, f, &bf, s->if_stmt.else_s);
                uiox_ir_instr_t *je =
                    uiox_ir_emit(bf, IR_JMP, uiox_ir_none(),
                                 uiox_ir_label_op(le), uiox_ir_none());
                strncpy(je->label, le, UIOX_IR_NAME_MAX - 1);
            }
            *b = uiox_ir_add_block(f, le);
            break;
        }

        case AST_WHILE_STMT: {
            char *lc = uiox_cg_new_label(cg, "wh_c");
            char *lb = uiox_cg_new_label(cg, "wh_b");
            char *le = uiox_cg_new_label(cg, "wh_e");
            uiox_ir_instr_t *jmp =
                uiox_ir_emit(*b, IR_JMP, uiox_ir_none(),
                             uiox_ir_label_op(lc), uiox_ir_none());
            strncpy(jmp->label, lc, UIOX_IR_NAME_MAX - 1);
            uiox_ir_block_t *bc = uiox_ir_add_block(f, lc);
            uiox_ir_operand_t cond =
                uiox_cg_lower_expr(cg, f, &bc, s->loop.cond);
            uiox_ir_instr_t *jz =
                uiox_ir_emit(bc, IR_JZ, uiox_ir_none(), cond,
                             uiox_ir_label_op(le));
            strncpy(jz->label, le, UIOX_IR_NAME_MAX - 1);
            uiox_ir_block_t *bb = uiox_ir_add_block(f, lb);
            strncpy(cg->break_label, le, sizeof(cg->break_label) - 1);
            strncpy(cg->cont_label,  lc, sizeof(cg->cont_label)  - 1);
            uiox_cg_lower_stmt(cg, f, &bb, s->loop.body);
            uiox_ir_instr_t *bj =
                uiox_ir_emit(bb, IR_JMP, uiox_ir_none(),
                             uiox_ir_label_op(lc), uiox_ir_none());
            strncpy(bj->label, lc, UIOX_IR_NAME_MAX - 1);
            *b = uiox_ir_add_block(f, le);
            break;
        }

        case AST_BREAK_STMT: {
            uiox_ir_instr_t *j =
                uiox_ir_emit(*b, IR_JMP, uiox_ir_none(),
                             uiox_ir_label_op(cg->break_label),
                             uiox_ir_none());
            strncpy(j->label, cg->break_label, UIOX_IR_NAME_MAX - 1);
            break;
        }

        case AST_CONTINUE_STMT: {
            uiox_ir_instr_t *j =
                uiox_ir_emit(*b, IR_JMP, uiox_ir_none(),
                             uiox_ir_label_op(cg->cont_label),
                             uiox_ir_none());
            strncpy(j->label, cg->cont_label, UIOX_IR_NAME_MAX - 1);
            break;
        }

        default:
            break;
    }
}

/* ── Function lowering ───────────────────────────────────── */
uiox_ir_func_t *uiox_cg_lower_func(uiox_codegen_t *cg,
                                     uiox_ast_node_t *fn)
{
    uiox_ir_func_t *irf = uiox_ir_add_func(&cg->ir, fn->func.name);
    uiox_ir_block_t *entry = uiox_ir_add_block(irf, "entry");
    uiox_ir_emit(entry, IR_ENTER,
                 uiox_ir_none(), uiox_ir_none(), uiox_ir_none());

    uiox_symtab_push(&cg->symtab);

    /* parameters */
    for (int i = 0; i < fn->func.params.count; i++) {
        uiox_ast_node_t *p = fn->func.params.items[i];
        uiox_symbol_t *sym = uiox_symtab_insert(&cg->symtab,
                                                  p->param.name,
                                                  SYM_VAR);
        irf->frame_size += 8;
        sym->offset      = irf->frame_size;
        irf->param_count++;
        uiox_ir_operand_t pslot = uiox_ir_imm(i, 4);
        uiox_ir_operand_t pdst  = uiox_ir_none();
        uiox_ir_instr_t *pi =
            uiox_ir_emit(entry, IR_PARAM, pdst, pslot, uiox_ir_none());
        pi->offset = sym->offset;
    }

    uiox_ir_block_t *cur = entry;
    uiox_cg_lower_stmt(cg, irf, &cur, fn->func.body);

    /* ensure every function ends with a RET */
    uiox_ir_emit(cur, IR_LEAVE,
                 uiox_ir_none(), uiox_ir_none(), uiox_ir_none());
    uiox_ir_emit(cur, IR_RET,
                 uiox_ir_none(), uiox_ir_none(), uiox_ir_none());

    uiox_symtab_pop(&cg->symtab);
    return irf;
}

/* ── Translation unit lowering ───────────────────────────── */
void uiox_cg_lower_unit(uiox_codegen_t *cg, uiox_ast_node_t *tu)
{
    for (int i = 0; i < tu->unit.decls.count; i++) {
        uiox_ast_node_t *d = tu->unit.decls.items[i];
        switch (d->kind) {
            case AST_FUNC_DECL:
                uiox_cg_lower_func(cg, d);
                break;
            case AST_VAR_DECL: {
                /* global variable — insert into top-level symbol table */
                uiox_symbol_t *sym =
                    uiox_symtab_insert(&cg->symtab,
                                       d->var.name, SYM_VAR);
                sym->storage   = STORAGE_STATIC;
                sym->is_defined = 1;
                break;
            }
            default:
                break;
        }
    }
}

int uiox_codegen_run(uiox_codegen_t *cg,
                      uiox_ast_node_t *tu,
                      uiox_object_t   *out)
{
    (void)out;
    uiox_cg_lower_unit(cg, tu);
    return uiox_diag_has_error(cg->diag) ? -1 : 0;
}

