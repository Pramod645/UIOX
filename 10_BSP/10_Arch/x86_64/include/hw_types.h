/*
 * 10_BSP/10_Arch/x86_64/include/hw_types.h
 *
 * x86-64 hardware context type.
 *
 * hw_context_t captures the CPU register state pushed by the IDT stub
 * before calling the C dispatcher.  Layout mirrors the push order in
 * 10_Arch/x86_64/src/vectors.S (System V AMD64 ABI callee-save order
 * followed by error code and vector number pushed by the stub).
 *
 *   Pushed by CPU  : error_code, rip, cs, rflags, rsp, ss
 *   Pushed by stub : rax,rcx,rdx,rbx,rbp,rsi,rdi,r8–r15, vector
 *
 * @version 1.0.0  @date 2026-07-22
 */
#ifndef UIOX_HW_TYPES_X86_64_H
#define UIOX_HW_TYPES_X86_64_H

#ifndef UIOX_BASETYPES_COMPAT
#  define UIOX_BASETYPES_COMPAT
#endif
#include "uiox_base_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hw_context {
    /* GPRs — saved by the IDT entry stub (push order: r15→rax) */
    uint64_t  r15, r14, r13, r12;
    uint64_t  r11, r10, r9,  r8;
    uint64_t  rdi, rsi, rbp, rbx;
    uint64_t  rdx, rcx, rax;
    /* Vector number and error code pushed by stub / CPU */
    uint64_t  vector;      /* IDT vector number (= IRQ number for hardware IRQs) */
    uint64_t  error_code;  /* 0 for hardware IRQs; non-zero for exceptions        */
    /* CPU-pushed frame (iretq frame) */
    uint64_t  rip;
    uint64_t  cs;
    uint64_t  rflags;
    uint64_t  rsp;         /* user RSP (ring 3→0 transition) */
    uint64_t  ss;
    /* Dispatcher-filled field */
    int       irq_num;     /* same as (int)vector for hardware IRQs */
    uint32_t  reserved;
} hw_context_t;

typedef void (*hw_irq_handler_t)(int irq, hw_context_t *ctx, void *dev_id);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_HW_TYPES_X86_64_H */
