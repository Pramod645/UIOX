## 7_Memory_Map

| Region | Start Address | End Address | Size | Type | Owner / Content | Arch | Source File | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Kernel Load | 0x40080000 | 0x407FFFFF | 7.5 MiB | Reserved/ROM | UIOX kernel image (ksign-verified) | All | ksign_img | load_addr field in uiox_ks_img_hdr_t |
| Kernel Entry | 0x40080040 | 0x40080043 | 4 B | Code | Kernel entry point | All | ksign_img | entry_addr field in uiox_ks_img_hdr_t |
| TF-A / BL31 | 0x40000000 | 0x4007FFFF | 512 KiB | Secure | TF-A secure firmware | ARM64 | DTB_arm64 | no-map; PSCI handler lives here |
| RAM (total) | 0x40000000 | 0x13FFFFFFF | 4 GiB | DRAM | LPDDR4X | ARM64 | DTB_arm64 | #address-cells=2; 4 GiB from 0x40000000 |
| initrd | 0x48000000 | 0x487FFFFF | 8 MiB | Reserved | Initial ramdisk | ARM64 | DTB_arm64 | linux,initrd-start/end in chosen node |
| DMA coherent | 0x4F000000 | 0x4FFFFFFF | 16 MiB | Reserved | Kernel CMA pool | ARM64 | DTB_arm64 | shared-dma-pool; linux,cma-default |
| OS usable RAM | 0x50000000 | 0x13FFFFFFF | ~3.7 GiB | RAM | Kernel + user | ARM64 | DTB_arm64 |  |
| SMP mbox | 0x40010000 | 0x4001001F | 32 B | SRAM | Spin-table release addrs (4 cores×8 B) | ARM64 | cpu_hw | smp_mbox_base; cpu-release-addr |
| MMIO region | 0xFC000000 | 0xFFFFFFFF | 64 MiB | MMIO | All SoC peripherals | ARM64 | cpu_hw | Non-cacheable device memory |
| BIOS/Legacy | 0x00000000 | 0x000FFFFF | 1 MiB | Reserved | x86 legacy IVT/BIOS area | x86-64 | DTB_x86 | no-map |
| RAM (x86) | 0x00100000 | 0x3FFFFFFFF | ~16 GiB | DRAM | DDR5 | x86-64 | DTB_x86 | memory@100000 in DTB |
| DMA coherent | 0x4F000000 | 0x50FFFFFF | 32 MiB | Reserved | Kernel CMA pool | x86-64 | DTB_x86 | shared-dma-pool |
| PCIe ECAM | 0xE0000000 | 0xEFFFFFFF | 256 MiB | MMIO/PCIe | Enhanced Cfg Access (256 buses) | x86-64 | DTB_x86 | pci-host-ecam-generic |
| PCIe MEM | 0xA0000000 | 0xCFFFFFFF | 768 MiB | MMIO/PCIe | PCIe outbound memory | x86-64 | DTB_x86 | TB4 + AX200 BARs |
| PCIe I/O | 0xD0000000 | 0xD0FFFFFF | 16 MiB | MMIO/PCIe | PCIe outbound I/O | x86-64 | DTB_x86 |  |
| LAPIC | 0xFEE00000 | 0xFEE00FFF | 4 KiB | MMIO | xAPIC registers | x86-64 | cpu_hw | UIOX_CPU_CAP_HYPERTHREAD; RDMSR/WRMSR |
| IOAPIC | 0xFEC00000 | 0xFEC00FFF | 4 KiB | MMIO | I/O APIC registers | x86-64 | cpu_hw |  |
| HPET | 0xFED00000 | 0xFED00FFF | 4 KiB | MMIO | HPET registers | x86-64 | cpu_hw | timer_base field; timer_freq_hz |
| TPM MMIO | 0xFED40000 | 0xFED44FFF | 20 KiB | MMIO | TPM 2.0 TIS | x86-64 | ksign_key | PCR bank for ksign sys_ksign_quote() |
| M-mode FW | 0x40000000 | 0x4007FFFF | 512 KiB | Secure | M-mode firmware | RISC-V | DTB_rv | no-map; SBI runtime |
| RAM (RV) | 0x40000000 | 0x13FFFFFFF | 4 GiB | DRAM | LPDDR4X | RISC-V | DTB_rv |  |
| DMA coherent | 0x4F000000 | 0x4FFFFFFF | 16 MiB | Reserved | Kernel CMA | RISC-V | DTB_rv |  |
| CLINT | 0x02000000 | 0x0200FFFF | 64 KiB | MMIO | Core-Local Interruptor | RISC-V | cpu_hw | clint_base; mtime/mtimecmp/msip |
| PLIC | 0x0C000000 | 0x0FFFFFFF | 64 MiB | MMIO | Platform-Level Interrupt Ctrl | RISC-V | cpu_hw | gic_base field; 64 sources |
| UART/Periph | 0x10000000 | 0x1FFFFFFF | 256 MiB | MMIO | All SoC peripherals | RISC-V | cpu_hw |  |
| PCIe host | 0x30000000 | 0x7FFFFFFF | 1.25 GiB | MMIO/PCIe | DBI + ATU + I/O + MEM | RISC-V | DTB_rv | sifive,fu740-pcie |
| NOR: firmware | Flash 0x000000 | Flash 0x07FFFF | 512 KiB | Flash | TF-A/BL31/M-mode FW | All | ksign_key | read-only; signed by ksign Root CA |
| NOR: ksign KRL | Flash 0x080000 | Flash 0x08FFFF | 64 KiB | Flash | Key Revocation List | All | ksign_key | uiox_ks_krl_magic=0x554B4B52; UIOX_KS_KRL_MAGIC |
| NOR: meas log | Flash 0x090000 | Flash 0x09FFFF | 64 KiB | Flash | ksign PCR measurement log | All | ksign_meas | log_serialise() → NVRAM; UIOX_KS_LOG_MAGIC |
| NOR: uboot env | Flash 0x0A0000 | Flash 0x0BFFFF | 128 KiB | Flash | U-Boot environment (ARM64) | ARM64 | DTB_arm64 |  |
