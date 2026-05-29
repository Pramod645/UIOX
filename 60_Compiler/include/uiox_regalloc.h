#ifndef UIOX_REGALLOC_H
#define UIOX_REGALLOC_H
/*
 * uiox_regalloc.h - UIOX register allocator (linear scan)
 */
#include "uiox_ir.h"

#define UIOX_MAX_REGS    16
#define UIOX_MAX_VREGS   1024

typedef enum uiox_reg_class {
    REGCLS_GPR,    /* general-purpose integer register       */
    REGCLS_FPR,    /* floating-point register                */
} uiox_reg_class_t;

typedef struct uiox_phys_reg {
    int              id;
    const char      *name;       /* e.g. "rax", "x0", "r0"    */
    uiox_reg_class_t cls;
    int              is_callee_saved;
    int              is_arg_reg;
    int              arg_idx;    /* which argument (0-based)   */
} uiox_phys_reg_t;

typedef struct uiox_vreg {
    int              id;         /* virtual register number    */
    int              phys_id;    /* assigned physical register */
    int              spill_slot; /* stack slot if spilled      */
    int              start;      /* live range start (instr#)  */
    int              end;        /* live range end  (instr#)   */
    uiox_reg_class_t cls;
    int              size_bytes;
} uiox_vreg_t;

typedef struct uiox_regalloc {
    const uiox_phys_reg_t *phys_regs;
    int                    num_phys;
    uiox_vreg_t            vregs[UIOX_MAX_VREGS];
    int                    vreg_count;
    int                    spill_offset; /* current frame offset */
    /* active list for linear scan */
    int                    active[UIOX_MAX_REGS];
    int                    active_count;
    /* free list */
    int                    free_regs[UIOX_MAX_REGS];
    int                    free_count;
} uiox_regalloc_t;

/* Architecture-specific physical register tables */
extern const uiox_phys_reg_t uiox_regs_x86_64[];
extern const uiox_phys_reg_t uiox_regs_arm64[];
extern const uiox_phys_reg_t uiox_regs_arm32[];
extern const int              uiox_nregs_x86_64;
extern const int              uiox_nregs_arm64;
extern const int              uiox_nregs_arm32;

void uiox_regalloc_init    (uiox_regalloc_t *ra,
                             const uiox_phys_reg_t *pregs, int npregs);
void uiox_regalloc_free    (uiox_regalloc_t *ra);
void uiox_regalloc_run     (uiox_regalloc_t *ra, uiox_ir_func_t *f);
int  uiox_regalloc_lookup  (const uiox_regalloc_t *ra, int vreg_id);
int  uiox_regalloc_spilled (const uiox_regalloc_t *ra, int vreg_id);
int  uiox_regalloc_slot    (const uiox_regalloc_t *ra, int vreg_id);

#endif /* UIOX_REGALLOC_H */
