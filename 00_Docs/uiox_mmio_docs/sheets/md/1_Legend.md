## 1_Legend

| UIOX MMIO Address Table |  |  |  |
| --- | --- | --- | --- |
| Repository | https://github.com/Pramod645/UIOX (branch UIX00) |  |  |
| Generated | 2026-07-12 |  |  |
| Version | 1.0.0 |  |  |

| Sheet | Contents | Primary Source | Architectures Covered |
| 1_Legend | This sheet — coverage, legend, source index | — | All |
| 2_ARM64_Map | ARM64 (Cortex-A76) full MMIO map + register offsets | uiox_cpu_hw.h / DTB | ARM64 |
| 3_x86_Map | x86-64 full MMIO / I-O port map + register offsets | uiox_cpu_hw.h / DTB | x86-64 |
| 4_RISCV_Map | RISC-V RV64GC full MMIO map + register offsets | uiox_cpu_hw.h / DTB | RISC-V |
| 5_RegOffsets | Per-peripheral register offset tables (all archs) | usb.md / wifi.md / ksign | All |
| 6_IRQ_Table | Interrupt number assignments (GIC SPI / PLIC / IOAPIC) | uiox_cpu_hw.h / DTB | All |
| 7_Memory_Map | Physical memory layout, reserved regions, flash partitions | uiox_ksign_image.h | All |
| 8_Syscalls | UIOX ksign syscall numbers (sys_kernel_verify etc.) | uiox_ksign_runtime.h | All |

| Legend — Column meanings (sheets 2-4) |  |  |  |
| Column | Meaning |  |  |
| Peripheral | Hardware block name (e.g. GIC-600, USB3-xHCI) |  |  |
| Base Address | Hex base address of the MMIO region |  |  |
| End Address | Inclusive last byte of the region |  |  |
| Size | Region size in bytes (hex) |  |  |
| IRQ | Interrupt line (SPI/PPI/PLIC/IOAPIC number) |  |  |
| Bus | AHB / APB / AXI / PCIe / I/O-port |  |  |
| Access Width | 8 / 16 / 32 / 64-bit register access width |  |  |
| Clock Source | Clock that gates this block |  |  |
| Source File | UIOX repo file that defines this peripheral |  |  |
| Notes | Functional notes, speed, sub-modes, cross-refs |  |  |

| Source File Index |  |  |  |
| Abbreviation | Full path in repo |  |  |
| cpu_hw | 50_UIX/20_uios/uiox-cpu/include/uiox_cpu_hw.h |  |  |
| cpu_pm | 50_UIX/20_uios/uiox-cpu/include/uiox_cpu_pm.h |  |  |
| cpu_feat | 50_UIX/20_uios/uiox-cpu/include/uiox_cpu_feat.h |  |  |
| cpu_subsys | 50_UIX/20_uios/uiox-cpu/include/uiox_cpu_subsys.h |  |  |
| usb_hw | 50_UIX/20_uios/usb_/include/uiox_usb_hw.h |  |  |
| wifi_hw | 50_UIX/20_uios/wifi_/include/uiox_wifi_hw.h |  |  |
| tb4_hw | 50_UIX/20_uios/tb4_/include/uiox_tb4_hw.h |  |  |
| therm_hw | 50_UIX/20_uios/thermal_/include/uiox_thermal_hw.h |  |  |
| ksign_img | 50_UIX/12_ksign/include/uiox_ksign_image.h |  |  |
| ksign_rt | 50_UIX/12_ksign/include/uiox_ksign_runtime.h |  |  |
| ksign_key | 50_UIX/12_ksign/include/uiox_ksign_key.h |  |  |
| ksign_meas | 50_UIX/12_ksign/include/uiox_ksign_measure.h |  |  |
| DTB_arm64 | 50_UIX/dts/uiox-arm64.dts (generated) |  |  |
| DTB_x86 | 50_UIX/dts/uiox-x86_64.dts (generated) |  |  |
| DTB_rv | 50_UIX/dts/uiox-riscv64.dts (generated) |  |  |
