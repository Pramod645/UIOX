# UIOX MMIO Address Table — Documentation Package

Converted from `uiox_mmio_table.xlsx` (generated from the UIOX repository).

## Files

| File | Format | Contents |
|---|---|---|
| `uiox_mmio_table.rst` | reStructuredText | All 8 sheets in one RST file (list-table directives) |
| `uiox_mmio_table.md`  | Markdown (GFM)   | All 8 sheets in one Markdown file (pipe tables) |
| `sheets/rst/*.rst`    | RST per sheet    | One RST file per sheet |
| `sheets/md/*.md`      | MD per sheet     | One MD file per sheet |

## Sheets

| Sheet | Description |
|---|---|
| 1_Legend | Coverage map, column key, source file index |
| 2_ARM64_Map | ARM64 Cortex-A76 MMIO map (GIC-600, timers, UART, USB, PCIe, Crypto, OTP…) |
| 3_x86_Map | x86-64 MMIO + I/O port map (xAPIC, IOAPIC, HPET, MSRs, PCIe, TPM…) |
| 4_RISCV_Map | RISC-V RV64GC map (CLINT, PLIC, UART, SPI, USB, PCIe, CSRs…) |
| 5_RegOffsets | Per-peripheral register offset tables (PL011, GIC, SP804, xHCI, Crypto, OTP) |
| 6_IRQ_Table | Full interrupt assignments (GIC PPI/SPI, IOAPIC, PLIC, CLINT) |
| 7_Memory_Map | Physical memory layout + SPI NOR flash partition table |
| 8_Syscalls | ksign syscall numbers 220–222 + magic constants + algorithm IDs |

## Usage

### Sphinx (RST)
```rst
.. include:: uiox_mmio_table.rst
```
Or reference individual sheets:
```rst
.. include:: sheets/rst/2_ARM64_Map.rst
```

### MkDocs / GitHub (Markdown)
```md
[MMIO Table](uiox_mmio_table.md)
```
Or import individual sheets into any Markdown site.

## Source cross-reference

All entries trace back to:
- `50_UIX/20_uios/uiox-cpu/include/uiox_cpu_hw.h` — GIC/APIC/PLIC/CLINT bases, timer, SMP, PMU
- `50_UIX/20_uios/usb_/include/uiox_usb_hw.h` — USB FS/HS/SS/SS+, OTG, DMA, PHY
- `50_UIX/20_uios/wifi_/include/uiox_wifi_hw.h` — SDIO/SPI/PCIe Wi-Fi vtables
- `50_UIX/12_ksign/include/uiox_ksign_image.h` — kernel load/entry addresses
- `50_UIX/12_ksign/include/uiox_ksign_runtime.h` — syscall numbers 220–222
- `50_UIX/12_ksign/include/uiox_ksign_key.h` — OTP Root CA, KRL storage
- `50_UIX/12_ksign/include/uiox_ksign_measure.h` — PCR measurement log
