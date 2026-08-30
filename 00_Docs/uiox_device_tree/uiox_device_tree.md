
What the repo tells us about the hardware:
• 3 supported architectures: ARM Cortex-A76 (AArch64/ARMv8.2-A), x86-64, RISC-V RV64GC
• Interrupt controllers: GIC (ARM), APIC (x86), PLIC+CLINT (RISC-V)
• Peripherals confirmed: UART, USB (FS/HS/SS/SS+, OTG), Wi-Fi (SDIO/SPI/PCIe — CYW43xx/ESP8266/Intel AX200), Thunderbolt 4, Thermal sensor, Timer (ARM CNTPCT / x86 TSC / RISC-V mtime), PMU, SMP spin-table, IPI mailbox, DVFS/power management
• Memory layout: kernel loads at 0x40080000 (from ksign image header), entry 0x40080040
• Platform build target: primary PLATFORM=X8664, but ARM64 is the canonical embedded target

generate three DTS files (one per arch) plus a Makefile to compile them to .dtb, all in a zip.
uioxdevicetree.zip

What's in the package

Five files, one per architecture plus build tooling:

| File | Size | Target |
|---|---|---|
| uiox-arm64.dts | 24 KB | ARM Cortex-A76, ARMv8.2-A |
| uiox-x8664.dts | 9.7 KB | x86-64 (Intel/AMD, AVX-512) |
| uiox-riscv64.dts | 12.8 KB | RISC-V RV64GC (SV39/SV48) |
| Makefile | — | make → compiles all three to .dtb |
| README.md | — | Full cross-reference table + memory map |

> Note: A .dtb is the compiled binary form of a .dts text file. You compile it with dtc (Device Tree Compiler). The package gives you the .dts sources and a Makefile to produce the .dtb files on your machine, because .dtb is a machine-generated binary — shipping a pre-compiled one without your exact dtc version would risk subtle incompatibilities.

Build in 2 commands

``bash
Install dtc (once)
sudo apt install device-tree-compiler   # Debian / Ubuntu
brew install dtc                         # macOS

Compile all three
make
`

This produces uiox-arm64.dtb, uiox-x8664.dtb, and uiox-riscv64.dtb. To inspect any blob after compilation:

`bash
dtc -I dtb -O dts uiox-arm64.dtb
`

Every node is cross-referenced to the repo

| DTS node | UIOX source it came from |
|---|---|
| cpu@0..3 (A76 / x86 / RV64) | uioxcpuhw.h — UIOXCPUARCH*, model string |
| GIC-600 / xAPIC+IOAPIC / PLIC+CLINT | uioxcpuhw.h — gicbase, clintbase |
| ARM generic timer / HPET / mtime | uioxcpuhw.h — timerbase, timerfreqhz |
| usb@… (FS/HS/SS/SS+, OTG) | usb.md — MMIO, DMA rings, EP table, VBUS/OTG |
| Wi-Fi SDIO (CYW43xx) | wifi.md — SDIO vtable |
| Wi-Fi SPI (ESP8266) | wifi.md — SPI vtable |
| Wi-Fi PCIe (Intel AX200) | wifi.md — PCIe vtable |
| thunderbolt@… (TB4) | 50UIX/20uios/tb4/ subsystem |
| thermal-zones + DVFS OPP table | uioxcpupm.h — P-states, throttling trips |
| crypto@… (SHA-256/384, RSA, ECDSA) | 12ksign/uioxksignimage.h |
| otp@… root key | uioxksignkey.h — Root CA burned into OTP/ROM |
| SPI NOR partitions (KRL + PCR log) | uioxksignkey.h + uioxksignmeasure.c |
| kernel-load-addr = 0x40080000 | uioxksignimage.h — loadaddr field |
| PMU node | uioxcpuhw.h — UIOXPMUMAXCOUNTERS = 8 |
| SMP cpu-release-addr / smpmboxbase | uioxcpuhw.h — spintable, smpmboxbase` |

=============================================================================


| File | Target | Notes |
|---|---|---|
| uiox-arm64.dts | ARM Cortex-A76, ARMv8.2-A | GIC-600 @ 0xFE000000, SP804 timer, USB3, PCIe Gen4, TB4, Wi-Fi, Crypto, OTP, SPI NOR |
| uiox-x8664.dts | x86-64 (SSE4.2 / AVX-512) | xAPIC+IOAPIC, HPET, PCIe ECAM, TPM 2.0, USB3 |
| uiox-riscv64.dts | RISC-V RV64GC (SV39/SV48) | PLIC+CLINT, SiFive-style MMIO map, PCIe Gen3 |
| Makefile | Build all three | make → compiles all .dts → .dtb with dtc |
| README.md | Full cross-reference | Maps every DTS node back to its UIOX source file |

To compile the .dts files into actual .dtb binaries on your machine:

``bash
Install dtc (once)
sudo apt install device-tree-compiler   # Debian/Ubuntu
brew install dtc                         # macOS

Compile all three
make

Or individually
dtc -I dts -O dtb -o uiox-arm64.dtb   uiox-arm64.dts
dtc -I dts -O dtb -o uiox-x8664.dtb  uiox-x86_64.dts
dtc -I dts -O dtb -o uiox-riscv64.dtb uiox-riscv64.dts
`

A .dtb is a machine-generated binary — it must be compiled by dtc on your host rather than pre-shipped, since the exact binary format depends on your dtc version and target endianness. The .dts` sources in the zip are the authoritative input.


---------------------------
https://devicetree-specification.readthedocs.io/en/stable/flattened-format.html

To decompile a DTB back to a readable DTS text file:

dtc -I dtb -O dts -o output_hardware_profile.dts input_file.dtb


To compile a DTS file into a DTB binary:

dtc -I dts -O dtb -o new_board_profile.dtb input_file.dts


examples:
/dts-v1/;

/ {
    model = "My Board Name";
    compatible = "vendor,board-model";

    cpus {
        #address-cells = <1>;
        #size-cells = <0>;

        cpu@0 {
            device_type = "cpu";
            compatible = "arm,cortex-a9";
            reg = <0>;
        };
    };

    memory {
        device_type = "memory";
        reg = <0x80000000 0x40000000>; /* 1GB RAM at 0x80000000 */
    };
};

Structure Elements
    Root Node (/): The single starting point containing all other hardware nodes.
    Nodes (node_name@address): Represent physical devices, buses, or sub-blocks.
    Properties (name = value): Key-value pairs defining characteristics like interrupts, compatible strings, and memory addresses (reg).
    Labels (label:): Unique identifiers used to reference or override nodes inside included files.

    https://docs.kernel.org/devicetree/bindings/dts-coding-style.html

Compilation Workflow
    Source to Blob: Text files (.dts/.dtsi) are compiled by the Device Tree Compiler (DTC) into a binary file called a DTB (Device Tree Blob) (.dtb).
    Runtime Execution: Bootloaders (such as U-Boot) load the .dtb into memory and pass it directly to the kernel at system startup so it can initialize device drivers dynamically.

    https://devicetree-specification.readthedocs.io/en/v0.3/source-language.html

