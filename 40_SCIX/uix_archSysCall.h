#ifndef __ARCH_SYS_CALL__H
#define __ARCH_SYS_CALL__H

/*
 * 40_SCIX/uix_archSysCall.h
 *
 * The USERSPACE half of the syscall mechanism — the trap.
 *
 *   my_syscall0 … my_syscall6   put the number and the arguments in the
 *   registers the architecture's SVC/ECALL/SYSCALL entry reads, execute
 *   the trap instruction, and return what the kernel left behind.
 *
 * Nothing here calls a C function.  The trap is the only door from EL0
 * to EL1, so these macros are the entire userspace side of the boundary;
 * 40_SCIX/uix_archSysCall.c is the kernel side.
 *
 * ── why the file is per-arch ────────────────────────────────────────
 * The register assignments are not a convention this tree chose — they
 * are what each architecture's trap entry has to agree with, and all of
 * them are already implemented in 10_Arch:
 *
 *   ARM64    number x8   args x0-x5   return x0   svc #0
 *            arch_irq.S arm64_svc_entry reads x8, shifts x0-x5 down one
 *            register, and does str x0, [sp, #OFF_X0] on return.
 *
 *   ARM32    number r7   args r0-r5   return r0   svc #0
 *            arch_syscall_entry's own comment: "On ARM32 EABI, syscall
 *            number is in r7.  The vector stub extracts r7 and passes it
 *            as the last argument here (nr)".  Args r0-r5; r6 is unused.
 *
 *   RISCV64  number a7   args a0-a5   return a0   ecall
 *            the same shape as ARM64 with different register names.
 *
 *   X86_64   number rax  args rdi,rsi,rdx,r10,r8,r9   return rax
 *            int 0x80.  NOTE: this is the Linux x86_64 order, NOT the
 *            SysV C order — r10 replaces rcx as the fourth argument,
 *            because the syscall instruction clobbers rcx.  The kernel
 *            side must read them in this order.
 *
 * ── a wrong register is not a compile error ─────────────────────────
 * If a macro and its trap entry disagree about one register, the call
 * routes with a garbage argument rather than failing.  That is why each
 * arch's list above is quoted from the entry that implements it.
 *
 * ── six arguments, no more ──────────────────────────────────────────
 * my_syscall6 is the ceiling on every architecture, because that is how
 * many registers each kernel entry passes through to the dispatchers.
 */

#if defined(__aarch64__)

/* ═══════════════════════════════════════════════════════════════════
 * ARM64 (AArch64) — svc #0
 *   x8 = number   x0-x5 = args   x0 = return
 * ═══════════════════════════════════════════════════════════════════ */

#define my_syscall0(num)                                                      \
({                                                                            \
    register long _num  __asm__ ("x8") = (num);                           \
    register long _arg1 __asm__ ("x0");                                   \
                                          \
    __asm__ volatile (                                                    \
        "svc #0\n"                                                    \
        : "=r"(_arg1)                                                 \
        : "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall1(num, arg1)                                                \
({                                                                            \
    register long _num  __asm__ ("x8") = (num);                           \
    register long _arg1 __asm__ ("x0") = (long)(arg1);                    \
                                          \
    __asm__ volatile (                                                    \
        "svc #0\n"                                                    \
        : "=r"(_arg1)                                                 \
        : "r"(_arg1),                                                 \
          "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall2(num, arg1, arg2)                                          \
({                                                                            \
    register long _num  __asm__ ("x8") = (num);                           \
    register long _arg1 __asm__ ("x0") = (long)(arg1);                    \
    register long _arg2 __asm__ ("x1") = (long)(arg2);                    \
                                          \
    __asm__ volatile (                                                    \
        "svc #0\n"                                                    \
        : "=r"(_arg1)                                                 \
        : "r"(_arg1), "r"(_arg2),                                     \
          "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall3(num, arg1, arg2, arg3)                                    \
({                                                                            \
    register long _num  __asm__ ("x8") = (num);                           \
    register long _arg1 __asm__ ("x0") = (long)(arg1);                    \
    register long _arg2 __asm__ ("x1") = (long)(arg2);                    \
    register long _arg3 __asm__ ("x2") = (long)(arg3);                    \
                                          \
    __asm__ volatile (                                                    \
        "svc #0\n"                                                    \
        : "=r"(_arg1)                                                 \
        : "r"(_arg1), "r"(_arg2), "r"(_arg3),                         \
          "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall4(num, arg1, arg2, arg3, arg4)                              \
({                                                                            \
    register long _num  __asm__ ("x8") = (num);                           \
    register long _arg1 __asm__ ("x0") = (long)(arg1);                    \
    register long _arg2 __asm__ ("x1") = (long)(arg2);                    \
    register long _arg3 __asm__ ("x2") = (long)(arg3);                    \
    register long _arg4 __asm__ ("x3") = (long)(arg4);                    \
                                          \
    __asm__ volatile (                                                    \
        "svc #0\n"                                                    \
        : "=r"(_arg1)                                                 \
        : "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4),             \
          "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall5(num, arg1, arg2, arg3, arg4, arg5)                        \
({                                                                            \
    register long _num  __asm__ ("x8") = (num);                           \
    register long _arg1 __asm__ ("x0") = (long)(arg1);                    \
    register long _arg2 __asm__ ("x1") = (long)(arg2);                    \
    register long _arg3 __asm__ ("x2") = (long)(arg3);                    \
    register long _arg4 __asm__ ("x3") = (long)(arg4);                    \
    register long _arg5 __asm__ ("x4") = (long)(arg5);                    \
                                          \
    __asm__ volatile (                                                    \
        "svc #0\n"                                                    \
        : "=r" (_arg1)                                                \
        : "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), \
          "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall6(num, arg1, arg2, arg3, arg4, arg5, arg6)                  \
({                                                                            \
    register long _num  __asm__ ("x8") = (num);                           \
    register long _arg1 __asm__ ("x0") = (long)(arg1);                    \
    register long _arg2 __asm__ ("x1") = (long)(arg2);                    \
    register long _arg3 __asm__ ("x2") = (long)(arg3);                    \
    register long _arg4 __asm__ ("x3") = (long)(arg4);                    \
    register long _arg5 __asm__ ("x4") = (long)(arg5);                    \
    register long _arg6 __asm__ ("x5") = (long)(arg6);                    \
                                          \
    __asm__ volatile (                                                    \
        "svc #0\n"                                                    \
        : "=r" (_arg1)                                                \
        : "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), \
          "r"(_arg6), "r"(_num)                                       \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#elif defined(__arm__)

/* ═══════════════════════════════════════════════════════════════════
 * ARM32 (ARMv7-A, EABI) — svc #0
 *   r7 = number   r0-r5 = args   r0 = return
 *
 * The number goes in r7, NOT r0 — arch_syscall_entry's own comment says
 * so: "On ARM32 EABI, syscall number is in r7.  The vector stub extracts
 * r7 and passes it as the last argument here (nr)."  The stub is:
 *
 *     svc_entry:
 *       push {r0-r12, lr}
 *       mrs  r12, spsr
 *       push {r12}
 *       bl   arch_syscall_entry
 *
 * so r0-r5 arrive as the six arguments and r7 as the number.
 * ═══════════════════════════════════════════════════════════════════ */

#define my_syscall0(num)                                                      \
({                                                                            \
    register long _num  __asm__ ("r7") = (num);                           \
    register long _arg1 __asm__ ("r0");                                   \
                                          \
    __asm__ volatile (                                                    \
        "svc #0\n"                                                    \
        : "=r"(_arg1)                                                 \
        : "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall1(num, arg1)                                                \
({                                                                            \
    register long _num  __asm__ ("r7") = (num);                           \
    register long _arg1 __asm__ ("r0") = (long)(arg1);                    \
                                          \
    __asm__ volatile (                                                    \
        "svc #0\n"                                                    \
        : "=r"(_arg1)                                                 \
        : "r"(_arg1),                                                 \
          "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall2(num, arg1, arg2)                                          \
({                                                                            \
    register long _num  __asm__ ("r7") = (num);                           \
    register long _arg1 __asm__ ("r0") = (long)(arg1);                    \
    register long _arg2 __asm__ ("r1") = (long)(arg2);                    \
                                          \
    __asm__ volatile (                                                    \
        "svc #0\n"                                                    \
        : "=r"(_arg1)                                                 \
        : "r"(_arg1), "r"(_arg2),                                     \
          "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall3(num, arg1, arg2, arg3)                                    \
({                                                                            \
    register long _num  __asm__ ("r7") = (num);                           \
    register long _arg1 __asm__ ("r0") = (long)(arg1);                    \
    register long _arg2 __asm__ ("r1") = (long)(arg2);                    \
    register long _arg3 __asm__ ("r2") = (long)(arg3);                    \
                                          \
    __asm__ volatile (                                                    \
        "svc #0\n"                                                    \
        : "=r"(_arg1)                                                 \
        : "r"(_arg1), "r"(_arg2), "r"(_arg3),                         \
          "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall4(num, arg1, arg2, arg3, arg4)                              \
({                                                                            \
    register long _num  __asm__ ("r7") = (num);                           \
    register long _arg1 __asm__ ("r0") = (long)(arg1);                    \
    register long _arg2 __asm__ ("r1") = (long)(arg2);                    \
    register long _arg3 __asm__ ("r2") = (long)(arg3);                    \
    register long _arg4 __asm__ ("r3") = (long)(arg4);                    \
                                          \
    __asm__ volatile (                                                    \
        "svc #0\n"                                                    \
        : "=r"(_arg1)                                                 \
        : "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4),             \
          "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall5(num, arg1, arg2, arg3, arg4, arg5)                        \
({                                                                            \
    register long _num  __asm__ ("r7") = (num);                           \
    register long _arg1 __asm__ ("r0") = (long)(arg1);                    \
    register long _arg2 __asm__ ("r1") = (long)(arg2);                    \
    register long _arg3 __asm__ ("r2") = (long)(arg3);                    \
    register long _arg4 __asm__ ("r3") = (long)(arg4);                    \
    register long _arg5 __asm__ ("r4") = (long)(arg5);                    \
                                          \
    __asm__ volatile (                                                    \
        "svc #0\n"                                                    \
        : "=r" (_arg1)                                                \
        : "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), \
          "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall6(num, arg1, arg2, arg3, arg4, arg5, arg6)                  \
({                                                                            \
    register long _num  __asm__ ("r7") = (num);                           \
    register long _arg1 __asm__ ("r0") = (long)(arg1);                    \
    register long _arg2 __asm__ ("r1") = (long)(arg2);                    \
    register long _arg3 __asm__ ("r2") = (long)(arg3);                    \
    register long _arg4 __asm__ ("r3") = (long)(arg4);                    \
    register long _arg5 __asm__ ("r4") = (long)(arg5);                    \
    register long _arg6 __asm__ ("r5") = (long)(arg6);                    \
                                          \
    __asm__ volatile (                                                    \
        "svc #0\n"                                                    \
        : "=r" (_arg1)                                                \
        : "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), \
          "r"(_arg6), "r"(_num)                                       \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#elif defined(__riscv)

/* ═══════════════════════════════════════════════════════════════════
 * RISC-V 64 (RV64) — ecall
 *   a7 = number   a0-a5 = args   a0 = return
 * ═══════════════════════════════════════════════════════════════════ */

#define my_syscall0(num)                                                      \
({                                                                            \
    register long _num  __asm__ ("a7") = (num);                           \
    register long _arg1 __asm__ ("a0");                                   \
                                          \
    __asm__ volatile (                                                    \
        "ecall\n"                                                     \
        : "=r"(_arg1)                                                 \
        : "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall1(num, arg1)                                                \
({                                                                            \
    register long _num  __asm__ ("a7") = (num);                           \
    register long _arg1 __asm__ ("a0") = (long)(arg1);                    \
                                          \
    __asm__ volatile (                                                    \
        "ecall\n"                                                     \
        : "=r"(_arg1)                                                 \
        : "r"(_arg1),                                                 \
          "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall2(num, arg1, arg2)                                          \
({                                                                            \
    register long _num  __asm__ ("a7") = (num);                           \
    register long _arg1 __asm__ ("a0") = (long)(arg1);                    \
    register long _arg2 __asm__ ("a1") = (long)(arg2);                    \
                                          \
    __asm__ volatile (                                                    \
        "ecall\n"                                                     \
        : "=r"(_arg1)                                                 \
        : "r"(_arg1), "r"(_arg2),                                     \
          "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall3(num, arg1, arg2, arg3)                                    \
({                                                                            \
    register long _num  __asm__ ("a7") = (num);                           \
    register long _arg1 __asm__ ("a0") = (long)(arg1);                    \
    register long _arg2 __asm__ ("a1") = (long)(arg2);                    \
    register long _arg3 __asm__ ("a2") = (long)(arg3);                    \
                                          \
    __asm__ volatile (                                                    \
        "ecall\n"                                                     \
        : "=r"(_arg1)                                                 \
        : "r"(_arg1), "r"(_arg2), "r"(_arg3),                         \
          "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall4(num, arg1, arg2, arg3, arg4)                              \
({                                                                            \
    register long _num  __asm__ ("a7") = (num);                           \
    register long _arg1 __asm__ ("a0") = (long)(arg1);                    \
    register long _arg2 __asm__ ("a1") = (long)(arg2);                    \
    register long _arg3 __asm__ ("a2") = (long)(arg3);                    \
    register long _arg4 __asm__ ("a3") = (long)(arg4);                    \
                                          \
    __asm__ volatile (                                                    \
        "ecall\n"                                                     \
        : "=r"(_arg1)                                                 \
        : "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4),             \
          "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall5(num, arg1, arg2, arg3, arg4, arg5)                        \
({                                                                            \
    register long _num  __asm__ ("a7") = (num);                           \
    register long _arg1 __asm__ ("a0") = (long)(arg1);                    \
    register long _arg2 __asm__ ("a1") = (long)(arg2);                    \
    register long _arg3 __asm__ ("a2") = (long)(arg3);                    \
    register long _arg4 __asm__ ("a3") = (long)(arg4);                    \
    register long _arg5 __asm__ ("a4") = (long)(arg5);                    \
                                          \
    __asm__ volatile (                                                    \
        "ecall\n"                                                     \
        : "=r" (_arg1)                                                \
        : "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), \
          "r"(_num)                                                   \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall6(num, arg1, arg2, arg3, arg4, arg5, arg6)                  \
({                                                                            \
    register long _num  __asm__ ("a7") = (num);                           \
    register long _arg1 __asm__ ("a0") = (long)(arg1);                    \
    register long _arg2 __asm__ ("a1") = (long)(arg2);                    \
    register long _arg3 __asm__ ("a2") = (long)(arg3);                    \
    register long _arg4 __asm__ ("a3") = (long)(arg4);                    \
    register long _arg5 __asm__ ("a4") = (long)(arg5);                    \
    register long _arg6 __asm__ ("a5") = (long)(arg6);                    \
                                          \
    __asm__ volatile (                                                    \
        "ecall\n"                                                     \
        : "=r" (_arg1)                                                \
        : "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), \
          "r"(_arg6), "r"(_num)                                       \
        : "memory", "cc"                                              \
    );                                                                    \
    _arg1;                                                                \
})

#elif defined(__x86_64__)

/* ═══════════════════════════════════════════════════════════════════
 * x86-64 (AMD64) — syscall
 *
 *   rax = number
 *   rdi, rsi, rdx, r10, r8, r9 = args    ← r10, NOT rcx
 *   rax = return
 *
 * The kernel entry is Intel's SYSCALL, and its register order is NOT the
 * SysV C calling convention: the fourth argument is r10, not rcx,
 * because SYSCALL itself clobbers rcx (it saves the return address
 * there).  A caller that puts the fourth argument in rcx would have it
 * destroyed before the kernel could read it — a wrong argument rather
 * than a failure.
 *
 * AArch64/x86_64 both put the number in a separate register from the
 * arguments, which is why the num register here is rax and not rdi.
 * ═══════════════════════════════════════════════════════════════════ */

#define my_syscall0(num)                                                      \
({                                                                            \
    register long _num  __asm__ ("rax") = (num);                          \
    register long _arg1 __asm__ ("rdi");                                  \
                                          \
    __asm__ volatile (                                                    \
        "syscall\n"                                                   \
        : "=r"(_arg1)                                                 \
        : "r"(_num)                                                   \
        : "rcx", "r11", "memory", "cc"                                \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall1(num, arg1)                                                \
({                                                                            \
    register long _num  __asm__ ("rax") = (num);                          \
    register long _arg1 __asm__ ("rdi") = (long)(arg1);                   \
                                          \
    __asm__ volatile (                                                    \
        "syscall\n"                                                   \
        : "=r"(_arg1)                                                 \
        : "r"(_arg1),                                                 \
          "r"(_num)                                                   \
        : "rcx", "r11", "memory", "cc"                                \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall2(num, arg1, arg2)                                          \
({                                                                            \
    register long _num  __asm__ ("rax") = (num);                          \
    register long _arg1 __asm__ ("rdi") = (long)(arg1);                   \
    register long _arg2 __asm__ ("rsi") = (long)(arg2);                   \
                                          \
    __asm__ volatile (                                                    \
        "syscall\n"                                                   \
        : "=r"(_arg1)                                                 \
        : "r"(_arg1), "r"(_arg2),                                     \
          "r"(_num)                                                   \
        : "rcx", "r11", "memory", "cc"                                \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall3(num, arg1, arg2, arg3)                                    \
({                                                                            \
    register long _num  __asm__ ("rax") = (num);                          \
    register long _arg1 __asm__ ("rdi") = (long)(arg1);                   \
    register long _arg2 __asm__ ("rsi") = (long)(arg2);                   \
    register long _arg3 __asm__ ("rdx") = (long)(arg3);                   \
                                          \
    __asm__ volatile (                                                    \
        "syscall\n"                                                   \
        : "=r"(_arg1)                                                 \
        : "r"(_arg1), "r"(_arg2), "r"(_arg3),                         \
          "r"(_num)                                                   \
        : "rcx", "r11", "memory", "cc"                                \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall4(num, arg1, arg2, arg3, arg4)                              \
({                                                                            \
    register long _num  __asm__ ("rax") = (num);                          \
    register long _arg1 __asm__ ("rdi") = (long)(arg1);                   \
    register long _arg2 __asm__ ("rsi") = (long)(arg2);                   \
    register long _arg3 __asm__ ("rdx") = (long)(arg3);                   \
    register long _arg4 __asm__ ("r10") = (long)(arg4);                   \
                                          \
    __asm__ volatile (                                                    \
        "syscall\n"                                                   \
        : "=r"(_arg1)                                                 \
        : "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4),             \
          "r"(_num)                                                   \
        : "rcx", "r11", "memory", "cc"                                \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall5(num, arg1, arg2, arg3, arg4, arg5)                        \
({                                                                            \
    register long _num  __asm__ ("rax") = (num);                          \
    register long _arg1 __asm__ ("rdi") = (long)(arg1);                   \
    register long _arg2 __asm__ ("rsi") = (long)(arg2);                   \
    register long _arg3 __asm__ ("rdx") = (long)(arg3);                   \
    register long _arg4 __asm__ ("r10") = (long)(arg4);                   \
    register long _arg5 __asm__ ("r8")  = (long)(arg5);                   \
                                          \
    __asm__ volatile (                                                    \
        "syscall\n"                                                   \
        : "=r" (_arg1)                                                \
        : "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), \
          "r"(_num)                                                   \
        : "rcx", "r11", "memory", "cc"                                \
    );                                                                    \
    _arg1;                                                                \
})

#define my_syscall6(num, arg1, arg2, arg3, arg4, arg5, arg6)                  \
({                                                                            \
    register long _num  __asm__ ("rax") = (num);                          \
    register long _arg1 __asm__ ("rdi") = (long)(arg1);                   \
    register long _arg2 __asm__ ("rsi") = (long)(arg2);                   \
    register long _arg3 __asm__ ("rdx") = (long)(arg3);                   \
    register long _arg4 __asm__ ("r10") = (long)(arg4);                   \
    register long _arg5 __asm__ ("r8")  = (long)(arg5);                   \
    register long _arg6 __asm__ ("r9")  = (long)(arg6);                   \
                                          \
    __asm__ volatile (                                                    \
        "syscall\n"                                                   \
        : "=r" (_arg1)                                                \
        : "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), \
          "r"(_arg6), "r"(_num)                                       \
        : "rcx", "r11", "memory", "cc"                                \
    );                                                                    \
    _arg1;                                                                \
})

#else
#error "uix_archSysCall.h: no syscall convention for this target"
#endif

#endif /* __ARCH_SYS_CALL__H */
