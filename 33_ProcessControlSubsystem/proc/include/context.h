#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdint.h>
#include <setjmp.h>

#define NGPR    16      /* number of general-purpose registers */
#define KERNEL_STACK_SZ 4096

/* ── Register Context ───────────────────────────────────────── */
typedef struct reg_context {
    uintptr_t  rc_pc;           /* program counter             */
    uintptr_t  rc_sp;           /* stack pointer               */
    uint32_t   rc_sr;           /* status / flags register     */
    uint32_t   rc_carry;        /* carry bit (for syscall err) */
    uintptr_t  rc_gpr[NGPR];    /* general purpose registers   */
    uint32_t   rc_r0;           /* return value register 0     */
    uint32_t   rc_r1;           /* return value register 1     */
} reg_context_t;

/* ── User Address Space ─────────────────────────────────────── */
typedef struct user_addr_space {
    uintptr_t  uas_text_base;   /* base of text region         */
    uint32_t   uas_text_size;
    uintptr_t  uas_data_base;   /* base of data region         */
    uint32_t   uas_data_size;
    uintptr_t  uas_stack_base;  /* base of stack region        */
    uint32_t   uas_stack_size;
    uintptr_t  uas_shmem_base;  /* shared memory base          */
    uint32_t   uas_shmem_size;
} user_addr_space_t;

/* ── System-Level Context ───────────────────────────────────── */
#define MAX_CONTEXT_LAYERS  8

typedef struct sys_context {
    void               *sc_proc_entry;   /* process table ptr  */
    void               *sc_uarea;        /* u area pointer     */
    void               *sc_pregion;      /* per-proc regions   */
    uint8_t             sc_kstack[KERNEL_STACK_SZ]; /* kernel stack */
    int                 sc_layer_top;    /* context layer index*/
    reg_context_t       sc_layers[MAX_CONTEXT_LAYERS]; /* stack */
    struct sys_context *sc_prev;         /* previous layer     */
} sys_context_t;

/* ── Full Process Context ───────────────────────────────────── */
typedef union proc_context {
    user_addr_space_t  pc_user;     /* user-level context      */
    reg_context_t      pc_regs;     /* register context        */
    sys_context_t      pc_sys;      /* system-level context    */
} proc_context_t;

/* ── Interrupt Vector Table ─────────────────────────────────── */
#define NVEC    256

typedef void (*intr_handler_t)(int vector, reg_context_t *ctx);

typedef struct intr_vector {
    int             iv_num;         /* interrupt number        */
    intr_handler_t  iv_handler;     /* handler function        */
    const char     *iv_name;        /* descriptive name        */
} intr_vector_t;

extern intr_vector_t intr_vector_table[NVEC];
extern sys_context_t *current_context;

/* ── Context Algorithm Prototypes ───────────────────────────── */
void context_save   (sys_context_t *ctx, reg_context_t *regs);
void context_restore(sys_context_t *ctx, reg_context_t *regs);
void context_switch (sys_context_t *from, sys_context_t *to);
void inthand        (int vec, reg_context_t *regs);
void intr_register  (int vec, intr_handler_t handler,
                     const char *name);

#endif /* CONTEXT_H */
