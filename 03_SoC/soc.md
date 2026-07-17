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