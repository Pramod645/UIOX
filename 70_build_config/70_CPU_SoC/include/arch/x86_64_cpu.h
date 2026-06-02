#ifndef X86_64_CPU_H
#define X86_64_CPU_H
/*
 * x86_64_cpu.h - x86-64 CPU specific definitions
 * Reference: Intel SDM Vol. 3A
 */
#include "../cpu_types.h"

/* -- CPUID leaves ------------------------------------------- */
#define CPUID_VENDOR        0x00000000u
#define CPUID_FEATURES      0x00000001u
#define CPUID_CACHE_TLB     0x00000002u
#define CPUID_EXT_FEATURES  0x00000007u
#define CPUID_EXT_INFO      0x80000000u
#define CPUID_BRAND_STR1    0x80000002u
#define CPUID_BRAND_STR2    0x80000003u
#define CPUID_BRAND_STR3    0x80000004u
#define CPUID_ADDR_SIZES    0x80000008u

/* -- CPUID feature bits (EDX leaf 1) ------------------------ */
#define CPUID_EDX_FPU       (1u <<  0)
#define CPUID_EDX_SSE       (1u << 25)
#define CPUID_EDX_SSE2      (1u << 26)
#define CPUID_EDX_APIC      (1u <<  9)

/* -- CPUID feature bits (ECX leaf 1) ------------------------ */
#define CPUID_ECX_SSE3      (1u <<  0)
#define CPUID_ECX_AES       (1u << 25)
#define CPUID_ECX_AVX       (1u << 28)
#define CPUID_ECX_RDRAND    (1u << 30)
#define CPUID_ECX_VMX       (1u <<  5)

/* -- MSR addresses ------------------------------------------ */
#define MSR_EFER            0xC0000080u
#define MSR_STAR            0xC0000081u
#define MSR_LSTAR           0xC0000082u
#define MSR_SFMASK          0xC0000084u
#define MSR_FS_BASE         0xC0000100u
#define MSR_GS_BASE         0xC0000101u
#define MSR_KERNEL_GS_BASE  0xC0000102u
#define MSR_APIC_BASE       0x0000001Bu
#define MSR_IA32_PERF_CTL   0x00000199u
#define MSR_IA32_TSC_AUX    0xC0000103u

/* -- EFER bits ---------------------------------------------- */
#define EFER_SCE   (1u <<  0)   /* SYSCALL enable               */
#define EFER_LME   (1u <<  8)   /* Long mode enable             */
#define EFER_LMA   (1u << 10)   /* Long mode active             */
#define EFER_NXE   (1u << 11)   /* No-execute enable            */

/* -- CR0 bits ----------------------------------------------- */
#define CR0_PE     (1ULL <<  0)
#define CR0_WP     (1ULL << 16)
#define CR0_PG     (1ULL << 31)

/* -- CR4 bits ----------------------------------------------- */
#define CR4_PAE      (1ULL <<  5)
#define CR4_PGE      (1ULL <<  7)
#define CR4_OSFXSR   (1ULL <<  9)
#define CR4_FSGSBASE (1ULL << 16)
#define CR4_SMEP     (1ULL << 20)
#define CR4_SMAP     (1ULL << 21)

/* -- RFLAGS bits -------------------------------------------- */
#define RFLAGS_CF   (1u <<  0)
#define RFLAGS_ZF   (1u <<  6)
#define RFLAGS_SF   (1u <<  7)
#define RFLAGS_IF   (1u <<  9)
#define RFLAGS_OF   (1u << 11)

/* -- GDT segment selectors ---------------------------------- */
#define GDT_SEL_NULL    0x00u
#define GDT_SEL_CODE64  0x08u
#define GDT_SEL_DATA64  0x10u
#define GDT_SEL_USER64  0x18u
#define GDT_SEL_TSS     0x28u

/* -- IDT vector numbers ------------------------------------- */
#define IDT_VEC_DE      0u    /* Divide error                   */
#define IDT_VEC_DB      1u    /* Debug                          */
#define IDT_VEC_NMI     2u    /* NMI                            */
#define IDT_VEC_BP      3u    /* Breakpoint                     */
#define IDT_VEC_UD      6u    /* Invalid opcode                 */
#define IDT_VEC_GP      13u   /* General protection             */
#define IDT_VEC_PF      14u   /* Page fault                     */
#define IDT_VEC_MC      18u   /* Machine check                  */
#define IDT_VEC_IRQ0    0x20u /* First PIC/APIC IRQ vector      */

/* -- LAPIC register offsets --------------------------------- */
#define LAPIC_ID_REG    0x020u
#define LAPIC_VER_REG   0x030u
#define LAPIC_TPR       0x080u
#define LAPIC_EOI       0x0B0u
#define LAPIC_SVR       0x0F0u
#define LAPIC_ICR_LO    0x300u
#define LAPIC_ICR_HI    0x310u
#define LAPIC_TIMER_LVT 0x320u
#define LAPIC_TIMER_ICR 0x380u
#define LAPIC_TIMER_CCR 0x390u
#define LAPIC_TIMER_DCR 0x3E0u

/* -- x86-64 specific functions ------------------------------ */
int  x86_cpu_init       (void);
void x86_gdt_init       (void);
void x86_idt_init       (void);
void x86_tss_init       (cpu_addr_t stack0);
void x86_enable_sse     (void);
void x86_enable_avx     (void);
void x86_paging_init    (cpu_addr_t pml4);
void x86_syscall_init   (cpu_addr_t handler);
void x86_cpu_print_info (void);
cpu_u64_t x86_read_tsc  (void);

/* -- GDT descriptor (8 bytes) ------------------------------- */
typedef struct __attribute__((packed)) x86_gdt_entry {
    cpu_u16_t limit_lo;
    cpu_u16_t base_lo;
    cpu_u8_t  base_mid;
    cpu_u8_t  access;
    cpu_u8_t  granularity;
    cpu_u8_t  base_hi;
} x86_gdt_entry_t;

/* -- IDT gate (16 bytes) ------------------------------------ */
typedef struct __attribute__((packed)) x86_idt_gate {
    cpu_u16_t offset_lo;
    cpu_u16_t selector;
    cpu_u8_t  ist;
    cpu_u8_t  type_attr;
    cpu_u16_t offset_mid;
    cpu_u32_t offset_hi;
    cpu_u32_t reserved;
} x86_idt_gate_t;

/* -- TSS (64-bit) ------------------------------------------- */
typedef struct __attribute__((packed)) x86_tss {
    cpu_u32_t reserved0;
    cpu_u64_t rsp[3];      /* RSP0-RSP2                         */
    cpu_u64_t reserved1;
    cpu_u64_t ist[7];      /* IST1-IST7                         */
    cpu_u64_t reserved2;
    cpu_u16_t reserved3;
    cpu_u16_t io_map_base;
} x86_tss_t;

#endif /* X86_64_CPU_H */

