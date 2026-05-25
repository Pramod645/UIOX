#ifndef __ARCH_X86_64_CPU_H
#define __ARCH_X86_64_CPU_H

/*
 * cpu.h  —  x86_64 CPU register layout, context switch,
 *           and CPUID definitions.
 *
 * Mirrors: 10_Arch/arm32/include/cpu.h
 */

#include <stdint.h>

/* ── General-purpose register file (saved on context switch) */
typedef struct arch_regs {
    /* callee-saved (System V AMD64 ABI) */
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbp;
    uint64_t rbx;
    /* caller-saved */
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rax;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    /* pushed by hardware on interrupt / exception */
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} arch_regs_t;

/* ── Interrupt frame (hardware-pushed subset) ────────────── */
typedef struct arch_iframe {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} arch_iframe_t;

/* ── RFLAGS bits ─────────────────────────────────────────── */
#define RFLAGS_CF   (1UL <<  0)   /* carry                   */
#define RFLAGS_PF   (1UL <<  2)   /* parity                  */
#define RFLAGS_AF   (1UL <<  4)   /* auxiliary carry         */
#define RFLAGS_ZF   (1UL <<  6)   /* zero                    */
#define RFLAGS_SF   (1UL <<  7)   /* sign                    */
#define RFLAGS_TF   (1UL <<  8)   /* trap                    */
#define RFLAGS_IF   (1UL <<  9)   /* interrupt enable        */
#define RFLAGS_DF   (1UL << 10)   /* direction               */
#define RFLAGS_OF   (1UL << 11)   /* overflow                */
#define RFLAGS_IOPL (3UL << 12)   /* I/O privilege level     */
#define RFLAGS_ID   (1UL << 21)   /* CPUID available         */

/* ── Control registers ───────────────────────────────────── */
#define CR0_PE   (1UL <<  0)  /* protected mode enable      */
#define CR0_MP   (1UL <<  1)  /* monitor coprocessor        */
#define CR0_EM   (1UL <<  2)  /* emulate FPU                */
#define CR0_WP   (1UL << 16)  /* write protect              */
#define CR0_PG   (1UL << 31)  /* paging enable              */

#define CR4_PAE  (1UL <<  5)  /* physical address extension */
#define CR4_PGE  (1UL <<  7)  /* global pages               */
#define CR4_OSFXSR (1UL << 9) /* SSE                        */
#define CR4_OSXMMEXCPT (1UL << 10)
#define CR4_FSGSBASE (1UL << 16)

/* ── MSR addresses ───────────────────────────────────────── */
#define MSR_EFER        0xC0000080UL
#define MSR_STAR        0xC0000081UL
#define MSR_LSTAR       0xC0000082UL  /* syscall RIP (64-bit) */
#define MSR_CSTAR       0xC0000083UL  /* syscall RIP (compat) */
#define MSR_SFMASK      0xC0000084UL
#define MSR_FS_BASE     0xC0000100UL
#define MSR_GS_BASE     0xC0000101UL
#define MSR_KERN_GS_BASE 0xC0000102UL

#define EFER_SCE  (1UL << 0)   /* syscall enable             */
#define EFER_LME  (1UL << 8)   /* long mode enable           */
#define EFER_LMA  (1UL << 10)  /* long mode active           */
#define EFER_NXE  (1UL << 11)  /* no-execute enable          */

/* ── CPUID leaves ────────────────────────────────────────── */
#define CPUID_VENDOR        0x00000000
#define CPUID_FEATURES      0x00000001
#define CPUID_EXT_FEATURES  0x00000007
#define CPUID_EXT_MAX       0x80000000
#define CPUID_EXT_INFO      0x80000001

typedef struct cpuid_result {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} cpuid_result_t;

typedef struct cpu_info {
    char     vendor[13];      /* null-terminated vendor string */
    uint32_t family;
    uint32_t model;
    uint32_t stepping;
    uint32_t features_ecx;   /* CPUID leaf 1 ECX             */
    uint32_t features_edx;   /* CPUID leaf 1 EDX             */
    uint32_t ext_features;   /* CPUID leaf 7 EBX             */
    int      has_nx;          /* NX / XD bit available        */
    int      has_sse2;
    int      has_avx;
    int      has_rdrand;
} cpu_info_t;

extern cpu_info_t boot_cpu_info;

/* ── Function prototypes ─────────────────────────────────── */
void          cpu_init(void);
void          cpu_identify(cpu_info_t *info);
cpuid_result_t cpu_cpuid(uint32_t leaf, uint32_t subleaf);

uint64_t      cpu_read_cr0(void);
uint64_t      cpu_read_cr2(void);
uint64_t      cpu_read_cr3(void);
uint64_t      cpu_read_cr4(void);
void          cpu_write_cr0(uint64_t val);
void          cpu_write_cr3(uint64_t val);
void          cpu_write_cr4(uint64_t val);

uint64_t      cpu_read_msr(uint32_t msr);
void          cpu_write_msr(uint32_t msr, uint64_t val);

uint64_t      cpu_read_rflags(void);
void          cpu_enable_interrupts(void);
void          cpu_disable_interrupts(void);
uint64_t      cpu_save_and_disable_interrupts(void);
void          cpu_restore_interrupts(uint64_t flags);

void          cpu_context_switch(arch_regs_t *old_ctx,
                                  arch_regs_t *new_ctx);
void          cpu_context_init(arch_regs_t *ctx,
                                uint64_t entry,
                                uint64_t stack_top,
                                uint64_t arg);

void          cpu_tlb_flush(void);
void          cpu_tlb_flush_page(uint64_t vaddr);

#endif /* __ARCH_X86_64_CPU_H */
