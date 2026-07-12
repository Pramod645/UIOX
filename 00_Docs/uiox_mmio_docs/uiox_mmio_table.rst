.. UIOX MMIO Address Table
.. Generated from: https://github.com/Pramod645/UIOX (branch UIX00)
.. Date: 2026-07-12

UIOX MMIO Address Table
#######################

.. contents:: Table of Contents
   :depth: 2
   :local:

**Repository:** https://github.com/Pramod645/UIOX (branch UIX00)

**Generated:** 2026-07-12

**Architectures covered:** ARM64 (Cortex-A76) · x86-64 (SSE4.2/AVX-512) · RISC-V RV64GC (SV39/SV48)

----

1_Legend
========

.. list-table::
   :header-rows: 1
   :widths: auto

   * - UIOX MMIO Address Table
     - 
     - 
     - 
   * - Repository
     - https://github.com/Pramod645/UIOX (branch UIX00)
     - 
     - 
   * - Generated
     - 2026-07-12
     - 
     - 
   * - Version
     - 1.0.0
     - 
     - 

   * - Sheet
     - Contents
     - Primary Source
     - Architectures Covered
   * - 1_Legend
     - This sheet — coverage, legend, source index
     - —
     - All
   * - 2_ARM64_Map
     - ARM64 (Cortex-A76) full MMIO map + register offsets
     - uiox_cpu_hw.h / DTB
     - ARM64
   * - 3_x86_Map
     - x86-64 full MMIO / I-O port map + register offsets
     - uiox_cpu_hw.h / DTB
     - x86-64
   * - 4_RISCV_Map
     - RISC-V RV64GC full MMIO map + register offsets
     - uiox_cpu_hw.h / DTB
     - RISC-V
   * - 5_RegOffsets
     - Per-peripheral register offset tables (all archs)
     - usb.md / wifi.md / ksign
     - All
   * - 6_IRQ_Table
     - Interrupt number assignments (GIC SPI / PLIC / IOAPIC)
     - uiox_cpu_hw.h / DTB
     - All
   * - 7_Memory_Map
     - Physical memory layout, reserved regions, flash partitions
     - uiox_ksign_image.h
     - All
   * - 8_Syscalls
     - UIOX ksign syscall numbers (sys_kernel_verify etc.)
     - uiox_ksign_runtime.h
     - All

   * - Legend — Column meanings (sheets 2-4)
     - 
     - 
     - 
   * - Column
     - Meaning
     - 
     - 
   * - Peripheral
     - Hardware block name (e.g. GIC-600, USB3-xHCI)
     - 
     - 
   * - Base Address
     - Hex base address of the MMIO region
     - 
     - 
   * - End Address
     - Inclusive last byte of the region
     - 
     - 
   * - Size
     - Region size in bytes (hex)
     - 
     - 
   * - IRQ
     - Interrupt line (SPI/PPI/PLIC/IOAPIC number)
     - 
     - 
   * - Bus
     - AHB / APB / AXI / PCIe / I/O-port
     - 
     - 
   * - Access Width
     - 8 / 16 / 32 / 64-bit register access width
     - 
     - 
   * - Clock Source
     - Clock that gates this block
     - 
     - 
   * - Source File
     - UIOX repo file that defines this peripheral
     - 
     - 
   * - Notes
     - Functional notes, speed, sub-modes, cross-refs
     - 
     - 

   * - Source File Index
     - 
     - 
     - 
   * - Abbreviation
     - Full path in repo
     - 
     - 
   * - cpu_hw
     - 50_UIX/20_uios/uiox-cpu/include/uiox_cpu_hw.h
     - 
     - 
   * - cpu_pm
     - 50_UIX/20_uios/uiox-cpu/include/uiox_cpu_pm.h
     - 
     - 
   * - cpu_feat
     - 50_UIX/20_uios/uiox-cpu/include/uiox_cpu_feat.h
     - 
     - 
   * - cpu_subsys
     - 50_UIX/20_uios/uiox-cpu/include/uiox_cpu_subsys.h
     - 
     - 
   * - usb_hw
     - 50_UIX/20_uios/usb_/include/uiox_usb_hw.h
     - 
     - 
   * - wifi_hw
     - 50_UIX/20_uios/wifi_/include/uiox_wifi_hw.h
     - 
     - 
   * - tb4_hw
     - 50_UIX/20_uios/tb4_/include/uiox_tb4_hw.h
     - 
     - 
   * - therm_hw
     - 50_UIX/20_uios/thermal_/include/uiox_thermal_hw.h
     - 
     - 
   * - ksign_img
     - 50_UIX/12_ksign/include/uiox_ksign_image.h
     - 
     - 
   * - ksign_rt
     - 50_UIX/12_ksign/include/uiox_ksign_runtime.h
     - 
     - 
   * - ksign_key
     - 50_UIX/12_ksign/include/uiox_ksign_key.h
     - 
     - 
   * - ksign_meas
     - 50_UIX/12_ksign/include/uiox_ksign_measure.h
     - 
     - 
   * - DTB_arm64
     - 50_UIX/dts/uiox-arm64.dts (generated)
     - 
     - 
   * - DTB_x86
     - 50_UIX/dts/uiox-x86_64.dts (generated)
     - 
     - 
   * - DTB_rv
     - 50_UIX/dts/uiox-riscv64.dts (generated)
     - 
     - 

----

2_ARM64_Map
===========

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Peripheral
     - Block / Function
     - Base Address
     - End Address
     - Size (hex)
     - IRQ (GIC SPI#)
     - Bus
     - Access Width
     - Clock Source
     - Source File
     - Notes
   * - GIC-600 GICD
     - Global Interrupt Distributor
     - 0xFE000000
     - 0xFE00FFFF
     - 0x10000
     - —
     - AXI
     - 32
     - —
     - cpu_hw / DTB_arm64
     - GIC-v3; 256 SPI lines; IRQ enable/pend/prio/route regs
   * - GIC-600 GICR0
     - Redistributor — CPU0
     - 0xFE020000
     - 0xFE03FFFF
     - 0x20000
     - PPI 9
     - AXI
     - 32
     - —
     - cpu_hw / DTB_arm64
     - Per-core redistr; LPI pending table; 128 KiB each
   * - GIC-600 GICR1
     - Redistributor — CPU1
     - 0xFE040000
     - 0xFE05FFFF
     - 0x20000
     - PPI 9
     - AXI
     - 32
     - —
     - cpu_hw / DTB_arm64
     - 
   * - GIC-600 GICR2
     - Redistributor — CPU2
     - 0xFE060000
     - 0xFE07FFFF
     - 0x20000
     - PPI 9
     - AXI
     - 32
     - —
     - cpu_hw / DTB_arm64
     - 
   * - GIC-600 GICR3
     - Redistributor — CPU3
     - 0xFE080000
     - 0xFE09FFFF
     - 0x20000
     - PPI 9
     - AXI
     - 32
     - —
     - cpu_hw / DTB_arm64
     - 
   * - ARM Generic Timer
     - CNTPCT_EL0 (virtual)
     - —
     - —
     - —
     - PPI 10/11/13/14
     - —
     - 64 (sysreg)
     - osc24M (24 MHz)
     - cpu_hw
     - timer_freq_hz=24000000; uiox_cpu_read_sysreg(cntpct_el0)
   * - SP804 / UIOX Timer
     - System timer MMIO fallback
     - 0xFC010000
     - 0xFC010FFF
     - 0x1000
     - SPI 36
     - APB
     - 32
     - apb_clk (100 MHz)
     - cpu_hw / DTB_arm64
     - Dual-channel countdown; timer_base field in uiox_cpu_hw_t
   * - UART0 (PL011)
     - Primary serial console
     - 0xFC020000
     - 0xFC020FFF
     - 0x1000
     - SPI 37
     - APB
     - 32
     - apb_clk
     - cpu_hw / DTB_arm64
     - 115200 n8; stdout-path; uiox_fw_printf target
   * - UART1 (PL011)
     - Secondary UART
     - 0xFC021000
     - 0xFC021FFF
     - 0x1000
     - SPI 38
     - APB
     - 32
     - apb_clk
     - cpu_hw / DTB_arm64
     - disabled by default
   * - I2C0
     - Fast-mode 400 kHz; thermal sensor
     - 0xFC030000
     - 0xFC030FFF
     - 0x1000
     - SPI 40
     - APB
     - 32
     - apb_clk
     - therm_hw / DTB_arm64
     - Thermal sensor @ 0x48; uiox_cpu_pm.h thermal throttle
   * - I2C1
     - Fast-mode 400 kHz; expansion
     - 0xFC031000
     - 0xFC031FFF
     - 0x1000
     - SPI 41
     - APB
     - 32
     - apb_clk
     - DTB_arm64
     - 
   * - SPI0 (QSPI Master)
     - Wi-Fi SPI + NOR flash
     - 0xFC040000
     - 0xFC040FFF
     - 0x1000
     - SPI 42
     - APB
     - 32
     - apb_clk
     - wifi_hw / DTB_arm64
     - CS0: ESP8266 Wi-Fi @ 40 MHz; wifi.md SPI vtable
   * - SDMMC0 (SDIO)
     - CYW43xx Wi-Fi SDIO
     - 0xFC050000
     - 0xFC05FFFF
     - 0x10000
     - SPI 43
     - AHB
     - 32
     - ahb_clk (200 MHz)
     - wifi_hw / DTB_arm64
     - 4-bit SDIO; CYW43xx real SDIO vtable (wifi.md)
   * - SDMMC1 (eMMC)
     - Boot storage
     - 0xFC060000
     - 0xFC06FFFF
     - 0x10000
     - SPI 44
     - AHB
     - 32
     - ahb_clk
     - DTB_arm64
     - 8-bit HS200 1.8V; non-removable
   * - GPIO0
     - 32-bit GPIO bank 0 (pins 0–31)
     - 0xFC070000
     - 0xFC070FFF
     - 0x1000
     - SPI 50
     - APB
     - 32
     - apb_clk
     - DTB_arm64
     - Interrupt-capable; gpio-ranges 0–31
   * - GPIO1
     - 32-bit GPIO bank 1 (pins 32–63)
     - 0xFC071000
     - 0xFC071FFF
     - 0x1000
     - SPI 51
     - APB
     - 32
     - apb_clk
     - DTB_arm64
     - Interrupt-capable; gpio-ranges 32–63
   * - Watchdog (SP805)
     - Hardware watchdog
     - 0xFC080000
     - 0xFC080FFF
     - 0x1000
     - SPI 53
     - APB
     - 32
     - apb_clk
     - DTB_arm64
     - 30 s timeout; arm,sp805 compat
   * - RTC
     - Real-time clock
     - 0xFC090000
     - 0xFC090FFF
     - 0x1000
     - SPI 54
     - APB
     - 32
     - osc24M
     - DTB_arm64
     - uiox,uiox-rtc
   * - USB3 xHCI (OTG)
     - USB 3.2 Gen2×2 Host+OTG
     - 0xFC100000
     - 0xFC10FFFF
     - 0x10000
     - SPI 60
     - AHB
     - 32
     - ahb_clk
     - usb_hw / DTB_arm64
     - FS/HS/SS/SS+; DMA TX/RX rings; EP table; VBUS/OTG; usb.md
   * - USB2 EHCI
     - USB 2.0 Host
     - 0xFC110000
     - 0xFC11FFFF
     - 0x10000
     - SPI 61
     - AHB
     - 32
     - ahb_clk
     - usb_hw / DTB_arm64
     - FS/HS only; generic-ehci
   * - SPI NOR Flash
     - KRL + measurement log NVRAM
     - 0xFC310000
     - 0xFC310FFF
     - 0x1000
     - SPI 45
     - APB
     - 32
     - apb_clk
     - ksign_key / ksign_meas / DTB_arm64
     - 50 MHz; partitions: fw@0, krl@80000, mlog@90000, uboot@A0000
   * - Crypto Engine
     - SHA-256/384, RSA-2048/4096, ECDSA-P256, Ed25519
     - 0xFC200000
     - 0xFC20FFFF
     - 0x10000
     - SPI 70
     - AHB
     - 32
     - ahb_clk
     - ksign_img / ksign_key / DTB_arm64
     - uiox,ksign-hw-accel; alg IDs 1-4 in uiox_ks_alg_t
   * - OTP / eFuse
     - Root CA public key storage
     - 0xFC300000
     - 0xFC300FFF
     - 0x1000
     - —
     - APB
     - 32
     - —
     - ksign_key / DTB_arm64
     - Root CA @ offset 0x100, 256 B; burned at mfg; uiox_ksign_key.h
   * - PCIe DBI
     - PCIe host DBI config
     - 0xFD000000
     - 0xFD3FFFFF
     - 0x400000
     - SPI 64
     - AXI
     - 32
     - ahb_clk
     - DTB_arm64
     - DesignWare PCIe; 4-lane Gen4 for TB4
   * - PCIe ATU
     - PCIe iATU region
     - 0xFD400000
     - 0xFD7FFFFF
     - 0x400000
     - SPI 65
     - AXI
     - 32
     - ahb_clk
     - DTB_arm64
     - MSI + INTA; TB4 controller + Intel AX200 Wi-Fi
   * - PCIe I/O window
     - PCIe I/O translated
     - 0xFD800000
     - 0xFD8FFFFF
     - 0x100000
     - —
     - PCIe
     - 32
     - —
     - DTB_arm64
     - 
   * - PCIe MEM window
     - PCIe memory mapped
     - 0xC0000000
     - 0xDFFFFFFF
     - 0x20000000
     - —
     - PCIe
     - 32
     - —
     - DTB_arm64
     - TB4 + AX200; 512 MiB outbound window
   * - GIC GICD_CTLR
     - GIC Distributor Control
     - 0xFE000000
     - 0xFE000003
     - 0x4
     - —
     - AXI
     - 32
     - —
     - cpu_hw
     - Enable grp0/grp1; ARE_S/ARE_NS bits
   * - GIC GICD_IGROUPR0
     - IRQ Group 0 (SPIs 0-31)
     - 0xFE000080
     - 0xFE000083
     - 0x4
     - —
     - AXI
     - 32
     - —
     - cpu_hw
     - 1=Group1(NS); 0=Group0(S)
   * - GIC GICD_ISENABLER0
     - SPI Enable Set 0
     - 0xFE000100
     - 0xFE000103
     - 0x4
     - —
     - AXI
     - 32
     - —
     - cpu_hw
     - Set bit n → enable SPI n
   * - GIC GICD_ICENABLER0
     - SPI Enable Clear 0
     - 0xFE000180
     - 0xFE000183
     - 0x4
     - —
     - AXI
     - 32
     - —
     - cpu_hw
     - Set bit n → disable SPI n
   * - GIC GICD_IPRIORITYR0
     - Priority regs base
     - 0xFE000400
     - 0xFE0007FF
     - 0x400
     - —
     - AXI
     - 32
     - —
     - cpu_hw
     - 8 priorities per reg; lower=higher priority
   * - GIC GICD_ITARGETSR0
     - Target CPU base (v2)
     - 0xFE000800
     - 0xFE000BFF
     - 0x400
     - —
     - AXI
     - 32
     - —
     - cpu_hw
     - GICv3: use IROUTER instead
   * - GIC GICD_IROUTER0
     - Route SPI to affinity
     - 0xFE006000
     - 0xFE007FFF
     - 0x2000
     - —
     - AXI
     - 64
     - —
     - cpu_hw
     - 64-bit per SPI; GICv3 affinity routing
   * - PMU
     - Hardware perf counters
     - —
     - —
     - —
     - PPI 7
     - —
     - 64 (sysreg)
     - —
     - cpu_feat
     - UIOX_PMU_MAX_COUNTERS=8; arm,cortex-a76-pmu
   * - SMP Spin-Table
     - Secondary core release addr
     - 0x40010000
     - 0x4001001F
     - 0x20
     - —
     - —
     - 64
     - —
     - cpu_hw
     - smp_mbox_base; cpu-release-addr per core @0,8,10,18

----

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

----

4_RISCV_Map
===========

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Peripheral
     - Block / Function
     - Base Address
     - End Address
     - Size (hex)
     - IRQ (PLIC source#)
     - Bus
     - Access Width
     - Clock Source
     - Source File
     - Notes
   * - CLINT
     - Core-Local Interruptor
     - 0x02000000
     - 0x0200FFFF
     - 0x10000
     - —
     - AXI
     - 64
     - —
     - cpu_hw / DTB_rv
     - clint_base field; mtime@0xBFF8; mtimecmp0@0x4000; IPI msip@0x0
   * - CLINT msip0
     - Machine SW IRQ hart 0
     - 0x02000000
     - 0x02000003
     - 0x4
     - —
     - AXI
     - 32
     - —
     - cpu_hw
     - Write 1→trigger MIP.MSIP (software IPI); ipi_send vtable
   * - CLINT msip1
     - Machine SW IRQ hart 1
     - 0x02000004
     - 0x02000007
     - 0x4
     - —
     - AXI
     - 32
     - —
     - cpu_hw
     - 
   * - CLINT msip2
     - Machine SW IRQ hart 2
     - 0x02000008
     - 0x0200000B
     - 0x4
     - —
     - AXI
     - 32
     - —
     - cpu_hw
     - 
   * - CLINT msip3
     - Machine SW IRQ hart 3
     - 0x0200000C
     - 0x0200000F
     - 0x4
     - —
     - AXI
     - 32
     - —
     - cpu_hw
     - 
   * - CLINT mtimecmp0
     - Machine timer compare h0
     - 0x02004000
     - 0x02004007
     - 0x8
     - —
     - AXI
     - 64
     - —
     - cpu_hw
     - isr_timer vtable fires when mtime >= mtimecmp
   * - CLINT mtimecmp1
     - Machine timer compare h1
     - 0x02004008
     - 0x0200400F
     - 0x8
     - —
     - AXI
     - 64
     - —
     - cpu_hw
     - 
   * - CLINT mtimecmp2
     - Machine timer compare h2
     - 0x02004010
     - 0x02004017
     - 0x8
     - —
     - AXI
     - 64
     - —
     - cpu_hw
     - 
   * - CLINT mtimecmp3
     - Machine timer compare h3
     - 0x02004018
     - 0x0200401F
     - 0x8
     - —
     - AXI
     - 64
     - —
     - cpu_hw
     - 
   * - CLINT mtime
     - Global machine timer
     - 0x0200BFF8
     - 0x0200BFFF
     - 0x8
     - —
     - AXI
     - 64
     - 1 MHz ref
     - cpu_hw
     - timebase-frequency=1000000; uiox_cpu_csr_read(mtime)
   * - PLIC
     - Platform-Level Interrupt Ctrl
     - 0x0C000000
     - 0x0FFFFFFF
     - 0x4000000
     - —
     - AXI
     - 32
     - —
     - cpu_hw / DTB_rv
     - gic_base field used for PLIC on RV64; 64 sources; 4 harts × 2 ctxs
   * - PLIC Priority base
     - Source priority regs
     - 0x0C000000
     - 0x0C0000FF
     - 0x100
     - —
     - AXI
     - 32
     - —
     - cpu_hw
     - 4 B per source; 0=disabled; 1..7 priority
   * - PLIC Pending base
     - Pending bits
     - 0x0C001000
     - 0x0C00107F
     - 0x80
     - —
     - AXI
     - 32
     - —
     - cpu_hw
     - Read-only; bit n=source n pending
   * - PLIC Enable ctx0 (M)
     - Hart0 M-mode enables
     - 0x0C002000
     - 0x0C00207F
     - 0x80
     - —
     - AXI
     - 32
     - —
     - cpu_hw
     - Bit n=enable source n for this ctx
   * - PLIC Enable ctx1 (S)
     - Hart0 S-mode enables
     - 0x0C002080
     - 0x0C0020FF
     - 0x80
     - —
     - AXI
     - 32
     - —
     - cpu_hw
     - 
   * - PLIC Threshold ctx0
     - Hart0 M threshold
     - 0x0C200000
     - 0x0C200003
     - 0x4
     - —
     - AXI
     - 32
     - —
     - cpu_hw
     - 
   * - PLIC Claim/Complete ctx0
     - Hart0 M claim/cmp
     - 0x0C200004
     - 0x0C200007
     - 0x4
     - —
     - AXI
     - 32
     - —
     - cpu_hw
     - Read=highest-prio IRQ; Write=complete
   * - UART0 (SiFive)
     - Primary console
     - 0x10010000
     - 0x10010FFF
     - 0x1000
     - PLIC 4
     - APB
     - 32
     - periph_clk (125 MHz)
     - DTB_rv
     - 115200 n8; txdata@0; rxdata@4; txctrl@8; rxctrl@C; ie@10; ip@14; div@18
   * - UART1 (SiFive)
     - Secondary
     - 0x10011000
     - 0x10011FFF
     - 0x1000
     - PLIC 5
     - APB
     - 32
     - periph_clk
     - DTB_rv
     - disabled
   * - SPI0 / QSPI
     - QSPI NOR flash + Wi-Fi SPI
     - 0x10040000
     - 0x10040FFF
     - 0x1000
     - PLIC 51
     - APB
     - 32
     - periph_clk
     - wifi_hw / DTB_rv
     - CS0: SPI NOR (KRL/mlog); sifive,spi0 compat; 50 MHz max
   * - SDMMC0
     - eMMC / SD + CYW43xx SDIO
     - 0x10050000
     - 0x1005FFFF
     - 0x10000
     - PLIC 52
     - AHB
     - 32
     - bus_clk (500 MHz)
     - wifi_hw / DTB_rv
     - 4-bit SDIO IRQ; cap-sdio-irq; CYW43xx vtable
   * - I2C0
     - 400 kHz; thermal sensor
     - 0x10060000
     - 0x10060FFF
     - 0x1000
     - PLIC 53
     - APB
     - 32
     - periph_clk
     - therm_hw / DTB_rv
     - Sensor @ 0x48; sifive,i2c0
   * - GPIO0
     - 16-bit GPIO + IRQ
     - 0x10070000
     - 0x10070FFF
     - 0x1000
     - PLIC 7–22
     - APB
     - 32
     - periph_clk
     - DTB_rv
     - sifive,gpio0; 16 GPIO lines; IRQ per pin
   * - Watchdog
     - HW watchdog
     - 0x10080000
     - 0x10080FFF
     - 0x1000
     - PLIC 61
     - APB
     - 32
     - periph_clk
     - DTB_rv
     - sifive,wdog0; 30 s timeout
   * - RTC
     - Real-time clock
     - 0x10090000
     - 0x10090FFF
     - 0x1000
     - PLIC 62
     - APB
     - 32
     - periph_clk
     - DTB_rv
     - uiox,uiox-rtc
   * - USB3 xHCI (OTG)
     - USB 3.0 Host+OTG
     - 0x10100000
     - 0x1010FFFF
     - 0x10000
     - PLIC 54
     - AHB
     - 32
     - bus_clk
     - usb_hw / DTB_rv
     - FS/HS/SS; DMA TX/RX rings; EP table; VBUS; usb.md
   * - Crypto Engine
     - SHA-256/384, RSA, ECDSA
     - 0x10200000
     - 0x1020FFFF
     - 0x10000
     - PLIC 60
     - AHB
     - 32
     - bus_clk
     - ksign_img / DTB_rv
     - ksign-hw-accel flag; alg IDs 1-4 (uiox_ks_alg_t)
   * - PCIe Host
     - PCIe 3.0 ×4; AX200 + TB4
     - 0x30000000
     - 0x3FFFFFFF
     - 0x10000000
     - PLIC 56,57
     - AXI
     - 32
     - —
     - DTB_rv
     - sifive,fu740-pcie; I/O@0x60000000; MEM@0x61000000
   * - PCIe I/O window
     - Translated PCIe I/O
     - 0x60000000
     - 0x60FFFFFF
     - 0x1000000
     - —
     - PCIe
     - 32
     - —
     - DTB_rv
     - 
   * - PCIe MEM window
     - Translated PCIe memory
     - 0x61000000
     - 0x7FFFFFFF
     - 0x1F000000
     - —
     - PCIe
     - 32
     - —
     - DTB_rv
     - TB4 + Intel AX200
   * - CSR mstatus
     - Machine status
     - CSR 0x300
     - —
     - 64-bit
     - —
     - CSR
     - 64
     - —
     - cpu_hw
     - MIE/MPIE/MPP bits; uiox_cpu_csr_read/write
   * - CSR mie
     - Machine interrupt enable
     - CSR 0x304
     - —
     - 64-bit
     - —
     - CSR
     - 64
     - —
     - cpu_hw
     - MSIE/MTIE/MEIE bits; isr_timer/isr_ipi hooks
   * - CSR mip
     - Machine interrupt pending
     - CSR 0x344
     - —
     - 64-bit
     - —
     - CSR
     - 64
     - —
     - cpu_hw
     - Read: pending sources
   * - CSR mhartid
     - Hardware thread ID
     - CSR 0xF14
     - —
     - 64-bit
     - —
     - CSR
     - 64
     - —
     - cpu_hw
     - uiox_cpu_this_id() = csr_read(mhartid)&0xFF
   * - CSR mcycle
     - Machine cycle counter
     - CSR 0xB00
     - —
     - 64-bit
     - —
     - CSR
     - 64
     - —
     - cpu_feat
     - PMU event; uiox_cpu_hw_timestamp()
   * - CSR minstret
     - Machine instr-retired counter
     - CSR 0xB02
     - —
     - 64-bit
     - —
     - CSR
     - 64
     - —
     - cpu_feat
     - UIOX_PMU_MAX_COUNTERS=8 includes this
   * - CSR satp
     - Supervisor Addr Trans. & Prot.
     - CSR 0x180
     - —
     - 64-bit
     - —
     - CSR
     - 64
     - —
     - cpu_hw
     - MODE=8 → SV39; MODE=9 → SV48; ASID + PPN

----

5_RegOffsets
============

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Peripheral (ARM64 base)
     - Register Name
     - Offset (hex)
     - Width
     - Access
     - Reset Value
     - Description
     - Source File
   * - UART0 PL011 @ 0xFC020000
     - UARTDR
     - 0x000
     - 32
     - R/W
     - 0x0000
     - Data Register: [10:8]=error flags; [7:0]=data
     - usb_hw/cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTRSR
     - 0x004
     - 32
     - R/W
     - 0x0000
     - Receive Status/Error Clear
     - cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTFR
     - 0x018
     - 32
     - R
     - 0x0090
     - Flag Reg: [7]=TXFE [6]=RXFF [5]=TXFF [4]=RXFE [3]=BUSY
     - cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTIBRD
     - 0x024
     - 32
     - R/W
     - 0x0000
     - Integer Baud Rate Divisor
     - cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTFBRD
     - 0x028
     - 32
     - R/W
     - 0x0000
     - Fractional Baud Rate Divisor
     - cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTLCR_H
     - 0x02C
     - 32
     - R/W
     - 0x0000
     - Line Ctrl: [6:5]=WLEN [4]=FEN [1]=PEN
     - cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTCR
     - 0x030
     - 32
     - R/W
     - 0x0300
     - Ctrl: [15]=CTSEN [14]=RTSEN [9]=RXE [8]=TXE [0]=UARTEN
     - cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTIMSC
     - 0x038
     - 32
     - R/W
     - 0x0000
     - Interrupt Mask Set/Clear
     - cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTMIS
     - 0x040
     - 32
     - R
     - 0x0000
     - Masked Interrupt Status
     - cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTICR
     - 0x044
     - 32
     - W
     - —
     - Interrupt Clear Register
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_CTLR
     - 0x000
     - 32
     - R/W
     - 0x0
     - Distributor Ctrl; ARE_S/NS bits
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_TYPER
     - 0x004
     - 32
     - R
     - —
     - Interrupt type; ITLinesNumber field
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_IGROUPR0
     - 0x080
     - 32
     - R/W
     - 0x0
     - IRQ Group 0 (SPIs 0-31)
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_ISENABLER0
     - 0x100
     - 32
     - W
     - —
     - Enable Set; bit n=SPI n
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_ICENABLER0
     - 0x180
     - 32
     - W
     - —
     - Enable Clear
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_ISPENDR0
     - 0x200
     - 32
     - W
     - —
     - Pending Set
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_ICPENDR0
     - 0x280
     - 32
     - W
     - —
     - Pending Clear
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_IPRIORITYR0
     - 0x400
     - 32
     - R/W
     - 0x0
     - Priority (8b/SPI)
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_IROUTER0
     - 0x6000
     - 64
     - R/W
     - 0x0
     - Affinity routing (GICv3)
     - cpu_hw
   * - SP804 Timer @ 0xFC010000
     - TIMER1LOAD
     - 0x000
     - 32
     - R/W
     - 0x0
     - Load value; reloaded on wrap
     - cpu_hw
   * - SP804 Timer @ 0xFC010000
     - TIMER1VALUE
     - 0x004
     - 32
     - R
     - —
     - Current counter value
     - cpu_hw
   * - SP804 Timer @ 0xFC010000
     - TIMER1CTRL
     - 0x008
     - 32
     - R/W
     - 0x20
     - [7]=En [6]=Periodic [5]=IntEn [3]=Size [2:1]=Pre [0]=Wrap
     - cpu_hw
   * - SP804 Timer @ 0xFC010000
     - TIMER1INTCLR
     - 0x00C
     - 32
     - W
     - —
     - Interrupt clear (write any)
     - cpu_hw
   * - SP804 Timer @ 0xFC010000
     - TIMER1MIS
     - 0x014
     - 32
     - R
     - 0x0
     - Masked interrupt status
     - cpu_hw
   * - USB3 xHCI @ 0xFC100000
     - CAPLENGTH
     - 0x000
     - 8
     - R
     - —
     - Capability regs length
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - HCIVERSION
     - 0x002
     - 16
     - R
     - —
     - HCI version (0x0110=USB 3.1)
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - HCSPARAMS1
     - 0x004
     - 32
     - R
     - —
     - [31:24]=MaxPorts [16:8]=MaxIntrs [7:0]=MaxSlots
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - HCSPARAMS2
     - 0x008
     - 32
     - R
     - —
     - [7:4]=IST [11:8]=ERST Max
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - HCCPARAMS1
     - 0x010
     - 32
     - R
     - —
     - [0]=AC64 [1]=BNC [2]=CSZ [3]=PPC
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - USBCMD
     - 0x020
     - 32
     - R/W
     - 0x0
     - [0]=RS [1]=HCRST [2]=INTE [3]=HSEE
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - USBSTS
     - 0x024
     - 32
     - R/W
     - 0x0
     - [0]=HCH [2]=HSE [3]=EINT [4]=PCD
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - CRCR
     - 0x038
     - 64
     - R/W
     - 0x0
     - Command Ring Control (DMA addr + RCS)
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - DCBAAP
     - 0x050
     - 64
     - R/W
     - 0x0
     - Device Context Base Addr Array Ptr
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - CONFIG
     - 0x058
     - 32
     - R/W
     - 0x0
     - [7:0]=MaxSlotsEn
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - PORTSC(n)
     - 0x420+n*0x10
     - 32
     - R/W
     - —
     - Port n status/ctrl; CCS/PED/PR/PLS/PP/PIC/LWS
     - usb_hw
   * - Crypto @ 0xFC200000
     - CTRL
     - 0x000
     - 32
     - R/W
     - 0x0
     - [3:0]=ALG_SEL: 1=RSA2048,2=RSA4096,3=ECDSA,4=ED25519; [4]=START; [5]=INT_EN
     - ksign_img
   * - Crypto @ 0xFC200000
     - STATUS
     - 0x004
     - 32
     - R
     - 0x0
     - [0]=BUSY [1]=DONE [2]=ERR
     - ksign_img
   * - Crypto @ 0xFC200000
     - SHA_IN
     - 0x010
     - 32
     - W
     - —
     - SHA-256/384 input data FIFO
     - ksign_img
   * - Crypto @ 0xFC200000
     - SHA_LEN
     - 0x014
     - 32
     - R/W
     - 0x0
     - Message byte length for final SHA block
     - ksign_img
   * - Crypto @ 0xFC200000
     - SHA_DIGEST
     - 0x020
     - 32
     - R
     - —
     - SHA digest output [0..7]=SHA-256; [0..11]=SHA-384
     - ksign_img
   * - Crypto @ 0xFC200000
     - RSA_MOD
     - 0x100
     - 32
     - W
     - —
     - RSA modulus input (256B/512B for 2048/4096)
     - ksign_img
   * - Crypto @ 0xFC200000
     - RSA_EXP
     - 0x200
     - 32
     - W
     - —
     - RSA exponent input
     - ksign_img
   * - Crypto @ 0xFC200000
     - RSA_SIG
     - 0x300
     - 32
     - W
     - —
     - RSA signature input
     - ksign_img
   * - Crypto @ 0xFC200000
     - RSA_OUT
     - 0x400
     - 32
     - R
     - —
     - RSA result (PKCS#1 v1.5 padded plaintext)
     - ksign_img
   * - Crypto @ 0xFC200000
     - EC_CURVE
     - 0x500
     - 32
     - R/W
     - 0x3
     - 0=none 3=P-256 4=Ed25519
     - ksign_img
   * - CLINT @ 0x02000000
     - msip[0]
     - 0x0000
     - 32
     - R/W
     - 0x0
     - hart0 SW IRQ; write 1=pend MIP.MSIP
     - cpu_hw
   * - CLINT @ 0x02000000
     - msip[1]
     - 0x0004
     - 32
     - R/W
     - 0x0
     - hart1 SW IRQ
     - cpu_hw
   * - CLINT @ 0x02000000
     - mtimecmp[0]
     - 0x4000
     - 64
     - R/W
     - 0xFFFFFFFFFFFFFFFF
     - hart0 timer compare
     - cpu_hw
   * - CLINT @ 0x02000000
     - mtimecmp[1]
     - 0x4008
     - 64
     - R/W
     - 0xFFFFFFFFFFFFFFFF
     - hart1 timer compare
     - cpu_hw
   * - CLINT @ 0x02000000
     - mtime
     - 0xBFF8
     - 64
     - R/W
     - 0x0
     - Global monotonic timer
     - cpu_hw
   * - OTP @ 0xFC300000
     - CTRL
     - 0x000
     - 32
     - R/W
     - 0x0
     - [0]=PROG_EN [1]=READ_EN [2]=LOCK
     - ksign_key
   * - OTP @ 0xFC300000
     - ADDR
     - 0x004
     - 32
     - R/W
     - 0x0
     - Word address (byte addr / 4)
     - ksign_key
   * - OTP @ 0xFC300000
     - WDATA
     - 0x008
     - 32
     - W
     - —
     - Write data (one 32-bit word)
     - ksign_key
   * - OTP @ 0xFC300000
     - RDATA
     - 0x00C
     - 32
     - R
     - —
     - Read data
     - ksign_key
   * - OTP @ 0xFC300000
     - STATUS
     - 0x010
     - 32
     - R
     - 0x0
     - [0]=BUSY [1]=FAIL [2]=LOCKED
     - ksign_key
   * - OTP @ 0xFC300000
     - ROOT_KEY_BASE
     - 0x100
     - —
     - R
     - —
     - ksign Root CA pubkey (256 B = 64 words)
     - ksign_key

----

6_IRQ_Table
===========

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Arch
     - Controller
     - IRQ Type
     - Number
     - Peripheral
     - Trigger
     - Priority (default)
     - Source File
     - Notes
   * - ARM64
     - GIC-600
     - PPI
     - 7
     - PMU (Cortex-A76)
     - Level
     - —
     - cpu_feat
     - arm,cortex-a76-pmu; UIOX_PMU_MAX_COUNTERS=8
   * - ARM64
     - GIC-600
     - PPI
     - 9
     - GIC maintenance
     - Level
     - —
     - cpu_hw
     - GIC self-maintenance interrupt
   * - ARM64
     - GIC-600
     - PPI
     - 10
     - vTimer (EL2)
     - Level
     - —
     - cpu_hw
     - Hypervisor virtual timer
   * - ARM64
     - GIC-600
     - PPI
     - 11
     - vTimer (EL1)
     - Level
     - —
     - cpu_hw
     - Virtual timer for EL1
   * - ARM64
     - GIC-600
     - PPI
     - 13
     - Secure Physical Timer
     - Level
     - —
     - cpu_hw
     - TF-A / BL31 secure world
   * - ARM64
     - GIC-600
     - PPI
     - 14
     - Non-secure Physical Timer
     - Level
     - —
     - cpu_hw
     - uiox_cpu_isr_timer hook; timer_freq_hz=24 MHz
   * - ARM64
     - GIC-600
     - SPI
     - 36
     - SP804 System Timer
     - Level High
     - 4
     - cpu_hw
     - timer_base=0xFC010000; fallback MMIO timer
   * - ARM64
     - GIC-600
     - SPI
     - 37
     - UART0 (PL011)
     - Level High
     - 4
     - cpu_hw
     - 115200 console; stdout-path
   * - ARM64
     - GIC-600
     - SPI
     - 38
     - UART1 (PL011)
     - Level High
     - 4
     - cpu_hw
     - disabled
   * - ARM64
     - GIC-600
     - SPI
     - 40
     - I2C0
     - Level High
     - 4
     - cpu_hw
     - Thermal sensor @ 0x48 on bus
   * - ARM64
     - GIC-600
     - SPI
     - 41
     - I2C1
     - Level High
     - 4
     - cpu_hw
     - expansion
   * - ARM64
     - GIC-600
     - SPI
     - 42
     - SPI0 master
     - Level High
     - 4
     - wifi_hw
     - ESP8266 Wi-Fi SPI vtable
   * - ARM64
     - GIC-600
     - SPI
     - 43
     - SDMMC0 (SDIO)
     - Level High
     - 4
     - wifi_hw
     - CYW43xx SDIO vtable
   * - ARM64
     - GIC-600
     - SPI
     - 44
     - SDMMC1 (eMMC)
     - Level High
     - 4
     - cpu_hw
     - 
   * - ARM64
     - GIC-600
     - SPI
     - 45
     - SPI NOR Flash
     - Level High
     - 4
     - ksign_key
     - KRL + measurement log NVRAM
   * - ARM64
     - GIC-600
     - SPI
     - 50
     - GPIO0 bank
     - Level High
     - 4
     - cpu_hw
     - 32 GPIO lines; shared IRQ
   * - ARM64
     - GIC-600
     - SPI
     - 51
     - GPIO1 bank
     - Level High
     - 4
     - cpu_hw
     - 
   * - ARM64
     - GIC-600
     - SPI
     - 53
     - Watchdog (SP805)
     - Level High
     - 1
     - cpu_hw
     - 30 s; highest effective priority
   * - ARM64
     - GIC-600
     - SPI
     - 54
     - RTC
     - Level High
     - 4
     - cpu_hw
     - 
   * - ARM64
     - GIC-600
     - SPI
     - 55
     - Wi-Fi SPI IRQ (ESP8266)
     - Level High
     - 3
     - wifi_hw
     - SDIO/SPI/PCIe vtable; RSSI low evt
   * - ARM64
     - GIC-600
     - SPI
     - 56
     - Wi-Fi SDIO IRQ (CYW43xx)
     - Level High
     - 3
     - wifi_hw
     - in-band SDIO IRQ; WMM AC queues
   * - ARM64
     - GIC-600
     - SPI
     - 60
     - USB3 xHCI OTG
     - Level High
     - 3
     - usb_hw
     - FS/HS/SS/SS+; DMA rings; EP table
   * - ARM64
     - GIC-600
     - SPI
     - 61
     - USB2 EHCI
     - Level High
     - 3
     - usb_hw
     - 
   * - ARM64
     - GIC-600
     - SPI
     - 64
     - PCIe MSI
     - Edge
     - 3
     - cpu_hw
     - TB4 + AX200 Wi-Fi PCIe MSI
   * - ARM64
     - GIC-600
     - SPI
     - 65
     - PCIe INT A
     - Level Low
     - 3
     - cpu_hw
     - TB4 INTx fallback
   * - ARM64
     - GIC-600
     - SPI
     - 70
     - Crypto Engine
     - Level High
     - 2
     - ksign_img
     - SHA/RSA/ECDSA done IRQ; ksign-hw-accel
   * - x86-64
     - IOAPIC
     - ISA
     - 0
     - HPET Timer 0
     - Edge
     - —
     - cpu_hw
     - intel,ce4100-hpet; uiox_cpu_rdtsc()
   * - x86-64
     - IOAPIC
     - ISA
     - 3
     - UART COM2
     - Edge
     - —
     - cpu_hw
     - disabled
   * - x86-64
     - IOAPIC
     - ISA
     - 4
     - UART COM1 (16550A)
     - Edge
     - —
     - cpu_hw
     - 115200 console
   * - x86-64
     - IOAPIC
     - ISA
     - 6
     - TPM 2.0
     - Level
     - —
     - ksign_key
     - tcg,tpm-tis; PCR bank for ksign quotes
   * - x86-64
     - IOAPIC
     - PCI
     - 16
     - PCIe root complex
     - Level Low
     - —
     - cpu_hw
     - ECAM @ 0xE0000000
   * - x86-64
     - IOAPIC
     - PCI
     - 20
     - USB3 xHCI
     - Level Low
     - —
     - usb_hw
     - FS/HS/SS/SS+; OTG
   * - x86-64
     - IOAPIC
     - PCI
     - 22
     - Crypto Engine
     - Level Low
     - —
     - ksign_img
     - ksign-hw-accel
   * - x86-64
     - LAPIC
     - IPI
     - —
     - IPI vector (any)
     - Edge
     - —
     - cpu_hw
     - ipi_send vtable; ICR_LO@0xFEE00300; SIPI for AP wakeup
   * - x86-64
     - LAPIC
     - LVT
     - —
     - LAPIC Local Timer
     - Periodic
     - —
     - cpu_hw
     - LVT_TIMER@0xFEE00320; uiox_cpu_pm tick
   * - RISC-V
     - PLIC
     - EXT
     - 4
     - UART0 (SiFive)
     - Level High
     - 1
     - cpu_hw
     - 115200 console; txdata/rxdata regs
   * - RISC-V
     - PLIC
     - EXT
     - 5
     - UART1
     - Level High
     - 1
     - cpu_hw
     - disabled
   * - RISC-V
     - PLIC
     - EXT
     - 7–22
     - GPIO0 per-pin IRQs
     - Level/Edge
     - 2
     - cpu_hw
     - 16 GPIO lines; one PLIC source each
   * - RISC-V
     - PLIC
     - EXT
     - 51
     - SPI0 / QSPI
     - Level High
     - 2
     - wifi_hw
     - NOR flash + ESP8266 SPI
   * - RISC-V
     - PLIC
     - EXT
     - 52
     - SDMMC0
     - Level High
     - 2
     - wifi_hw
     - CYW43xx SDIO
   * - RISC-V
     - PLIC
     - EXT
     - 53
     - I2C0
     - Level High
     - 2
     - cpu_hw
     - Thermal sensor
   * - RISC-V
     - PLIC
     - EXT
     - 54
     - USB3 xHCI OTG
     - Level High
     - 3
     - usb_hw
     - FS/HS/SS; DMA rings
   * - RISC-V
     - PLIC
     - EXT
     - 56
     - PCIe MSI
     - Edge
     - 3
     - cpu_hw
     - AX200 + TB4
   * - RISC-V
     - PLIC
     - EXT
     - 57
     - PCIe INT A
     - Level Low
     - 3
     - cpu_hw
     - INTx fallback
   * - RISC-V
     - PLIC
     - EXT
     - 60
     - Crypto Engine
     - Level High
     - 1
     - ksign_img
     - ksign-hw-accel
   * - RISC-V
     - PLIC
     - EXT
     - 61
     - Watchdog
     - Level High
     - 1
     - cpu_hw
     - 30 s timeout
   * - RISC-V
     - PLIC
     - EXT
     - 62
     - RTC
     - Level High
     - 2
     - cpu_hw
     - 
   * - RISC-V
     - CLINT
     - SW
     - —
     - msip (per-hart IPI)
     - Edge
     - —
     - cpu_hw
     - ipi_send vtable; msip@0x02000000+hart×4
   * - RISC-V
     - CLINT
     - TIMER
     - —
     - mtimecmp (per-hart timer)
     - Level
     - —
     - cpu_hw
     - isr_timer vtable; timer_freq_hz ref=1 MHz

----

7_Memory_Map
============

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Region
     - Start Address
     - End Address
     - Size
     - Type
     - Owner / Content
     - Arch
     - Source File
     - Notes
   * - Kernel Load
     - 0x40080000
     - 0x407FFFFF
     - 7.5 MiB
     - Reserved/ROM
     - UIOX kernel image (ksign-verified)
     - All
     - ksign_img
     - load_addr field in uiox_ks_img_hdr_t
   * - Kernel Entry
     - 0x40080040
     - 0x40080043
     - 4 B
     - Code
     - Kernel entry point
     - All
     - ksign_img
     - entry_addr field in uiox_ks_img_hdr_t
   * - TF-A / BL31
     - 0x40000000
     - 0x4007FFFF
     - 512 KiB
     - Secure
     - TF-A secure firmware
     - ARM64
     - DTB_arm64
     - no-map; PSCI handler lives here
   * - RAM (total)
     - 0x40000000
     - 0x13FFFFFFF
     - 4 GiB
     - DRAM
     - LPDDR4X
     - ARM64
     - DTB_arm64
     - #address-cells=2; 4 GiB from 0x40000000
   * - initrd
     - 0x48000000
     - 0x487FFFFF
     - 8 MiB
     - Reserved
     - Initial ramdisk
     - ARM64
     - DTB_arm64
     - linux,initrd-start/end in chosen node
   * - DMA coherent
     - 0x4F000000
     - 0x4FFFFFFF
     - 16 MiB
     - Reserved
     - Kernel CMA pool
     - ARM64
     - DTB_arm64
     - shared-dma-pool; linux,cma-default
   * - OS usable RAM
     - 0x50000000
     - 0x13FFFFFFF
     - ~3.7 GiB
     - RAM
     - Kernel + user
     - ARM64
     - DTB_arm64
     - 
   * - SMP mbox
     - 0x40010000
     - 0x4001001F
     - 32 B
     - SRAM
     - Spin-table release addrs (4 cores×8 B)
     - ARM64
     - cpu_hw
     - smp_mbox_base; cpu-release-addr
   * - MMIO region
     - 0xFC000000
     - 0xFFFFFFFF
     - 64 MiB
     - MMIO
     - All SoC peripherals
     - ARM64
     - cpu_hw
     - Non-cacheable device memory
   * - BIOS/Legacy
     - 0x00000000
     - 0x000FFFFF
     - 1 MiB
     - Reserved
     - x86 legacy IVT/BIOS area
     - x86-64
     - DTB_x86
     - no-map
   * - RAM (x86)
     - 0x00100000
     - 0x3FFFFFFFF
     - ~16 GiB
     - DRAM
     - DDR5
     - x86-64
     - DTB_x86
     - memory@100000 in DTB
   * - DMA coherent
     - 0x4F000000
     - 0x50FFFFFF
     - 32 MiB
     - Reserved
     - Kernel CMA pool
     - x86-64
     - DTB_x86
     - shared-dma-pool
   * - PCIe ECAM
     - 0xE0000000
     - 0xEFFFFFFF
     - 256 MiB
     - MMIO/PCIe
     - Enhanced Cfg Access (256 buses)
     - x86-64
     - DTB_x86
     - pci-host-ecam-generic
   * - PCIe MEM
     - 0xA0000000
     - 0xCFFFFFFF
     - 768 MiB
     - MMIO/PCIe
     - PCIe outbound memory
     - x86-64
     - DTB_x86
     - TB4 + AX200 BARs
   * - PCIe I/O
     - 0xD0000000
     - 0xD0FFFFFF
     - 16 MiB
     - MMIO/PCIe
     - PCIe outbound I/O
     - x86-64
     - DTB_x86
     - 
   * - LAPIC
     - 0xFEE00000
     - 0xFEE00FFF
     - 4 KiB
     - MMIO
     - xAPIC registers
     - x86-64
     - cpu_hw
     - UIOX_CPU_CAP_HYPERTHREAD; RDMSR/WRMSR
   * - IOAPIC
     - 0xFEC00000
     - 0xFEC00FFF
     - 4 KiB
     - MMIO
     - I/O APIC registers
     - x86-64
     - cpu_hw
     - 
   * - HPET
     - 0xFED00000
     - 0xFED00FFF
     - 4 KiB
     - MMIO
     - HPET registers
     - x86-64
     - cpu_hw
     - timer_base field; timer_freq_hz
   * - TPM MMIO
     - 0xFED40000
     - 0xFED44FFF
     - 20 KiB
     - MMIO
     - TPM 2.0 TIS
     - x86-64
     - ksign_key
     - PCR bank for ksign sys_ksign_quote()
   * - M-mode FW
     - 0x40000000
     - 0x4007FFFF
     - 512 KiB
     - Secure
     - M-mode firmware
     - RISC-V
     - DTB_rv
     - no-map; SBI runtime
   * - RAM (RV)
     - 0x40000000
     - 0x13FFFFFFF
     - 4 GiB
     - DRAM
     - LPDDR4X
     - RISC-V
     - DTB_rv
     - 
   * - DMA coherent
     - 0x4F000000
     - 0x4FFFFFFF
     - 16 MiB
     - Reserved
     - Kernel CMA
     - RISC-V
     - DTB_rv
     - 
   * - CLINT
     - 0x02000000
     - 0x0200FFFF
     - 64 KiB
     - MMIO
     - Core-Local Interruptor
     - RISC-V
     - cpu_hw
     - clint_base; mtime/mtimecmp/msip
   * - PLIC
     - 0x0C000000
     - 0x0FFFFFFF
     - 64 MiB
     - MMIO
     - Platform-Level Interrupt Ctrl
     - RISC-V
     - cpu_hw
     - gic_base field; 64 sources
   * - UART/Periph
     - 0x10000000
     - 0x1FFFFFFF
     - 256 MiB
     - MMIO
     - All SoC peripherals
     - RISC-V
     - cpu_hw
     - 
   * - PCIe host
     - 0x30000000
     - 0x7FFFFFFF
     - 1.25 GiB
     - MMIO/PCIe
     - DBI + ATU + I/O + MEM
     - RISC-V
     - DTB_rv
     - sifive,fu740-pcie
   * - NOR: firmware
     - Flash 0x000000
     - Flash 0x07FFFF
     - 512 KiB
     - Flash
     - TF-A/BL31/M-mode FW
     - All
     - ksign_key
     - read-only; signed by ksign Root CA
   * - NOR: ksign KRL
     - Flash 0x080000
     - Flash 0x08FFFF
     - 64 KiB
     - Flash
     - Key Revocation List
     - All
     - ksign_key
     - uiox_ks_krl_magic=0x554B4B52; UIOX_KS_KRL_MAGIC
   * - NOR: meas log
     - Flash 0x090000
     - Flash 0x09FFFF
     - 64 KiB
     - Flash
     - ksign PCR measurement log
     - All
     - ksign_meas
     - log_serialise() → NVRAM; UIOX_KS_LOG_MAGIC
   * - NOR: uboot env
     - Flash 0x0A0000
     - Flash 0x0BFFFF
     - 128 KiB
     - Flash
     - U-Boot environment (ARM64)
     - ARM64
     - DTB_arm64
     - 

----

8_Syscalls
==========

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Syscall Name
     - Number (decimal)
     - Number (hex)
     - Arg 0 (a0/rdi/a0)
     - Arg 1 (a1/rsi/a1)
     - Arg 2 (a2/rdx/a2)
     - Arg 3 (a3/rcx/a3)
     - Return Value
     - Source File
     - Description
   * - sys_kernel_verify
     - 220
     - 0xDC
     - image_addr (uintptr_t)
     - image_size (size_t)
     - flags (long)
     - — (unused)
     - 0=OK, <0=uiox_ks_err_t
     - ksign_rt
     - Re-runs full ksign boot-time verify on a mapped kernel image from user-space. Parses header, checks rollback floor, SHA-256/384 hash, signature chain against locked keystore.
   * - sys_ksign_status
     - 221
     - 0xDD
     - buf (char* user)
     - buf_size (size_t)
     - — (unused)
     - — (unused)
     - 0=OK, <0=err
     - ksign_rt
     - Copies compact status string into user buffer: region count, total violations. Format: 'UIOX_KSIGN_STATUS regions=N violations=M'.
   * - sys_ksign_quote
     - 222
     - 0xDE
     - buf (uint8_t* user)
     - buf_size (size_t)
     - — (unused)
     - — (unused)
     - >0=bytes written, <0=err
     - ksign_rt
     - Serialises the live PCR measurement log into user buffer via uiox_ks_log_serialise(). Output is a flat binary blob (magic+version+count+PCR+entries) for remote attestation. Caller should sign the blob with a platform key for a full TPM-style quote.
   * - — (constant)
     - —
     - —
     - UIOX_KS_IMG_MAGIC = 0x554B5349
     - 'UKSI'
     - Signed image magic
     - —
     - —
     - ksign_types
     - uiox_ks_img_hdr_t.magic
   * - — (constant)
     - —
     - —
     - UIOX_KS_KEY_MAGIC = 0x554B4B45
     - 'UKKE'
     - Key entry magic
     - —
     - —
     - ksign_types
     - uiox_ks_key_entry_t.magic
   * - — (constant)
     - —
     - —
     - UIOX_KS_KRL_MAGIC = 0x554B4B52
     - 'UKKR'
     - KRL magic
     - —
     - —
     - ksign_types
     - uiox_ks_krl_t.magic
   * - — (constant)
     - —
     - —
     - UIOX_KS_LOG_MAGIC = 0x554B4C47
     - 'UKLG'
     - Meas. log magic
     - —
     - —
     - ksign_types
     - uiox_ks_log_t.magic
   * - — (constant)
     - —
     - —
     - UIOX_KS_FORMAT_VERSION = 1
     - —
     - Format version
     - —
     - —
     - ksign_types
     - All ksign on-disk structures
   * - — (alg IDs)
     - —
     - —
     - UIOX_KS_ALG_NONE=0
     - RSA2048_SHA256=1
     - RSA4096_SHA256=2
     - ECDSA_P256=3
     - ED25519=4
     - ksign_types
     - uiox_ks_alg_t enum
   * - — (sizes)
     - —
     - —
     - SHA256_LEN=32 B
     - SHA384_LEN=48 B
     - SHA512_LEN=64 B
     - KEY_ID_LEN=32 B
     - IMG_HDR_SIZE=512 B
     - ksign_types
     - Fixed-size constants in uiox_ksign_types.h
   * - — (img hdr)
     - —
     - —
     - kernel load_addr=0x40080000
     - entry_addr=0x40080040
     - payload_offset=512
     - payload_size=var
     - build_time=unix ts
     - ksign_img
     - uiox_ks_img_hdr_t field layout

----
