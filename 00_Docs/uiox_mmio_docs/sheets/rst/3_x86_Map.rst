3_x86_Map
=========

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Peripheral
     - Block / Function
     - Base Address (MMIO or I/O)
     - End Address
     - Size / Range
     - IRQ (IOAPIC#)
     - Bus
     - Access Width
     - Clock Source
     - Source File
     - Notes
   * - xAPIC / LAPIC
     - Local APIC (per-CPU MMIO)
     - 0xFEE00000
     - 0xFEE00FFF
     - 0x1000
     - —
     - MMIO
     - 32
     - —
     - cpu_hw / DTB_x86
     - LAPIC_ID@0x20; LAPIC_VER@0x30; APIC timer@0x320; ICR@0x300; SVR@0xF0
   * - IOAPIC
     - I/O APIC
     - 0xFEC00000
     - 0xFEC00FFF
     - 0x1000
     - —
     - MMIO
     - 32
     - —
     - cpu_hw / DTB_x86
     - IOREGSEL@0x00; IOWIN@0x10; 24 IRQ lines; PCI INT A-D routed here
   * - HPET
     - High Precision Event Timer
     - 0xFED00000
     - 0xFED00FFF
     - 0x1000
     - IOAPIC 0
     - MMIO
     - 64
     - HPET OSC
     - cpu_hw / DTB_x86
     - Cap@0x00; Config@0x10; Timer0 Conf@0x100; uiox_cpu_rdtsc() for TSC
   * - TSC
     - Time Stamp Counter (MSR 0x10)
     - MSR 0x10
     - —
     - 64-bit
     - —
     - MSR
     - 64
     - Core clock
     - cpu_hw
     - uiox_cpu_rdtsc(); rdtsc asm; invariant TSC cap
   * - LAPIC Timer
     - APIC local timer
     - 0xFEE00320
     - 0xFEE00323
     - 0x4
     - —
     - MMIO
     - 32
     - CPU clock
     - cpu_hw
     - Initial count@0x380; CCR@0x390; DCR@0x3E0
   * - UART COM1 (16550A)
     - Serial console / debug
     - I/O 0x3F8
     - I/O 0x3FF
     - 8 ports
     - IOAPIC 4
     - I/O port
     - 8
     - 1.8432 MHz
     - cpu_hw / DTB_x86
     - 115200 n8; RBR/THR@0; IER@1; IIR@2; LCR@3; LSR@5
   * - UART COM2 (16550A)
     - Secondary serial
     - I/O 0x2F8
     - I/O 0x2FF
     - 8 ports
     - IOAPIC 3
     - I/O port
     - 8
     - 1.8432 MHz
     - DTB_x86
     - disabled
   * - PCIe ECAM
     - Enhanced Cfg Access Mechanism
     - 0xE0000000
     - 0xEFFFFFFF
     - 0x10000000
     - IOAPIC 16
     - PCIe
     - 32
     - —
     - DTB_x86
     - 256 buses × 256 fns; NVMe + AX200 Wi-Fi + TB4
   * - PCIe I/O window
     - Translated PCIe I/O
     - 0xD0000000
     - 0xD0FFFFFF
     - 0x1000000
     - —
     - PCIe
     - 32
     - —
     - DTB_x86
     - 
   * - PCIe MEM window
     - Translated PCIe memory
     - 0xA0000000
     - 0xCFFFFFFF
     - 0x30000000
     - —
     - PCIe
     - 32
     - —
     - DTB_x86
     - TB4 + Intel AX200 (slot 1 & 2)
   * - USB3 xHCI (OTG)
     - USB 3.2 Gen2×2 Host+OTG
     - 0xE8000000
     - 0xE800FFFF
     - 0x10000
     - IOAPIC 20
     - PCIe
     - 32
     - —
     - usb_hw / DTB_x86
     - FS/HS/SS/SS+; DMA rings; EP table; OTG; usb.md
   * - Crypto Engine
     - SHA-256/384, RSA, ECDSA
     - 0xE9000000
     - 0xE900FFFF
     - 0x10000
     - IOAPIC 22
     - MMIO
     - 32
     - —
     - ksign_img / DTB_x86
     - uiox,ksign-hw-accel; alg IDs match uiox_ks_alg_t
   * - TPM 2.0
     - Trusted Platform Module
     - 0xFED40000
     - 0xFED44FFF
     - 0x5000
     - IOAPIC 6
     - MMIO
     - 32
     - —
     - ksign_key / DTB_x86
     - tcg,tpm-tis-mmio; PCR bank for ksign attestation quotes
   * - Thermal (MSR)
     - CPU digital thermal sensor
     - I/O 0x40B
     - —
     - 1 port
     - —
     - MSR / I/O
     - 8
     - —
     - cpu_pm / DTB_x86
     - THERM_STATUS MSR 0x19C; Tj_max MSR 0x1A2; uiox_cpu_pm throttle
   * - LAPIC_ID
     - Local APIC ID register
     - 0xFEE00020
     - 0xFEE00023
     - 0x4
     - —
     - MMIO
     - 32
     - —
     - cpu_hw
     - Bits [31:24]=APIC_ID
   * - LAPIC_TPR
     - Task Priority Register
     - 0xFEE00080
     - 0xFEE00083
     - 0x4
     - —
     - MMIO
     - 32
     - —
     - cpu_hw
     - Bits [7:4]=priority class
   * - LAPIC_EOI
     - End of Interrupt
     - 0xFEE000B0
     - 0xFEE000B3
     - 0x4
     - —
     - MMIO
     - 32
     - —
     - cpu_hw
     - Write 0 to acknowledge IRQ
   * - LAPIC_SVR
     - Spurious Interrupt Vector
     - 0xFEE000F0
     - 0xFEE000F3
     - 0x4
     - —
     - MMIO
     - 32
     - —
     - cpu_hw
     - Bit 8=APIC enable; bits [7:0]=spurious vector
   * - LAPIC_ICR_LO
     - Interrupt Command Reg low
     - 0xFEE00300
     - 0xFEE00303
     - 0x4
     - —
     - MMIO
     - 32
     - —
     - cpu_hw
     - IPI send; ipi_send vtable in cpu_hw
   * - LAPIC_ICR_HI
     - Interrupt Command Reg high
     - 0xFEE00310
     - 0xFEE00313
     - 0x4
     - —
     - MMIO
     - 32
     - —
     - cpu_hw
     - Destination APIC ID for IPI
   * - LAPIC_LVT_TIMER
     - LVT Timer entry
     - 0xFEE00320
     - 0xFEE00323
     - 0x4
     - —
     - MMIO
     - 32
     - —
     - cpu_hw
     - Periodic/one-shot; vector bits [7:0]
   * - LAPIC_INIT_COUNT
     - Initial timer count
     - 0xFEE00380
     - 0xFEE00383
     - 0x4
     - —
     - MMIO
     - 32
     - —
     - cpu_hw
     - 
   * - LAPIC_CUR_COUNT
     - Current timer count
     - 0xFEE00390
     - 0xFEE00393
     - 0x4
     - —
     - MMIO
     - 32
     - —
     - cpu_hw
     - Read-only countdown
   * - LAPIC_DCR
     - Divide Config Register
     - 0xFEE003E0
     - 0xFEE003E3
     - 0x4
     - —
     - MMIO
     - 32
     - —
     - cpu_hw
     - Divider for LAPIC timer clock
   * - MSR_EFER
     - Extended Feature Enable
     - MSR 0xC0000080
     - —
     - 64-bit
     - —
     - MSR
     - 64
     - —
     - cpu_hw
     - SCE/LME/LMA/NXE; uiox_cpu_rdmsr()/wrmsr()
   * - MSR_FS_BASE
     - FS segment base (thread ptr)
     - MSR 0xC0000100
     - —
     - 64-bit
     - —
     - MSR
     - 64
     - —
     - cpu_hw
     - uiox thread-local storage
   * - MSR_GS_BASE
     - GS segment base (CPU local)
     - MSR 0xC0000101
     - —
     - 64-bit
     - —
     - MSR
     - 64
     - —
     - cpu_hw
     - Kernel per-CPU area
   * - MSR_TSC_AUX
     - TSC_AUX (RDTSCP aux)
     - MSR 0xC0000103
     - —
     - 64-bit
     - —
     - MSR
     - 64
     - —
     - cpu_hw
     - CPU ID in RDTSCP result
   * - MSR_APIC_BASE
     - APIC base address + enable
     - MSR 0x1B
     - —
     - 64-bit
     - —
     - MSR
     - 64
     - —
     - cpu_hw
     - Bit 11=APIC EN; bits [31:12]=base>>12
   * - MSR_PERF_CTL0
     - Perf counter control 0
     - MSR 0x186
     - —
     - 64-bit
     - —
     - MSR
     - 64
     - —
     - cpu_feat
     - UIOX_PMU_MAX_COUNTERS=8; IA32_PERFEVTSELx
   * - MSR_THERM_STATUS
     - Thermal Status
     - MSR 0x19C
     - —
     - 64-bit
     - —
     - MSR
     - 64
     - —
     - cpu_pm
     - Bit 0=Thermal Throttle; Bits[22:16]=temp reading
