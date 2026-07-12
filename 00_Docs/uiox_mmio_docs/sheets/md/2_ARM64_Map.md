## 2_ARM64_Map

| Peripheral | Block / Function | Base Address | End Address | Size (hex) | IRQ (GIC SPI#) | Bus | Access Width | Clock Source | Source File | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| GIC-600 GICD | Global Interrupt Distributor | 0xFE000000 | 0xFE00FFFF | 0x10000 | — | AXI | 32 | — | cpu_hw / DTB_arm64 | GIC-v3; 256 SPI lines; IRQ enable/pend/prio/route regs |
| GIC-600 GICR0 | Redistributor — CPU0 | 0xFE020000 | 0xFE03FFFF | 0x20000 | PPI 9 | AXI | 32 | — | cpu_hw / DTB_arm64 | Per-core redistr; LPI pending table; 128 KiB each |
| GIC-600 GICR1 | Redistributor — CPU1 | 0xFE040000 | 0xFE05FFFF | 0x20000 | PPI 9 | AXI | 32 | — | cpu_hw / DTB_arm64 |  |
| GIC-600 GICR2 | Redistributor — CPU2 | 0xFE060000 | 0xFE07FFFF | 0x20000 | PPI 9 | AXI | 32 | — | cpu_hw / DTB_arm64 |  |
| GIC-600 GICR3 | Redistributor — CPU3 | 0xFE080000 | 0xFE09FFFF | 0x20000 | PPI 9 | AXI | 32 | — | cpu_hw / DTB_arm64 |  |
| ARM Generic Timer | CNTPCT_EL0 (virtual) | — | — | — | PPI 10/11/13/14 | — | 64 (sysreg) | osc24M (24 MHz) | cpu_hw | timer_freq_hz=24000000; uiox_cpu_read_sysreg(cntpct_el0) |
| SP804 / UIOX Timer | System timer MMIO fallback | 0xFC010000 | 0xFC010FFF | 0x1000 | SPI 36 | APB | 32 | apb_clk (100 MHz) | cpu_hw / DTB_arm64 | Dual-channel countdown; timer_base field in uiox_cpu_hw_t |
| UART0 (PL011) | Primary serial console | 0xFC020000 | 0xFC020FFF | 0x1000 | SPI 37 | APB | 32 | apb_clk | cpu_hw / DTB_arm64 | 115200 n8; stdout-path; uiox_fw_printf target |
| UART1 (PL011) | Secondary UART | 0xFC021000 | 0xFC021FFF | 0x1000 | SPI 38 | APB | 32 | apb_clk | cpu_hw / DTB_arm64 | disabled by default |
| I2C0 | Fast-mode 400 kHz; thermal sensor | 0xFC030000 | 0xFC030FFF | 0x1000 | SPI 40 | APB | 32 | apb_clk | therm_hw / DTB_arm64 | Thermal sensor @ 0x48; uiox_cpu_pm.h thermal throttle |
| I2C1 | Fast-mode 400 kHz; expansion | 0xFC031000 | 0xFC031FFF | 0x1000 | SPI 41 | APB | 32 | apb_clk | DTB_arm64 |  |
| SPI0 (QSPI Master) | Wi-Fi SPI + NOR flash | 0xFC040000 | 0xFC040FFF | 0x1000 | SPI 42 | APB | 32 | apb_clk | wifi_hw / DTB_arm64 | CS0: ESP8266 Wi-Fi @ 40 MHz; wifi.md SPI vtable |
| SDMMC0 (SDIO) | CYW43xx Wi-Fi SDIO | 0xFC050000 | 0xFC05FFFF | 0x10000 | SPI 43 | AHB | 32 | ahb_clk (200 MHz) | wifi_hw / DTB_arm64 | 4-bit SDIO; CYW43xx real SDIO vtable (wifi.md) |
| SDMMC1 (eMMC) | Boot storage | 0xFC060000 | 0xFC06FFFF | 0x10000 | SPI 44 | AHB | 32 | ahb_clk | DTB_arm64 | 8-bit HS200 1.8V; non-removable |
| GPIO0 | 32-bit GPIO bank 0 (pins 0–31) | 0xFC070000 | 0xFC070FFF | 0x1000 | SPI 50 | APB | 32 | apb_clk | DTB_arm64 | Interrupt-capable; gpio-ranges 0–31 |
| GPIO1 | 32-bit GPIO bank 1 (pins 32–63) | 0xFC071000 | 0xFC071FFF | 0x1000 | SPI 51 | APB | 32 | apb_clk | DTB_arm64 | Interrupt-capable; gpio-ranges 32–63 |
| Watchdog (SP805) | Hardware watchdog | 0xFC080000 | 0xFC080FFF | 0x1000 | SPI 53 | APB | 32 | apb_clk | DTB_arm64 | 30 s timeout; arm,sp805 compat |
| RTC | Real-time clock | 0xFC090000 | 0xFC090FFF | 0x1000 | SPI 54 | APB | 32 | osc24M | DTB_arm64 | uiox,uiox-rtc |
| USB3 xHCI (OTG) | USB 3.2 Gen2×2 Host+OTG | 0xFC100000 | 0xFC10FFFF | 0x10000 | SPI 60 | AHB | 32 | ahb_clk | usb_hw / DTB_arm64 | FS/HS/SS/SS+; DMA TX/RX rings; EP table; VBUS/OTG; usb.md |
| USB2 EHCI | USB 2.0 Host | 0xFC110000 | 0xFC11FFFF | 0x10000 | SPI 61 | AHB | 32 | ahb_clk | usb_hw / DTB_arm64 | FS/HS only; generic-ehci |
| SPI NOR Flash | KRL + measurement log NVRAM | 0xFC310000 | 0xFC310FFF | 0x1000 | SPI 45 | APB | 32 | apb_clk | ksign_key / ksign_meas / DTB_arm64 | 50 MHz; partitions: fw@0, krl@80000, mlog@90000, uboot@A0000 |
| Crypto Engine | SHA-256/384, RSA-2048/4096, ECDSA-P256, Ed25519 | 0xFC200000 | 0xFC20FFFF | 0x10000 | SPI 70 | AHB | 32 | ahb_clk | ksign_img / ksign_key / DTB_arm64 | uiox,ksign-hw-accel; alg IDs 1-4 in uiox_ks_alg_t |
| OTP / eFuse | Root CA public key storage | 0xFC300000 | 0xFC300FFF | 0x1000 | — | APB | 32 | — | ksign_key / DTB_arm64 | Root CA @ offset 0x100, 256 B; burned at mfg; uiox_ksign_key.h |
| PCIe DBI | PCIe host DBI config | 0xFD000000 | 0xFD3FFFFF | 0x400000 | SPI 64 | AXI | 32 | ahb_clk | DTB_arm64 | DesignWare PCIe; 4-lane Gen4 for TB4 |
| PCIe ATU | PCIe iATU region | 0xFD400000 | 0xFD7FFFFF | 0x400000 | SPI 65 | AXI | 32 | ahb_clk | DTB_arm64 | MSI + INTA; TB4 controller + Intel AX200 Wi-Fi |
| PCIe I/O window | PCIe I/O translated | 0xFD800000 | 0xFD8FFFFF | 0x100000 | — | PCIe | 32 | — | DTB_arm64 |  |
| PCIe MEM window | PCIe memory mapped | 0xC0000000 | 0xDFFFFFFF | 0x20000000 | — | PCIe | 32 | — | DTB_arm64 | TB4 + AX200; 512 MiB outbound window |
| GIC GICD_CTLR | GIC Distributor Control | 0xFE000000 | 0xFE000003 | 0x4 | — | AXI | 32 | — | cpu_hw | Enable grp0/grp1; ARE_S/ARE_NS bits |
| GIC GICD_IGROUPR0 | IRQ Group 0 (SPIs 0-31) | 0xFE000080 | 0xFE000083 | 0x4 | — | AXI | 32 | — | cpu_hw | 1=Group1(NS); 0=Group0(S) |
| GIC GICD_ISENABLER0 | SPI Enable Set 0 | 0xFE000100 | 0xFE000103 | 0x4 | — | AXI | 32 | — | cpu_hw | Set bit n → enable SPI n |
| GIC GICD_ICENABLER0 | SPI Enable Clear 0 | 0xFE000180 | 0xFE000183 | 0x4 | — | AXI | 32 | — | cpu_hw | Set bit n → disable SPI n |
| GIC GICD_IPRIORITYR0 | Priority regs base | 0xFE000400 | 0xFE0007FF | 0x400 | — | AXI | 32 | — | cpu_hw | 8 priorities per reg; lower=higher priority |
| GIC GICD_ITARGETSR0 | Target CPU base (v2) | 0xFE000800 | 0xFE000BFF | 0x400 | — | AXI | 32 | — | cpu_hw | GICv3: use IROUTER instead |
| GIC GICD_IROUTER0 | Route SPI to affinity | 0xFE006000 | 0xFE007FFF | 0x2000 | — | AXI | 64 | — | cpu_hw | 64-bit per SPI; GICv3 affinity routing |
| PMU | Hardware perf counters | — | — | — | PPI 7 | — | 64 (sysreg) | — | cpu_feat | UIOX_PMU_MAX_COUNTERS=8; arm,cortex-a76-pmu |
| SMP Spin-Table | Secondary core release addr | 0x40010000 | 0x4001001F | 0x20 | — | — | 64 | — | cpu_hw | smp_mbox_base; cpu-release-addr per core @0,8,10,18 |
