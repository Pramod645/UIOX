/*
 * gdt.c  —  x86_64 GDT and TSS setup.
 *
 * Mirrors: 10_Arch/arm32/src/gdt.c
 */

 #include "../include/arch.h"
 #include <string.h>
 
 /* ── GDT storage ─────────────────────────────────────────── */
 ARCH_ALIGNED(8) gdt_entry_t gdt_table[GDT_ENTRY_COUNT];
 tss64_t         kernel_tss;
 gdtr_t          gdtr_value;
 
 /* ── Helpers ─────────────────────────────────────────────── */
 static void gdt_set_entry(int idx,
                             uint32_t base,
                             uint32_t limit,
                             uint8_t  access,
                             uint8_t  gran)
 {
     gdt_table[idx].base_low  = base  & 0xFFFF;
     gdt_table[idx].base_mid  = (base  >> 16) & 0xFF;
     gdt_table[idx].base_hi   = (base  >> 24) & 0xFF;
     gdt_table[idx].limit_low = limit & 0xFFFF;
     gdt_table[idx].flags_limit_hi = ((limit >> 16) & 0x0F) | (gran & 0xF0);
     gdt_table[idx].access    = access;
 }
 
 static void gdt_set_tss_entry(int idx, uint64_t base, uint32_t limit)
 {
     gdt_sys_entry_t *e = (gdt_sys_entry_t *)&gdt_table[idx];
     e->limit_low  = limit & 0xFFFF;
     e->base_low   = base  & 0xFFFF;
     e->base_mid   = (base  >> 16) & 0xFF;
     e->access     = GDT_ACCESS_PRESENT | GDT_ACCESS_TSS;
     e->granularity = ((limit >> 16) & 0x0F);
     e->base_high  = (base  >> 24) & 0xFF;
     e->base_upper = (base  >> 32) & 0xFFFFFFFF;
     e->reserved   = 0;
 }
 
 /* ── gdt_init ────────────────────────────────────────────── */
 void gdt_init(void)
 {
     memset(gdt_table, 0, sizeof(gdt_table));
     memset(&kernel_tss, 0, sizeof(kernel_tss));
 
     /* 0x00: null descriptor */
     gdt_set_entry(0, 0, 0, 0, 0);
 
     /* 0x08: kernel code — 64-bit (L=1, D/B=0) */
     gdt_set_entry(1, 0, 0xFFFFF,
                   GDT_ACCESS_PRESENT |
                   GDT_ACCESS_DPL0    |
                   GDT_ACCESS_CODE_SEG|
                   GDT_ACCESS_RW,
                   GDT_GRAN_64BIT | GDT_GRAN_4K);
 
     /* 0x10: kernel data */
     gdt_set_entry(2, 0, 0xFFFFF,
                   GDT_ACCESS_PRESENT |
                   GDT_ACCESS_DPL0    |
                   GDT_ACCESS_DATA_SEG|
                   GDT_ACCESS_RW,
                   GDT_GRAN_32BIT | GDT_GRAN_4K);
 
     /* 0x18: user code — 64-bit */
     gdt_set_entry(3, 0, 0xFFFFF,
                   GDT_ACCESS_PRESENT |
                   GDT_ACCESS_DPL3    |
                   GDT_ACCESS_CODE_SEG|
                   GDT_ACCESS_RW,
                   GDT_GRAN_64BIT | GDT_GRAN_4K);
 
     /* 0x20: user data */
     gdt_set_entry(4, 0, 0xFFFFF,
                   GDT_ACCESS_PRESENT |
                   GDT_ACCESS_DPL3    |
                   GDT_ACCESS_DATA_SEG|
                   GDT_ACCESS_RW,
                   GDT_GRAN_32BIT | GDT_GRAN_4K);
 
     /* 0x28/0x30: TSS (16-byte system descriptor) */
     kernel_tss.iopb_offset = sizeof(tss64_t);
     gdt_set_tss_entry(5,
                        (uint64_t)&kernel_tss,
                        sizeof(tss64_t) - 1);
 
     /* load GDTR */
     gdtr_value.limit = (uint16_t)(sizeof(gdt_table) - 1);
     gdtr_value.base  = (uint64_t)gdt_table;
 
     gdt_load();
 
     /* load TSS selector */
     __asm__ volatile("ltr %0" :: "r"((uint16_t)GDT_TSS_SEL));
 }
 
 void gdt_set_kernel_stack(uint64_t rsp0)
 {
     kernel_tss.rsp0 = rsp0;
 }
 
 void gdt_load(void)
 {
     __asm__ volatile(
         "lgdt  %0              \n"
         "movw  $0x10, %%ax     \n"   /* kernel data selector */
         "movw  %%ax,  %%ds     \n"
         "movw  %%ax,  %%es     \n"
         "movw  %%ax,  %%fs     \n"
         "movw  %%ax,  %%gs     \n"
         "movw  %%ax,  %%ss     \n"
         /* far return to reload CS */
         "pushq $0x08           \n"
         "leaq  1f(%%rip), %%rax\n"
         "pushq %%rax           \n"
         "lretq                 \n"
         "1:                    \n"
         :: "m"(gdtr_value)
         : "rax", "memory"
     );
 }
 