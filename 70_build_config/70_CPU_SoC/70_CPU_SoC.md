70_CPU_SoC/
├── include/
│   ├── cpu_types.h           ← base types for all 3 arches
│   ├── cpu_features.h        ← feature detection / capability bits
│   ├── cpu_regs.h            ← register access abstractions
│   ├── cpu_cache.h           ← cache management interface
│   ├── cpu_mmu.h             ← MMU / page-table interface
│   ├── cpu_irq.h             ← interrupt controller interface
│   ├── cpu_timer.h           ← system timer interface
│   ├── cpu_power.h           ← power management (WFI/WFE/HLT)
│   ├── cpu_smp.h             ← multi-core / SMP interface
│   ├── cpu_context.h         ← CPU context save/restore
│   ├── cpu_debug.h           ← debug / breakpoint interface
│   ├── cpu_io.h              ← device access (MMIO / port I/O)
│   ├── cpu_soc.h             ← SoC-level master include
│   ├── arch/
│   │   ├── cortex_a76.h      ← ARM Cortex-A76 specifics
│   │   ├── x86_64_cpu.h      ← x86-64 specifics
│   │   └── riscv64.h         ← RISC-V RV64GC specifics
│   └── drivers/
│       ├── cpu_drv_gic.h     ← GIC-600 driver (ARM)
│       ├── cpu_drv_apic.h    ← LAPIC/IOAPIC driver (x86)
│       ├── cpu_drv_plic.h    ← PLIC driver (RISC-V)
│       ├── cpu_drv_timer.h   ← generic timer driver
│       └── cpu_drv_uart.h    ← early-boot UART driver
├── src/
│   ├── cpu_features.c
│   ├── cpu_cache.c
│   ├── cpu_mmu.c
│   ├── cpu_irq.c
│   ├── cpu_timer.c
│   ├── cpu_power.c
│   ├── cpu_smp.c
│   ├── cpu_context.c
│   ├── cpu_debug.c
│   ├── cpu_io.c
│   ├── arch/
│   │   ├── cortex_a76.c
│   │   ├── x86_64_cpu.c
│   │   └── riscv64.c
│   └── drivers/
│       ├── cpu_drv_gic.c
│       ├── cpu_drv_apic.c
│       ├── cpu_drv_plic.c
│       ├── cpu_drv_timer.c
│       └── cpu_drv_uart.c
└── Makefile
===================================================
Layer	Files	Purpose
Kernel subsystem	cpu_types.h cpu_features.h cpu_soc.h	Types, feature detection, SoC descriptor + init pipeline
Register access	cpu_regs.h	MRS/MSR (ARM64), RDMSR/WRMSR (x86), CSR (RISC-V), MMIO helpers
Feature impl	cpu_cache.h/c cpu_mmu.h/c cpu_irq.h/c cpu_timer.h/c cpu_power.h/c cpu_smp.h/c cpu_context.h/c cpu_debug.h/c cpu_io.h/c	Cache flush, MMU/TLB, IRQ dispatch, timer, power/PSCI/SBI, SMP spinlock, context switch, HW breakpoints, MMIO/port I/O
Arch specifics	cortex_a76.h/c x86_64_cpu.h/c riscv64.h/c	MIDR/CPUID/MISA decode, GDT/IDT/TSS, TCR/MAIR/SCTLR, SATP/SV39, PMU, errata
IF drivers	cpu_drv_gic.h/c cpu_drv_apic.h/c cpu_drv_plic.h/c cpu_drv_timer.h/c cpu_drv_uart.h/c	GIC-600, LAPIC/IOAPIC/8259A, PLIC, generic/LAPIC/CLINT timer, PL011/16550/SiFive UART
Build	Makefile	Multi-arch build with single make ARCH=arm64|x86_64|riscv64
=============================
A. Kernel Subsystem (CPU/SoC Identity & State)
File	Layer	Purpose
include/cpu_types.h	10_Arch/*/include/	Base types (cpu_u8_t…cpu_u64_t), arch enum, error codes, page/cache size constants for all 3 arches
include/cpu_features.h	10_Arch/*/include/	CPU capability bitmask (CPU_FEAT_FPU, CPU_FEAT_SVE, CPU_FEAT_AVX…), cpu_id_t struct, cpu_features_detect()
include/cpu_soc.h	10_Arch/*/include/	Master include — cpu_soc_desc_t SoC descriptor, cpu_soc_init() pipeline, 3 SoC presets
src/cpu_features.c	10_Arch/*/src/	Reads MIDR_EL1 (ARM64) / CPUID (x86-64) / MISA CSR (RISC-V), fills g_cpu_id, prints detected features
src/cpu_soc.c	10_Arch/*/src/	3 SoC descriptors (Cortex-A76 / x86-64 / RV64GC); early_init (UART + GDT/IDT) → late_init (cache + IRQ + IC + timer + SMP)
===================================================
B. Feature Implementation (Device Access)
File	Layer	Purpose
include/cpu_regs.h	10_Arch/*/include/	All register access in one header: ARM64 MRS/MSR + barriers + WFI; x86 CPUID/RDMSR/WRMSR/INB/OUTB; RISC-V CSR_READ/WRITE/SET/CLR + WFI; common MMIO read/write (8/16/32/64-bit) via #ifdef
include/cpu_cache.h	10_Arch/*/include/	Cache flush/clean/invalidate/DMA-sync API
src/cpu_cache.c	10_Arch/*/src/	ARM64: dc civac/ic ivau by VA + all-way set/way flush; x86: clflushopt+mfence/wbinvd; RISC-V: fence/fence.i
include/cpu_mmu.h	10_Arch/*/include/	MMU enable/disable, map/unmap, TLB flush all/by-VA, page-table get/set, ASID management, virt_to_phys()
src/cpu_mmu.c	10_Arch/*/src/	ARM64: SCTLR_EL1 M+C+I, AT S1E1R for V2P; x86: CR0.PG/CR3/CR4.PAE/EFER.NXE; RISC-V: SATP SV39 + sfence.vma
include/cpu_irq.h	10_Arch/*/include/	IRQ descriptor table (1024 entries), register/unregister/dispatch/EOI, trigger types, priority
src/cpu_irq.c	10_Arch/*/src/	g_irq_table[1024] init, dispatch by IRQ number, enable/disable/priority management
include/cpu_timer.h	10_Arch/*/include/	Timer init, set period, start/stop, tick count, udelay()/mdelay(), ns↔tick conversion
src/cpu_timer.c	10_Arch/*/src/	ARM64: CNTVCT_EL0; x86: RDTSC; RISC-V: time CSR; busy-wait udelay/mdelay
include/cpu_power.h	10_Arch/*/include/	cpu_idle() (WFI/HLT), cpu_halt(), cpu_core_on/off(), cpu_system_reset/off(), PSCI/SBI/ACPI constants
src/cpu_power.c	10_Arch/*/src/	ARM64: PSCI via SMC; RISC-V: SBI HSM/SRST ecall; x86: port 0x64 reset, QEMU ACPI off via port 0x604
include/cpu_smp.h	10_Arch/*/include/	Core info table (CPU_MAX_CORES=16), boot secondary, IPI send/broadcast, spinlock, all-core barrier
src/cpu_smp.c	10_Arch/*/src/	Core ID via MPIDR/APIC-ID/mhartid; boot via PSCI/LAPIC-SIPI/SBI; IPI via GIC SGI/LAPIC ICR/SBI; atomic spinlock
include/cpu_context.h	10_Arch/*/include/	Per-arch CPU context frame: ARM64 (X0-X30+SP+PC+PSTATE+ESR+FAR); x86 (all 16 GPRs+RIP+RFLAGS+SS+error); RISC-V (ra/sp/gp/tp/s/a/sepc/sstatus/scause)
src/cpu_context.c	10_Arch/*/src/	cpu_context_init() sets arch-specific entry/stack/arg; cpu_context_print() per-arch register dump
include/cpu_debug.h	10_Arch/*/include/	HW breakpoints + watchpoints, debug enable/disable, handler registration, cpu_debug_num_bp/wp()
src/cpu_debug.c	10_Arch/*/src/	ARM64: DBGBVR/DBGBCR/DBGWVR/DBGWCR via MSR, MDSCR_EL1 enable; x86: DR0-DR3 + DR7 encode; cpu_debug_num_bp() via ID_AA64DFR0_EL1
include/cpu_io.h	10_Arch/*/include/	MMIO region registry (32 regions), safe read/write, set/clr/mod_bits32, poll_set/clr32 with timeout, x86 port I/O
src/cpu_io.c	10_Arch/*/src/	Region table management, bit-field helpers, polling with cpu_timer_udelay, x86 inb/outb wrappers
============================================
C. Architecture Specific (10_Arch/arm64, 10_Arch/arm32, 10_Arch/x86_64)
File	Repo path	Purpose
include/arch/cortex_a76.h	10_Arch/arm64/include/	MIDR PartNum 0xD0B, L1/L2/L3 sizes, GIC-600 offsets, PL011 base, timer IRQs (27/30/33), SCTLR_EL1 bits, TCR_EL1 48-bit macros, MAIR attribute indices, PMU event numbers
src/arch/cortex_a76.c	10_Arch/arm64/src/	Errata 1463225 (DSB+ISB before ERET), cache enable (SCTLR M+C+I), MMU setup (MAIR+TCR+TTBR0+TTBR1), PMU init (PMCR_EL0 + PMCNTENSET_EL0), cortex_a76_print_info()
include/arch/x86_64_cpu.h	10_Arch/x86_64/include/	CPUID leaves, MSR addresses (EFER/STAR/LSTAR/FS_BASE/APIC_BASE), CR0/CR4/EFER/RFLAGS bits, GDT selectors, IDT vector numbers, LAPIC/IOAPIC offsets, x86_gdt_entry_t/x86_idt_gate_t/x86_tss_t packed structs
src/arch/x86_64_cpu.c	10_Arch/x86_64/src/	GDT init (null/code64/data64/user64/TSS), LGDT + long-mode CS reload, IDT init (LIDT), TSS init (RSP0), SSE enable (CR0.EM/MP + CR4.OSFXSR), AVX (XGETBV/XSETBV), paging init (PML4/CR3 + EFER.NXE + CR0.PG), SYSCALL (STAR/LSTAR/SFMASK), x86_read_tsc()
include/arch/riscv64.h	10_Arch/riscv64/include/	All CSR addresses, MSTATUS/MIE/MIP bits, SATP modes (SV39/SV48/SV57), MCAUSE exception codes, CLINT/PLIC/UART base addresses, SV39 PTE flag macros + riscv_pte() helper
src/arch/riscv64.c	10_Arch/riscv64/src/	Trap delegation (medeleg/mideleg), FP enable (sstatus FS=Initial + zero f0-f31), SV39 SATP paging, stvec setup, riscv64_get_hartid(), full ISA extension string (IMAFDCVHSU)
==============================================
20_DriverInterfaces — Hardware Abstraction Interface Layer
These headers define the interface contracts between the arch layer and device drivers:
File	Repo path	Hardware	Purpose
include/drivers/cpu_drv_gic.h	20_DriverInterfaces/include/	ARM GIC-600	Distributor + CPU interface + redistributor register offsets, gic_ctx_t, full enable/disable/priority/target/config/EOI/SGI API
include/drivers/cpu_drv_apic.h	20_DriverInterfaces/include/	x86 LAPIC + IOAPIC + 8259A	All LAPIC/IOAPIC register offsets, IPI delivery modes, apic_ctx_t, LAPIC init/EOI/IPI/SIPI/timer + IOAPIC redirection table + 8259A disable/init
include/drivers/cpu_drv_plic.h	20_DriverInterfaces/include/	RISC-V PLIC	Priority/enable/threshold/claim/complete offsets, S-mode context formula, plic_ctx_t
include/drivers/cpu_drv_timer.h	20_DriverInterfaces/include/	ARM CNTV / x86 LAPIC timer / RISC-V CLINT	Timer type enum, cpu_drv_timer_ctx_t, period set / start / stop / count / freq API
include/drivers/cpu_drv_uart.h	20_DriverInterfaces/include/	PL011 / 16550 / SiFive UART	All register offsets for 3 UART types, cpu_uart_ctx_t, init/putc/puts/getc/poll/printf API
===================================================================
30_DeviceDrivers — Device Driver Implementations
File	Repo path	Hardware	Purpose
src/drivers/cpu_drv_gic.c	30_DeviceDrivers/src/	ARM GIC-600	Distributor init (GICD_CTLR + IGROUPR all group-1 + IPRIORITYR=0xA0 + ITARGETSR=CPU0 + ICFGR=level), CPU interface init (GICC_PMR=0xF0 + GICC_CTLR=1), IRQ enable/disable/priority/target/config, ACK/EOI/SGI send
src/drivers/cpu_drv_apic.c	30_DeviceDrivers/src/	x86 LAPIC + IOAPIC + 8259A	LAPIC init (SVR enable, TPR=0, LDR/DFR flat), LAPIC write/read, INIT-IPI + SIPI, LAPIC timer (DCR/LVT/ICR), IOAPIC redirection table set/mask/unmask, 8259A ICW1-ICW4 init + EOI
src/drivers/cpu_drv_plic.c	30_DeviceDrivers/src/	RISC-V PLIC	Per-IRQ priority set, per-context enable bitmap read-modify-write, threshold register, claim/complete MMIO, S-mode context = hart×2+1
src/drivers/cpu_drv_timer.c	30_DeviceDrivers/src/	Generic Timer	ARM: CNTV_CVAL_EL0 deadline + CTL enable; x86: LAPIC ICR periodic/one-shot; RISC-V: mtimecmp MMIO write + sie.STIE; auto-rearm handler; cpu_timer_udelay-based calibration
src/drivers/cpu_drv_uart.c	30_DeviceDrivers/src/	PL011 / 16550 / SiFive	PL011: IBRD/FBRD baud + LCR_H 8N1 + CR TX+RX enable; 16550: DLAB divisor + LCR 8N1 + FCR FIFO; SiFive: TXCTRL/RXCTRL enable + DIV baud; TX/RX poll loops; uart_printf() via vsnprintf