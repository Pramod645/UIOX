# UIOX Device Tree Package

| File | Target | Interrupt Controller |
|---|---|---|
| uiox-arm64.dts  | ARM Cortex-A76, ARMv8.2-A | GIC-400 |
| uiox-arm32.dts  | ARMv7-A, QEMU versatilepb, Cortex-A9 | PL190 VIC |
| uiox-riscv64.dts | RISC-V RV64GC | PLIC + CLINT |
| uiox-x86_64.dts | x86-64 (QEMU q35) | xAPIC + IOAPIC |

## Build
```bash
make          # build all four .dtb
make check    # round-trip validate
```

## Structure rule
Inside a `simple-bus` `/soc`, every direct child is a memory-mapped
device with a `reg`. Non-MMIO nodes (PMU, generic timer, thermal-zones,
fixed clocks) live at the root. Bus nodes with children (flash with
partitions) declare `ranges;`.

## Memory map (ARM64 / RISC-V)
| Region | Base | Size |
|---|---|---|
| TF-A / BL31 / M-mode | 0x40000000 | 512 KiB |
| UIOX kernel image | 0x40080000 | 7.5 MiB |
| initrd | 0x48000000 | 8 MiB |
| OS usable RAM | 0x41000000+ | ~3.7 GiB |
