/*
 * uiox_emit.c - UIOX machine code emitter (x86_64 / ARM64 / ARM32)
 */
#include "../include/uiox_emit.h"
#include <string.h>
#include <stdio.h>


/* ── x86_64 emitter ─────────────────────────────────────── */
void uiox_emit_init(uiox_emit_ctx_t *ctx, uiox_target_arch_t arch,
                     uiox_object_t *obj, uiox_regalloc_t *ra)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->arch = arch;
    ctx->obj  = obj;
    ctx->ra   = ra;
    ctx->text   = uiox_sect_create(".text",   SECT_TEXT,
                                    SECT_F_ALLOC|SECT_F_EXEC, 16);
    ctx->rodata = uiox_sect_create(".rodata", SECT_RODATA,
                                    SECT_F_ALLOC, 8);
    ctx->data   = uiox_sect_create(".data",   SECT_DATA,
                                    SECT_F_ALLOC|SECT_F_WRITE, 8);
    ctx->bss    = uiox_sect_create(".bss",    SECT_BSS,
                                    SECT_F_ALLOC|SECT_F_WRITE|SECT_F_NOLOAD,
                                    8);
    if (obj->hdr.sect_count < UIOX_OBJ_MAX_SECTS) {
        unsigned int si = obj->hdr.sect_count;
        obj->sects[si++] = ctx->text;
        obj->sects[si++] = ctx->rodata;
        obj->sects[si++] = ctx->data;
        obj->sects[si++] = ctx->bss;
        obj->hdr.sect_count = si;
    }
}






/* ── x86_64 helpers ─────────────────────────────────────── */

/* Write REX.W prefix */
static void x86_rex_w(uiox_emit_ctx_t *ctx, int dst, int src)
{
    unsigned char rex = 0x48;
    if (src >= 8) rex |= 0x04;   /* REX.R */
    if (dst >= 8) rex |= 0x01;   /* REX.B */
    uiox_sect_write8(ctx->text, rex);
}

/* Write ModRM byte mod=11 (register direct) */
static void x86_modrm_rr(uiox_emit_ctx_t *ctx, int reg, int rm)
{
    uiox_sect_write8(ctx->text,
        (unsigned char)(0xC0 | ((reg & 7) << 3) | (rm & 7)));
}

/* Write ModRM byte mod=10 (reg + disp32) */
static void x86_modrm_disp32(uiox_emit_ctx_t *ctx, int reg, int base)
{
    uiox_sect_write8(ctx->text,
        (unsigned char)(0x80 | ((reg & 7) << 3) | (base & 7)));
}

void uiox_emit_x86_mov_rr(uiox_emit_ctx_t *ctx, int dst, int src, int sz)
{
    (void)sz;
    x86_rex_w(ctx, dst, src);
    uiox_sect_write8(ctx->text, 0x89);   /* MOV r/m64, r64 */
    x86_modrm_rr(ctx, src, dst);
}

void uiox_emit_x86_mov_ri(uiox_emit_ctx_t *ctx, int dst, long long imm,
                            int sz)
{
    (void)sz;
    unsigned char rex = (unsigned char)(0x48 | (dst >= 8 ? 0x01 : 0x00));
    uiox_sect_write8(ctx->text, rex);
    uiox_sect_write8(ctx->text,
                     (unsigned char)(0xB8 | (dst & 7)));  /* MOV r64,imm64 */
    uiox_sect_write64(ctx->text, (unsigned long long)imm);
}

void uiox_emit_x86_alu_rr(uiox_emit_ctx_t *ctx, unsigned char opc,
                            int dst, int src, int sz)
{
    (void)sz;
    x86_rex_w(ctx, dst, src);
    uiox_sect_write8(ctx->text, opc);
    x86_modrm_rr(ctx, src, dst);
}

void uiox_emit_x86_push(uiox_emit_ctx_t *ctx, int reg)
{
    if (reg >= 8) uiox_sect_write8(ctx->text, 0x41); /* REX.B */
    uiox_sect_write8(ctx->text, (unsigned char)(0x50 | (reg & 7)));
}

void uiox_emit_x86_pop(uiox_emit_ctx_t *ctx, int reg)
{
    if (reg >= 8) uiox_sect_write8(ctx->text, 0x41);
    uiox_sect_write8(ctx->text, (unsigned char)(0x58 | (reg & 7)));
}

void uiox_emit_x86_call(uiox_emit_ctx_t *ctx, const char *sym)
{
    uiox_sect_write8(ctx->text, 0xE8);   /* CALL rel32 */
    unsigned int patch_off = uiox_sect_pos(ctx->text);
    uiox_sect_write32(ctx->text, 0);     /* placeholder — reloc fills this */
    /* record relocation */
    int sym_idx = uiox_object_add_sym(ctx->obj, sym, 0, 0, 0,
                                       UIOX_BIND_GLOBAL, UIOX_SYM_FUNC);
    uiox_object_add_reloc(ctx->obj, patch_off, (unsigned int)sym_idx,
                           UIOX_RELOC_REL32, -4, 0);
}

void uiox_emit_x86_ret(uiox_emit_ctx_t *ctx)
{
    uiox_sect_write8(ctx->text, 0xC3);
}

void uiox_emit_x86_jmp(uiox_emit_ctx_t *ctx, const char *label)
{
    (void)label;
    uiox_sect_write8(ctx->text, 0xE9);
    uiox_sect_write32(ctx->text, 0);   /* linker/backpatch fills */
}

void uiox_emit_x86_jcc(uiox_emit_ctx_t *ctx, unsigned char cc,
                         const char *label)
{
    (void)label;
    uiox_sect_write8(ctx->text, 0x0F);
    uiox_sect_write8(ctx->text, (unsigned char)(0x80 | (cc & 0xF)));
    uiox_sect_write32(ctx->text, 0);
}

void uiox_emit_x86_label(uiox_emit_ctx_t *ctx, const char *label)
{
    unsigned int off = uiox_sect_pos(ctx->text);
    uiox_object_add_sym(ctx->obj, label, 0,
                         (unsigned long long)off, 0,
                         UIOX_BIND_LOCAL, UIOX_SYM_NOTYPE);
}

void uiox_emit_x86_load(uiox_emit_ctx_t *ctx,
                          int dst, int base, int disp, int sz)
{
    (void)sz;
    x86_rex_w(ctx, dst, base);
    uiox_sect_write8(ctx->text, 0x8B);   /* MOV r64, r/m64 */
    x86_modrm_disp32(ctx, dst, base);
    uiox_sect_write32(ctx->text, (unsigned int)disp);
}

void uiox_emit_x86_store(uiox_emit_ctx_t *ctx,
                           int src, int base, int disp, int sz)
{
    (void)sz;
    x86_rex_w(ctx, src, base);
    uiox_sect_write8(ctx->text, 0x89);   /* MOV r/m64, r64 */
    x86_modrm_disp32(ctx, src, base);
    uiox_sect_write32(ctx->text, (unsigned int)disp);
}

void uiox_emit_x86_prologue(uiox_emit_ctx_t *ctx, int frame_size)
{
    /* push rbp */
    uiox_emit_x86_push(ctx, 5 /* rbp */);
    /* mov rbp, rsp */
    uiox_emit_x86_mov_rr(ctx, 5 /*rbp*/, 4 /*rsp*/, 8);
    /* sub rsp, frame_size */
    if (frame_size > 0) {
        uiox_sect_write8(ctx->text, 0x48);   /* REX.W */
        uiox_sect_write8(ctx->text, 0x81);   /* SUB r/m64, imm32 */
        uiox_sect_write8(ctx->text, 0xEC);   /* ModRM: /5 rsp */
        uiox_sect_write32(ctx->text, (unsigned int)frame_size);
    }
}

void uiox_emit_x86_epilogue(uiox_emit_ctx_t *ctx)
{
    /* mov rsp, rbp */
    uiox_emit_x86_mov_rr(ctx, 4 /*rsp*/, 5 /*rbp*/, 8);
    /* pop rbp */
    uiox_emit_x86_pop(ctx, 5 /*rbp*/);
    uiox_emit_x86_ret(ctx);
}

/* ── ARM64 helpers ──────────────────────────────────────── */

static void a64_emit32(uiox_emit_ctx_t *ctx, unsigned int instr)
{
    uiox_sect_write32(ctx->text, instr);
}

void uiox_emit_a64_mov_rr(uiox_emit_ctx_t *ctx, int dst, int src)
{
    /* ORR Xdst, XZR, Xsrc  — canonical MOV alias */
    unsigned int ins = 0xAA0003E0u |
                       ((unsigned int)(src & 0x1F) << 16) |
                       ((unsigned int)(dst & 0x1F));
    a64_emit32(ctx, ins);
}

void uiox_emit_a64_mov_imm(uiox_emit_ctx_t *ctx, int dst, long long imm)
{
    /* MOVZ Xdst, #imm16  (low 16 bits) */
    unsigned int ins = 0xD2800000u |
                       (((unsigned int)(imm & 0xFFFF)) << 5) |
                       ((unsigned int)(dst & 0x1F));
    a64_emit32(ctx, ins);
    /* MOVK for upper bits if needed */
    if (imm >> 16) {
        unsigned int ins2 = 0xF2A00000u |
                            (((unsigned int)((imm >> 16) & 0xFFFF)) << 5) |
                            ((unsigned int)(dst & 0x1F));
        a64_emit32(ctx, ins2);
    }
}

void uiox_emit_a64_alu(uiox_emit_ctx_t *ctx, unsigned int opc,
                         int dst, int src1, int src2)
{
    /* Data processing (register): sf=1 opc shift=0 N=0 */
    unsigned int ins = (1u << 31) |           /* sf=1 64-bit */
                       (opc << 29) |
                       (0x0Bu << 24) |         /* fixed=01011 */
                       ((unsigned int)(src2 & 0x1F) << 16) |
                       ((unsigned int)(src1 & 0x1F) <<  5) |
                       ((unsigned int)(dst  & 0x1F));
    a64_emit32(ctx, ins);
}

void uiox_emit_a64_ldr(uiox_emit_ctx_t *ctx, int dst, int base, int off)
{
    /* LDR Xdst, [Xbase, #off] unsigned offset, size=11 (64-bit) */
    unsigned int uoff = ((unsigned int)off >> 3) & 0xFFF;
    unsigned int ins  = 0xF9400000u |
                        (uoff << 10) |
                        ((unsigned int)(base & 0x1F) << 5) |
                        ((unsigned int)(dst  & 0x1F));
    a64_emit32(ctx, ins);
}

void uiox_emit_a64_str(uiox_emit_ctx_t *ctx, int src, int base, int off)
{
    unsigned int uoff = ((unsigned int)off >> 3) & 0xFFF;
    unsigned int ins  = 0xF9000000u |
                        (uoff << 10) |
                        ((unsigned int)(base & 0x1F) << 5) |
                        ((unsigned int)(src  & 0x1F));
    a64_emit32(ctx, ins);
}

void uiox_emit_a64_bl(uiox_emit_ctx_t *ctx, const char *sym)
{
    unsigned int patch_off = uiox_sect_pos(ctx->text);
    a64_emit32(ctx, 0x94000000u);   /* BL +0  placeholder */
    int sym_idx = uiox_object_add_sym(ctx->obj, sym, 0, 0, 0,
                                       UIOX_BIND_GLOBAL, UIOX_SYM_FUNC);
    uiox_object_add_reloc(ctx->obj, patch_off, (unsigned int)sym_idx,
                           UIOX_RELOC_A64_CALL, 0, 0);
}

void uiox_emit_a64_ret(uiox_emit_ctx_t *ctx)
{
    a64_emit32(ctx, 0xD65F03C0u);   /* RET X30 */
}

void uiox_emit_a64_b(uiox_emit_ctx_t *ctx, const char *label)
{
    (void)label;
    a64_emit32(ctx, 0x14000000u);   /* B +0  (linker fills) */
}

void uiox_emit_a64_bcond(uiox_emit_ctx_t *ctx, unsigned int cc,
                           const char *label)
{
    (void)label;
    unsigned int ins = 0x54000000u | (cc & 0xF);
    a64_emit32(ctx, ins);
}

void uiox_emit_a64_prologue(uiox_emit_ctx_t *ctx, int frame_size)
{
    /* STP X29, X30, [SP, #-frame_size]! */
    int sz = (frame_size > 0) ? frame_size : 16;
    unsigned int imm7 = (unsigned int)((-sz >> 3) & 0x7F);
    unsigned int ins  = 0xA9800000u | (imm7 << 15) |
                        (30u << 10) | (31u << 5) | 29u;
    a64_emit32(ctx, ins);
    /* MOV X29, SP */
    a64_emit32(ctx, 0x910003FDu);
}

void uiox_emit_a64_epilogue(uiox_emit_ctx_t *ctx)
{
    /* LDP X29, X30, [SP], #16 */
    a64_emit32(ctx, 0xA8C17BFDu);
    uiox_emit_a64_ret(ctx);
}

/* ── ARM32 helpers ──────────────────────────────────────── */

static void a32_emit32(uiox_emit_ctx_t *ctx, unsigned int instr)
{
    uiox_sect_write32(ctx->text, instr);
}

void uiox_emit_a32_mov_rr(uiox_emit_ctx_t *ctx, int dst, int src)
{
    /* MOV Rdst, Rsrc  AL cond, MOV, S=0, Rn=0 */
    unsigned int ins = 0xE1A00000u |
                       ((unsigned int)(dst & 0xF) << 12) |
                       ((unsigned int)(src & 0xF));
    a32_emit32(ctx, ins);
}

void uiox_emit_a32_mov_imm(uiox_emit_ctx_t *ctx, int dst, unsigned int imm)
{
    /* MOV Rd, #imm8  (simple 8-bit rotated immediate) */
    unsigned int ins = 0xE3A00000u |
                       ((unsigned int)(dst & 0xF) << 12) |
                       (imm & 0xFF);
    a32_emit32(ctx, ins);
}

void uiox_emit_a32_alu(uiox_emit_ctx_t *ctx, unsigned int opc,
                         int dst, int rn, int rm)
{
    /* Data processing: AL cond, opc, S=0 */
    unsigned int ins = 0xE0000000u |
                       ((opc & 0xF) << 21) |
                       ((unsigned int)(rn  & 0xF) << 16) |
                       ((unsigned int)(dst & 0xF) << 12) |
                       ((unsigned int)(rm  & 0xF));
    a32_emit32(ctx, ins);
}

void uiox_emit_a32_ldr(uiox_emit_ctx_t *ctx, int dst, int base, int off)
{
    /* LDR Rd, [Rn, #off]  P=1 U=1 W=0 L=1 */
    unsigned int u   = (off >= 0) ? 1u : 0u;
    unsigned int abs = (unsigned int)((off < 0) ? -off : off);
    unsigned int ins = 0xE5100000u |
                       (u << 23) |
                       ((unsigned int)(base & 0xF) << 16) |
                       ((unsigned int)(dst  & 0xF) << 12) |
                       (abs & 0xFFF);
    if (u) ins |= (1u << 23); else ins &= ~(1u << 23);
    ins |= (1u << 20); /* L=1 load */
    a32_emit32(ctx, ins);
}

void uiox_emit_a32_str(uiox_emit_ctx_t *ctx, int src, int base, int off)
{
    unsigned int u   = (off >= 0) ? 1u : 0u;
    unsigned int abs = (unsigned int)((off < 0) ? -off : off);
    unsigned int ins = 0xE5000000u |
                       (u << 23) |
                       ((unsigned int)(base & 0xF) << 16) |
                       ((unsigned int)(src  & 0xF) << 12) |
                       (abs & 0xFFF);
    a32_emit32(ctx, ins);
}

void uiox_emit_a32_bl(uiox_emit_ctx_t *ctx, const char *sym)
{
    unsigned int patch_off = uiox_sect_pos(ctx->text);
    a32_emit32(ctx, 0xEB000000u);   /* BL +0 placeholder */
    int sym_idx = uiox_object_add_sym(ctx->obj, sym, 0, 0, 0,
                                       UIOX_BIND_GLOBAL, UIOX_SYM_FUNC);
    uiox_object_add_reloc(ctx->obj, patch_off, (unsigned int)sym_idx,
                           UIOX_RELOC_ARM_B26, -8, 0);
}

void uiox_emit_a32_bx_lr(uiox_emit_ctx_t *ctx)
{
    a32_emit32(ctx, 0xE12FFF1Eu);   /* BX LR */
}

void uiox_emit_a32_prologue(uiox_emit_ctx_t *ctx, int frame_size)
{
    /* PUSH {R11, LR} */
    a32_emit32(ctx, 0xE92D4800u);
    /* MOV R11, SP */
    uiox_emit_a32_mov_rr(ctx, 11, 13);
    /* SUB SP, SP, #frame_size */
    if (frame_size > 0 && frame_size <= 255) {
        unsigned int ins = 0xE24DD000u | ((unsigned int)frame_size & 0xFF);
        a32_emit32(ctx, ins);
    }
}

void uiox_emit_a32_epilogue(uiox_emit_ctx_t *ctx)
{
    /* MOV SP, R11 */
    uiox_emit_a32_mov_rr(ctx, 13, 11);
    /* POP {R11, PC} */
    a32_emit32(ctx, 0xE8BD8800u);
}

/* ── IR instruction dispatch ────────────────────────────── */

static void emit_x86_ir_instr(uiox_emit_ctx_t *ctx,
                                uiox_ir_instr_t *ins)
{
    /* Map virtual register to physical */
    int dst_p  = (ins->dst.kind  == IR_OP_TEMP)
                 ? uiox_regalloc_lookup(ctx->ra, ins->dst.temp_id)  : 0;
    int src1_p = (ins->src1.kind == IR_OP_TEMP)
                 ? uiox_regalloc_lookup(ctx->ra, ins->src1.temp_id) : 0;
    int src2_p = (ins->src2.kind == IR_OP_TEMP)
                 ? uiox_regalloc_lookup(ctx->ra, ins->src2.temp_id) : 0;

    switch (ins->op) {
        case IR_MOV:
            if (ins->src1.kind == IR_OP_IMM)
                uiox_emit_x86_mov_ri(ctx, dst_p, ins->src1.imm, 8);
            else
                uiox_emit_x86_mov_rr(ctx, dst_p, src1_p, 8);
            break;
        case IR_ADD:
            uiox_emit_x86_alu_rr(ctx, 0x03, dst_p, src2_p, 8);
            break;
        case IR_SUB:
            uiox_emit_x86_alu_rr(ctx, 0x2B, dst_p, src2_p, 8);
            break;
        case IR_AND:
            uiox_emit_x86_alu_rr(ctx, 0x23, dst_p, src2_p, 8);
            break;
        case IR_OR:
            uiox_emit_x86_alu_rr(ctx, 0x0B, dst_p, src2_p, 8);
            break;
        case IR_XOR:
            uiox_emit_x86_alu_rr(ctx, 0x33, dst_p, src2_p, 8);
            break;
        case IR_LOAD:
            uiox_emit_x86_load(ctx, dst_p, 5 /*rbp*/,
                                -ins->offset, 8);
            break;
        case IR_STORE:
            uiox_emit_x86_store(ctx, src1_p, 5 /*rbp*/,
                                 -ins->offset, 8);
            break;
        case IR_CALL:
            uiox_emit_x86_call(ctx, ins->label);
            break;
        case IR_RET:
            if (ins->src1.kind == IR_OP_TEMP)
                uiox_emit_x86_mov_rr(ctx, 0 /*rax*/, src1_p, 8);
            uiox_emit_x86_epilogue(ctx);
            break;
        case IR_ENTER:
            uiox_emit_x86_prologue(ctx, ctx->frame_size);
            break;
        case IR_LEAVE:
            break;   /* epilogue emitted on RET */
        case IR_JMP:
            uiox_emit_x86_jmp(ctx, ins->label);
            break;
        case IR_JZ:
            /* TEST src, src then JE */
            uiox_emit_x86_alu_rr(ctx, 0x85, src1_p, src1_p, 8);
            uiox_emit_x86_jcc(ctx, 0x4, ins->label); /* JE */
            break;
        case IR_JNZ:
            uiox_emit_x86_alu_rr(ctx, 0x85, src1_p, src1_p, 8);
            uiox_emit_x86_jcc(ctx, 0x5, ins->label); /* JNE */
            break;
        case IR_LABEL:
            uiox_emit_x86_label(ctx, ins->label);
            break;
        default:
            break;
    }
    (void)src2_p;
}

static void emit_a64_ir_instr(uiox_emit_ctx_t *ctx,
                                uiox_ir_instr_t *ins)
{
    int dst_p  = (ins->dst.kind  == IR_OP_TEMP)
                 ? uiox_regalloc_lookup(ctx->ra, ins->dst.temp_id)  : 0;
    int src1_p = (ins->src1.kind == IR_OP_TEMP)
                 ? uiox_regalloc_lookup(ctx->ra, ins->src1.temp_id) : 0;
    int src2_p = (ins->src2.kind == IR_OP_TEMP)
                 ? uiox_regalloc_lookup(ctx->ra, ins->src2.temp_id) : 0;

    switch (ins->op) {
        case IR_MOV:
            if (ins->src1.kind == IR_OP_IMM)
                uiox_emit_a64_mov_imm(ctx, dst_p, ins->src1.imm);
            else
                uiox_emit_a64_mov_rr(ctx, dst_p, src1_p);
            break;
        case IR_ADD: uiox_emit_a64_alu(ctx, 0, dst_p, src1_p, src2_p); break;
        case IR_SUB: uiox_emit_a64_alu(ctx, 2, dst_p, src1_p, src2_p); break;
        case IR_AND: uiox_emit_a64_alu(ctx, 0, dst_p, src1_p, src2_p); break;
        case IR_OR:  uiox_emit_a64_alu(ctx, 1, dst_p, src1_p, src2_p); break;
        case IR_XOR: uiox_emit_a64_alu(ctx, 2, dst_p, src1_p, src2_p); break;
        case IR_LOAD:
            uiox_emit_a64_ldr(ctx, dst_p, 29 /*x29*/, -ins->offset);
            break;
        case IR_STORE:
            uiox_emit_a64_str(ctx, src1_p, 29 /*x29*/, -ins->offset);
            break;
        case IR_CALL:
            uiox_emit_a64_bl(ctx, ins->label);
            break;
        case IR_RET:
            if (ins->src1.kind == IR_OP_TEMP)
                uiox_emit_a64_mov_rr(ctx, 0 /*x0*/, src1_p);
            uiox_emit_a64_epilogue(ctx);
            break;
        case IR_ENTER:
            uiox_emit_a64_prologue(ctx, ctx->frame_size);
            break;
        case IR_JMP:
            uiox_emit_a64_b(ctx, ins->label);
            break;
        case IR_LABEL: {
            unsigned int off = uiox_sect_pos(ctx->text);
            uiox_object_add_sym(ctx->obj, ins->label, 0,
                                (unsigned long long)off, 0,
                                UIOX_BIND_LOCAL, UIOX_SYM_NOTYPE);
            break;
        }
        default:
            break;
    }
    (void)src2_p;
}

static void emit_a32_ir_instr(uiox_emit_ctx_t *ctx,
                                uiox_ir_instr_t *ins)
{
    int dst_p  = (ins->dst.kind  == IR_OP_TEMP)
                 ? uiox_regalloc_lookup(ctx->ra, ins->dst.temp_id)  : 0;
    int src1_p = (ins->src1.kind == IR_OP_TEMP)
                 ? uiox_regalloc_lookup(ctx->ra, ins->src1.temp_id) : 0;
    int src2_p = (ins->src2.kind == IR_OP_TEMP)
                 ? uiox_regalloc_lookup(ctx->ra, ins->src2.temp_id) : 0;

    switch (ins->op) {
        case IR_MOV:
            if (ins->src1.kind == IR_OP_IMM)
                uiox_emit_a32_mov_imm(ctx, dst_p,
                                       (unsigned int)ins->src1.imm);
            else
                uiox_emit_a32_mov_rr(ctx, dst_p, src1_p);
            break;
        case IR_ADD: uiox_emit_a32_alu(ctx, 4, dst_p, src1_p, src2_p); break;
        case IR_SUB: uiox_emit_a32_alu(ctx, 2, dst_p, src1_p, src2_p); break;
        case IR_AND: uiox_emit_a32_alu(ctx, 0, dst_p, src1_p, src2_p); break;
        case IR_OR:  uiox_emit_a32_alu(ctx, 12, dst_p, src1_p, src2_p); break;
        case IR_XOR: uiox_emit_a32_alu(ctx, 1, dst_p, src1_p, src2_p); break;
        case IR_LOAD:
            uiox_emit_a32_ldr(ctx, dst_p, 11 /*r11*/, -ins->offset);
            break;
        case IR_STORE:
            uiox_emit_a32_str(ctx, src1_p, 11 /*r11*/, -ins->offset);
            break;
        case IR_CALL:
            uiox_emit_a32_bl(ctx, ins->label);
            break;
        case IR_RET:
            if (ins->src1.kind == IR_OP_TEMP)
                uiox_emit_a32_mov_rr(ctx, 0 /*r0*/, src1_p);
            uiox_emit_a32_epilogue(ctx);
            break;
        case IR_ENTER:
            uiox_emit_a32_prologue(ctx, ctx->frame_size);
            break;
        default:
            break;
    }
    (void)src2_p;
}

/* ── Function emitter ───────────────────────────────────── */
void uiox_emit_func(uiox_emit_ctx_t *ctx, uiox_ir_func_t *f)
{
    ctx->frame_size = f->frame_size;
    strncpy(ctx->cur_func, f->name, sizeof(ctx->cur_func) - 1);

    /* Export function symbol */
    unsigned int fn_off = uiox_sect_pos(ctx->text);
    uiox_object_add_sym(ctx->obj, f->name, 0,
                         (unsigned long long)fn_off, 0,
                         UIOX_BIND_GLOBAL, UIOX_SYM_FUNC);

    for (uiox_ir_block_t *b = f->blocks; b; b = b->next) {
        /* Block label */
        unsigned int blk_off = uiox_sect_pos(ctx->text);
        uiox_object_add_sym(ctx->obj, b->label, 0,
                             (unsigned long long)blk_off, 0,
                             UIOX_BIND_LOCAL, UIOX_SYM_NOTYPE);

        for (uiox_ir_instr_t *ins = b->head; ins; ins = ins->next) {
            switch (ctx->arch) {
                case UIOX_TARGET_X86_64:
                    emit_x86_ir_instr(ctx, ins);
                    break;
                case UIOX_TARGET_ARM64:
                    emit_a64_ir_instr(ctx, ins);
                    break;
                case UIOX_TARGET_ARM32:
                    emit_a32_ir_instr(ctx, ins);
                    break;
                }
        }
    }
}
        
/* ── Module emitter ─────────────────────────────────────── */
void uiox_emit_module(uiox_emit_ctx_t *ctx, uiox_ir_module_t *m)
{
    for (uiox_ir_func_t *f = m->funcs; f; f = f->next)
        uiox_emit_func(ctx, f);
}
        