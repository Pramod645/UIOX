#ifndef __ARCH_X86_64_GDT_H
#define __ARCH_X86_64_GDT_H

/*
 * gdt.h  —  x86_64 Global Descriptor Table
 *
 * In 64-bit mode the GDT is minimal:
 *   0x00  null descriptor
 *   0x08  kernel code  (CS, DPL=0)
 *   0x10  kernel data  (DS, DPL=0)
 *   0x18  user   code  (CS, DPL=3)
 *   0x20  user   data  (DS, DPL=3)
 *   0x28  TSS low  (64-bit TSS takes 2 slots)
 *   0x30  TSS high
 */

#include <stdint.h>
#include "arch.h"

/* ── Segment selectors ───────────────────────────────────── */
#define GDT_NULL_SEL        0x00
#define GDT_KERN_CODE_SEL   0x08
#define GDT_KERN_DATA_SEL   0x10
#define GDT_USER_CODE_SEL   (0x18 | 3)   /* RPL=3 */
#define GDT_USER_DATA_SEL   (0x20 | 3)   /* RPL=3 */
#define GDT_TSS_SEL         0x28

#define GDT_ENTRY_COUNT     7             /* 5 + 2 for TSS      */

/* ── Raw 64-bit GDT entry ────────────────────────────────── */
typedef struct ARCH_PACKED gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;          /* P DPL S Type                  */
    uint8_t  granularity;     /* G D/B L AVL limit_high        */
    uint8_t  base_high;
} gdt_entry_t;

/* ── 128-bit system descriptor (used for TSS) ────────────── */
typedef struct ARCH_PACKED gdt_sys_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
    uint32_t base_upper;
    uint32_t reserved;
} gdt_sys_entry_t;

/* ── GDTR ────────────────────────────────────────────────── */
typedef struct ARCH_PACKED gdtr {
    uint16_t limit;
    uint64_t base;
} gdtr_t;

/* ── TSS (64-bit) ────────────────────────────────────────── */
typedef struct ARCH_PACKED tss64 {
    uint32_t reserved0;
    uint64_t rsp0;            /* kernel stack for ring 0       */
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];          /* interrupt stack table         */
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} tss64_t;

/* ── Access byte flags ───────────────────────────────────── */
#define GDT_ACCESS_PRESENT  0x80
#define GDT_ACCESS_DPL0     0x00
#define GDT_ACCESS_DPL3     0x60
#define GDT_ACCESS_CODE_SEG 0x18
#define GDT_ACCESS_DATA_SEG 0x10
#define GDT_ACCESS_RW       0x02
#define GDT_ACCESS_EX       0x08
#define GDT_ACCESS_TSS      0x09

/* ── Granularity byte flags ──────────────────────────────── */
#define GDT_GRAN_64BIT      0x20   /* L bit — long mode        */
#define GDT_GRAN_32BIT      0x40   /* D/B bit                  */
#define GDT_GRAN_4K         0x80   /* G bit — 4KiB granularity */

extern gdt_entry_t  gdt_table[GDT_ENTRY_COUNT];
extern tss64_t      kernel_tss;
extern gdtr_t       gdtr_value;

/* ── Function prototypes ─────────────────────────────────── */
void gdt_init(void);
void gdt_set_kernel_stack(uint64_t rsp0);
void gdt_load(void);

#endif /* __ARCH_X86_64_GDT_H */
