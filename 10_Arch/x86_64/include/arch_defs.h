#ifndef UIOX_ARCH_DEFS_X86_64_H
#define UIOX_ARCH_DEFS_X86_64_H

/*
 * arch/x86_64/include/arch_defs.h
 *
 * AMD64 / Intel 64-bit architecture definitions.
 */

/* ── Identity ─────────────────────────────────────────────── */
#define UIOX_ARCH_NAME          "x86_64"
#define UIOX_ARCH_X86_64        1
#define UIOX_BITS               64
#define UIOX_ENDIAN_LITTLE      1

/* ── Word / pointer sizes ─────────────────────────────────── */
#define UIOX_PTR_SIZE           8
#define UIOX_PAGE_SIZE          4096
#define UIOX_PAGE_SHIFT         12
#define UIOX_CACHE_LINE         64

/* ── Physical memory map (QEMU q35 / i440fx) ─────────────── */
#define PHYS_DRAM_BASE          0x0000000000100000UL  /* 1 MB      */
#define PHYS_DRAM_SIZE          0x0000000008000000UL  /* 128 MB    */
#define PHYS_MMIO_BASE          0x00000000FEC00000UL  /* IOAPIC    */
#define PHYS_MMIO_SIZE          0x0000000001000000UL

/* ── APIC / IOAPIC ───────────────────────────────────────── */
#define LAPIC_BASE              0xFEE00000UL
#define LAPIC_EOI               (LAPIC_BASE + 0x0B0)
#define LAPIC_SPURIOUS          (LAPIC_BASE + 0x0F0)
#define LAPIC_ICR_LO            (LAPIC_BASE + 0x300)
#define LAPIC_ICR_HI            (LAPIC_BASE + 0x310)
#define IOAPIC_BASE             0xFEC00000UL
#define IOAPIC_IOREGSEL         (IOAPIC_BASE + 0x000)
#define IOAPIC_IOWIN            (IOAPIC_BASE + 0x010)

/* ── 16550 UART (COM1) ────────────────────────────────────── */
#define COM1_PORT               0x3F8   /* IO port base           */
#define COM1_IRQ                4

/* ── 8259A PIC ports ─────────────────────────────────────── */
#define PIC1_CMD                0x20
#define PIC1_DATA               0x21
#define PIC2_CMD                0xA0
#define PIC2_DATA               0xA1
#define PIC_EOI                 0x20

/* ── 8254 PIT timer ──────────────────────────────────────── */
#define PIT_CH0_PORT            0x40
#define PIT_CMD_PORT            0x43
#define PIT_IRQ                 0

/* ── IDE disk ────────────────────────────────────────────── */
#define IDE_DATA_PORT           0x1F0
#define IDE_STATUS_PORT         0x1F7
#define IDE_IRQ                 14

/* ── RFLAGS bits ─────────────────────────────────────────── */
#define RFLAGS_CF               (1UL <<  0)
#define RFLAGS_PF               (1UL <<  2)
#define RFLAGS_AF               (1UL <<  4)
#define RFLAGS_ZF               (1UL <<  6)
#define RFLAGS_SF               (1UL <<  7)
#define RFLAGS_TF               (1UL <<  8)
#define RFLAGS_IF               (1UL <<  9)   /* interrupt enable  */
#define RFLAGS_DF               (1UL << 10)
#define RFLAGS_OF               (1UL << 11)

/* ── x86_64 MSR numbers ──────────────────────────────────── */
#define MSR_IA32_APIC_BASE      0x0000001B
#define MSR_EFER                0xC0000080
#define MSR_STAR                0xC0000081
#define MSR_LSTAR               0xC0000082
#define MSR_GS_BASE             0xC0000101
#define MSR_KERNEL_GS_BASE      0xC0000102

/* ── Compiler helpers ────────────────────────────────────── */
#define ARCH_INLINE             __attribute__((always_inline)) inline
#define ARCH_NORETURN           __attribute__((noreturn))
#define ARCH_PACKED             __attribute__((packed))
#define ARCH_ALIGNED(n)         __attribute__((aligned(n)))
#define ARCH_WEAK               __attribute__((weak))
#define ARCH_SECTION(s)         __attribute__((section(s)))

/* ── Barrier macros ───────────────────────────────────────── */
#define arch_mb()   __asm__ volatile("mfence\n\t" ::: "memory")
#define arch_rmb()  __asm__ volatile("lfence\n\t" ::: "memory")
#define arch_wmb()  __asm__ volatile("sfence\n\t" ::: "memory")
#define arch_isb()  __asm__ volatile(""           ::: "memory")
#define arch_dsb()  __asm__ volatile("mfence\n\t" ::: "memory")
#define arch_nop()  __asm__ volatile("nop\n\t")
#define arch_wfi()  __asm__ volatile("hlt\n\t")
#define arch_wfe()  __asm__ volatile("pause\n\t")
#define arch_sev()  /* no equivalent on x86 */

/* ── IRQ control ─────────────────────────────────────────── */
#define arch_irq_disable()      __asm__ volatile("cli\n\t" ::: "memory")
#define arch_irq_enable()       __asm__ volatile("sti\n\t" ::: "memory")

#define arch_irq_save(flags) \
    __asm__ volatile("pushfq\n\t popq %0\n\t cli\n\t" \
                     : "=r"(flags) :: "memory")

#define arch_irq_restore(flags) \
    __asm__ volatile("pushq %0\n\t popfq\n\t" \
                     :: "r"(flags) : "memory", "cc")

/* ── Port I/O helpers ────────────────────────────────────── */
#define outb(port, val) \
    __asm__ volatile("outb %0, %1\n\t" :: "a"((uint8_t)(val)), \
                                          "Nd"((uint16_t)(port)))
#define inb(port) \
    ({ uint8_t _v; \
       __asm__ volatile("inb %1, %0\n\t" : "=a"(_v) \
                                         : "Nd"((uint16_t)(port))); \
       _v; })

/* ── Cross-layer UIOX config ─────────────────────────────── */
#define UIOX_BLOCK_SIZE         512
#define UIOX_MAX_BLOCKS         1024
#define UIOX_MAX_INODES         128
#define UIOX_MAJOR_BLK_MAX      8
#define UIOX_MAJOR_CHR_MAX      8
#define UIOX_CBLOCK_POOL        256
#define UIOX_IRQ_MAX            64
#define UIOX_DMA_MAX_DESC       16
#define UIOX_MMIO_REGIONS       8

#endif /* UIOX_ARCH_DEFS_X86_64_H */
