/*
 * cpu.c  —  x86_64 CPU initialisation, CPUID, MSR,
 *           context-switch wrappers and register helpers.
 *
 * Mirrors: 10_Arch/arm32/src/cpu.c
 */

 #include "../include/arch.h"
 #include <stdint.h>
 #include <string.h>
 
 cpu_info_t boot_cpu_info;
 
 /* ── CPUID helper ────────────────────────────────────────── */
 cpuid_result_t cpu_cpuid(uint32_t leaf, uint32_t subleaf)
 {
     cpuid_result_t r;
     __asm__ volatile(
         "cpuid"
         : "=a"(r.eax), "=b"(r.ebx),
           "=c"(r.ecx), "=d"(r.edx)
         : "a"(leaf),   "c"(subleaf)
     );
     return r;
 }
 
 /* ── Control register read/write ─────────────────────────── */
 uint64_t cpu_read_cr0(void)
 {
     uint64_t v;
     __asm__ volatile("movq %%cr0, %0" : "=r"(v));
     return v;
 }
 void cpu_write_cr0(uint64_t v)
 {
     __asm__ volatile("movq %0, %%cr0" :: "r"(v) : "memory");
 }
 uint64_t cpu_read_cr2(void)
 {
     uint64_t v;
     __asm__ volatile("movq %%cr2, %0" : "=r"(v));
     return v;
 }
 uint64_t cpu_read_cr3(void)
 {
     uint64_t v;
     __asm__ volatile("movq %%cr3, %0" : "=r"(v));
     return v;
 }
 void cpu_write_cr3(uint64_t v)
 {
     __asm__ volatile("movq %0, %%cr3" :: "r"(v) : "memory");
 }
 uint64_t cpu_read_cr4(void)
 {
     uint64_t v;
     __asm__ volatile("movq %%cr4, %0" : "=r"(v));
     return v;
 }
 void cpu_write_cr4(uint64_t v)
 {
     __asm__ volatile("movq %0, %%cr4" :: "r"(v) : "memory");
 }
 
 /* ── MSR read/write ──────────────────────────────────────── */
 uint64_t cpu_read_msr(uint32_t msr)
 {
     uint32_t lo, hi;
     __asm__ volatile("rdmsr"
                      : "=a"(lo), "=d"(hi)
                      : "c"(msr));
     return ((uint64_t)hi << 32) | lo;
 }
 
 void cpu_write_msr(uint32_t msr, uint64_t val)
 {
     __asm__ volatile("wrmsr"
                      :: "c"(msr),
                         "a"((uint32_t)(val & 0xFFFFFFFF)),
                         "d"((uint32_t)(val >> 32)));
 }
 
 /* ── RFLAGS ──────────────────────────────────────────────── */
 uint64_t cpu_read_rflags(void)
 {
     uint64_t v;
     __asm__ volatile("pushfq; popq %0" : "=r"(v));
     return v;
 }
 
 void cpu_enable_interrupts(void)
 {
     __asm__ volatile("sti" ::: "memory");
 }
 
 void cpu_disable_interrupts(void)
 {
     __asm__ volatile("cli" ::: "memory");
 }
 
 uint64_t cpu_save_and_disable_interrupts(void)
 {
     uint64_t flags = cpu_read_rflags();
     cpu_disable_interrupts();
     return flags;
 }
 
 void cpu_restore_interrupts(uint64_t flags)
 {
     __asm__ volatile("pushq %0; popfq"
                      :: "r"(flags) : "memory", "cc");
 }
 
 /* ── TLB flush ───────────────────────────────────────────── */
 void cpu_tlb_flush(void)
 {
     uint64_t cr3 = cpu_read_cr3();
     cpu_write_cr3(cr3);
 }
 
 void cpu_tlb_flush_page(uint64_t vaddr)
 {
     __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
 }
 
 /* ── CPU identification ──────────────────────────────────── */
 void cpu_identify(cpu_info_t *info)
 {
     cpuid_result_t r;
 
     /* vendor string: EBX, EDX, ECX of leaf 0 */
     r = cpu_cpuid(0, 0);
     memcpy(info->vendor + 0, &r.ebx, 4);
     memcpy(info->vendor + 4, &r.edx, 4);
     memcpy(info->vendor + 8, &r.ecx, 4);
     info->vendor[12] = '\0';
 
     /* family / model / stepping from leaf 1 */
     r = cpu_cpuid(1, 0);
     info->stepping      = r.eax & 0xF;
     info->model         = (r.eax >> 4)  & 0xF;
     info->family        = (r.eax >> 8)  & 0xF;
     info->features_ecx  = r.ecx;
     info->features_edx  = r.edx;
 
     /* extended features */
     r = cpu_cpuid(7, 0);
     info->ext_features  = r.ebx;
 
     /* NX bit */
     r = cpu_cpuid(0x80000001, 0);
     info->has_nx        = (r.edx >> 20) & 1;
 
     /* SSE2, AVX, RDRAND */
     info->has_sse2      = (info->features_edx >> 26) & 1;
     info->has_avx       = (info->features_ecx >> 28) & 1;
     info->has_rdrand    = (info->features_ecx >> 30) & 1;
 }
 
 /* ── Context initialisation ──────────────────────────────── */
 void cpu_context_init(arch_regs_t *ctx,
                        uint64_t entry,
                        uint64_t stack_top,
                        uint64_t arg)
 {
     memset(ctx, 0, sizeof(arch_regs_t));
     ctx->rip    = entry;
     ctx->rsp    = stack_top - 8;   /* fake return address slot */
     ctx->rdi    = arg;             /* first argument (SysV ABI) */
     ctx->rflags = RFLAGS_IF;       /* interrupts enabled        */
     ctx->cs     = GDT_KERN_CODE_SEL;
     ctx->ss     = GDT_KERN_DATA_SEL;
 }
 
 /* ── Context switch (calls assembly helper) ──────────────── */
 /* declared in boot.S */
 extern void cpu_context_switch_asm(arch_regs_t *old,
                                     arch_regs_t *new_ctx);
 
 void cpu_context_switch(arch_regs_t *old_ctx,
                          arch_regs_t *new_ctx)
 {
     cpu_context_switch_asm(old_ctx, new_ctx);
 }
 
 /* ── CPU init ────────────────────────────────────────────── */
 void cpu_init(void)
 {
     cpu_identify(&boot_cpu_info);
 
     /* enable NX if available */
     if (boot_cpu_info.has_nx) {
         uint64_t efer = cpu_read_msr(MSR_EFER);
         cpu_write_msr(MSR_EFER, efer | EFER_NXE);
     }
 
     /* enable SSE */
     uint64_t cr0 = cpu_read_cr0();
     cr0 &= ~CR0_EM;
     cr0 |=  CR0_MP;
     cpu_write_cr0(cr0);
 
     uint64_t cr4 = cpu_read_cr4();
     cr4 |= CR4_OSFXSR | CR4_OSXMMEXCPT;
     cpu_write_cr4(cr4);
 }
 