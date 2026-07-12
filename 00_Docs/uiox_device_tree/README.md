# UIOX Device Tree Package

Generated from cross-referencing the UIOX repository at
https://github.com/Pramod645/UIOX (branch UIX00).

## Files

| File | Target | Interrupt Controller |
|---|---|---|
| uiox-arm64.dts  | ARM Cortex-A76, ARMv8.2-A | GIC-600 (GICD @ 0xFE000000) |
| uiox-x86_64.dts | x86-64 (Intel/AMD, SSE4.2/AVX-512) | xAPIC + IOAPIC |
| uiox-riscv64.dts | RISC-V RV64GC (SV39/SV48, M/S/U) | PLIC + CLINT |
| Makefile | Build all → .dtb | — |

## Source cross-references

| DTS section | UIOX source |
|---|---|
| CPU nodes | uiox_cpu_hw.h — UIOX_CPU_ARCH_*, Cortex-A76 / x86-64 / RV64GC |
| GIC / APIC / PLIC | uiox_cpu_hw.h — gic_base, APIC MMIO, clint_base |
| Timer | uiox_cpu_hw.h — timer_base, timer_freq_hz, CNTPCT / TSC / mtime |
| USB | usb.md — FS/HS/SS/SS+, OTG, DMA rings, EP table |
| Wi-Fi | wifi.md — SDIO (CYW43xx), SPI (ESP8266), PCIe (Intel AX200) |
| Thunderbolt 4 | 50_UIX/20_uios/tb4_/ subsystem |
| Thermal | uiox_cpu_pm.h DVFS + thermal_/ subsystem |
| ksign crypto | 12_ksign/uiox_ksign_image.h (load=0x40080000, entry=0x40080040) |
| OTP / eFuse | uiox_ksign_key.h — Root CA burned into OTP / ROM |
| NVRAM / KRL | uiox_ksign_key.h — KRL stored in NVRAM; SPI NOR partitions |
| Measurement log | uiox_ksign_measure.c — log_serialise → NVRAM partition |
| SMP spin-table | uiox_cpu_hw.h — spin_table, smp_mbox_base |
| DVFS OPP | uiox_cpu_pm.h — P-state / OPP table per arch |
| PMU | uiox_cpu_hw.h — UIOX_PMU_MAX_COUNTERS = 8 |

## Build

```bash
# Install dtc
sudo apt install device-tree-compiler   # Debian/Ubuntu
brew install dtc                         # macOS

# Build all
make

# Inspect a decompiled blob
dtc -I dtb -O dts uiox-arm64.dtb
```

## Memory map (ARM64 / RISC-V)

| Region | Base | Size | Owner |
|---|---|---|---|
| TF-A / BL31 / M-mode | 0x40000000 | 512 KiB | Secure firmware |
| UIOX kernel image | 0x40080000 | 7.5 MiB | ksign verified |
| initrd (ARM64) | 0x48000000 | 8 MiB | Boot loader |
| DMA coherent pool | 0x4F000000 | 16 MiB | Kernel |
| OS usable RAM | 0x41000000+ | ~3.7 GiB | Kernel |

## SPI NOR Flash layout (ARM64 / RISC-V)

| Partition | Offset | Size |
|---|---|---|
| TF-A / firmware | 0x000000 | 512 KiB |
| ksign KRL | 0x080000 | 64 KiB |
| ksign measurement log | 0x090000 | 64 KiB |
| U-Boot env (ARM64) | 0x0A0000 | 128 KiB |
