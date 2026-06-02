/*
 * x86_64_cpu.c - x86-64 CPU specific initialisation
 */
#include "../../include/arch/x86_64_cpu.h"
#include "../../include/cpu_regs.h"
#include <string.h>
#include <stdio.h>

/* -- GDT (minimal: null, code64, data64) -------------------- */
static x86_gdt_entry_t g_gdt[8] __attribute__((aligned(8)));
static x86_tss_t       g_tss    __attribute__((aligned(16)));

/* -- IDT (256 gates) ---------------------------------------- */
static x86_idt_gate_t  g_idt[256] __attribute__((aligned(16)));

static void gdt_set_entry(cpu_u32_t idx, cpu_u32_t base,
                            cpu_u32_t limit, cpu_u8_t access,
                            cpu_u8_t gran)
{
    g_gdt[idx].base_lo    = (cpu_u16_t)(base & 0xFFFF);
    g_gdt[idx].base_mid   = (cpu_u8_t)((base >> 16) & 0xFF);
    g_gdt[idx].base_hi    = (cpu_u8_t)((base >> 24) & 0xFF);
    g_gdt[idx].limit_lo   = (cpu_u16_t)(limit & 0xFFFF);
    g_gdt[idx].granularity= (cpu_u8_t)(((limit >> 16) & 0x0F) | gran);
    g_gdt[idx].access     = access;
}

void x86_gdt_init(void)
{
    memset(g_gdt, 0, sizeof(g_gdt));
    gdt_set_entry(0, 0, 0,      0x00, 0x00); /* null             */
    gdt_set_entry(1, 0, 0xFFFFF,0x9A, 0xAF); /* code64 ring0     */
    gdt_set_entry(2, 0, 0xFFFFF,0x92, 0xCF); /* data64 ring0     */
    gdt_set_entry(3, 0, 0xFFFFF,0xFA, 0xAF); /* code64 ring3     */
    gdt_set_entry(4, 0, 0xFFFFF,0xF2, 0xCF); /* data64 ring3     */

    struct { cpu_u16_t limit; cpu_u64_t base; }
        __attribute__((packed)) gdtr = {
        sizeof(g_gdt) - 1,
        (cpu_u64_t)(cpu_addr_t)g_gdt
    };
    __asm__ volatile("lgdt %0\n\t"
                     "pushq $0x08\n\t"
                     "leaq 1f(%%rip), %%rax\n\t"
                     "pushq %%rax\n\t"
                     "lretq\n\t"
                     "1:\n\t"
                     "movw $0x10, %%ax\n\t"
                     "movw %%ax, %%ds\n\t"
                     "movw %%ax, %%es\n\t"
                     "movw %%ax, %%ss\n\t"
                     "xorw %%ax, %%ax\n\t"
                     "movw %%ax, %%fs\n\t"
                     "movw %%ax, %%gs\n\t"
                     :: "m"(gdtr) : "rax", "memory");
}

void x86_idt_init(void)
{
    memset(g_idt, 0, sizeof(g_idt));
    /* gates installed by individual subsystems via cpu_irq_register */
    struct { cpu_u16_t limit; cpu_u64_t base; }
        __attribute__((packed)) idtr = {
        sizeof(g_idt) - 1,
        (cpu_u64_t)(cpu_addr_t)g_idt
    };
    __asm__ volatile("lidt %0\n\t" :: "m"(idtr) : "memory");
}

void x86_tss_init(cpu_addr_t stack0)
{
    memset(&g_tss, 0, sizeof(g_tss));
    g_tss.rsp[0] = stack0;
    cpu_u64_t base = (cpu_u64_t)(cpu_addr_t)&g_tss;
    /* TSS descriptor in GDT slot 5 (16-byte descriptor) */
    g_gdt[5].limit_lo    = sizeof(g_tss) - 1;
    g_gdt[5].base_lo     = (cpu_u16_t)(base & 0xFFFF);
    g_gdt[5].base_mid    = (cpu_u8_t)((base >> 16) & 0xFF);
    g_gdt[5].access      = 0x89;   /* present, TSS64 available   */
    g_gdt[5].granularity = 0;
    /* upper 8 bytes of TSS descriptor */
    cpu_u32_t *tss_hi = (cpu_u32_t *)&g_gdt[6];
    *tss_hi = (cpu_u32_t)(base >> 32);

    __asm__ volatile("ltr %0\n\t" :: "r"((cpu_u16_t)GDT_SEL_TSS));
}

void x86_enable_sse(void)
{
    cpu_u64_t cr0, cr4;
    __asm__ volatile("movq %%cr0,%0" : "=r"(cr0));
    cr0 &= ~(1ULL << 2);   /* clear EM */
    cr0 |=  (1ULL << 1);   /* set MP   */
    __asm__ volatile("movq %0,%%cr0" :: "r"(cr0));

    __asm__ volatile("movq %%cr4,%0" : "=r"(cr4));
    cr4 |= CR4_OSFXSR | (1ULL << 10); /* OSFXSR + OSXMMEXCPT */
    __asm__ volatile("movq %0,%%cr4" :: "r"(cr4));
}

void x86_enable_avx(void)
{
    /* requires XSAVE support */
    cpu_u64_t xcr0;
    __asm__ volatile(
        "xgetbv\n\t"
        "orq $0x7, %%rax\n\t"   /* x87 | SSE | AVX */
        "xsetbv\n\t"
        :: "c"(0u) : "rax", "rdx");
    (void)xcr0;
}

void x86_paging_init(cpu_addr_t pml4)
{
    /* enable PAE */
    cpu_u64_t cr4;
    __asm__ volatile("movq %%cr4,%0" : "=r"(cr4));
    cr4 |= CR4_PAE | CR4_PGE | CR4_SMEP;
    __asm__ volatile("movq %0,%%cr4" :: "r"(cr4));

    /* set EFER.NXE */
    cpu_u64_t efer = cpu_rdmsr(MSR_EFER);
    efer |= EFER_NXE | EFER_SCE;
    cpu_wrmsr(MSR_EFER, efer);

    /* load PML4 into CR3 */
    __asm__ volatile("movq %0,%%cr3" :: "r"(pml4) : "memory");

    /* enable paging: set CR0.PG */
    cpu_u64_t cr0;
    __asm__ volatile("movq %%cr0,%0" : "=r"(cr0));
    cr0 |= CR0_PG | CR0_WP;
    __asm__ volatile("movq %0,%%cr0" :: "r"(cr0) : "memory");
}

void x86_syscall_init(cpu_addr_t handler)
{
    /* STAR: CS/SS selectors for SYSCALL/SYSRET */
    cpu_u64_t star = ((cpu_u64_t)GDT_SEL_CODE64 << 32) |
                     ((cpu_u64_t)(GDT_SEL_USER64 | 3u) << 48);
    cpu_wrmsr(MSR_STAR,   star);
    cpu_wrmsr(MSR_LSTAR,  handler);
    cpu_wrmsr(MSR_SFMASK, RFLAGS_IF);

    /* enable SYSCALL in EFER */
    cpu_u64_t efer = cpu_rdmsr(MSR_EFER);
    efer |= EFER_SCE;
    cpu_wrmsr(MSR_EFER, efer);
}

cpu_u64_t x86_read_tsc(void)
{
    cpu_u32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((cpu_u64_t)hi << 32) | lo;
}

int x86_cpu_init(void)
{
    x86_gdt_init();
    x86_idt_init();
    x86_enable_sse();
    printf("[x86] GDT + IDT + SSE initialised\n");
    return CPU_OK;
}

void x86_cpu_print_info(void)
{
    cpu_u32_t eax, ebx, ecx, edx;
    char brand[49] = {0};
    cpu_cpuid(0x80000002u, &eax, &ebx, &ecx, &edx);
    memcpy(brand +  0, &eax, 4);
    memcpy(brand +  4, &ebx, 4);
    memcpy(brand +  8, &ecx, 4);
    memcpy(brand + 12, &edx, 4);
    cpu_cpuid(0x80000003u, &eax, &ebx, &ecx, &edx);
    memcpy(brand + 16, &eax, 4);
    memcpy(brand + 20, &ebx, 4);
    memcpy(brand + 24, &ecx, 4);
    memcpy(brand + 28, &edx, 4);
    cpu_cpuid(0x80000004u, &eax, &ebx, &ecx, &edx);
    memcpy(brand + 32, &eax, 4);
    memcpy(brand + 36, &ebx, 4);
    memcpy(brand + 40, &ecx, 4);
    memcpy(brand + 44, &edx, 4);
    printf("[x86] CPU: %s\n", brand);

    cpu_u64_t efer = cpu_rdmsr(MSR_EFER);
    printf("[x86] EFER=0x%016llx  LMA=%u  NXE=%u  SCE=%u\n",
           (unsigned long long)efer,
           (cpu_u32_t)((efer >> 10) & 1u),
           (cpu_u32_t)((efer >> 11) & 1u),
           (cpu_u32_t)((efer >>  0) & 1u));
}
