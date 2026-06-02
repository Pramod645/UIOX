#ifndef CPU_CONTEXT_H
#define CPU_CONTEXT_H
/*
 * cpu_context.h - CPU context save / restore
 */
#include "cpu_types.h"

/* ============================================================
   ARM64 context frame
   ============================================================ */
#if defined(UIOX_ARCH_ARM64)
typedef struct cpu_context {
    cpu_u64_t x[31];      /* X0-X30                             */
    cpu_u64_t sp;
    cpu_u64_t pc;
    cpu_u64_t pstate;
    cpu_u64_t esr;
    cpu_u64_t far;
    cpu_u64_t elr;
    cpu_u64_t spsr;
} cpu_context_t;

/* ============================================================
   x86-64 context frame (matches hardware interrupt stack)
   ============================================================ */
#elif defined(UIOX_ARCH_X86_64)
typedef struct cpu_context {
    cpu_u64_t r15, r14, r13, r12;
    cpu_u64_t r11, r10, r9,  r8;
    cpu_u64_t rbp, rdi, rsi, rdx;
    cpu_u64_t rcx, rbx, rax;
    cpu_u64_t rip;
    cpu_u64_t cs;
    cpu_u64_t rflags;
    cpu_u64_t rsp;
    cpu_u64_t ss;
    cpu_u64_t error_code;
    cpu_u64_t vector;
} cpu_context_t;

/* ============================================================
   RISC-V context frame
   ============================================================ */
#elif defined(UIOX_ARCH_RISCV64)
typedef struct cpu_context {
    cpu_u64_t ra;
    cpu_u64_t sp;
    cpu_u64_t gp;
    cpu_u64_t tp;
    cpu_u64_t t[7];    /* t0-t6                                 */
    cpu_u64_t s[12];   /* s0-s11                                */
    cpu_u64_t a[8];    /* a0-a7                                 */
    cpu_u64_t sepc;
    cpu_u64_t sstatus;
    cpu_u64_t scause;
    cpu_u64_t stval;
} cpu_context_t;
#else
typedef struct cpu_context { cpu_u64_t dummy; } cpu_context_t;
#endif

/* -- API ---------------------------------------------------- */
void cpu_context_save    (cpu_context_t *ctx);
void cpu_context_restore (const cpu_context_t *ctx);
void cpu_context_init    (cpu_context_t *ctx,
                           cpu_addr_t entry,
                           cpu_addr_t stack,
                           cpu_u64_t  arg);
void cpu_context_print   (const cpu_context_t *ctx);
void cpu_context_switch  (cpu_context_t *old_ctx,
                           const cpu_context_t *new_ctx);

#endif /* CPU_CONTEXT_H */
