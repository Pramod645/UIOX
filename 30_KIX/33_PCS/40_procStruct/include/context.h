/*
 * 30_KIX/33_PCS/40_procStruct/include/context.h
 *
 * Freestanding fixes (v2.0)
 *
 * @version 2.0.0  @date 2026-07-24
 */
#ifndef CONTEXT_H
#define CONTEXT_H
#include "uiox_klibc.h"
#define NGPR            16
#define KERNEL_STACK_SZ 4096
typedef struct reg_context {
    uintptr_t  rc_pc; uintptr_t rc_sp; uint32_t rc_sr; uint32_t rc_carry;
    uintptr_t  rc_gpr[NGPR]; uint32_t rc_r0; uint32_t rc_r1;
} reg_context_t;
typedef struct user_addr_space {
    uintptr_t uas_text_base;  uint32_t uas_text_size;
    uintptr_t uas_data_base;  uint32_t uas_data_size;
    uintptr_t uas_stack_base; uint32_t uas_stack_size;
    uintptr_t uas_shmem_base; uint32_t uas_shmem_size;
} user_addr_space_t;
#define MAX_CONTEXT_LAYERS  8
typedef struct sys_context {
    void               *sc_proc_entry; void *sc_uarea; void *sc_pregion;
    uint8_t             sc_kstack[KERNEL_STACK_SZ];
    int                 sc_layer_top;
    reg_context_t       sc_layers[MAX_CONTEXT_LAYERS];
    struct sys_context *sc_prev;
} sys_context_t;
typedef union proc_context {
    user_addr_space_t pc_user; reg_context_t pc_regs; sys_context_t pc_sys;
} proc_context_t;
#define NVEC  256
typedef void (*intr_handler_t)(int vector, reg_context_t *ctx);
typedef struct intr_vector {
    int iv_num; intr_handler_t iv_handler; const char *iv_name;
} intr_vector_t;
extern intr_vector_t  intr_vector_table[NVEC];
extern sys_context_t *current_context;
void context_save   (sys_context_t *ctx, reg_context_t *regs);
void context_restore(sys_context_t *ctx, reg_context_t *regs);
void context_switch (sys_context_t *from, sys_context_t *to);
void inthand        (int vec, reg_context_t *regs);
void intr_register  (int vec, intr_handler_t handler, const char *name);
#endif /* CONTEXT_H */
