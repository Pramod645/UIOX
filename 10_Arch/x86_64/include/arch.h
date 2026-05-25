#ifndef __ARCH_X86_64_H
#define __ARCH_X86_64_H

/*
 * arch.h  —  x86_64 top-level architecture header
 *
 * Mirrors: 10_Arch/arm32/include/arch.h
 *
 * Included by all subsystems that need arch-specific
 * definitions (register widths, page size, word size, etc.)
 */

#include <stdint.h>
#include <stddef.h>

/* ── Word / pointer sizes ────────────────────────────────── */
#define ARCH_BITS           64
#define ARCH_WORD_SIZE      8           /* bytes                */
#define ARCH_PTR_SIZE       8           /* bytes                */
#define ARCH_REG_COUNT      16          /* general-purpose regs */

/* ── Byte order ──────────────────────────────────────────── */
#define ARCH_LITTLE_ENDIAN  1

/* ── Page geometry ───────────────────────────────────────── */
#define ARCH_PAGE_SHIFT     12
#define ARCH_PAGE_SIZE      (1UL << ARCH_PAGE_SHIFT)   /* 4 KiB  */
#define ARCH_PAGE_MASK      (~(ARCH_PAGE_SIZE - 1))
#define ARCH_HUGE_PAGE_SIZE (2UL << 20)                /* 2 MiB  */

/* ── Address-space split ─────────────────────────────────── */
#define ARCH_USER_START     0x0000000000001000UL
#define ARCH_USER_END       0x00007FFFFFFFFFFFUL
#define ARCH_KERN_START     0xFFFF800000000000UL
#define ARCH_KERN_END       0xFFFFFFFFFFFFFFFFUL

/* ── Stack ───────────────────────────────────────────────── */
#define ARCH_STACK_ALIGN    16          /* ABI: 16-byte aligned */
#define ARCH_STACK_TOP      ARCH_USER_END
#define ARCH_KERN_STACK_SZ  (4 * ARCH_PAGE_SIZE)  /* 16 KiB   */

/* ── Cache line ──────────────────────────────────────────── */
#define ARCH_CACHE_LINE_SZ  64

/* ── Privilege levels (rings) ────────────────────────────── */
#define ARCH_RING_KERNEL    0
#define ARCH_RING_USER      3

/* ── Null pointer ────────────────────────────────────────── */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* ── Architecture identifier string ─────────────────────── */
#define ARCH_NAME           "x86_64"

/* ── Compiler helpers ────────────────────────────────────── */
#define ARCH_INLINE         static inline __attribute__((always_inline))
#define ARCH_NORETURN       __attribute__((noreturn))
#define ARCH_PACKED         __attribute__((packed))
#define ARCH_ALIGNED(n)     __attribute__((aligned(n)))

/* ── Barrier macros ──────────────────────────────────────── */
#define arch_mb()   __asm__ volatile("mfence" ::: "memory")
#define arch_rmb()  __asm__ volatile("lfence" ::: "memory")
#define arch_wmb()  __asm__ volatile("sfence" ::: "memory")
#define arch_isb()  __asm__ volatile("" ::: "memory")   /* no-op */

/* ── NOP / HALT ──────────────────────────────────────────── */
#define arch_nop()  __asm__ volatile("nop")
#define arch_hlt()  __asm__ volatile("hlt")
#define arch_cli()  __asm__ volatile("cli")
#define arch_sti()  __asm__ volatile("sti")

/* sub-headers */
#include "cpu.h"
#include "mmu.h"
#include "gdt.h"
#include "idt.h"
#include "io.h"

#endif /* __ARCH_X86_64_H */
