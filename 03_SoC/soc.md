I already have the full file list from the earlier fetches in this session. Here is the complete segregation:

02FwHal/include/ — SoC vs Peripheral
SoC Files (chip-wide infrastructure)

These define the silicon fabric — clocks, power domains, MMIO address map, reset controller, and the master SoC descriptor. Nothing runs without these.

| File | Category | What It Defines |
|---|---|---|
| uioxsoc.h | SoC | Master umbrella include — uioxsocinit(), uioxsocgetdesc(), uioxsocgetclk(), uioxsocgetpm() |
| uioxsoctypes.h | SoC | uioxsocidt enum (QEMU/BCM2711/IMX8/RK3588/SiFive…), uioxsocdesct, UIOXSOCCAP capability flags |
| uioxsocmap.h | SoC | MMIO base addresses and IRQ numbers for all 4 architectures (SOCDRAMBASE, SOCGICDISTBASE, SOCUART0BASE, SOCCLINTBASE, SOCPLICBASE) |
| uioxsocclk.h | SoC | Clock tree: uioxclkidt, uioxpllcfgt, uioxclkctxt, uioxclkinit(), uioxclkenable(), uioxclkgethz() |
| uioxsocpm.h | SoC | Power domains + reset controller: uioxpdidt, uioxrstidt, uioxpmctxt, uioxpmdomainon/off(), uioxrstassert/deassert(), uioxpmcpuon() |
| uioxfwtypes.h | SoC | Base integer types, uioxfwarcht enum, uioxfwerrt error codes, UIOXFWBIT() macro — shared by everything |
| uioxfwhw.h | SoC | Platform HW abstraction vtable (uioxfwhwopst 18-op table), uioxfwplatformt descriptor, UIOXFWCAP flags, uioxfwhwregister() |
| uioxfwclock.h | SoC | Clock/PLL management: uioxfwclkidt, uioxfwclockt, uioxfwclockinit/enable/disable/gethz() |
| uioxfwpower.h | SoC | System-level power: PSCI function IDs, ACPI PM1, uioxfwpwrstatet, uioxfwpowerctxt, uioxfwpowerreset/shutdown/cpuon() |
| uioxfwpsci.h | SoC | PSCI 1.1 dispatch table: all PSCI function IDs (CPUON/OFF/SUSPEND, SYSTEMOFF/RESET), uioxpscictxt, SMC/HVC handler |
| uioxfwtz.h | SoC | ARM TrustZone EL3 setup: SCREL3 bits, TZC-400 region config, secure/non-secure GIC group assignments, ERET to EL1 |
| uioxfwirq.h | SoC | IRQ controller abstraction: GIC/APIC/PLIC init, irqregister(), irqenable(), irqack() |
| uioxfwmem.h | SoC | Memory management: DDR region descriptors, bump allocator, uioxfwmemset/memcpy() no-libc helpers |
| uioxfwdma.h | SoC | DMA controller: PL080/PL330/Intel 8237A, uioxdmactrlt, uioxdmaxfert, uioxfwdmatransfer(), 8 channels |
| uioxfwpost.h | SoC | Power-On Self Test: CPU register sanity, DRAM walk test, ROM CRC, peripheral smoke tests, uioxpostreportt |
| uioxfwsecboot.h | SoC | Secure boot chain of trust: RoT key, FVC certificate, SHA-256, RSA/ECDSA signature verify, anti-rollback, uioxsecbootctxt |
| uioxfw.h | SoC | Master firmware include — pulls in all FwHal headers, defines FWLOG(), uioxfwprintf() |

Peripheral Files (individual device drivers)

These configure one specific device at a time. They depend on SoC init being complete first.

| File | Category | What It Defines |
|---|---|---|
| uioxfwuart.h | Peripheral | PL011 / 16550A / NS16550A UART: baud, parity, FIFO, RTS/CTS, uioxfwuartt, uioxfwuartinit/putc/getc/irq() |
| uioxfwspi.h | Peripheral | PL022 SSP / Intel ICH SPI: CPOL/CPHA modes, CS0–3, uioxspidevt, uioxfwspiinit/transfer/write/read() |
| uioxfwi2c.h | Peripheral | DesignWare APB / Intel SMBus: 100K/400K/1M/3.4M speeds, uioxi2cdevt, uioxfwi2cwrite/read/transfer() |
| uioxfwtimer.h | Peripheral | SP804 / PIT 8254 / ARM Generic Timer / RISC-V CLINT: uioxfwtimert, uioxfwtimerinit/start/stop/ticks() |
| uioxfwgpio.h | Peripheral | GPIO controller: direction/pull/IRQ modes, 128 pins, uioxfwgpiot, uioxfwgpiosetdir/write/read/irqen() |
| uioxfwwdt.h | Peripheral | Watchdog timer SP805/iTCO: load/kick/disable, timeout formula, hardware reset on expiry |
| uioxfwpcie.h | Peripheral | PCIe ECAM: config space read/write, BAR assignment, device scan, uioxpciectrlt, uioxpciedevt |
| uioxfwusb.h | Peripheral | xHCI/EHCI/OHCI host: capability registers, USBCMD/USBSTS, port reset, enumerate, ctrl/bulk transfer |
| uioxfweth.h | Peripheral | Ethernet MAC: VirtIO-net / SMSC LAN9118 / RTL8139, uioxethdevt, send/receive/ISR |
| uioxfwnet.h | Peripheral | Network HAL: VirtIO-net / SMSC / e1000, MAC frame, RX callback, link status |
| uioxfwsd.h | Peripheral | SD/SDHC/SDXC SDIO host: CMD0/8/17/24/ACMD41, 4-bit bus, card detect, uioxsddevt |
| uioxfwemmc.h | Peripheral | eMMC 5.1 HS400/HS200: HC registers, boot/RPMB partitions, 8-bit bus, uioxemmcdevt |
| uioxfwsata.h | Peripheral | SATA III AHCI: GHC/PI/IS registers, port map, ATACMDIDENTIFY/READDMA/WRITEDMA |
| uioxfwnvme.h | Peripheral | NVMe M.2 PCIe BAR0: CAP/CC/CSTS registers, Admin+IO queues, identify/read/write/trim |
| uioxfwstorage.h | Peripheral | Block storage HAL: unified interface over SD/eMMC/NVMe/SATA |
| uioxfwwifi.h | Peripheral | Wi-Fi 6 SDIO/PCIe: 802.11n/ac/ax, firmware load, scan/connect/disconnect, uioxwifidevt |
| uioxfwbt.h | Peripheral | Bluetooth HCI transport: UART/USB, firmware patch load, HCI send/receive, GPIO reset/pwren pins |
| uioxfwgpu.h | Peripheral | VirtIO-GPU / Mali framebuffer: resource create, scanout, flush, uioxgpudevt |
| uioxfwhdmi.h | Peripheral | HDMI output: DDC/EDID read, mode set, HPD interrupt |
| uioxfwmonitor.h | Peripheral | Display monitor EDID/DDC over I2C: uioxmonitordevt, readedid, get preferred mode |
| uioxfwcamera.h | Peripheral | MIPI CSI-2 camera: OV5640/IMX219 I2C config, lane enable, uioxcameradevt |
| uioxfwsensor.h | Peripheral | Sensor HAL: BMI270 IMU, TSL2591 ALS, BMP390 pressure — I2C/SPI, uioxfwsenstypet |
| uioxfwals.h | Peripheral | Ambient light sensor: VEML7700/OPT3001 over I2C, lux×1000, auto-gain, interrupt threshold |
| uioxfwthermal.h | Peripheral | Thermal sensor + trip points: LM75/NCT7802 over I2C, zone descriptors, fan PWM |
| uioxfwfan.h | Peripheral | PWM fan + tach RPM: timer PWM output, duty cycle 0–100%, minrpm fault |
| uioxfwpmic.h | Peripheral | PMIC over I2C: DA9062/RK808/ACT8865, BUCK/LDO rails, voltage set, power sequencing |
| uioxfwcharger.h | Peripheral | USB-C PD / barrel charger: BQ25895/FUSB302, VBUS/VBAT/ICHG ADC, OTG enable |
| uioxfwbms.h | Peripheral | Battery management: BQ27742 over I2C, SOC%, remaining capacity, avg current |
| uioxfwramrtc.h | Peripheral | Battery-backed RTC + NVRAM: DS1307/DS3231/MC146818, time read/write, 56-byte NVRAM |
| uioxfwspeaker.h | Peripheral | I2S audio DAC + amplifier: BCLK/LRCK/DATA pins, sample rate, volume, mute |
| uioxfwmic.h | Peripheral | MEMS microphone PDM/I2S: CLK/DATA pins, sample rate, channel count |
| uioxfwmouse.h | Peripheral | PS/2 / USB HID mouse: dx/dy/dz event, button bits, IRQ handler |
| uioxfwkeyboard.h | Peripheral | PS/2 / USB HID keyboard: scan codes, key event callback |
| uioxfwtouchpwd.h | Peripheral | Fingerprint sensor over I2C: FPC1020/Goodix, enroll/verify, 10 template slots, INT/RESET GPIO |
| uioxfwtb4.h | Peripheral | Thunderbolt 4 / USB4 (Intel JHL8540): NHI BAR0, ICM messaging, device approve, hotplug |
| uioxfwnvme.h | Peripheral | (listed above)* |
| uioxfwdevsw.h | Peripheral | Device switch / power button: GPIO debounce, long-press detect, callback |

02FwHal/src/ — SoC vs Peripheral
SoC Source Files

| File | Category | What It Implements |
|---|---|---|
| uioxsocarm64.c | SoC | ARM64 SoC detect (MIDREL1), GIC-400 init, PL011 clock config, PM init |
| uioxsocarm32.c | SoC | ARM32 SoC detect, GIC init, SP804 clock, PM init |
| uioxsocx86.c | SoC | x86-64 CPUID detect, LAPIC enable, COM1 init, HPET init |
| uioxsocriscv64.c | SoC | RISC-V SBI probe, CLINT init, PLIC threshold, NS16550A, S-mode delegation |
| uioxfwpsci.c | SoC | PSCI 1.1 dispatch — CPUON/OFF, SYSTEMOFF/RESET SMC handlers |
| uioxfwsecboot.c | SoC | SHA-256, RSA/ECDSA verify, anti-rollback chain-of-trust |
| uioxfwpower.c | SoC | PSCI ARM / ACPI x86 / SBI RISC-V power (reset, shutdown, CPU hot-plug) |
| uioxfwmem.c | SoC | no-libc memset/memcpy/memcmp, bump allocator, DDR region table |
| uioxfwriscv.c | SoC | RISC-V HAL vtable — uioxfwhwopst for rv64: PLIC IRQ, SBI timer/power |
| uioxfwpost.c | SoC | POST test runner — CPU sanity, DRAM walk, ROM CRC, UART smoke |

Peripheral Source Files

| File | Category | What It Implements |
|---|---|---|
| uioxfwuart.c | Peripheral | PL011 baud divisor, 16550A port-IO init, putc/getc, RX ISR |
| uioxfwspi.c | Peripheral | PL022 SSP CR0/CR1, CPOL/CPHA, CS assert/release, transfer loop |
| uioxfwtimer.c | Peripheral | SP804 load/ctrl, PIT 8254 divisor, ARM Generic Timer CSR, CLINT settimer |
| uioxfwsd.c | Peripheral | SD CMD sequence, card init, block read/write |
| uioxfwnet.c | Peripheral | VirtIO-net send/receive, SMSC LAN9118 register access |
| uioxfwnvme.c | Peripheral | NVMe Admin queue, identify, IO read/write submit/complete |
| uioxfwpcie.c | Peripheral | ECAM config read/write, BAR probe and assign, device scan |
| uioxfwusb.c | Peripheral | xHCI USBCMD reset, port init, control transfer |
| uioxfwsata.c | Peripheral | AHCI GHC enable, port detect, FIS construct, DMA read/write |
| uioxfwstorage.c | Peripheral | Unified block ops — dispatches to SD/eMMC/NVMe/SATA backend |
| uioxfwthermal.c | Peripheral | I2C LM75/NCT7802 temp read, trip-point callback |
| uioxfwwdt.c | Peripheral | SP805 load/enable/kick, iTCO unlock+set |
| uioxfwpmic.c | Peripheral | DA9062/RK808 I2C rail voltage set, power sequencing |
| uioxfwsensor.c | Peripheral | BMI270/TSL2591/BMP390 init, read, interrupt config |
| uioxfwwifi.c | Peripheral | Firmware load, scan, connect/disconnect, RX callback |
| uioxfwspeaker.c | Peripheral | I2S DAC init, volume, mute, play buffer |
| uioxfwmic.c | Peripheral | PDM/I2S start/stop, frame read |
| uioxfwmouse.c | Peripheral | PS/2 byte decode, USB HID report parse, event callback |
| uioxfwramrtc.c | Peripheral | DS1307/MC146818 time read/write, NVRAM access |
| uioxfwgpio.c | Peripheral | Direction set, read/write, IRQ enable/ISR dispatch |
| uioxfw_dma.c | Peripheral | PL080/PL330 channel alloc, transfer submit, ISR |

One-Line Rule

> SoC file = affects the whole chip, must run before any device can be touched.
> Peripheral file = configures exactly one device, requires SoC init to have run first.
=====================================
	SoC	Architecture
What it is	A specific chip product	An instruction set + CPU design rules
Who defines it	The chip manufacturer (Broadcom, NXP, SiFive, Intel)	The ISA owner (ARM Ltd, RISC-V International, Intel/AMD)
Examples	BCM2711, RK3588, i.MX8, SiFive U74, Intel Q35	ARMv8-A (AArch64), ARMv7-A, AMD64, RV64IMAFDC
Scope	One specific product — peripherals, memory map, clock tree	All chips that implement the ISA
Changes per chip	UART base address, PLL divisors, IRQ numbers, DDR config	Never — the ISA is fixed across all chips that use it
UIOX module	02_FwHal/src/uiox_soc_*.c	10_Arch/<arch>/src/arch_init.c
================
Great question. Here is the complete, precise difference between SoC and Architecture in the context of UIOX and embedded systems generally.

SoC vs Architecture — The Core Difference

| | SoC | Architecture |
|---|---|---|
| What it is | A specific chip product | An instruction set + CPU design rules |
| Who defines it | The chip manufacturer (Broadcom, NXP, SiFive, Intel) | The ISA owner (ARM Ltd, RISC-V International, Intel/AMD) |
| Examples | BCM2711, RK3588, i.MX8, SiFive U74, Intel Q35 | ARMv8-A (AArch64), ARMv7-A, AMD64, RV64IMAFDC |
| Scope | One specific product — peripherals, memory map, clock tree | All chips that implement the ISA |
| Changes per chip | UART base address, PLL divisors, IRQ numbers, DDR config | Never — the ISA is fixed across all chips that use it |
| UIOX module | 02FwHal/src/uioxsoc.c | 10Arch/<arch>/src/archinit.c |

The Clearest Way to Think About It

``
Architecture  =  The CPU instruction set + core design rules
                 (what instructions exist, how registers work,
                  how exceptions are taken, how the MMU works)

SoC           =  Architecture + everything else on the chip
                 (DRAM controller, clock tree, USB, PCIe,
                  UART, GPIO, interrupt controller, power domains)
`

Think of it this way:

`
Architecture (ARMv8-A)
      │
      │  defines:  A64 instruction set, EL0-EL3 privilege levels,
      │            SCTLREL1, TTBR0/TTBR1, VBAREL1, DAIF,
      │            generic timer (CNTPCTEL0/CNTFRQEL0),
      │            exception model, cache maintenance ops
      │
      ├──▶ SoC A:  BCM2711 (Raspberry Pi 4)
      │              UART0 @ 0xFE201000
      │              GIC-400 @ 0xFF841000
      │              PCIe @ 0x7D500000
      │              40-pin GPIO header
      │              VideoCore VI GPU
      │
      ├──▶ SoC B:  RK3588 (Rock Pi)
      │              UART0 @ 0xFEB50000
      │              GIC-600 @ 0xFE600000
      │              PCIe 3.0 @ 0xF5000000
      │              NPU, ISP, VPU
      │
      └──▶ SoC C:  QEMU virt (simulation)
                     UART0 @ 0x09000000
                     GIC-400 @ 0x08000000
                     VirtIO @ 0x0A000000
`

Same architecture (ARMv8-A), three completely different SoCs. All three use the exact same A64 instruction set, the same MRS SCTLREL1 to enable the MMU, the same TLBI VMALLE1IS to flush the TLB. But their UART is at a different address, their GIC is a different version, their clock tree is wired differently, and their peripherals are completely different.

Concrete Examples from UIOX
Architecture defines these — same on EVERY ARMv8-A chip:

`c
/ 10Arch/arm64/include/archdefs.h /

/ These are ARM Architecture constants — same on BCM2711, RK3588, IMX8, QEMU /
#define PSTATEEL1h          0x05u    / exception level 1, SPEL1           /
#define PSTATEIBIT         (1u<<7)  / IRQ mask bit in DAIF                /

/ These are Architecture barrier instructions — same on all ARMv8-A chips   /
#define archmb()   asm volatile("dmb sy"  ::: "memory")
#define archisb()  asm volatile("isb"     ::: "memory")
#define archwfi()  asm volatile("wfi")

/ Enable MMU — same instruction on every ARMv8-A chip /
/ MSR SCTLREL1, x0  (bit 0 = M = MMU enable)        /
`

SoC defines these — different on every chip:

`c
/ 02FwHal/include/uioxsocmap.h /

/ QEMU virt — these addresses ONLY apply to this specific SoC /
#define SOCDRAMBASE          0x40000000UL   / BCM2711 uses 0x00000000     /
#define SOCGICDISTBASE      0x08000000UL   / RK3588 uses 0xFE600000      /
#define SOCUART0BASE         0x09000000UL   / BCM2711 uses 0xFE201000     /
#define SOCCLINTBASE         0x02000000UL   / Only on RISC-V SoCs         /

/ BCM2711 (Raspberry Pi 4) — completely different from QEMU /
/ #define SOCDRAMBASE       0x00000000UL                                  /
/ #define SOCGICDISTBASE   0xFF841000UL                                  /
/ #define SOCUART0BASE      0xFE201000UL                                  /
`

The Five Key Differences Explained
1 · Instruction Set vs Silicon

Architecture is the contract between software and hardware — it specifies exactly what instructions exist and what they do. ADD X0, X1, X2 adds two registers. This is true on every ARMv8-A chip ever made.

SoC is the actual silicon that implements that contract, plus adds its own set of peripherals at its own addresses. Two SoCs can both be ARMv8-A but have completely different memory maps.

2 · Stable vs Variable

Architecture never changes for a given ISA version. ARMv8-A Cortex-A53 from 2013 and Cortex-A76 from 2018 both use the same MSR SCTLREL1 instruction to enable the MMU. The architecture is fixed.

SoC varies enormously between chips and even between chip revisions. BCM2711 rev B0 has different errata workarounds from rev C0.

3 · Who Knows What

Architecture init (archinit.c) needs to know:
• What privilege level to set (EL1, SVC, S-mode)
• What system registers control the MMU (SCTLREL1, CR0, satp)
• How to flush the TLB (TLBI VMALLE1IS, INVLPG, SFENCE.VMA)
• How to enable/disable interrupts (DAIF, CPSR, RFLAGS, sstatus)

SoC init (uioxsoc.c) needs to know:
• What physical address is the GIC/APIC/PLIC at
• What PLL multiplier gives 1.5 GHz on this chip
• What power domain register enables DDR
• What IRQ number does UART0 use on this SoC

4 · Universal vs Chip-Specific Code

`c
/ Architecture code — compiles identically for BCM2711, RK3588, IMX8 /
void enablemmu(void)
{
    uint64t sctlr;
    asm volatile("mrs %0, sctlrel1" : "=r"(sctlr));
    sctlr |= (1u << 0);   / M bit — same on ALL ARMv8-A chips /
    asm volatile("msr sctlrel1, %0" :: "r"(sctlr) : "memory");
    asm volatile("isb");
}

/ SoC code — completely different per chip /
void uartinitqemuvirt(void)
{
    mmiowrite32(0x09000000UL + 0x024u, 13u);  / IBRD — QEMU virt only    /
}

void uartinitrpi4(void)
{
    mmiowrite32(0xFE201000UL + 0x024u, 13u);  / IBRD — BCM2711 (RPi4)    /
}
`

5 · In UIOX Source Tree

`
10Arch/              ← ARCHITECTURE layer
│                        Same code for all SoCs that share the ISA
│
├── arm64/
│   ├── include/archdefs.h    ← ARMv8-A ISA constants (same on ALL ARM64 SoCs)
│   └── src/archinit.c        ← CPU core setup (MMU, cache, DAIF, VBAR, GIC CPU iface)
│
├── arm32/
│   ├── include/archdefs.h    ← ARMv7-A ISA constants
│   └── src/archinit.c
│
├── x8664/
│   ├── include/archdefs.h    ← AMD64 constants (CR0, CR3, EFER, RFLAGS)
│   └── src/archinit.c
│
└── riscv64/
    ├── include/archdefs.h    ← RV64 CSR names, CLINT/PLIC offsets
    └── src/archinit.c

02FwHal/             ← SoC layer
│                        Different per chip / board
│
├── include/
│   ├── uioxsocmap.h         ← MMIO addresses (CHANGE per SoC)
│   ├── uioxsocclk.h         ← Clock tree (CHANGE per SoC)
│   ├── uioxsocpm.h          ← Power domains (CHANGE per SoC)
│   └── uioxsoctypes.h       ← SoC ID enum (one entry per SoC product)
│
└── src/
    ├── uioxsocarm64.c       ← BCM2711 / RK3588 / IMX8 / QEMU detect + init
    ├── uioxsocarm32.c       ← BCM2836 / IMX6 / OMAP detect + init
    ├── uioxsocx86.c         ← QEMU Q35 / generic x86 detect + init
    └── uioxsocriscv64.c     ← SiFive U74 / QEMU virt detect + init
`

Summary in One Table

| Question | Architecture Answer | SoC Answer |
|---|---|---|
| What CPU instruction enables the MMU? | MSR SCTLREL1 (same on all ARM64) | N/A — SoC doesn't define instructions |
| Where is the UART? | Defined by the SoC, not the architecture | 0x09000000 (QEMU), 0xFE201000 (RPi4) |
| How do you flush the TLB? | TLBI VMALLE1IS (same on all ARM64) | N/A |
| What clock speed is the CPU? | Architecture defines max; actual speed is SoC | 1.5 GHz (BCM2711), 2.4 GHz (BCM2712) |
| What IRQ number is the UART? | Architecture defines IRQ model; actual number is SoC | 33 (QEMU virt), 153 (BCM2711) |
| How many privilege levels? | 4 (EL0–EL3 for ARM64) — architecture defines this | N/A |
| Can two chips share arch init code? | Yes — all ARMv8-A chips use the same archinit.c | No — BCM2711 and RK3588 need different uioxsoc*.c` |
=========================================
SoC	File	Status
QEMU_VIRT_A64	uiox_soc_arm64.c	✅ existing
BCM2711	uiox_soc_arm64.c	✅ existing
BCM2712	uiox_soc_arm64.c	✅ existing
IMX8MP	uiox_soc_imx8mp.c	✅ new
RK3588	uiox_soc_rk3588.c	✅ new
QEMU_VIRT_A32	uiox_soc_arm32.c	✅ existing
BCM2836	uiox_soc_arm32.c	✅ existing
IMX6Q	uiox_soc_arm32.c	✅ existing
OMAP4430	uiox_soc_omap4430.c	✅ new
X86_QEMU_Q35	uiox_soc_x86.c	✅ existing
X86_QEMU_I440	uiox_soc_x86.c	✅ covered (same code path)
X86_GENERIC	uiox_soc_x86.c	✅ covered (default path)
QEMU_VIRT_RV64	uiox_soc_riscv64.c	✅ existing
SIFIVE_U74	uiox_soc_riscv64.c	✅ existing
TH1520	uiox_soc_th1520.c	✅ new