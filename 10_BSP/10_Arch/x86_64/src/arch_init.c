/*
 * 10_Arch/x86_64/src/arch_init.c
 * AMD64 / Intel 64-bit architecture initialisation.
 *
 * Scope — architecture layer only:
 *   1. GDT reload (64-bit segment descriptors — ISA concept)
 *   2. IDT install (interrupt descriptor table — ISA concept)
 *   3. 8259A PIC remap + mask (required before LAPIC takes over)
 *   4. Local APIC enable via IA32_APIC_BASE MSR (ISA register)
 *   5. IRQ handler registration via 20_DriverInterfaces
 *   6. CR0 / CR4 feature enable (ISA control registers)
 *
 * NOT in this file (all in 03_SoC):
 *   - HPET base address configuration  → 03_SoC/uiox_soc_x86.c
 *   - COM1 baud rate divisor            → 03_SoC/uiox_soc_x86.c
 *   - PCI bus enumeration              → 03_SoC/uiox_soc_pcie.c
 *   - ACPI table parsing               → 03_SoC/uiox_soc_pm.c
 *
 * Called by:
 *   uiox_kernel_main() → arch_init() → uiox_soc_init()
 */

 #include "arch_defs.h"
 #include "hw_types.h"   // resolved via -I<arch>/include (already in Makefile)
 #include "mmio.h"
 #include "irq.h"
 #include "cpu.h"
 #include "../../../03_SoC/include/uiox_soc_stdio.h"
 #include "../../../03_SoC/include/uiox_soc_string.h"



/*
 * arch_syscall_entry — x86-64 (AMD64)
 *
 * Called via the SYSCALL instruction — the fastest x86-64
 * syscall path. SYSCALL loads RIP from LSTAR MSR and
 * switches to Ring 0 without going through the IDT.
 *
 * x86-64 SysV syscall convention:
 *   rax = syscall number
 *   rdi = arg 0
 *   rsi = arg 1
 *   rdx = arg 2
 *   r10 = arg 3   (NOT rcx — rcx is clobbered by SYSCALL)
 *   r8  = arg 4
 *   r9  = arg 5
 *
 * SYSCALL hardware behaviour:
 *   rcx ← rip  (return address — saved by CPU)
 *   r11 ← rflags
 *   rflags &= ~FMASK  (from SFMASK MSR — clears IF, DF etc.)
 *   cs  ← STAR[47:32]
 *   ss  ← STAR[47:32] + 8
 *   rip ← LSTAR
 *
 * The assembly stub sets up rsp (kernel stack), saves registers,
 * then calls this C function.
 *
 * Returns: value placed in rax before SYSRETQ to Ring 3.
 */

 #include "uiox_syscall.h"

 /* MSR addresses */
 #define MSR_EFER       0xC0000080UL
 #define MSR_STAR       0xC0000081UL   /* segment selectors for SYSCALL */
 #define MSR_LSTAR      0xC0000082UL   /* 64-bit SYSCALL entry RIP      */
 #define MSR_SFMASK     0xC0000084UL   /* RFLAGS mask on SYSCALL        */
 #define EFER_SCE       (1UL << 0)     /* syscall enable bit in EFER    */
 
 /* Kernel/user segment selectors (must match GDT layout) */
 #define UIOX_KERNEL_CS 0x08UL         /* GDT entry 1 — kernel code     */
 #define UIOX_USER_CS   0x18UL         /* GDT entry 3 — user code       */
 
 /* RFLAGS bits to mask on SYSCALL entry */
 #define SYSCALL_RFLAGS_MASK  0x200UL  /* clear IF (interrupts) on entry*/
 
 static void wrmsr(uint32_t msr, uint64_t val)
 {
     uint32_t lo = (uint32_t)(val & 0xFFFFFFFFUL);
     uint32_t hi = (uint32_t)(val >> 32);
     __asm__ volatile("wrmsr" :: "c"(msr), "a"(lo), "d"(hi) : "memory");
 }
 
 static uint64_t rdmsr(uint32_t msr)
 {
     uint32_t lo, hi;
     __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
     return ((uint64_t)hi << 32) | lo;
 }
 
 /* Forward declaration — assembly stub defined below */
 extern void syscall_entry_stub(void);
 
 /*
  * arch_syscall_setup — call this from arch_init() to enable
  * SYSCALL/SYSRET and point LSTAR at the entry stub.
  */
 static void arch_syscall_setup(void)
 {
     /* 1 — Enable SYSCALL/SYSRET in EFER */
     uint64_t efer = rdmsr(MSR_EFER);
     efer |= EFER_SCE;
     wrmsr(MSR_EFER, efer);
 
     /*
      * 2 — STAR MSR layout:
      *   [63:48] = user CS - 16  (SYSRET sets cs=this+16, ss=this+8)
      *   [47:32] = kernel CS     (SYSCALL sets cs=this, ss=this+8)
      *   [31:0]  = reserved (0)
      */
     uint64_t star = ((uint64_t)(UIOX_USER_CS - 16) << 48) |
                     ((uint64_t)UIOX_KERNEL_CS       << 32);
     wrmsr(MSR_STAR, star);
 
     /* 3 — LSTAR = RIP of our assembly entry stub */
     wrmsr(MSR_LSTAR, (uint64_t)(uintptr_t)syscall_entry_stub);
 
     /* 4 — SFMASK = RFLAGS bits to clear on SYSCALL entry */
     wrmsr(MSR_SFMASK, SYSCALL_RFLAGS_MASK);
 }
 
 /*
  * arch_syscall_entry — C handler, called from syscall_entry_stub.
  *
  * Arguments arrive already in the correct C registers
  * (rdi, rsi, rdx, r10→rcx_saved, r8, r9) because the
  * stub remaps r10 → rcx before the call.
  */
 long arch_syscall_entry(unsigned long nr,   /* rax — moved to rdi by stub */
                         unsigned long a0,   /* rdi */
                         unsigned long a1,   /* rsi */
                         unsigned long a2,   /* rdx */
                         unsigned long a3,   /* r10 → passed via stack/reg  */
                         unsigned long a4,   /* r8  */
                         unsigned long a5)   /* r9  */
 {
     uiox_syscall_frame_t frame = {
         .nr = nr,
         .a0 = a0,
         .a1 = a1,
         .a2 = a2,
         .a3 = a3,
         .a4 = a4,
         .a5 = a5,
     };
     return (long)uiox_syscall_dispatch(&frame);
 }
 


/* Add inside arch_init() after IDT/LAPIC setup */
arch_syscall_setup();
printf("[arch_init] SYSCALL/SYSRET enabled — LSTAR → syscall_entry_stub\n");


 
 /* ── Forward declarations for IRQ handlers ─────────────────────────── */
 extern void uart_irq_handler (int irq, hw_context_t *ctx, void *id);
 extern void timer_irq_handler(int irq, hw_context_t *ctx, void *id);
 
 /* =========================================================================
  * 1. CR0 / CR4 ISA feature setup
  *    Write-protect (WP), NX (via EFER), OSFXSR (SSE) —
  *    all ISA-defined control registers.
  * ====================================================================== */
 static void x86_cr_setup(void)
 {
     /* Enable Write-Protect in CR0 so kernel cannot write read-only pages */
     unsigned long cr0;
     __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
     cr0 |= CR0_WP;
     __asm__ volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
 
     /* Enable global pages and SSE in CR4 */
     unsigned long cr4;
     __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
     cr4 |= CR4_PGE | CR4_OSFXSR;
     __asm__ volatile("mov %0, %%cr4" :: "r"(cr4) : "memory");
 
     /* Enable NX (No-Execute) via EFER MSR */
     unsigned int lo, hi;
     __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(MSR_EFER));
     lo |= MSR_EFER_NXE;
     __asm__ volatile("wrmsr" :: "c"(MSR_EFER), "a"(lo), "d"(hi));
 
     printf("[x86] CR0=0x%lx  CR4=0x%lx  EFER.NXE set\n", cr0, cr4);
 }
 
 /* =========================================================================
  * 2. 8259A PIC remap
  *    ISA-level operation: relocate legacy PIC vectors away from CPU
  *    exception range (0x00-0x1F) before enabling the APIC.
  *    Master → 0x20, Slave → 0x28.  Then mask all lines so LAPIC wins.
  * ====================================================================== */
 static void x86_pic_remap(void)
 {
     /* ICW1: cascade mode, ICW4 needed */
     arch_outb(0x20u, 0x11u);
     arch_outb(0xA0u, 0x11u);
 
     /* ICW2: base vectors */
     arch_outb(0x21u, 0x20u);   /* master → 0x20 */
     arch_outb(0xA1u, 0x28u);   /* slave  → 0x28 */
 
     /* ICW3: slave on master IRQ2 */
     arch_outb(0x21u, 0x04u);
     arch_outb(0xA1u, 0x02u);
 
     /* ICW4: 8086 mode */
     arch_outb(0x21u, 0x01u);
     arch_outb(0xA1u, 0x01u);
 
     /* OCW1: mask all — LAPIC takes all hardware IRQs */
     arch_outb(0x21u, 0xFFu);
     arch_outb(0xA1u, 0xFFu);
 
     printf("[x86] 8259A PIC remapped (master=0x20 slave=0x28) and masked\n");
 }
 
 /* =========================================================================
  * 3. Local APIC enable
  *    MSR_IA32_APIC_BASE is an ISA-defined MSR (same on every x86_64 CPU).
  *    LAPIC_BASE is the default MMIO address from arch_defs.h.
  * ====================================================================== */
 static void x86_lapic_init(void)
 {
     /* Enable LAPIC globally via IA32_APIC_BASE MSR */
     unsigned int lo, hi;
     __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi)
                      : "c"(MSR_IA32_APIC_BASE));
     lo |= MSR_IA32_APIC_BASE_EN;
     __asm__ volatile("wrmsr" :: "c"(MSR_IA32_APIC_BASE),
                                 "a"(lo), "d"(hi));
 
     /* Set spurious interrupt vector and enable LAPIC via SVR */
     mmio_write32(LAPIC_SPURIOUS, LAPIC_SPURIOUS_ENABLE);
 
     printf("[x86] LAPIC enabled at 0x%08lx (SVR=0x%08x)\n",
            (unsigned long)LAPIC_BASE, LAPIC_SPURIOUS_ENABLE);
 }
 
 /* =========================================================================
  * 4. CPU identification via CPUID  (ISA instruction — same on all x86_64)
  * ====================================================================== */
 static void x86_cpu_identify(void)
 {
     unsigned int eax = 0u, ebx = 0u, ecx = 0u, edx = 0u;
     char vendor[13] = {0};
 
     /* CPUID leaf 0: vendor string */
     __asm__ volatile("cpuid"
                      : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                      : "a"(0u), "c"(0u));
 
     /* vendor is EBX:EDX:ECX */
     unsigned int *vp = (unsigned int *)vendor;
     vp[0] = ebx; vp[1] = edx; vp[2] = ecx;
 
     /* CPUID leaf 1: feature flags */
     __asm__ volatile("cpuid"
                      : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                      : "a"(1u), "c"(0u));
 
     unsigned int family  = ((eax >> 8u)  & 0xFu) + ((eax >> 20u) & 0xFFu);
     unsigned int model   = ((eax >> 4u)  & 0xFu) | ((eax >> 12u) & 0xF0u);
     unsigned int logical = (ebx >> 16u)  & 0xFFu;
 
     printf("[x86] CPU vendor='%s'  family=%u  model=%u  logical=%u\n",
            vendor, family, model, logical);
 }
 
 /* =========================================================================
  * arch_init — called by uiox_kernel_main() before uiox_soc_init()
  * ====================================================================== */
 void arch_init(void)
 {
     printf("\n[arch_init] *** x86_64 (AMD64) ***\n");
 
     /* 1. Identify CPU (ISA: CPUID instruction) */
     x86_cpu_identify();
 
     /* 2. Set ISA control register features (CR0.WP, CR4.PGE, EFER.NXE) */
     x86_cr_setup();
 
     /* 3. Remap and mask legacy 8259A PIC (ISA-level prerequisite) */
     x86_pic_remap();
 
     /* 4. Enable Local APIC (ISA MSR) */
     x86_lapic_init();
 
     /* 5. Initialise driver-interface layer */
     mmio_init();
     irq_init();
 
     /* 6. Register IRQ handlers
      *    IRQ 0 = timer (PIT/HPET — vector 0x20 after PIC remap)
      *    IRQ 4 = COM1 UART (vector 0x24 after PIC remap)
      *    Actual handler implementations are in 20_DriverInterfaces.    */
     //irq_register(0, timer_irq_handler, NULL);
     //irq_register(4, uart_irq_handler,  NULL);
     irq_request(TIMER0_IRQ, timer_irq_handler, NULL, "timer");
     irq_request(UART0_IRQ,  uart_irq_handler,  NULL, "uart");
     /* 7. Unmask global interrupts (ISA: STI instruction) */
     __asm__ volatile("sti" ::: "memory");
 
     printf("[arch_init] x86_64 architecture layer ready\n");
     /* uiox_soc_init() follows immediately in uiox_kernel_main() */
 }
 
 void arch_fini(void)
 {
     __asm__ volatile("cli" ::: "memory");
     irq_free(0);
     irq_free(4);
     printf("[arch_fini] x86_64 architecture layer torn down\n");
 }
 