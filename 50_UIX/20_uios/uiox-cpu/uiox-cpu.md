Application / Device Access API        (uiox_cpu_device)
  → CPU Subsystem: scheduler, power, frequency, topology (uiox_cpu_subsys)
    → CPU Features: cache, branch, perf counters, ISA     (uiox_cpu_feat)
    → Power Management: DVFS, idle, thermal, WFI/WFE      (uiox_cpu_pm)
    → Interface driver: core init, reset, MMU, exception   (uiox_cpu_if)
      → Hardware Abstraction: MMIO, registers, SMP, ISR    (uiox_cpu_hw)
    ↔ Buffer Manager: per-CPU data, IPI message pool       (uiox_cpu_buf)
============================================================================
uiox-cpu/
├── include/
│   ├── uiox_cpu_hw.h          # Layer 1  — HAL: MMIO, regs, SMP, ISR
│   ├── uiox_cpu_buf.h         # Per-CPU data + IPI message pool
│   ├── uiox_cpu_if.h          # Layer 2  — IF: core init, MMU, exception
│   ├── uiox_cpu_pm.h          # Power management: DVFS, idle, thermal
│   ├── uiox_cpu_feat.h        # Layer 3  — Features: cache, ISA, perf
│   ├── uiox_cpu_subsys.h      # Layer 4  — Subsystem: topology, scheduler
│   └── uiox_cpu_device.h      # Layer 5  — Application-facing API
└── src/
    ├── uiox_cpu_hw.c
    ├── uiox_cpu_buf.c
    ├── uiox_cpu_if.c
    ├── uiox_cpu_pm.c
    ├── uiox_cpu_feat.c
    ├── uiox_cpu_subsys.c
    ├── uiox_cpu_device.c
    └── uiox_cpu_demo.c
=================================================================
uiox-cpu/
├── include/
│   ├── uiox_cpu_hw.h      # Layer 1   — HAL: system register access
│   │                      #              (MRS/MSR ARM64, RDMSR/WRMSR x86,
│   │                      #              CSR read/write RISC-V),
│   │                      #              SoC MMIO, GIC/APIC/PLIC/CLINT,
│   │                      #              SMP spin-table, TSC/CNTPCT/mtime,
│   │                      #              hw_ops vtable, core descriptor
│   ├── uiox_cpu_buf.h     # Layer 1.5 — Per-CPU data block (cache-line
│   │                      #              padded, 64-byte aligned), IPI ring
│   │                      #              buffer (SPSC, 32-entry per src core),
│   │                      #              deferred work queue pool (64 entries)
│   ├── uiox_cpu_if.h      # Layer 2   — Interface: exception vector table
│   │                      #              init (VBAR_EL1 / IDT / mtvec),
│   │                      #              MMU page-table modes (4K-39/48,
│   │                      #              x86 PML4/PML5, RISC-V Sv39),
│   │                      #              architectural timer init, SMP boot
│   ├── uiox_cpu_pm.h      # Layer 2b  — Power management: OPP table (up to
│   │                      #              16 freq/voltage/power points), DVFS
│   │                      #              governors (performance/powersave/
│   │                      #              ondemand/schedutil/conservative),
│   │                      #              C-state idle (C0..C3), thermal
│   │                      #              throttle + critical shutdown
│   ├── uiox_cpu_feat.h    # Layer 3   — Feature detection: CPUID/MIDR/misa
│   │                      #              ISA extension flags (NEON/AVX/RVV/
│   │                      #              crypto/virtualisation), cache sizes,
│   │                      #              PMU counter start/stop/read (8 HW
│   │                      #              counters), topology (sockets/cores/
│   │                      #              threads), brand string
│   ├── uiox_cpu_subsys.h  # Layer 4   — Subsystem: SMP core up/down,
│   │                      #              IPI send + ring dispatch, deferred
│   │                      #              work execution, DVFS tick, thermal
│   │                      #              event, timer tick, event callbacks
│   └── uiox_cpu_device.h  # Layer 5   — Application API: open/start/stop/
│                          #              close/tick/core_up/core_down/
│                          #              send_ipi/set_governor/set_opp/
│                          #              read_temp/update_load/cache_flush/
│                          #              pmu_start/pmu_stop/pmu_read/
│                          #              freq/cycles/timestamp/uptime/
│                          #              core_state/print_info/print_stats
└── src/
    ├── uiox_cpu_hw.c      # HAL lifecycle: init/deinit/detect,
    │                      #   core_powerup/down, set_freq, read_temp,
    │                      #   cache_flush, ipi_send, timestamp
    ├── uiox_cpu_buf.c     # Per-CPU array init, IPI ring push/pop/empty,
    │                      #   work pool alloc/free/enqueue/dequeue,
    │                      #   uiox_this_cpu() arch-specific accessor
    ├── uiox_cpu_if.c      # Exception vector init (VBAR/IDT/mtvec),
    │                      #   MMU enable/disable (SCTLR/CR0/SATP),
    │                      #   architectural timer arm, SMP secondary boot
    ├── uiox_cpu_pm.c      # OPP table sorted insert, governor logic,
    │                      #   set_opp → freq + voltage transition,
    │                      #   idle WFI/HLT entry, thermal throttle DVFS tick
    ├── uiox_cpu_feat.c    # CPUID/MIDR/misa decode, ISA cap flags, cache
    │                      #   topology, PMU counter programme (PMSELR/
    │                      #   IA32_PERFEVTSEL/CSR), brand string extract
    ├── uiox_cpu_subsys.c  # Full init chain, start/stop, per-tick pipeline
    │                      #   (DVFS + thermal + IPI drain + work execute),
    │                      #   core_up/down, IPI push + HW send
    ├── uiox_cpu_device.c  # All API wrappers, arch/state/evt/gov name helpers,
    │                      #   print_info (OPP table + cache + caps),
    │                      #   print_stats (per-core freq/temp/load/state)
    └── uiox_cpu_demo.c    # 8-core SMP simulation, stub HAL for ARM64/x86/RV64,
                           #   CPUID detect, 5 OPPs, SMP core up/down, IPI,
                           #   PMU counters, load-based DVFS, governor switch,
                           #   cache flush, timestamp, statistics
=================================================================================
Key Design Decisions
Decision	Rationale
Architecture tri-target (ARM64 / x86-64 / RV64)	Single header-detected at compile time via __aarch64__ / __x86_64__ / __riscv; zero runtime overhead — architecture-specific intrinsics inlined directly
System register macros (MRS/RDMSR/CSR)	Direct __asm__ inline for zero-overhead register access; no function call overhead on the hot path (interrupt entry, cycle counter read)
Cache-line padded per-CPU data	__attribute__((aligned(64))) prevents false sharing between cores on the same cache line — critical for lock-free IPI ring performance on NUMA topologies
Lock-free SPSC IPI ring per core pair	One ring per (sender, receiver) pair — no lock needed; power-of-2 mask avoids modulo; overflow counter replaces blocking
OPP table sorted insert	Ascending frequency order enables O(1) governor decisions (index 0=min, last=max); voltage/power metadata supports energy-aware scheduling
Five DVFS governors	performance/powersave are simple index clamps; ondemand/schedutil use 80%/20% load thresholds; conservative uses 70%/30% for slower ramp-up
Thermal throttle in DVFS tick	Hard override to OPP[0] when temperature ≥ 95 °C regardless of governor; fires THERMAL_THROTTLE event to application
WFI/HLT/wfi for C1 idle	Architecture-portable idle: ARM64 wfi, x86 hlt, RISC-V wfi — all halt the pipeline and gate the clock until the next interrupt
MMU init in IF layer	TTBR0_EL1/CR3/SATP written in uiox_cpu_if_mmu_enable() — keeps page-table management in one place; application can swap page tables without touching HAL
Exception vector registration per fault type	Up to 16 fault handlers registered by enum index — uiox_cpu_if_register_fault() replaces the default handler; no vtable overhead
PMU programming per architecture	ARM64 uses PMSELR_EL0/PMXEVTYPER_EL0/PMCNTENSET_EL0; x86 uses IA32_PERFEVTSEL0/IA32_PMC0 MSRs; RISC-V uses minstret/mcycle CSRs — all behind the same pmu_start/read API
Deferred work queue pool (64 entries)	ISR-safe deferred execution without dynamic allocation; work items executed in uiox_cpu_subsys_tick() on the boot core
Vtable ops pattern	Concrete platform drivers (Raspberry Pi 5 GIC-600, Intel 12th-gen LAPIC, SiFive FU740 CLINT) plug in without modifying upper layers
=======================================
make clean
make PLATFORM=X86_64 BUILD=debug
==================
make PLATFORM=X86_64 BUILD=release
