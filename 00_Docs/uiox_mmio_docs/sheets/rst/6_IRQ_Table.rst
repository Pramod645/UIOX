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
