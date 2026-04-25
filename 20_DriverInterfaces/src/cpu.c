/*
 * src/cpu.c
 *
 * CPU-level operations: interrupt control, context save/restore,
 * control register access, memory barriers, and CPU identification.
 *
 * Real inline-asm blocks for ARM64 / ARM32 / x86_64 are enabled
 * when the compiler reports the matching target architecture.
 * When cross-compiling or unit-testing on a different host the
 * blocks fall through to portable C simulations so the file
 * always compiles cleanly.
 */

 #include "cpu.h"
 #include <stdio.h>
 #include <string.h>
 #include <setjmp.h>
 
 /* =============================================================
  * Simulated interrupt-enable flag (hosted build)
  * ============================================================= */
 static volatile uint64_t sim_flags = 0x200;  /* IF=1 (enabled)     */
 
 /* =============================================================
  * cpu_irq_disable
  *
  * ARM64:  MRS x0, DAIF  /  MSR DAIFSET, #2  (mask IRQ)
  * ARM32:  MRS r0, CPSR  /  ORR r0,r0,#0x80  /  MSR CPSR,r0
  * x86_64: PUSHFQ + CLI  →  return old RFLAGS
  * ============================================================= */
 uint64_t cpu_irq_disable(void)
 {
     uint64_t flags = sim_flags;
 
 #if defined(UIOX_ARCH_ARM64) && defined(__aarch64__)
     uint64_t daif;
     __asm__ volatile(
         "mrs %0, daif\n\t"
         "msr daifset, #2\n\t"   /* mask IRQ bit */
         : "=r"(daif) :: "memory"
     );
     return daif;
 
 #elif defined(UIOX_ARCH_ARM32) && defined(__arm__)
     uint32_t cpsr;
     __asm__ volatile(
         "mrs %0, cpsr\n\t"
         "orr r1, %0, #0x80\n\t"
         "msr cpsr_c, r1\n\t"
         : "=r"(cpsr) :: "r1", "memory"
     );
     return (uint64_t)cpsr;
 
 #elif defined(UIOX_ARCH_X86_64) && defined(__x86_64__)
     uint64_t rflags;
     __asm__ volatile(
         "pushfq\n\t"
         "popq %0\n\t"
         "cli\n\t"
         : "=r"(rflags) :: "memory"
     );
     sim_flags &= ~(uint64_t)X86_EFLAGS_IF;
     return rflags;
 
 #else
     /* Hosted simulation */
     sim_flags &= ~(uint64_t)X86_EFLAGS_IF;
     return flags;
 #endif
 }
 
 /* =============================================================
  * cpu_irq_enable — unconditionally unmask IRQs
  * ============================================================= */
 void cpu_irq_enable(void)
 {
 #if defined(UIOX_ARCH_ARM64) && defined(__aarch64__)
     __asm__ volatile("msr daifclr, #2\n\t" ::: "memory");
 
 #elif defined(UIOX_ARCH_ARM32) && defined(__arm__)
     __asm__ volatile(
         "mrs r0, cpsr\n\t"
         "bic r0, r0, #0x80\n\t"
         "msr cpsr_c, r0\n\t"
         ::: "r0", "memory"
     );
 
 #elif defined(UIOX_ARCH_X86_64) && defined(__x86_64__)
     __asm__ volatile("sti\n\t" ::: "memory");
 #endif
 
     sim_flags |= (uint64_t)X86_EFLAGS_IF;
 }
 
 /* =============================================================
  * cpu_irq_restore — restore flags saved by cpu_irq_disable
  * ============================================================= */
 void cpu_irq_restore(uint64_t flags)
 {
 #if defined(UIOX_ARCH_ARM64) && defined(__aarch64__)
     __asm__ volatile("msr daif, %0\n\t" :: "r"(flags) : "memory");
 
 #elif defined(UIOX_ARCH_ARM32) && defined(__arm__)
     __asm__ volatile("msr cpsr_c, %0\n\t" :: "r"((uint32_t)flags) : "memory");
 
 #elif defined(UIOX_ARCH_X86_64) && defined(__x86_64__)
     __asm__ volatile(
         "pushq %0\n\t"
         "popfq\n\t"
         :: "r"(flags) : "memory", "cc"
     );
 #endif
 
     sim_flags = flags;
 }
 
 /* =============================================================
  * cpu_irq_enabled
  * ============================================================= */
 int cpu_irq_enabled(void)
 {
 #if defined(UIOX_ARCH_X86_64) && defined(__x86_64__)
     uint64_t rflags;
     __asm__ volatile("pushfq\n\tpopq %0\n\t" : "=r"(rflags));
     return (int)((rflags >> 9) & 1);
 #else
     return (int)((sim_flags & X86_EFLAGS_IF) != 0);
 #endif
 }
 
 /* =============================================================
  * cpu_context_save / cpu_context_restore
  *
  * Software equivalent of the hardware's automatic register save
  * on interrupt entry.  Used by the device-open algorithm:
  *
  *   if (cpu_context_save(&saved_ctx) == 0)
  *   {
  *       // first entry — call driver open
  *       ret = driver_open(minor, mode);
  *       if (ret != 0)
  *           cpu_context_restore(&saved_ctx);  // longjmp out
  *   }
  *   else
  *   {
  *       // returned from restore — driver failed
  *       decrement_file_table_and_inode();
  *   }
  *
  * On a real target these would use inline assembly to capture
  * all callee-saved registers.  Here we delegate to POSIX setjmp.
  * ============================================================= */
 
 /* Use a jmp_buf embedded in hw_context_t's spare space */
 static jmp_buf *ctx_to_jmp(hw_context_t *ctx)
 {
     /* We stash the jmp_buf in the ARM32 r[] array (big enough) */
     return (jmp_buf *)(void *)ctx->r;
 }
 
 int cpu_context_save(hw_context_t *ctx)
 {
     /*
      * Capture current PC / SP symbolically.
      * Real implementation: inline asm STR SP, LDR PC equivalent.
      */
     ctx->sp = 0xDEADBEEF;          /* placeholder                  */
     ctx->pc = 0xCAFEBABE;
 
     printf("[cpu] context save (setjmp)\n");
     return setjmp(*ctx_to_jmp(ctx));
 }
 
 void cpu_context_restore(hw_context_t *ctx)
 {
     printf("[cpu] context restore (longjmp)\n");
     longjmp(*ctx_to_jmp(ctx), 1);
 }
 
 /* =============================================================
  * ARM64 system register access
  * ============================================================= */
 uint64_t arm64_read_daif(void)
 {
 #if defined(UIOX_ARCH_ARM64) && defined(__aarch64__)
     uint64_t v;
     __asm__ volatile("mrs %0, daif\n\t" : "=r"(v));
     return v;
 #else
     return sim_flags;
 #endif
 }
 
 void arm64_write_daif(uint64_t val)
 {
 #if defined(UIOX_ARCH_ARM64) && defined(__aarch64__)
     __asm__ volatile("msr daif, %0\n\t" :: "r"(val) : "memory");
 #else
     sim_flags = val;
 #endif
 }
 
 uint64_t arm64_read_elr_el1(void)
 {
 #if defined(UIOX_ARCH_ARM64) && defined(__aarch64__)
     uint64_t v;
     __asm__ volatile("mrs %0, elr_el1\n\t" : "=r"(v));
     return v;
 #else
     return 0;
 #endif
 }
 
 uint64_t arm64_read_spsr_el1(void)
 {
 #if defined(UIOX_ARCH_ARM64) && defined(__aarch64__)
     uint64_t v;
     __asm__ volatile("mrs %0, spsr_el1\n\t" : "=r"(v));
     return v;
 #else
     return 0;
 #endif
 }
 
 uint64_t arm64_read_mpidr(void)
 {
 #if defined(UIOX_ARCH_ARM64) && defined(__aarch64__)
     uint64_t v;
     __asm__ volatile("mrs %0, mpidr_el1\n\t" : "=r"(v));
     return v;
 #else
     return 0;
 #endif
 }
 
 uint64_t arm64_read_cntpct(void)
 {
 #if defined(UIOX_ARCH_ARM64) && defined(__aarch64__)
     uint64_t v;
     __asm__ volatile("mrs %0, cntpct_el0\n\t" : "=r"(v));
     return v;
 #else
     return 0;
 #endif
 }
 
 /* =============================================================
  * ARM32 CPSR access
  * ============================================================= */
 uint32_t arm32_read_cpsr(void)
 {
 #if defined(UIOX_ARCH_ARM32) && defined(__arm__)
     uint32_t v;
     __asm__ volatile("mrs %0, cpsr\n\t" : "=r"(v));
     return v;
 #else
     return (uint32_t)sim_flags;
 #endif
 }
 
 void arm32_write_cpsr(uint32_t val)
 {
 #if defined(UIOX_ARCH_ARM32) && defined(__arm__)
     __asm__ volatile("msr cpsr_c, %0\n\t" :: "r"(val) : "memory");
 #else
     sim_flags = val;
 #endif
 }
 
 /* =============================================================
  * x86_64 RFLAGS and MSR access
  * ============================================================= */
 uint64_t x86_read_rflags(void)
 {
 #if defined(UIOX_ARCH_X86_64) && defined(__x86_64__)
     uint64_t v;
     __asm__ volatile("pushfq\n\tpopq %0\n\t" : "=r"(v));
     return v;
 #else
     return sim_flags;
 #endif
 }
 
 void x86_write_rflags(uint64_t val)
 {
 #if defined(UIOX_ARCH_X86_64) && defined(__x86_64__)
     __asm__ volatile("pushq %0\n\tpopfq\n\t" :: "r"(val) : "memory","cc");
 #else
     sim_flags = val;
 #endif
 }
 
 uint64_t x86_read_msr(uint32_t msr_id)
 {
 #if defined(UIOX_ARCH_X86_64) && defined(__x86_64__)
     uint32_t lo, hi;
     __asm__ volatile("rdmsr\n\t" : "=a"(lo), "=d"(hi) : "c"(msr_id));
     return ((uint64_t)hi << 32) | lo;
 #else
     (void)msr_id;
     return 0;
 #endif
 }
 
 void x86_write_msr(uint32_t msr_id, uint64_t val)
 {
 #if defined(UIOX_ARCH_X86_64) && defined(__x86_64__)
     __asm__ volatile("wrmsr\n\t"
                      :: "c"(msr_id),
                         "a"((uint32_t)(val & 0xFFFFFFFF)),
                         "d"((uint32_t)(val >> 32))
                      : "memory");
 #else
     (void)msr_id; (void)val;
 #endif
 }
 
 uint32_t x86_cpuid_family(void)
 {
 #if defined(UIOX_ARCH_X86_64) && defined(__x86_64__)
     uint32_t eax, ebx, ecx, edx;
     __asm__ volatile("cpuid\n\t"
                      : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx)
                      : "a"(1u));
     return eax;    /* family / model / stepping in EAX bits       */
 #else
     return 0;
 #endif
 }
 
 /* =============================================================
  * Memory barriers
  * ============================================================= */
 void cpu_mb(void)
 {
 #if defined(UIOX_ARCH_ARM64) && defined(__aarch64__)
     __asm__ volatile("dmb ish\n\t" ::: "memory");
 #elif defined(UIOX_ARCH_ARM32) && defined(__arm__)
     __asm__ volatile("dmb\n\t" ::: "memory");
 #elif defined(UIOX_ARCH_X86_64) && defined(__x86_64__)
     __asm__ volatile("mfence\n\t" ::: "memory");
 #else
     __asm__ volatile("" ::: "memory");
 #endif
 }
 
 void cpu_rmb(void)
 {
 #if defined(UIOX_ARCH_ARM64) && defined(__aarch64__)
     __asm__ volatile("dmb ishld\n\t" ::: "memory");
 #elif defined(UIOX_ARCH_ARM32) && defined(__arm__)
     __asm__ volatile("dmb\n\t" ::: "memory");
 #elif defined(UIOX_ARCH_X86_64) && defined(__x86_64__)
     __asm__ volatile("lfence\n\t" ::: "memory");
 #else
     __asm__ volatile("" ::: "memory");
 #endif
 }
 
 void cpu_wmb(void)
 {
 #if defined(UIOX_ARCH_ARM64) && defined(__aarch64__)
     __asm__ volatile("dmb ishst\n\t" ::: "memory");
 #elif defined(UIOX_ARCH_ARM32) && defined(__arm__)
     __asm__ volatile("dmb\n\t" ::: "memory");
 #elif defined(UIOX_ARCH_X86_64) && defined(__x86_64__)
     __asm__ volatile("sfence\n\t" ::: "memory");
 #else
     __asm__ volatile("" ::: "memory");
 #endif
 }
 
 void cpu_isb(void)
 {
 #if defined(UIOX_ARCH_ARM64) && defined(__aarch64__)
     __asm__ volatile("isb\n\t" ::: "memory");
 #elif defined(UIOX_ARCH_ARM32) && defined(__arm__)
     __asm__ volatile("isb\n\t" ::: "memory");
 #else
     __asm__ volatile("" ::: "memory");
 #endif
 }
 
 void cpu_dsb(void)
 {
 #if defined(UIOX_ARCH_ARM64) && defined(__aarch64__)
     __asm__ volatile("dsb ish\n\t" ::: "memory");
 #elif defined(UIOX_ARCH_ARM32) && defined(__arm__)
     __asm__ volatile("dsb\n\t" ::: "memory");
 #else
     __asm__ volatile("" ::: "memory");
 #endif
 }
 
 /* =============================================================
  * CPU identification
  * ============================================================= */
 void cpu_identify(cpu_info_t *info)
 {
     if (!info) return;
     memset(info, 0, sizeof *info);
     info->ci_arch = UIOX_ARCH_NAME;
 
 #if defined(UIOX_ARCH_X86_64) && defined(__x86_64__)
     uint32_t eax = x86_cpuid_family();
     info->ci_family   = (eax >> 8)  & 0xF;
     info->ci_model    = (eax >> 4)  & 0xF;
     info->ci_stepping = (eax)       & 0xF;
     info->ci_ncores   = 1;
 
 #elif defined(UIOX_ARCH_ARM64) && defined(__aarch64__)
     uint64_t mpidr = arm64_read_mpidr();
     info->ci_ncores = (uint32_t)(mpidr & 0xFF) + 1;
 
 #elif defined(UIOX_ARCH_ARM32) && defined(__arm__)
     info->ci_ncores = 1;
 
 #else
     info->ci_ncores = 1;
 #endif
 }
 
 void cpu_print_info(const cpu_info_t *info)
 {
     printf("[cpu] arch=%-8s  family=0x%02x  model=0x%02x  "
            "stepping=0x%02x  ncores=%u\n",
            info->ci_arch,
            info->ci_family, info->ci_model,
            info->ci_stepping, info->ci_ncores);
 }
 
 /* =============================================================
  * Halt / idle helpers
  * ============================================================= */
 void cpu_halt(void)
 {
 #if defined(UIOX_ARCH_X86_64) && defined(__x86_64__)
     __asm__ volatile("hlt\n\t");
 #elif (defined(UIOX_ARCH_ARM64) && defined(__aarch64__)) || \
       (defined(UIOX_ARCH_ARM32) && defined(__arm__))
     __asm__ volatile("wfi\n\t");
 #else
     printf("[cpu] halt (sim)\n");
 #endif
 }
 
 void cpu_nop(void)
 {
 #if defined(__x86_64__) || defined(__aarch64__) || defined(__arm__)
     __asm__ volatile("nop\n\t");
 #endif
 }
 
 void cpu_wfe(void)
 {
 #if (defined(UIOX_ARCH_ARM64) && defined(__aarch64__)) || \
     (defined(UIOX_ARCH_ARM32) && defined(__arm__))
     __asm__ volatile("wfe\n\t");
 #else
     printf("[cpu] wfe (sim)\n");
 #endif
 }
 
 void cpu_sev(void)
 {
 #if (defined(UIOX_ARCH_ARM64) && defined(__aarch64__)) || \
     (defined(UIOX_ARCH_ARM32) && defined(__arm__))
     __asm__ volatile("sev\n\t");
 #else
     printf("[cpu] sev (sim)\n");
 #endif
 }
 