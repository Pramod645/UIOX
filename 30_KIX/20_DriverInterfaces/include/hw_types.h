#ifndef UIOX_HW_TYPES_H
#define UIOX_HW_TYPES_H

/*
 * hw_types.h
 *
 * Architecture-detection, primitive types, and constants shared
 * across the entire hardware control layer.
 *
 * Supported targets
 *   ARCH_ARM64   — AArch64 (ARMv8-A 64-bit)
 *   ARCH_ARM32   — ARMv7-A 32-bit
 *   ARCH_X86_64  — AMD64 / Intel 64
 *
 * If no -DARCH_xxx flag is supplied the Makefile injects the
 * correct define based on the compiler's target triple.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* =============================================================
 * Architecture detection
 * ============================================================= */
#if defined(__aarch64__) || defined(ARCH_ARM64)
#  define UIOX_ARCH_ARM64  1
#  define UIOX_ARCH_NAME   "arm64"
#elif defined(__arm__) || defined(ARCH_ARM32)
#  define UIOX_ARCH_ARM32  1
#  define UIOX_ARCH_NAME   "arm32"
#elif defined(__x86_64__) || defined(ARCH_X86_64)
#  define UIOX_ARCH_X86_64 1
#  define UIOX_ARCH_NAME   "x86_64"
#else
#  warning "Unknown architecture — defaulting to x86_64 simulation"
#  define UIOX_ARCH_X86_64 1
#  define UIOX_ARCH_NAME   "x86_64(sim)"
#endif

/* =============================================================
 * Physical / virtual address types
 * ============================================================= */
typedef uintptr_t  phys_addr_t;   /* physical address              */
typedef uintptr_t  virt_addr_t;   /* virtual  address              */
typedef uint32_t   reg32_t;       /* 32-bit MMIO register value    */
typedef uint64_t   reg64_t;       /* 64-bit MMIO register value    */

/* =============================================================
 * MMIO base addresses (simulated; replace with real SoC values)
 * ============================================================= */
#define MMIO_UART_BASE      0x09000000UL   /* PL011 (QEMU virt)     */
#define MMIO_TIMER_BASE     0x09010000UL   /* ARM SP804 / HPET sim  */
#define MMIO_INTC_BASE      0x08000000UL   /* GIC / APIC base       */
#define MMIO_DMA_BASE       0x09020000UL   /* DMA controller base   */
#define MMIO_DISK_BASE      0x09030000UL   /* virtio-blk base       */

/* =============================================================
 * IRQ numbers (platform-independent symbolic names)
 * ============================================================= */
#define IRQ_UART            1
#define IRQ_TIMER           2
#define IRQ_DISK            3
#define IRQ_DMA             4
#define IRQ_KEYBOARD        5
#define IRQ_MAX             64    /* maximum IRQ lines supported   */

/* =============================================================
 * CPU flag / status register bit masks
 * ============================================================= */

/* ARM64 DAIF bits */
#define ARM64_DAIF_D        (1u << 9)   /* debug                   */
#define ARM64_DAIF_A        (1u << 8)   /* SError                  */
#define ARM64_DAIF_I        (1u << 7)   /* IRQ                     */
#define ARM64_DAIF_F        (1u << 6)   /* FIQ                     */

/* ARM32 CPSR bits */
#define ARM32_CPSR_I        (1u << 7)   /* IRQ disable             */
#define ARM32_CPSR_F        (1u << 6)   /* FIQ disable             */

/* x86_64 EFLAGS bits */
#define X86_EFLAGS_IF       (1u << 9)   /* interrupt enable        */
#define X86_EFLAGS_TF       (1u << 8)   /* trap flag               */

/* =============================================================
 * DMA descriptor — scatter-gather entry
 * ============================================================= */
#define DMA_MAX_DESC        16

typedef struct {
    phys_addr_t  dma_src;    /* source physical address            */
    phys_addr_t  dma_dst;    /* destination physical address       */
    uint32_t     dma_len;    /* byte count for this segment        */
    uint32_t     dma_flags;  /* DMA_FLAG_* below                   */
} dma_desc_t;

#define DMA_FLAG_NONE       0x00
#define DMA_FLAG_IRQ        0x01    /* raise IRQ on completion     */
#define DMA_FLAG_LAST       0x02    /* last descriptor in chain    */
#define DMA_FLAG_DONE       0x04    /* set by hardware on complete */
#define DMA_FLAG_ERROR      0x08    /* set by hardware on error    */

/* =============================================================
 * CPU hardware context (saved on interrupt entry)
 *
 * Sized to hold the union of all three ISA register sets.
 * On a real SoC only the relevant fields are populated.
 * ============================================================= */
typedef struct {
    /* ── common ──────────────────────────────────────────── */
    uint64_t  pc;            /* program counter                    */
    uint64_t  sp;            /* stack pointer                      */
    uint32_t  irq_num;       /* IRQ that caused this save          */
    uint32_t  irq_depth;     /* nesting level                      */

    /* ── ARM64 ───────────────────────────────────────────── */
    uint64_t  x[31];         /* general-purpose registers x0-x30  */
    uint64_t  daif;          /* DAIF interrupt mask register       */
    uint64_t  spsr_el1;      /* saved program status register      */
    uint64_t  elr_el1;       /* exception link register            */

    /* ── ARM32 ───────────────────────────────────────────── */
    uint32_t  r[16];         /* general-purpose registers r0-r15   */
    uint32_t  cpsr;          /* current program status register    */
    uint32_t  spsr;          /* saved program status register      */

    /* ── x86_64 ──────────────────────────────────────────── */
    uint64_t  rax, rbx, rcx, rdx;
    uint64_t  rsi, rdi, rbp;
    uint64_t  r8,  r9,  r10, r11;
    uint64_t  r12, r13, r14, r15;
    uint64_t  rip, rflags, cs, ss;
} hw_context_t;

/* =============================================================
 * Simulated MMIO register file
 *
 * On a hosted (Linux/macOS) build we back each MMIO region with
 * a small byte array rather than writing to real hardware addresses.
 * ============================================================= */
#define MMIO_REGION_SIZE    0x1000   /* 4 KB per simulated region  */

typedef struct {
    phys_addr_t  mr_base;
    uint8_t      mr_data[MMIO_REGION_SIZE];
    const char  *mr_name;
} mmio_region_t;

#define MMIO_NUM_REGIONS    8

/* =============================================================
 * Error codes used by the hardware layer
 * ============================================================= */
#define HW_OK               0
#define HW_ERR_RANGE       -1    /* address out of range           */
#define HW_ERR_ALIGN       -2    /* misaligned access              */
#define HW_ERR_NOIRQ       -3    /* no handler registered          */
#define HW_ERR_BUSY        -4    /* resource busy                  */
#define HW_ERR_TIMEOUT     -5    /* operation timed out            */

#endif /* UIOX_HW_TYPES_H */
