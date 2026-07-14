#ifndef UIOX_IR_H
#define UIOX_IR_H
/*
 * uiox_ir.h - UIOX Three-Address Code Intermediate Representation
 */

#define UIOX_IR_NAME_MAX  64
#define UIOX_IR_POOL_MAX  65536

/* -- IR operand kinds --------------------------------------- */
typedef enum uiox_ir_op_kind {
    IR_OP_NONE,
    IR_OP_TEMP,      /* virtual register t0, t1, ...          */
    IR_OP_IMM,       /* integer immediate                     */
    IR_OP_FIMM,      /* float immediate                       */
    IR_OP_LABEL,     /* label reference                       */
    IR_OP_GLOBAL,    /* global variable name                  */
    IR_OP_STR,       /* string literal label                  */
} uiox_ir_op_kind_t;

typedef struct uiox_ir_operand {
    uiox_ir_op_kind_t kind;
    union {
        int        temp_id;
        long long  imm;
        double     fimm;
        char       name[UIOX_IR_NAME_MAX];
    };
    int size_bytes;   /* 1/2/4/8                               */
} uiox_ir_operand_t;

/* -- IR instruction opcodes --------------------------------- */
typedef enum uiox_ir_opcode {
    IR_NOP,
    /* Arithmetic */
    IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_MOD,
    IR_NEG,
    /* Bitwise */
    IR_AND, IR_OR, IR_XOR, IR_NOT,
    IR_SHL, IR_SHR, IR_SAR,
    /* Comparison */
    IR_EQ,  IR_NE,
    IR_LT,  IR_LE,
    IR_GT,  IR_GE,
    IR_ULT, IR_ULE, IR_UGT, IR_UGE,
    /* Move */
    IR_MOV,
    IR_LOAD,         /* dst = *[src + offset]                 */
    IR_STORE,        /* *[dst + offset] = src                 */
    IR_LEA,          /* dst = &sym                            */
    /* Control flow */
    IR_JMP,          /* goto label                            */
    IR_JZ,           /* if !cond goto label                   */
    IR_JNZ,          /* if  cond goto label                   */
    IR_LABEL,        /* label definition                      */
    /* Function */
    IR_CALL,         /* dst = call func (args...)             */
    IR_ARG,          /* push argument                         */
    IR_PARAM,        /* receive parameter                     */
    IR_RET,          /* return [val]                          */
    IR_ENTER,        /* function prologue                     */
    IR_LEAVE,        /* function epilogue                     */
    /* Type conversion */
    IR_ZEXT,         /* zero-extend                           */
    IR_SEXT,         /* sign-extend                           */
    IR_TRUNC,        /* truncate                              */
    IR_ITOF,         /* int to float                         */
    IR_FTOI,         /* float to int                         */
    /* Address-of / deref */
    IR_ADDR,         /* dst = &var                            */
    IR_DEREF,        /* dst = *ptr                            */
    /* Phi (for SSA) */
    IR_PHI,
} uiox_ir_opcode_t;

/* -- IR instruction ----------------------------------------- */
typedef struct uiox_ir_instr {
    uiox_ir_opcode_t   op;
    uiox_ir_operand_t  dst;
    uiox_ir_operand_t  src1;
    uiox_ir_operand_t  src2;
    int                offset;       /* for LOAD/STORE displacement */
    char               label[UIOX_IR_NAME_MAX]; /* for IR_LABEL/JMP */
    struct uiox_ir_instr *next;
} uiox_ir_instr_t;

/* -- IR basic block ----------------------------------------- */
typedef struct uiox_ir_block {
    char               label[UIOX_IR_NAME_MAX];
    uiox_ir_instr_t   *head;
    uiox_ir_instr_t   *tail;
    struct uiox_ir_block *next;
} uiox_ir_block_t;

/* -- IR function -------------------------------------------- */
typedef struct uiox_ir_func {
    char               name[UIOX_IR_NAME_MAX];
    uiox_ir_block_t   *entry;
    uiox_ir_block_t   *blocks;
    int                temp_count;
    int                param_count;
    int                frame_size;
    struct uiox_ir_func *next;
} uiox_ir_func_t;

/* -- IR module ---------------------------------------------- */
typedef struct uiox_ir_module {
    uiox_ir_func_t  *funcs;
    int              func_count;
    int              str_count;
} uiox_ir_module_t;

void              uiox_ir_module_init (uiox_ir_module_t *m);
void              uiox_ir_module_free (uiox_ir_module_t *m);
void              uiox_ir_module_print(const uiox_ir_module_t *m);
uiox_ir_func_t   *uiox_ir_add_func   (uiox_ir_module_t *m, const char *name);
uiox_ir_block_t  *uiox_ir_add_block  (uiox_ir_func_t *f, const char *label);
uiox_ir_instr_t  *uiox_ir_emit       (uiox_ir_block_t *b, uiox_ir_opcode_t op,
                                       uiox_ir_operand_t dst,
                                       uiox_ir_operand_t src1,
                                       uiox_ir_operand_t src2);
uiox_ir_operand_t uiox_ir_new_temp   (uiox_ir_func_t *f, int size);
uiox_ir_operand_t uiox_ir_imm        (long long val, int size);
uiox_ir_operand_t uiox_ir_label_op   (const char *name);
uiox_ir_operand_t uiox_ir_none       (void);
const char       *uiox_ir_opcode_str (uiox_ir_opcode_t op);

#endif /* UIOX_IR_H */
