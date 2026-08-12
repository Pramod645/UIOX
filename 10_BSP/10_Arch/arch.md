10_Arch/
├── Makefile                  ← build all four targets
├── uiox_arch_main.c          ← single kernel entry (parallel to Makefile)
├── uiox_arch_main.h          ← header (parallel to Makefile)
├── arm64/
│   ├── include/arch_defs.h
│   ├── include/arch_runtime.h
│   └── src/
│       ├── arch_init.c
│       ├── arch_runtime.c
│       ├── arch_context.S
│       └── arch_irq.S
├── arm32/   (same structure)
├── x86_64/  (same structure)
└── riscv64/ (same structure)
-----------------
/* 50_UIX/kernel/uiox_kernel_main.c */

#include "../../10_Arch/src/uiox_arch_main.h"
#include "../../33_ProcessControlSubsystem/include/proc.h"
#include "../../50_UIX/01_shell/shell.h"

void uiox_kernel_main(unsigned long dtb_pa)
{
    /* Step 1: arch_init() + uiox_soc_init() — single call */
    if (uiox_arch_main(dtb_pa) != 0)
        for (;;) ;   /* fatal — no recovery possible */

    /* Step 2: kernel subsystems */
    uiox_ks_boot_entry();   /* 12_ksign  */
    uiox_proc_init();        /* 33_PCS   */
    uiox_shell_start();      /* 50_UIX   */
}
===========================================================
What each step does
Step	Function	ISA / HW touched	Purpose
1	arm64_cpu_identify()	MIDR_EL1, MPIDR_EL1	Read CPU part number and core affinity
2	arm64_cache_enable()	SCTLR_EL1, IC IALLU, TLBI VMALLE1	Turn on D/I-cache, flush stale TLB and I-cache
3	arm64_gic_init()	GIC-400 MMIO (GICD_*, GICC_*)	Enable interrupt distributor, set priorities to 0xA0, unmask CPU interface
4	arm64_vbar_install()	VBAR_EL1, ISB	Point exception vector base at _vector_table linker symbol
5	arm64_timer_init()	CNTFRQ_EL0, CNTP_TVAL_EL0, CNTP_CTL_EL0	Start ARMv8 generic timer at 100 Hz
6	mmio_init() / irq_init()	20_DriverInterfaces	Initialise MMIO accessor and IRQ dispatch table
7	irq_register() / irq_enable()	GIC MMIO	Wire timer IRQ and UART0 IRQ to their handlers
8	DAIFCLR #2	DAIF PSTATE bits	Globally unmask IRQs at the CPU level


Layer boundaries:
uiox_kernel_main()
│
├── arch_init()          ← THIS MODULE (10_Arch/arm64)
│     ISA registers: MIDR, MPIDR, SCTLR, VBAR, CNTFRQ,
│                   CNTP_*, DAIF
│     MMIO: GIC-400 (architecture-defined base from arch_defs.h)
│
└── uiox_soc_init()      ← NEXT (03_SoC)
      SoC-specific: clocks, power management,
      peripheral MMIO map (uiox_soc_map.h)
===============================================

 ARCH INIT and ARCH needed during intilization, is arch or ISA not needed during runtime?
 No — arch_init() is only the one-time setup call. The ISA and arch layer are used continuously at runtime through a completely different set of paths. 

 Here's the full picture:

 What arch_init() sets up (one-time, boot only)
These run once, then the hardware stays configured:

SCTLR_EL1 — cache on
VBAR_EL1 — exception vector table installed
GIC distributor — interrupt routing configured
CNTP_CTL_EL0 — timer started

What the arch/ISA layer does at runtime (continuously)
1 — Every interrupt / exception
Every time an IRQ, FIQ, SVC, abort, or fault fires, the CPU jumps to VBAR_EL1 — the vector table installed by arch_vbar_install(). The arch layer handles this on every tick:
Hardware IRQ fires
    → CPU reads VBAR_EL1 → jumps to vector table (arch layer)
        → save registers (X0–X30, SP, ELR_EL1, SPSR_EL1)
        → irq_dispatch() → registered handler
        → restore registers
        → ERET back to interrupted code


2 — Every scheduler tick (100 Hz)
The generic timer fires ARCH_TIMER_IRQ_PHYS 100 times per second. The arch timer handler (registered in arch_init()) drives 33_PCS/01_schedular — every context switch goes through the arch layer:
CNTP_CTL_EL0 fires at 100 Hz
    → arch timer IRQ handler
        → uiox_sched_tick()       ← 33_PCS/01_schedular
        → pick next process
        → arch_context_switch()   ← saves/restores all registers
            MSR TPIDR_EL0         ← per-thread pointer
            LDR/STR X0-X30, SP, PC via ELR_EL1


3 — Every syscall (SVC instruction)
Every userspace syscall is an ISA SVC #0 instruction — lands in the arch exception vector, which dispatches into 40_SystemCallInterface:
User process: SVC #0
    → EL0 → EL1 transition (ISA)
    → VBAR_EL1 + 0x400 (sync from EL0)
    → arch_syscall_entry()
        → read syscall number from X8
        → syscall_dispatch(nr, args)   ← 40_SystemCallInterface
        → return value in X0
    → ERET → EL0


4 — Every memory fault (MMU)
Page faults and alignment faults land in the arch layer via ESR_EL1 (Exception Syndrome Register):
Process accesses unmapped/protected page
    → MMU raises Data Abort
    → VBAR_EL1 + 0x200 (sync from EL1) or +0x400 (from EL0)
    → arch reads ESR_EL1, FAR_EL1 (fault address)
    → uiox_mm_page_fault()   ← 33_PCS/02_MemMngnt
        → demand-page or SIGSEGV


5 — ASLR and context switch (33_PCS/05_sec + 01_schedular)
Every exec() and every thread switch touches ISA registers:
exec() → uiox_aslr_randomise_mm()
    → writes TTBR0_EL1 (page table base)
    → TLBI ASIDE1IS (TLB flush for old ASID)

Context switch:
    → MSR TPIDR_EL0  (thread-local pointer)
    → MSR TTBR0_EL1  (new process page table)
    → ISB + DSB barriers


6 — Live patching (33_PCS/06_kpatch)
uiox_kp_engine_init() patches live kernel text — requires arch-specific instruction encoding and cache maintenance:
kpatch apply:
    → write trampoline bytes to kernel .text
    → DC CVAU (clean data cache by VA)
    → IC IVAU (invalidate I-cache by VA)
    → DSB ISH + ISB  (barriers — ISA)
    → patched function now jumps to replacement


Bottom line
Phase	Arch/ISA usage
Boot	arch_init() — one-time configuration
Every 10ms	Timer IRQ → scheduler tick → context switch (register save/restore, TTBR0, TPIDR)
Every syscall	SVC → EL0→EL1 transition → vector table → dispatch
Every page fault	Data/instruction abort → ESR_EL1/FAR_EL1 → demand paging
Every exec()	TTBR0_EL1 swap, TLBI, ASLR entropy from TRNG
Every kpatch	DC CVAU, IC IVAU, DSB/ISB cache maintenance


================================
Summary — all four architectures side by side
┌────────────────┬─────────────┬──────────────┬────────────────────────┐
│ Arch           │ Instruction │ NR register  │ UIOX C entry           │
├────────────────┼─────────────┼──────────────┼────────────────────────┤
│ ARM64          │ SVC #0      │ x8           │ arch_syscall_entry()   │
│                │ EL0→EL1     │ args: x0–x5  │ wired via VBAR_EL1     │
├────────────────┼─────────────┼──────────────┼────────────────────────┤
│ ARM32          │ SVC #0      │ r7           │ arch_syscall_entry()   │
│                │ USR→SVC     │ args: r0–r5  │ wired via vector table │
├────────────────┼─────────────┼──────────────┼────────────────────────┤
│ RISC-V 64      │ ECALL       │ a7           │ arch_syscall_entry()   │
│                │ U→S mode    │ args: a0–a5  │ wired via stvec CSR    │
├────────────────┼─────────────┼──────────────┼────────────────────────┤
│ x86-64         │ SYSCALL     │ rax          │ arch_syscall_entry()   │
│                │ Ring3→Ring0 │ args:        │ wired via LSTAR MSR    │
│                │             │ rdi,rsi,rdx, │                        │
│                │             │ r10,r8,r9    │                        │
└────────────────┴─────────────┴──────────────┴────────────────────────┘

All four call:  uiox_syscall_dispatch(&frame)  →  33_PCS/src/uiox_syscall.c
