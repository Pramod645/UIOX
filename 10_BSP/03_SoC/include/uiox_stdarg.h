/*
 * 10_BSP/03_SoC/include/uiox_stdarg.h
 *
 * UIOX BSP — freestanding varargs header.
 *
 * Replaces <stdarg.h> for the BSP which is built with -nostdinc and
 * therefore cannot include the system stdarg.h.
 *
 * Implementation: GCC/Clang compiler built-ins.
 *   __builtin_va_list / __builtin_va_start / __builtin_va_arg /
 *   __builtin_va_end  / __builtin_va_copy  are part of the compiler
 *   itself — they are available under -nostdinc and -ffreestanding on
 *   all four supported architectures (ARM64, ARM32, RISC-V 64, x86-64).
 *
 * Standard aliases provided:
 *   va_list   va_start   va_arg   va_end   va_copy
 *
 * Idempotent: safe to include multiple times; include-guard prevents
 * double-definition even if a system stdarg.h is somehow also visible.
 *
 * Usage — in any BSP source file, replace:
 *   #include <stdarg.h>
 * with:
 *   #include "uiox_stdarg.h"
 *
 * @version 1.0.0
 * @date    2026-07-22
 */

#ifndef UIOX_STDARG_H
#define UIOX_STDARG_H

/* ── Standard aliases via GCC/Clang built-ins ────────────────────────────── */

typedef __builtin_va_list  va_list;

#define va_start(ap, last)   __builtin_va_start((ap), (last))
#define va_arg(ap, type)     __builtin_va_arg((ap), type)
#define va_end(ap)           __builtin_va_end(ap)
#define va_copy(dst, src)    __builtin_va_copy((dst), (src))

#endif /* UIOX_STDARG_H */
