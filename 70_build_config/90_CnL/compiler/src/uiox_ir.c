/*
 * uiox_ir.c - UIOX IR module / function / block / instruction
 */
#include "../include/uiox_ir.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void uiox_ir_module_init(uiox_ir_module_t *m)
{
    m->funcs      = NULL;
    m->func_count = 0;
    m->str_count  = 0;
}

void uiox_ir_module_free(uiox_ir_module_t *m)
{
    uiox_ir_func_t *f = m->funcs;
    while (f) {
        uiox_ir_func_t *nf = f->next;
        uiox_ir_block_t *b = f->blocks;
        while (b) {
            uiox_ir_block_t *nb = b->next;
            uiox_ir_instr_t *i = b->head;
            while (i) {
                uiox_ir_instr_t *ni = i->next;
                free(i);
                i = ni;
            }
            free(b);
            b = nb;
        }
        free(f);
        f = nf;
    }
    m->funcs      = NULL;
    m->func_count = 0;
}

uiox_ir_func_t *uiox_ir_add_func(uiox_ir_module_t *m, const char *name)
{
    uiox_ir_func_t *f = (uiox_ir_func_t *)calloc(1, sizeof(*f));
    strncpy(f->name, name, sizeof(f->name) - 1);
    f->next   = NULL;
    /* append */
    if (!m->funcs) {
        m->funcs = f;
    } else {
        uiox_ir_func_t *cur = m->funcs;
        while (cur->next) cur = cur->next;
        cur->next = f;
    }
    m->func_count++;
    return f;
}

uiox_ir_block_t *uiox_ir_add_block(uiox_ir_func_t *f, const char *label)
{
    uiox_ir_block_t *b = (uiox_ir_block_t *)calloc(1, sizeof(*b));
    strncpy(b->label, label, sizeof(b->label) - 1);
    b->next = NULL;
    if (!f->blocks) {
        f->blocks = b;
        f->entry  = b;
    } else {
        uiox_ir_block_t *cur = f->blocks;
        while (cur->next) cur = cur->next;
        cur->next = b;
    }
    return b;
}

uiox_ir_instr_t *uiox_ir_emit(uiox_ir_block_t *b,
                                uiox_ir_opcode_t op,
                                uiox_ir_operand_t dst,
                                uiox_ir_operand_t src1,
                                uiox_ir_operand_t src2)
{
    uiox_ir_instr_t *ins = (uiox_ir_instr_t *)calloc(1, sizeof(*ins));
    ins->op   = op;
    ins->dst  = dst;
    ins->src1 = src1;
    ins->src2 = src2;
    ins->next = NULL;
    if (!b->head) b->head = ins;
    else          b->tail->next = ins;
    b->tail = ins;
    return ins;
}

uiox_ir_operand_t uiox_ir_new_temp(uiox_ir_func_t *f, int size)
{
    uiox_ir_operand_t op;
    memset(&op, 0, sizeof(op));
    op.kind       = IR_OP_TEMP;
    op.temp_id    = f->temp_count++;
    op.size_bytes = size;
    return op;
}

uiox_ir_operand_t uiox_ir_imm(long long val, int size)
{
    uiox_ir_operand_t op;
    memset(&op, 0, sizeof(op));
    op.kind       = IR_OP_IMM;
    op.imm        = val;
    op.size_bytes = size;
    return op;
}

uiox_ir_operand_t uiox_ir_label_op(const char *name)
{
    uiox_ir_operand_t op;
    memset(&op, 0, sizeof(op));
    op.kind = IR_OP_LABEL;
    strncpy(op.name, name, sizeof(op.name) - 1);
    return op;
}

uiox_ir_operand_t uiox_ir_none(void)
{
    uiox_ir_operand_t op;
    memset(&op, 0, sizeof(op));
    op.kind = IR_OP_NONE;
    return op;
}

const char *uiox_ir_opcode_str(uiox_ir_opcode_t op)
{
    switch (op) {
        case IR_NOP:   return "nop";
        case IR_ADD:   return "add";
        case IR_SUB:   return "sub";
        case IR_MUL:   return "mul";
        case IR_DIV:   return "div";
        case IR_MOD:   return "mod";
        case IR_NEG:   return "neg";
        case IR_AND:   return "and";
        case IR_OR:    return "or";
        case IR_XOR:   return "xor";
        case IR_NOT:   return "not";
        case IR_SHL:   return "shl";
        case IR_SHR:   return "shr";
        case IR_MOV:   return "mov";
        case IR_LOAD:  return "load";
        case IR_STORE: return "store";
        case IR_LEA:   return "lea";
        case IR_JMP:   return "jmp";
        case IR_JZ:    return "jz";
        case IR_JNZ:   return "jnz";
        case IR_LABEL: return "label";
        case IR_CALL:  return "call";
        case IR_ARG:   return "arg";
        case IR_PARAM: return "param";
        case IR_RET:   return "ret";
        case IR_ENTER: return "enter";
        case IR_LEAVE: return "leave";
        case IR_EQ:    return "eq";
        case IR_NE:    return "ne";
        case IR_LT:    return "lt";
        case IR_LE:    return "le";
        case IR_GT:    return "gt";
        case IR_GE:    return "ge";
        case IR_ZEXT:  return "zext";
        case IR_SEXT:  return "sext";
        case IR_TRUNC: return "trunc";
        case IR_ADDR:  return "addr";
        case IR_DEREF: return "deref";
        default:       return "???";
    }
}

void uiox_ir_module_print(const uiox_ir_module_t *m)
{
    for (uiox_ir_func_t *f = m->funcs; f; f = f->next) {
        printf("func %s (temps=%d frame=%d)\n",
               f->name, f->temp_count, f->frame_size);
        for (uiox_ir_block_t *b = f->blocks; b; b = b->next) {
            printf("  .%s:\n", b->label);
            for (uiox_ir_instr_t *i = b->head; i; i = i->next) {
                printf("    %-8s", uiox_ir_opcode_str(i->op));
                if (i->dst.kind  == IR_OP_TEMP)
                    printf(" t%d",  i->dst.temp_id);
                if (i->src1.kind == IR_OP_TEMP)
                    printf(", t%d", i->src1.temp_id);
                else if (i->src1.kind == IR_OP_IMM)
                    printf(", %lld", i->src1.imm);
                else if (i->src1.kind == IR_OP_LABEL)
                    printf(", %s",  i->src1.name);
                if (i->src2.kind == IR_OP_TEMP)
                    printf(", t%d", i->src2.temp_id);
                else if (i->src2.kind == IR_OP_IMM)
                    printf(", %lld", i->src2.imm);
                if (i->label[0])
                    printf("  -> %s", i->label);
                printf("\n");
            }
        }
        printf("\n");
    }
}
