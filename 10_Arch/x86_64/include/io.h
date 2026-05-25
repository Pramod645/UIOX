#ifndef __ARCH_X86_64_IO_H
#define __ARCH_X86_64_IO_H

/*
 * io.h  —  x86_64 port I/O and MMIO helpers
 */

#include <stdint.h>
#include "arch.h"

/* ── Port I/O ────────────────────────────────────────────── */

ARCH_INLINE void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" :: "a"(val), "Nd"(port));
}

ARCH_INLINE void outw(uint16_t port, uint16_t val)
{
    __asm__ volatile("outw %0, %1" :: "a"(val), "Nd"(port));
}

ARCH_INLINE void outl(uint16_t port, uint32_t val)
{
    __asm__ volatile("outl %0, %1" :: "a"(val), "Nd"(port));
}

ARCH_INLINE uint8_t inb(uint16_t port)
{
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

ARCH_INLINE uint16_t inw(uint16_t port)
{
    uint16_t val;
    __asm__ volatile("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

ARCH_INLINE uint32_t inl(uint16_t port)
{
    uint32_t val;
    __asm__ volatile("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

ARCH_INLINE void io_delay(void)
{
    /* write to unused port 0x80 — classic PC I/O delay */
    outb(0x80, 0);
}

/* ── MMIO ────────────────────────────────────────────────── */

ARCH_INLINE uint8_t mmio_read8(volatile void *addr)
{
    return *(volatile uint8_t *)addr;
}

ARCH_INLINE uint16_t mmio_read16(volatile void *addr)
{
    return *(volatile uint16_t *)addr;
}

ARCH_INLINE uint32_t mmio_read32(volatile void *addr)
{
    return *(volatile uint32_t *)addr;
}

ARCH_INLINE uint64_t mmio_read64(volatile void *addr)
{
    return *(volatile uint64_t *)addr;
}

ARCH_INLINE void mmio_write8(volatile void *addr, uint8_t val)
{
    *(volatile uint8_t *)addr = val;
}

ARCH_INLINE void mmio_write16(volatile void *addr, uint16_t val)
{
    *(volatile uint16_t *)addr = val;
}

ARCH_INLINE void mmio_write32(volatile void *addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
}

ARCH_INLINE void mmio_write64(volatile void *addr, uint64_t val)
{
    *(volatile uint64_t *)addr = val;
}

/* ── PIC (8259A) ports ───────────────────────────────────── */
#define PIC1_CMD   0x20
#define PIC1_DATA  0x21
#define PIC2_CMD   0xA0
#define PIC2_DATA  0xA1
#define PIC_EOI    0x20

void pic_init(uint8_t offset1, uint8_t offset2);
void pic_send_eoi(uint8_t irq);
void pic_mask_irq(uint8_t irq);
void pic_unmask_irq(uint8_t irq);

#endif /* __ARCH_X86_64_IO_H */
