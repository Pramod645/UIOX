/*
 * 10_BSP/03_SoC/include/uiox_soc_map.h
 *
 * UIOX SoC platform MMIO base addresses — BOARD-SELECTED.
 *
 * Two-level selection:
 *   1. Architecture:  __aarch64__ / __arm__ / __x86_64__ / __riscv
 *   2. Board:         UIOX_BOARD_<ARCH>_<NAME> (from the build system)
 *
 * One board is selected per build. If only the arch is known, the
 * QEMU board for that arch is used as the default so nothing breaks.
 *
 * This header is the SINGLE SOURCE OF TRUTH for hardware addresses.
 * Board descriptors and drivers reference SOC_* macros — never literals.
 */
#ifndef UIOX_SOC_MAP_H
#define UIOX_SOC_MAP_H

#include "uiox_soc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =====================================================================
 * Board selection — normalise the user's choice to one macro per board.
 * Define exactly one on the command line (the Makefile does this).
 * ===================================================================== */
#if !defined(UIOX_BOARD_QEMU_ARM64)  && !defined(UIOX_BOARD_QEMU_ARM32)   && \
    !defined(UIOX_BOARD_QEMU_RISCV64)&& !defined(UIOX_BOARD_QEMU_X86_64)  && \
    !defined(UIOX_BOARD_GENERIC_ARM64)&& !defined(UIOX_BOARD_GENERIC_ARM32)&& \
    !defined(UIOX_BOARD_GENERIC_RISCV64) && !defined(UIOX_BOARD_GENERIC_X86_64)
/* No board chosen — default to the QEMU board matching the arch. */
#  if defined(__aarch64__) || defined(ARCH_ARM64)
#    define UIOX_BOARD_QEMU_ARM64   1
#  elif defined(__arm__) || defined(ARCH_ARM32)
#    define UIOX_BOARD_QEMU_ARM32   1
#  elif defined(__x86_64__) || defined(ARCH_X86_64)
#    define UIOX_BOARD_QEMU_X86_64  1
#  elif defined(__riscv) || defined(ARCH_RISCV64)
#    define UIOX_BOARD_QEMU_RISCV64 1
#  endif
#endif

/* =====================================================================
 * ARM64 boards
 * ===================================================================== */
#if defined(__aarch64__) || defined(ARCH_ARM64)

#  if defined(UIOX_BOARD_QEMU_ARM64)
    /* --- QEMU virt, Cortex-A57/A76 -------------------------------- */
#    define SOC_DRAM_BASE        0x40000000UL
#    define SOC_DRAM_SIZE        0x04000000UL   /* 64 MB               */
#    define SOC_MMIO_BASE        0x10000000UL
#    define SOC_GIC_DIST_BASE    0x08000000UL
#    define SOC_GIC_CPU_BASE     0x08010000UL
#    define SOC_GIC_REDIST_BASE  0x080A0000UL
#    define SOC_UART0_BASE       0x09000000UL
#    define SOC_UART1_BASE       0x09040000UL
#    define SOC_UART_IRQ         33u
#    define SOC_TIMER_BASE       0x09010000UL
#    define SOC_CLINT_BASE       0x02000000UL
#    define SOC_VIRTIO_BASE      0x0A000000UL
#    define SOC_VIRTIO_STRIDE    0x1000u
#    define SOC_VIRTIO_IRQ       48u
#    define UIOX_BOARD_STR       "qemu-virt-arm64"

#  elif defined(UIOX_BOARD_GENERIC_ARM64)
    /* --- Generic ARM64 SoC — TODO(board): real addresses ---------- */
    /*  Set SOC_* before including this header to override.           */
#    ifndef SOC_DRAM_BASE
#      define SOC_DRAM_BASE      0x40000000UL
#    endif
#    ifndef SOC_MMIO_BASE
#      define SOC_MMIO_BASE      0x00000000UL   /* TODO */
#    endif
#    ifndef SOC_GIC_DIST_BASE
#      define SOC_GIC_DIST_BASE  0x00000000UL   /* TODO */
#    endif
#    ifndef SOC_GIC_CPU_BASE
#      define SOC_GIC_CPU_BASE   0x00000000UL   /* TODO */
#    endif
#    ifndef SOC_UART0_BASE
#      define SOC_UART0_BASE     0x00000000UL   /* TODO */
#    endif
#    ifndef SOC_UART_IRQ
#      define SOC_UART_IRQ       0u
#    endif
#    ifndef SOC_TIMER_BASE
#      define SOC_TIMER_BASE     0x00000000UL   /* TODO */
#    endif
#    ifndef SOC_STORAGE_BASE
#      define SOC_STORAGE_BASE   0x00000000UL   /* TODO */
#    endif
#    ifndef SOC_CLK_BASE
#      define SOC_CLK_BASE       0x00000000UL   /* TODO: PLL/clock ctl */
#    endif
#    define UIOX_BOARD_STR      "generic-arm64"
#  else
#    error "no ARM64 board selected"
#  endif

/* =====================================================================
 * ARM32 boards
 * ===================================================================== */
#elif defined(__arm__) || defined(ARCH_ARM32)

#  if defined(UIOX_BOARD_QEMU_ARM32)
    /* --- QEMU versatilepb, Cortex-A9 ------------------------------ */
#    define SOC_DRAM_BASE        0x00100000UL
#    define SOC_DRAM_SIZE        0x0F000000UL   /* 240 MB              */
#    define SOC_MMIO_BASE        0x10000000UL
#    define SOC_UART0_BASE       0x101F1000UL   /* PL011               */
#    define SOC_UART_IRQ         12u
#    define SOC_VIC_BASE         0x10140000UL   /* PL190               */
#    define SOC_TIMER_BASE       0x101E2000UL   /* SP804               */
#    define SOC_VIRTIO_BASE      0x0A000000UL
#    define SOC_VIRTIO_STRIDE    0x1000u
#    define SOC_VIRTIO_IRQ       48u
#    define UIOX_BOARD_STR       "qemu-versatilepb-arm32"

#  elif defined(UIOX_BOARD_GENERIC_ARM32)
#    ifndef SOC_DRAM_BASE
#      define SOC_DRAM_BASE      0x00100000UL
#    endif
#    ifndef SOC_UART0_BASE
#      define SOC_UART0_BASE     0x00000000UL   /* TODO */
#    endif
#    ifndef SOC_VIC_BASE
#      define SOC_VIC_BASE       0x00000000UL   /* TODO */
#    endif
#    ifndef SOC_TIMER_BASE
#      define SOC_TIMER_BASE     0x00000000UL   /* TODO */
#    endif
#    ifndef SOC_STORAGE_BASE
#      define SOC_STORAGE_BASE   0x00000000UL   /* TODO */
#    endif
#    ifndef SOC_CLK_BASE
#      define SOC_CLK_BASE       0x00000000UL   /* TODO */
#    endif
#    define UIOX_BOARD_STR      "generic-arm32"
#  else
#    error "no ARM32 board selected"
#  endif

/* =====================================================================
 * x86-64 boards
 * ===================================================================== */
#elif defined(__x86_64__) || defined(ARCH_X86_64)

#  if defined(UIOX_BOARD_QEMU_X86_64) || defined(UIOX_BOARD_GENERIC_X86_64)
#    define SOC_DRAM_BASE        0x00000000UL
#    define SOC_DRAM_SIZE        0x100000000UL
#    define SOC_MMIO_BASE        0xFEC00000UL
#    define SOC_LAPIC_BASE       0xFEE00000UL
#    define SOC_IOAPIC_BASE      0xFEC00000UL
#    define SOC_PIT_BASE         0x0040UL      /* PIT channel 0 port */
#    define SOC_COM1_BASE        0x03F8UL      /* 16550 COM1 port    */
#    define SOC_VIRTIO_BASE      0x10001000UL
#    define SOC_VIRTIO_STRIDE    0x1000u
#    define SOC_VIRTIO_IRQ       1u
#    define UIOX_BOARD_STR       "x86_64"
#  else
#    error "no x86_64 board selected"
#  endif

/* =====================================================================
 * RISC-V 64 boards
 * ===================================================================== */
#elif defined(__riscv) || defined(ARCH_RISCV64)

#  if defined(UIOX_BOARD_QEMU_RISCV64)
#    define SOC_DRAM_BASE        0x80000000UL
#    define SOC_DRAM_SIZE        0x08000000UL   /* 128 MB              */
#    define SOC_MMIO_BASE        0x10000000UL
#    define SOC_UART0_BASE       0x10000000UL   /* NS16550A            */
#    define SOC_UART_IRQ         10u
#    define SOC_CLINT_BASE       0x02000000UL
#    define SOC_CLINT_SIZE       0x00010000UL
#    define SOC_PLIC_BASE        0x0C000000UL
#    define SOC_PLIC_SIZE        0x04000000UL
#    define SOC_TEST_BASE        0x00100000UL
#    define SOC_TEST_RESET       0x00005555UL
#    define SOC_VIRTIO_BASE      0x10001000UL
#    define SOC_VIRTIO_STRIDE    0x1000u
#    define SOC_VIRTIO_IRQ       1u
#    define UIOX_BOARD_STR       "qemu-virt-riscv64"

#  elif defined(UIOX_BOARD_GENERIC_RISCV64)
#    ifndef SOC_DRAM_BASE
#      define SOC_DRAM_BASE      0x80000000UL
#    endif
#    ifndef SOC_UART0_BASE
#      define SOC_UART0_BASE     0x00000000UL   /* TODO */
#    endif
#    ifndef SOC_CLINT_BASE
#      define SOC_CLINT_BASE     0x02000000UL   /* TODO: verify */
#    endif
#    ifndef SOC_PLIC_BASE
#      define SOC_PLIC_BASE      0x00000000UL   /* TODO */
#    endif
#    ifndef SOC_STORAGE_BASE
#      define SOC_STORAGE_BASE   0x00000000UL   /* TODO */
#    endif
#    define UIOX_BOARD_STR      "generic-riscv64"
#  else
#    error "no RISC-V board selected"
#  endif

#else
#  error "uiox_soc_map.h: unsupported architecture"
#endif

/* Common helpers */
#define SOC_IRQ_INVALID      0xFFFFu
#define SOC_IRQ_IS_VALID(n)  ((uint32_t)(n) != SOC_IRQ_INVALID)

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SOC_MAP_H */
