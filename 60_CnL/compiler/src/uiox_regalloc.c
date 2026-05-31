/*
 * uiox_regalloc.c - UIOX linear-scan register allocator
 */
#include "../include/uiox_regalloc.h"
#include <string.h>
#include <stdio.h>

/* ── Physical register tables ──────────────────────────────── */

/* x86_64: RAX RCX RDX RSI RDI R8-R11 (caller-saved first) */
const uiox_phys_reg_t uiox_regs_x86_64[] = {
    {0,  "rax", REGCLS_GPR, 0, 1, 0},
    {1,  "rcx", REGCLS_GPR, 0, 1, 1},
    {2,  "rdx", REGCLS_GPR, 0, 1, 2},
    {6,  "rsi", REGCLS_GPR, 0, 1, 3},
    {7,  "rdi", REGCLS_GPR, 0, 1, 4},
    {8,  "r8",  REGCLS_GPR, 0, 1, 5},
    {9,  "r9",  REGCLS_GPR, 0, 0, -1},
    {10, "r10", REGCLS_GPR, 0, 0, -1},
    {11, "r11", REGCLS_GPR, 0, 0, -1},
    {3,  "rbx", REGCLS_GPR, 1, 0, -1},
    {12, "r12", REGCLS_GPR, 1, 0, -1},
    {13, "r13", REGCLS_GPR, 1, 0, -1},
    {14, "r14", REGCLS_GPR, 1, 0, -1},
    {15, "r15", REGCLS_GPR, 1, 0, -1},
};
const int uiox_nregs_x86_64 =
    (int)(sizeof(uiox_regs_x86_64) / sizeof(uiox_regs_x86_64[0]));

/* ARM64: X0-X7 args, X9-X15 caller-saved, X19-X28 callee */
const uiox_phys_reg_t uiox_regs_arm64[] = {
    {0,  "x0",  REGCLS_GPR, 0, 1, 0},
    {1,  "x1",  REGCLS_GPR, 0, 1, 1},
    {2,  "x2",  REGCLS_GPR, 0, 1, 2},
    {3,  "x3",  REGCLS_GPR, 0, 1, 3},
    {4,  "x4",  REGCLS_GPR, 0, 1, 4},
    {5,  "x5",  REGCLS_GPR, 0, 1, 5},
    {6,  "x6",  REGCLS_GPR, 0, 1, 6},
    {7,  "x7",  REGCLS_GPR, 0, 1, 7},
    {9,  "x9",  REGCLS_GPR, 0, 0, -1},
    {10, "x10", REGCLS_GPR, 0, 0, -1},
    {11, "x11", REGCLS_GPR, 0, 0, -1},
    {12, "x12", REGCLS_GPR, 0, 0, -1},
    {19, "x19", REGCLS_GPR, 1, 0, -1},
    {20, "x20", REGCLS_GPR, 1, 0, -1},
    {21, "x21", REGCLS_GPR, 1, 0, -1},
    {22, "x22", REGCLS_GPR, 1, 0, -1},
};
const int uiox_nregs_arm64 =
    (int)(sizeof(uiox_regs_arm64) / sizeof(uiox_regs_arm64[0]));

/* ARM32: R0-R3 args, R4-R11 callee, R12 scratch */
const uiox_phys_reg_t uiox_regs_arm32[] = {
    {0,  "r0",  REGCLS_GPR, 0, 1, 0},
    {1,  "r1",  REGCLS_GPR, 0, 1, 1},
    {2,  "r2",  REGCLS_GPR, 0, 1, 2},
    {3,  "r3",  REGCLS_GPR, 0, 1, 3},
    {12, "r12", REGCLS_GPR, 0, 0, -1},
    {4,  "r4",  REGCLS_GPR, 1, 0, -1},
    {5,  "r5",  REGCLS_GPR, 1, 0, -1},
    {6,  "r6",  REGCLS_GPR, 1, 0, -1},
    {7,  "r7",  REGCLS_GPR, 1, 0, -1},
    {8,  "r8",  REGCLS_GPR, 1, 0, -1},
    {9,  "r9",  REGCLS_GPR, 1, 0, -1},
    {10, "r10", REGCLS_GPR, 1, 0, -1},
};
const int uiox_nregs_arm32 =
    (int)(sizeof(uiox_regs_arm32) / sizeof(uiox_regs_arm32[0]));

/* ── Allocator implementation ──────────────────────────────── */

void uiox_regalloc_init(uiox_regalloc_t *ra,
                         const uiox_phys_reg_t *pregs, int npregs)
{
    memset(ra, 0, sizeof(*ra));
    ra->phys_regs  = pregs;
    ra->num_phys   = npregs;
    ra->free_count = npregs;
    for (int i = 0; i < npregs && i < UIOX_MAX_REGS; i++)
        ra->free_regs[i] = pregs[i].id;
    ra->spill_offset = 0;
}

void uiox_regalloc_free(uiox_regalloc_t *ra)
{
    memset(ra, 0, sizeof(*ra));
}

/* Compute simple live ranges by scanning IR instructions */
static void compute_live_ranges(uiox_regalloc_t *ra, uiox_ir_func_t *f)
{
    int instr_num = 0;
    for (uiox_ir_block_t *b = f->blocks; b; b = b->next) {
        for (uiox_ir_instr_t *i = b->head; i; i = i->next) {
            /* define dst temp */
            if (i->dst.kind == IR_OP_TEMP) {
                int id = i->dst.temp_id;
                if (id < UIOX_MAX_VREGS) {
                    if (ra->vregs[id].start == 0)
                        ra->vregs[id].start = instr_num;
                    ra->vregs[id].end  = instr_num;
                    ra->vregs[id].id   = id;
                    ra->vregs[id].cls  = REGCLS_GPR;
                    ra->vregs[id].size_bytes = i->dst.size_bytes
                                               ? i->dst.size_bytes : 8;
                    ra->vregs[id].phys_id    = -1;
                    ra->vregs[id].spill_slot = -1;
                    if (id >= ra->vreg_count) ra->vreg_count = id + 1;
                }
            }
            /* use src temps — extend their live range */
            if (i->src1.kind == IR_OP_TEMP) {
                int id = i->src1.temp_id;
                if (id < UIOX_MAX_VREGS)
                    ra->vregs[id].end = instr_num;
            }
            if (i->src2.kind == IR_OP_TEMP) {
                int id = i->src2.temp_id;
                if (id < UIOX_MAX_VREGS)
                    ra->vregs[id].end = instr_num;
            }
            instr_num++;
        }
    }
}

static int alloc_free_reg(uiox_regalloc_t *ra)
{
    if (ra->free_count <= 0) return -1;
    return ra->free_regs[--ra->free_count];
}

static void free_reg(uiox_regalloc_t *ra, int reg_id)
{
    if (ra->free_count < UIOX_MAX_REGS)
        ra->free_regs[ra->free_count++] = reg_id;
}

static int spill_vreg(uiox_regalloc_t *ra, int vreg_id)
{
    ra->spill_offset += 8;
    ra->vregs[vreg_id].spill_slot = ra->spill_offset;
    ra->vregs[vreg_id].phys_id    = -1;
    return ra->spill_offset;
}

void uiox_regalloc_run(uiox_regalloc_t *ra, uiox_ir_func_t *f)
{
    compute_live_ranges(ra, f);

    /* Linear scan: process vregs in order of start point */
    for (int i = 0; i < ra->vreg_count; i++) {
        uiox_vreg_t *vr = &ra->vregs[i];
        if (vr->start == 0 && vr->end == 0) continue;

        /* Expire old intervals */
        for (int ai = 0; ai < ra->active_count; ) {
            int av = ra->active[ai];
            if (ra->vregs[av].end < vr->start) {
                free_reg(ra, ra->vregs[av].phys_id);
                ra->active[ai] = ra->active[--ra->active_count];
            } else {
                ai++;
            }
        }

        int preg = alloc_free_reg(ra);
        if (preg >= 0) {
            vr->phys_id = preg;
        } else {
            /* Spill: pick vreg with furthest end point */
            int spill_v = i;
            for (int ai = 0; ai < ra->active_count; ai++) {
                int av = ra->active[ai];
                if (ra->vregs[av].end > ra->vregs[spill_v].end)
                    spill_v = av;
            }
            if (spill_v != i) {
                vr->phys_id = ra->vregs[spill_v].phys_id;
                spill_vreg(ra, spill_v);
            } else {
                spill_vreg(ra, i);
            }
        }

        if (vr->phys_id >= 0 && ra->active_count < UIOX_MAX_REGS)
            ra->active[ra->active_count++] = i;
    }

    f->frame_size = ra->spill_offset;
}

int uiox_regalloc_lookup(const uiox_regalloc_t *ra, int vreg_id)
{
    if (vreg_id < 0 || vreg_id >= ra->vreg_count) return -1;
    return ra->vregs[vreg_id].phys_id;
}

int uiox_regalloc_spilled(const uiox_regalloc_t *ra, int vreg_id)
{
    if (vreg_id < 0 || vreg_id >= ra->vreg_count) return 0;
    return ra->vregs[vreg_id].phys_id < 0;
}

int uiox_regalloc_slot(const uiox_regalloc_t *ra, int vreg_id)
{
    if (vreg_id < 0 || vreg_id >= ra->vreg_count) return 0;
    return ra->vregs[vreg_id].spill_slot;
}
