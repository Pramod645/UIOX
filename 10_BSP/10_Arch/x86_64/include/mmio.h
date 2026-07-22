/*
 * 10_BSP/10_Arch/x86_64/include/mmio.h
 *
 * x86-64 Memory-Mapped I/O + Port I/O accessors.
 *
 * x86-64 has TWO I/O spaces:
 *   1. MMIO — volatile pointer reads/writes (LAPIC, IOAPIC, PCI BARs)
 *   2. Port I/O — IN/OUT instructions (COM1, PIC, PIT, legacy devices)
 *
 * Both are provided here.  Port I/O helpers are also in cpu.h for use
 * by arch_init.c directly; this header exposes them for the IRQ/SoC layer.
 *
 * @version 1.0.0  @date 2026-07-22
 */
#ifndef UIOX_MMIO_X86_64_H
#define UIOX_MMIO_X86_64_H

#ifndef UIOX_BASETYPES_COMPAT
#  define UIOX_BASETYPES_COMPAT
#endif
#include "uiox_base_types.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void mmio_init(void)
{ __asm__ volatile("" ::: "memory"); }

/* ── MMIO accessors ──────────────────────────────────────────────────────── */
static inline void     mmio_write32(uintptr_t a, uint32_t v)
{ *((volatile uint32_t *)a) = v; }
static inline uint32_t mmio_read32 (uintptr_t a)
{ return *((volatile uint32_t *)a); }

static inline void    mmio_write8(uintptr_t a, uint8_t v)
{ *((volatile uint8_t *)a) = v; }
static inline uint8_t mmio_read8 (uintptr_t a)
{ return *((volatile uint8_t *)a); }

static inline void     mmio_write64(uintptr_t a, uint64_t v)
{ *((volatile uint64_t *)a) = v; }
static inline uint64_t mmio_read64 (uintptr_t a)
{ return *((volatile uint64_t *)a); }

/* ── Port I/O accessors (x86-64 only — IN/OUT instructions) ─────────────── */
/* static inline void arch_outb(uint16_t port, uint8_t val)
{ __asm__ volatile("outb %0,%1" :: "a"(val), "Nd"(port)); }

static inline uint8_t arch_inb(uint16_t port)
{ uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(port)); return v; }

static inline void arch_outw(uint16_t port, uint16_t val)
{ __asm__ volatile("outw %0,%1" :: "a"(val), "Nd"(port)); }
*/
static inline uint16_t arch_inw(uint16_t port)
{ uint16_t v; __asm__ volatile("inw %1,%0":"=a"(v):"Nd"(port)); return v; }

/* ── LAPIC MMIO write (EOI, ICR, spurious vector, etc.) ─────────────────── */
#ifndef LAPIC_BASE
#  define LAPIC_BASE  0xFEE00000UL
#endif
static inline void lapic_write(uint32_t offset, uint32_t val)
{ mmio_write32((uintptr_t)(LAPIC_BASE + offset), val); }

static inline uint32_t lapic_read(uint32_t offset)
{ return mmio_read32((uintptr_t)(LAPIC_BASE + offset)); }

#ifdef __cplusplus
}
#endif
#endif /* UIOX_MMIO_X86_64_H */
