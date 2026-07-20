#ifndef UIOX_ARCH_DEFS_X86_64_H
#define UIOX_ARCH_DEFS_X86_64_H
/*
 * 10_Arch/x86_64/include/arch_defs.h
 * AMD64 / Intel 64-bit architecture definitions.
 */

/* ── Architecture identity ────────────────────────────────────────── */
#define UIOX_ARCH_NAME          "x86_64"
#define UIOX_ARCH_BITS          64
#define UIOX_ARCH_LITTLE_ENDIAN 1

/* ── Word / pointer sizes ─────────────────────────────────────────── */
#define UIOX_PTR_SIZE           8
#define UIOX_PAGE_SIZE          4096
#define UIOX_PAGE_SHIFT         12
#define UIOX_CACHE_LINE         64

/* ── Physical memory map (QEMU q35 / i440fx defaults) ────────────────
 * 1 MB lower-memory hole is the standard x86 conventional base.
 * Overridden per SoC/board by 03_SoC/include/uiox_soc_map.h.
 * ──────────────────────────────────────────────────────────────────── */
#define PHYS_DRAM_BASE          0x0000000000100000UL  /* 1 MB            */
#define PHYS_DRAM_SIZE          0x0000000008000000UL  /* 128 MB          */
#define PHYS_MMIO_BASE          0x00000000FEC00000UL  /* IOAPIC region   */
#define PHYS_MMIO_SIZE          0x0000000001000000UL

/* ── Local APIC ───────────────────────────────────────────────────── */
#define LAPIC_BASE              0xFEE00000UL
#define LAPIC_ID                (LAPIC_BASE + 0x020U)
#define LAPIC_VER               (LAPIC_BASE + 0x030U)
#define LAPIC_TPR               (LAPIC_BASE + 0x080U)
#define LAPIC_EOI               (LAPIC_BASE + 0x0B0U)
#define LAPIC_LDR               (LAPIC_BASE + 0x0D0U)
#define LAPIC_DFR               (LAPIC_BASE + 0x0E0U)
#define LAPIC_SPURIOUS          (LAPIC_BASE + 0x0F0U)
#define LAPIC_ESR               (LAPIC_BASE + 0x280U)
#define LAPIC_ICR_LO            (LAPIC_BASE + 0x300U)
#define LAPIC_ICR_HI            (LAPIC_BASE + 0x310U)
#define LAPIC_TIMER_LVT         (LAPIC_BASE + 0x320U)
#define LAPIC_TIMER_INIT        (LAPIC_BASE + 0x380U)
#define LAPIC_TIMER_CURRENT     (LAPIC_BASE + 0x390U)
#define LAPIC_TIMER_DIVIDE      (LAPIC_BASE + 0x3E0U)

/* LAPIC spurious vector value: enable (bit 8) + vector 0xFF */
#define LAPIC_SPURIOUS_ENABLE   0x1FFU

/* ── I/O APIC ─────────────────────────────────────────────────────── */
#define IOAPIC_BASE             0xFEC00000UL
#define IOAPIC_IOREGSEL         (IOAPIC_BASE + 0x000U)
#define IOAPIC_IOWIN            (IOAPIC_BASE + 0x010U)

/* I/O APIC register indices */
#define IOAPIC_REG_ID           0x00U
#define IOAPIC_REG_VER          0x01U
#define IOAPIC_REG_REDTBL(n)    (0x10U + (n) * 2U)

/* ── MSR addresses ────────────────────────────────────────────────── */
#define MSR_IA32_APIC_BASE      0x0000001BU
#define MSR_IA32_APIC_BASE_EN   (1U << 11)    /* Global enable bit      */
#define MSR_EFER                0xC0000080U
#define MSR_EFER_LME            (1U <<  8)    /* Long mode enable       */
#define MSR_EFER_LMA            (1U << 10)    /* Long mode active       */
#define MSR_EFER_NXE            (1U << 11)    /* NX enable              */
#define MSR_EFER_SCE            (1U <<  0)    /* SYSCALL enable         */

/* ── Control register bits ────────────────────────────────────────── */
#define CR0_PE                  (1UL <<  0)   /* Protected mode         */
#define CR0_WP                  (1UL << 16)   /* Write protect          */
#define CR0_PG                  (1UL << 31)   /* Paging enable          */
#define CR4_PAE                 (1UL <<  5)   /* PAE                    */
#define CR4_PGE                 (1UL <<  7)   /* Global pages           */
#define CR4_OSFXSR              (1UL <<  9)   /* SSE                    */

/* ── RFLAGS bits ──────────────────────────────────────────────────── */
#define RFLAGS_CF               (1UL <<  0)
#define RFLAGS_PF               (1UL <<  2)
#define RFLAGS_ZF               (1UL <<  6)
#define RFLAGS_SF               (1UL <<  7)
#define RFLAGS_IF               (1UL <<  9)   /* Interrupt flag         */
#define RFLAGS_DF               (1UL << 10)
#define RFLAGS_IOPL             (3UL << 12)

/* ── GDT / IDT segment selectors ─────────────────────────────────── */
#define SEG_KERN_CODE           0x08U
#define SEG_KERN_DATA           0x10U
#define SEG_USER_CODE           0x18U
#define SEG_USER_DATA           0x20U
#define SEG_TSS                 0x28U

/* ── Port I/O helpers ─────────────────────────────────────────────── */
#define arch_outb(port, val) \
    __asm__ volatile("outb %0,%1" :: "a"((unsigned char)(val)), "Nd"((unsigned short)(port)))
#define arch_outw(port, val) \
    __asm__ volatile("outw %0,%1" :: "a"((unsigned short)(val)),"Nd"((unsigned short)(port)))
#define arch_inb(port) ({ \
    unsigned char _v; \
    __asm__ volatile("inb %1,%0":"=a"(_v):"Nd"((unsigned short)(port))); _v; })

/* ── Barrier / CPU hint macros ────────────────────────────────────── */
#define arch_dsb_sy()   __asm__ volatile("mfence" ::: "memory")
#define arch_isb()      __asm__ volatile("" ::: "memory")   /* compiler fence */
#define arch_wfi()      __asm__ volatile("hlt")
#define arch_nop()      __asm__ volatile("nop")
#define arch_cli()      __asm__ volatile("cli" ::: "memory")
#define arch_sti()      __asm__ volatile("sti" ::: "memory")
#define arch_pause()    __asm__ volatile("pause")

/* TLB flush */
#define arch_tlbi_all() \
    do { unsigned long _cr3; \
         __asm__ volatile("mov %%cr3,%0":"=r"(_cr3)); \
         __asm__ volatile("mov %0,%%cr3"::"r"(_cr3):"memory"); } while(0)
#define arch_tlbi_mva(va) \
    __asm__ volatile("invlpg (%0)"::"r"(va):"memory")

/* ── Alignment helpers ────────────────────────────────────────────── */
#define ARCH_ALIGN_UP(v, a)     (((v) + ((a) - 1UL)) & ~((a) - 1UL))
#define ARCH_ALIGN_DOWN(v, a)   ((v) & ~((a) - 1UL))
#define ARCH_IS_ALIGNED(v, a)   (((v) & ((a) - 1UL)) == 0UL)
#define ARCH_PAGE_ALIGN(v)      ARCH_ALIGN_UP(v, UIOX_PAGE_SIZE)

/* ── Compiler helpers ─────────────────────────────────────────────── */
#define ARCH_INLINE             static inline __attribute__((always_inline))
#define ARCH_NORETURN           __attribute__((noreturn))
#define ARCH_PACKED             __attribute__((packed))
#define ARCH_ALIGNED(n)         __attribute__((aligned(n)))
#define ARCH_WEAK               __attribute__((weak))
#define ARCH_USED               __attribute__((used))
#define ARCH_UNUSED(x)          ((void)(x))

#endif /* UIOX_ARCH_DEFS_X86_64_H */
