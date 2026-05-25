#ifndef __ARCH_X86_64_IDT_H
#define __ARCH_X86_64_IDT_H

/*
 * idt.h  —  x86_64 Interrupt Descriptor Table
 */

#include <stdint.h>
#include "arch.h"

#define IDT_ENTRY_COUNT     256
#define IDT_TRAP_GATE       0x8F
#define IDT_INTR_GATE       0x8E
#define IDT_USER_INTR_GATE  0xEE  /* DPL=3 for software int   */

/* ── Exception vectors ───────────────────────────────────── */
#define EXC_DIVIDE_ERROR    0
#define EXC_DEBUG           1
#define EXC_NMI             2
#define EXC_BREAKPOINT      3
#define EXC_OVERFLOW        4
#define EXC_BOUND_RANGE     5
#define EXC_INVALID_OPCODE  6
#define EXC_DEVICE_NA       7
#define EXC_DOUBLE_FAULT    8
#define EXC_INVALID_TSS     10
#define EXC_SEG_NOT_PRESENT 11
#define EXC_STACK_FAULT     12
#define EXC_GENERAL_PROT    13
#define EXC_PAGE_FAULT      14
#define EXC_FP_ERROR        16
#define EXC_ALIGNMENT       17
#define EXC_MACHINE_CHECK   18
#define EXC_SIMD_FP         19

/* IRQ vectors (after 8259A remap to 0x20+) */
#define IRQ_BASE            0x20
#define IRQ_TIMER           (IRQ_BASE + 0)
#define IRQ_KEYBOARD        (IRQ_BASE + 1)
#define IRQ_SYSCALL         0x80

/* ── IDT gate descriptor (16 bytes) ─────────────────────── */
typedef struct ARCH_PACKED idt_entry {
    uint16_t offset_low;
    uint16_t selector;        /* code segment selector         */
    uint8_t  ist;             /* interrupt stack table index   */
    uint8_t  type_attr;       /* gate type + DPL + present     */
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} idt_entry_t;

/* ── IDTR ────────────────────────────────────────────────── */
typedef struct ARCH_PACKED idtr {
    uint16_t limit;
    uint64_t base;
} idtr_t;

/* ── Interrupt handler function pointer ──────────────────── */
typedef void (*irq_handler_t)(uint64_t vector, uint64_t error,
                               void *frame);

extern idt_entry_t idt_table[IDT_ENTRY_COUNT];
extern idtr_t      idtr_value;

/* ── Function prototypes ─────────────────────────────────── */
void idt_init(void);
void idt_set_gate(uint8_t vector, uint64_t handler,
                   uint16_t selector, uint8_t type,
                   uint8_t ist);
void idt_register_handler(uint8_t vector, irq_handler_t fn);
void idt_load(void);

/* exception / IRQ stubs (defined in boot.S) */
void exc_divide_error(void);
void exc_debug(void);
void exc_nmi(void);
void exc_breakpoint(void);
void exc_overflow(void);
void exc_invalid_opcode(void);
void exc_double_fault(void);
void exc_general_protection(void);
void exc_page_fault(void);
void irq_timer_stub(void);
void irq_syscall_stub(void);

#endif /* __ARCH_X86_64_IDT_H */
