/*
 * 30_KIX/33_PCS/src/uiox_syscall_entry.c
 *
 * The bridge — the ONE function the BSP calls into the kernel.
 *
 * ── the contract, which is fixed and not ours to change ───────────────
 *   arm64/src/arch_irq.S:121        bl      arch_syscall_dispatch
 *   arm32/src/arch_irq.S:68         bl      arch_syscall_dispatch
 *   riscv64/src/arch_irq.S:128      call    arch_syscall_dispatch
 *   x86_64/src/arch_irq.S:387,409   call    arch_syscall_dispatch
 *
 *   arch_runtime.c (all four)  long arch_syscall_dispatch(nr, a0..a5)
 *                              { return syscall_dispatch(nr, a0..a5); }
 *
 *   uiox_bsp_stubs.c:52        __attribute__((weak))
 *                              long syscall_dispatch(long nr, long a0..a5)
 *                              { return -1L; }
 *
 * So the BSP asks the kernel for exactly one symbol.  THIS FILE provides
 * the strong definition; the weak stub yields to it at link time.
 *
 * ── why regs is NULL here ─────────────────────────────────────────────
 * The arch stubs already store the return value themselves — arm64's svc
 * stub is explicit:  `str x0, [sp, #OFF_X0]`.  Passing a context here
 * would be a second write-back into a frame whose layout is arch-private.
 *
 * ── the number space ──────────────────────────────────────────────────
 * BSD's.  SYS_READ 3, SYS_WRITE 4, SYS_OPEN 5, SYS_CLOSE 6, SYS_IOCTL 54,
 * SYS_MMAP 49, SYS_GETPID 20.
 *
 * @version 1.0.0  @date 2026-10-03
 */
#include "uiox_klibc.h"

/* The router, in 40_SCIX.  Declared here rather than included, because
 * 40_SCIX's header drags the BSD number space into this file — and this
 * file must not depend on which numbers exist, only that a router does.
 *
 * NOTE: must match 40_SCIX/uix_archSysCall.c:217 exactly. */
extern int64_t uix_arch_syscall(uint64_t nr,
                                uintptr_t a0, uintptr_t a1,
                                uintptr_t a2, uintptr_t a3,
                                uintptr_t a4, uintptr_t a5,
                                void     *regs);

long syscall_dispatch(long nr,
                      long a0, long a1, long a2,
                      long a3, long a4, long a5)
{
    return (long)uix_arch_syscall((uint64_t)nr,
                                  (uintptr_t)a0, (uintptr_t)a1,
                                  (uintptr_t)a2, (uintptr_t)a3,
                                  (uintptr_t)a4, (uintptr_t)a5,
                                  (void *)0);
}

/* 30_KIX/common/uiox_kernel_main.c's banner says, in a comment and a log
 * string, that "the arch vector table calls uiox_syscall_dispatch()
 * directly".  It does not.  This alias exists so a caller written against
 * that older name links, and so a grep for it finds a definition. */
long uiox_syscall_dispatch_entry(long nr,
                                 long a0, long a1, long a2,
                                 long a3, long a4, long a5)
{
    return syscall_dispatch(nr, a0, a1, a2, a3, a4, a5);
}
