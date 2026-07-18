/*
 * 02_FwHal/include/uiox_soc_map.h
 * UIOX SoC abstraction layer — platform MMIO base addresses,
 * IRQ numbers, and address-space constants shared across all
 * architecture backends.
 *
 * Architecture-specific values are selected at compile time via
 * the ARCH_ARM64 / ARCH_ARM32 / ARCH_X86_64 / ARCH_RISCV64 macros
 * injected by the root Makefile.
 *
 * Override any address by defining it before including this header.
 */
#ifndef UIOX_SOC_MAP_H
#define UIOX_SOC_MAP_H

#include "uiox_soc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * ARM64 (QEMU virt machine) MMIO map
 * ====================================================================== */
#if defined(__aarch64__) || defined(ARCH_ARM64)

#  define SOC_DRAM_BASE         0x40000000UL
#  define SOC_DRAM_SIZE         0x04000000UL   /* 64 MB default            */
#  define SOC_MMIO_BASE         0x10000000UL

/* ── GIC-400 / GIC-600 ───────────────────────────────────────────────── */
#  define SOC_GIC_DIST_BASE     0x08000000UL
#  define SOC_GIC_CPU_BASE      0x08010000UL
#  define SOC_GIC_REDIST_BASE   0x080A0000UL   /* GICv3 redistributors     */
#  define SOC_GIC_ITS_BASE      0x08080000UL   /* GICv3 ITS                */

/* ── PL011 UART ──────────────────────────────────────────────────────── */
#  define SOC_UART0_BASE        0x09000000UL
#  define SOC_UART1_BASE        0x09040000UL
#  define SOC_UART_IRQ          33u

/* ── ARM Generic Timer ───────────────────────────────────────────────── */
#  define SOC_TIMER_IRQ_PHYS    30u            /* EL1 physical timer PPI   */
#  define SOC_TIMER_IRQ_VIRT    27u            /* EL1 virtual  timer PPI   */
#  define SOC_TIMER_IRQ_HYP     26u            /* EL2 timer PPI            */

/* ── VirtIO ──────────────────────────────────────────────────────────── */
#  define SOC_VIRTIO_BASE       0x0A000000UL
#  define SOC_VIRTIO_IRQ        48u

/* ── PCIe ECAM ───────────────────────────────────────────────────────── */
#  define SOC_PCIE_ECAM_BASE    0x4010000000UL
#  define SOC_PCIE_MMIO_BASE    0x10000000UL
#  define SOC_PCIE_IRQ_BASE     32u

/* ── CLINT (memory-mapped timer for SMP wake) ────────────────────────── */
#  define SOC_CLINT_BASE        0x2000000UL
#  define SOC_CLINT_SIZE        0x0010000UL

/* ── SMMU (ARM IOMMU) ────────────────────────────────────────────────── */
#  define SOC_SMMU_BASE         0x09050000UL

/* =========================================================================
 * ARM32 (QEMU versatilepb / virt) MMIO map
 * ====================================================================== */
#elif defined(__arm__) || defined(ARCH_ARM32)

#  define SOC_DRAM_BASE         0x60000000UL
#  define SOC_DRAM_SIZE         0x04000000UL
#  define SOC_MMIO_BASE         0x10000000UL

#  define SOC_GIC_DIST_BASE     0x08000000UL
#  define SOC_GIC_CPU_BASE      0x08010000UL

#  define SOC_UART0_BASE        0x10009000UL
#  define SOC_UART_IRQ          44u

#  define SOC_TIMER0_BASE       0x10011000UL
#  define SOC_TIMER0_IRQ        36u
#  define SOC_TIMER1_IRQ        37u

#  define SOC_IDE_BASE          0x1000A000UL
#  define SOC_IDE_IRQ           46u

#  define SOC_VIRTIO_BASE       0x0A000000UL
#  define SOC_VIRTIO_IRQ        48u

/* =========================================================================
 * x86-64 (QEMU Q35 / generic PC) I/O and MMIO map
 * ====================================================================== */
#elif defined(__x86_64__) || defined(ARCH_X86_64)

#  define SOC_DRAM_BASE         0x0000000000000000UL
#  define SOC_DRAM_SIZE         0x0000000100000000UL   /* 4 GB             */
#  define SOC_MMIO_BASE         0x00000000FEC00000UL

/* ── APIC ────────────────────────────────────────────────────────────── */
#  define SOC_LAPIC_BASE        0xFEE00000UL   /* Local APIC MMIO          */
#  define SOC_IOAPIC_BASE       0xFEC00000UL   /* I/O APIC MMIO            */
#  define SOC_IOAPIC_IRQ_BASE   32u

/* ── HPET ────────────────────────────────────────────────────────────── */
#  define SOC_HPET_BASE         0xFED00000UL
#  define SOC_HPET_IRQ          8u

/* ── Legacy 16550A UART (COM1 / COM2) ───────────────────────────────── */
#  define SOC_UART0_PORT        0x3F8u         /* COM1 port I/O            */
#  define SOC_UART1_PORT        0x2F8u         /* COM2 port I/O            */
#  define SOC_UART_IRQ          4u             /* COM1 IRQ                 */

/* ── PIT (8253/8254) ─────────────────────────────────────────────────── */
#  define SOC_PIT_PORT          0x40u
#  define SOC_PIT_IRQ           0u

/* ── PCI config space ────────────────────────────────────────────────── */
#  define SOC_PCI_CFG_ADDR      0xCF8u
#  define SOC_PCI_CFG_DATA      0xCFCu
#  define SOC_PCIE_ECAM_BASE    0x00000000E0000000UL

/* ── RTC ─────────────────────────────────────────────────────────────── */
#  define SOC_RTC_PORT_IDX      0x70u
#  define SOC_RTC_PORT_DAT      0x71u
#  define SOC_RTC_IRQ           8u

/* =========================================================================
 * RISC-V 64 (QEMU virt / SiFive) MMIO map
 * ====================================================================== */
#elif defined(__riscv) || defined(ARCH_RISCV64)

#  define SOC_DRAM_BASE         0x80000000UL
#  define SOC_DRAM_SIZE         0x08000000UL   /* 128 MB default           */
#  define SOC_MMIO_BASE         0x10000000UL

/* ── CLINT (Core Local INTerruptor) ──────────────────────────────────── */
#  define SOC_CLINT_BASE              0x02000000UL
#  define SOC_CLINT_SIZE              0x00010000UL
#  define SOC_CLINT_MSIP(hart)        (SOC_CLINT_BASE + 0x0000u + (hart)*4u)
#  define SOC_CLINT_MTIMECMP(hart)    (SOC_CLINT_BASE + 0x4000u + (hart)*8u)
#  define SOC_CLINT_MTIME             (SOC_CLINT_BASE + 0xBFF8u)

/* ── PLIC (Platform-Level Interrupt Controller) ──────────────────────── */
#  define SOC_PLIC_BASE               0x0C000000UL
#  define SOC_PLIC_SIZE               0x04000000UL
#  define SOC_PLIC_PRIORITY(n)        (SOC_PLIC_BASE + 4u*(n))
#  define SOC_PLIC_ENABLE(ctx)        (SOC_PLIC_BASE + 0x2000u + (ctx)*0x80u)
#  define SOC_PLIC_THRESHOLD(ctx)     (SOC_PLIC_BASE + 0x200000u + (ctx)*0x1000u)
#  define SOC_PLIC_CLAIM(ctx)         (SOC_PLIC_BASE + 0x200004u + (ctx)*0x1000u)
#  define SOC_PLIC_MAX_IRQ            127u

/* ── NS16550A UART ───────────────────────────────────────────────────── */
#  define SOC_UART0_BASE        0x10000000UL
#  define SOC_UART_IRQ          10u

/* ── VirtIO ──────────────────────────────────────────────────────────── */
#  define SOC_VIRTIO_BASE       0x10001000UL
#  define SOC_VIRTIO_IRQ        1u
#  define SOC_VIRTIO_STRIDE     0x1000u        /* 8 slots × 0x1000         */

#else
#  error "uiox_soc_map.h: unsupported architecture — \
define ARCH_ARM64, ARCH_ARM32, ARCH_X86_64, or ARCH_RISCV64"
#endif /* architecture selection */

/* =========================================================================
 * Architecture-independent IRQ utility macros
 * ====================================================================== */
#define SOC_IRQ_INVALID       0xFFFFu
#define SOC_IRQ_IS_VALID(n)   ((uint32_t)(n) != SOC_IRQ_INVALID)

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SOC_MAP_H */
