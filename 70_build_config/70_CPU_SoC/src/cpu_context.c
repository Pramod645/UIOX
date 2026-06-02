/*
 * cpu_context.c - CPU context save/restore/print
 */
#include "../include/cpu_context.h"
#include "../include/cpu_regs.h"
#include <string.h>
#include <stdio.h>

void cpu_context_init(cpu_context_t *ctx, cpu_addr_t entry,
                       cpu_addr_t stack, cpu_u64_t arg)
{
    memset(ctx, 0, sizeof(*ctx));
#if defined(UIOX_ARCH_ARM64)
    ctx->x[0]   = arg;
    ctx->sp     = stack;
    ctx->pc     = entry;
    ctx->pstate = 0x05u;  /* EL1h, IRQ masked */
#elif defined(UIOX_ARCH_X86_64)
    ctx->rdi    = arg;
    ctx->rsp    = stack;
    ctx->rip    = entry;
    ctx->rflags = 0x202ULL;  /* IF=1 */
    ctx->cs     = 0x08u;
    ctx->ss     = 0x10u;
#elif defined(UIOX_ARCH_RISCV64)
    ctx->a[0]   = arg;
    ctx->sp     = stack;
    ctx->sepc   = entry;
    ctx->sstatus= 0x120ULL;  /* SPP=1 SPIE=1 */
#endif
}

void cpu_context_save(cpu_context_t *ctx)
{
    (void)ctx;
    /* Implemented in assembly for full register save;
       C stub for reference only */
}

void cpu_context_restore(const cpu_context_t *ctx)
{
    (void)ctx;
    /* Implemented in assembly */
}

void cpu_context_switch(cpu_context_t *old_ctx,
                         const cpu_context_t *new_ctx)
{
    (void)old_ctx; (void)new_ctx;
    /* Implemented in assembly */
}

void cpu_context_print(const cpu_context_t *ctx)
{
#if defined(UIOX_ARCH_ARM64)
    printf("[ctx] PC=0x%016llx  SP=0x%016llx  PSTATE=0x%08llx\n",
           (unsigned long long)ctx->pc,
           (unsigned long long)ctx->sp,
           (unsigned long long)ctx->pstate);
    for (int i = 0; i < 16; i++)
        printf("  X%02d=0x%016llx  X%02d=0x%016llx\n",
               i, (unsigned long long)ctx->x[i],
               i+16, (unsigned long long)ctx->x[i+16]);
#elif defined(UIOX_ARCH_X86_64)
    printf("[ctx] RIP=0x%016llx  RSP=0x%016llx  RFLAGS=0x%016llx\n",
           (unsigned long long)ctx->rip,
           (unsigned long long)ctx->rsp,
           (unsigned long long)ctx->rflags);
    printf("  RAX=0x%016llx  RBX=0x%016llx\n",
           (unsigned long long)ctx->rax,
           (unsigned long long)ctx->rbx);
    printf("  RCX=0x%016llx  RDX=0x%016llx\n",
           (unsigned long long)ctx->rcx,
           (unsigned long long)ctx->rdx);
#elif defined(UIOX_ARCH_RISCV64)
    printf("[ctx] SEPC=0x%016llx  SP=0x%016llx  SSTATUS=0x%016llx\n",
           (unsigned long long)ctx->sepc,
           (unsigned long long)ctx->sp,
           (unsigned long long)ctx->sstatus);
    printf("  A0=0x%016llx  A1=0x%016llx  RA=0x%016llx\n",
           (unsigned long long)ctx->a[0],
           (unsigned long long)ctx->a[1],
           (unsigned long long)ctx->ra);
#endif
}
