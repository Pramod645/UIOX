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
 *
 * EVERY board block defines SOC_CLK_BASE and SOC_STORAGE_BASE (0 = TODO),
 * and every arch block provides the accessor macros its backend uses
 * (GIC-400 for arm32, CLINT/PLIC for riscv64), so a backend compiles
 * regardless of which board is selected.
 */
#ifndef UIOX_SOC_MAP_H
#define UIOX_SOC_MAP_H

#include "uiox_soc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =====================================================================
 * Board selection — normalise the user's choice to one macro per board.
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
#    define SOC_CLK_BASE         0x00000000UL   /* no PLL ctrl on QEMU */
#    define SOC_STORAGE_BASE     0x00000000UL   /* virtio is the medium */
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

/* ── ARM64 GIC accessors (used by SoC backends) ─────────────────────── */
#  ifndef SOC_GIC_DIST_CTLR
#    define SOC_GIC_DIST_CTLR      (SOC_GIC_DIST_BASE + 0x000u)
#  endif
#  ifndef SOC_GIC_DIST_IGROUPR
#    define SOC_GIC_DIST_IGROUPR   (SOC_GIC_DIST_BASE + 0x080u)
#  endif
#  ifndef SOC_GIC_DIST_ISENABLER
#    define SOC_GIC_DIST_ISENABLER(irq) \
        (SOC_GIC_DIST_BASE + 0x100u + 4u * (((irq) / 32u)))
#  endif
#  ifndef SOC_GIC_DIST_IPRIORITYR
#    define SOC_GIC_DIST_IPRIORITYR(irq) \
        (SOC_GIC_DIST_BASE + 0x400u + ((irq) & ~3u))
#  endif
#  ifndef SOC_GIC_CPU_CTLR
#    define SOC_GIC_CPU_CTLR       (SOC_GIC_CPU_BASE + 0x000u)
#  endif
#  ifndef SOC_GIC_CPU_PMR
#    define SOC_GIC_CPU_PMR        (SOC_GIC_CPU_BASE + 0x004u)
#  endif
#  ifndef SOC_GIC_CPU_IAR
#    define SOC_GIC_CPU_IAR        (SOC_GIC_CPU_BASE + 0x00Cu)
#  endif
#  ifndef SOC_GIC_CPU_EOIR
#    define SOC_GIC_CPU_EOIR       (SOC_GIC_CPU_BASE + 0x010u)
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
#    define SOC_CLK_BASE         0x00000000UL   /* no PLL ctrl on QEMU */
#    define SOC_STORAGE_BASE     0x00000000UL   /* virtio is the medium */
#    define SOC_GIC_DIST_BASE    0x08000000UL   /* GIC-400 distributor */
#    define SOC_GIC_CPU_BASE     0x08010000UL   /* GIC-400 CPU iface   */
#    ifndef SOC_TIMER0_BASE
#      define SOC_TIMER0_BASE    SOC_TIMER_BASE
#    endif
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
#    ifndef SOC_TIMER0_BASE
#      define SOC_TIMER0_BASE    SOC_TIMER_BASE
#    endif
#    ifndef SOC_STORAGE_BASE
#      define SOC_STORAGE_BASE   0x00000000UL   /* TODO */
#    endif
#    ifndef SOC_CLK_BASE
#      define SOC_CLK_BASE       0x00000000UL   /* TODO */
#    endif
#    ifndef SOC_GIC_DIST_BASE
#      define SOC_GIC_DIST_BASE  0x00000000UL   /* TODO */
#    endif
#    ifndef SOC_GIC_CPU_BASE
#      define SOC_GIC_CPU_BASE   0x00000000UL   /* TODO */
#    endif
#    define UIOX_BOARD_STR      "generic-arm32"
#  else
#    error "no ARM32 board selected"
#  endif

/* ── ARM32 GIC-400 accessors (arm32 backend uses a GIC, not the VIC) ── */
#  ifndef SOC_GIC_DIST_CTLR
#    define SOC_GIC_DIST_CTLR      (SOC_GIC_DIST_BASE + 0x000u)
#  endif
#  ifndef SOC_GIC_DIST_ISENABLER
#    define SOC_GIC_DIST_ISENABLER(irq) \
        (SOC_GIC_DIST_BASE + 0x100u + 4u * (((irq) / 32u)))
#  endif
#  ifndef SOC_GIC_CPU_CTLR
#    define SOC_GIC_CPU_CTLR       (SOC_GIC_CPU_BASE + 0x000u)
#  endif
#  ifndef SOC_GIC_CPU_PMR
#    define SOC_GIC_CPU_PMR        (SOC_GIC_CPU_BASE + 0x004u)
#  endif
#  ifndef SOC_GIC_CPU_IAR
#    define SOC_GIC_CPU_IAR        (SOC_GIC_CPU_BASE + 0x00Cu)
#  endif
#  ifndef SOC_GIC_CPU_EOIR
#    define SOC_GIC_CPU_EOIR       (SOC_GIC_CPU_BASE + 0x010u)
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
//#    define SOC_COM1_BASE        0x03F8UL      /* 16550 COM1 port    */
#    define SOC_PIT_PORT         0x0040UL      /* PIT ch0 I/O port   (== SOC_PIT_BASE) */
#    define SOC_PIT_BASE         0x0040UL      /* PIT as MMIO alias                    */
#    define SOC_UART0_PORT       0x03F8UL      /* COM1 I/O port      (== SOC_COM1_BASE)*/
#    define SOC_COM1_BASE        0x03F8UL      /* COM1 as MMIO alias                   */
#    define SOC_UART_IRQ         4u            /* COM1 legacy IRQ line                 */
#    define SOC_HPET_BASE        0xFED00000UL  /* HPET MMIO window                     */
#    define SOC_IOAPIC_IRQ_BASE  0x10UL        /* first routed APIC vector             */

#    define SOC_VIRTIO_BASE      0x10001000UL
#    define SOC_VIRTIO_STRIDE    0x1000u
#    define SOC_VIRTIO_IRQ       1u
#    ifndef SOC_CLK_BASE
#      define SOC_CLK_BASE       0x00000000UL   /* TODO: chipset ctl */
#    endif
#    ifndef SOC_STORAGE_BASE
#      define SOC_STORAGE_BASE   0x00000000UL   /* TODO: AHCI ABAR   */
#    endif
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
#    define SOC_CLK_BASE         SOC_CLINT_BASE /* CLINT holds mtime */
#    define SOC_STORAGE_BASE     0x00000000UL   /* virtio is the medium */
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
#    ifndef SOC_CLK_BASE
#      define SOC_CLK_BASE       SOC_CLINT_BASE /* CLINT holds mtime */
#    endif
#    define UIOX_BOARD_STR      "generic-riscv64"
#  else
#    error "no RISC-V board selected"
#  endif

/* ── CLINT accessors (msip / mtimecmp / mtime, per hart) ────────────── */
/* QEMU virt CLINT layout:                                                 */
/*   msip[hart]      at 0x0000 + 4*hart                                    */
/*   mtimecmp[hart]  at 0x4000 + 8*hart                                    */
/*   mtime           at 0xBFF8                                             */
#  ifndef SOC_CLINT_MSIP
#    define SOC_CLINT_MSIP(h)      (SOC_CLINT_BASE + 0x0000u + 4u  * (h))
#  endif
#  ifndef SOC_CLINT_MTIMECMP
#    define SOC_CLINT_MTIMECMP(h)  (SOC_CLINT_BASE + 0x4000u + 8u  * (h))
#  endif
#  ifndef SOC_CLINT_MTIME
#    define SOC_CLINT_MTIME        (SOC_CLINT_BASE + 0xBFF8u)
#  endif

/* ── PLIC accessors (priority / enable / threshold, per context) ────── */
/* QEMU virt PLIC layout:                                                  */
/*   priority[irq]  at 0x000000 + 4*irq                                    */
/*   enable[ctx]    at 0x002000 + 0x80*ctx                                 */
/*   threshold[ctx] at 0x200000 + 0x1000*ctx                               */
#  ifndef SOC_PLIC_PRIORITY
#    define SOC_PLIC_PRIORITY(i)   (SOC_PLIC_BASE + 0x000000u + 4u * (i))
#  endif
#  ifndef SOC_PLIC_ENABLE
#    define SOC_PLIC_ENABLE(c)     (SOC_PLIC_BASE + 0x002000u + 0x80u * (c))
#  endif
#  ifndef SOC_PLIC_THRESHOLD
#    define SOC_PLIC_THRESHOLD(c)  (SOC_PLIC_BASE + 0x200000u + 0x1000u * (c))
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
