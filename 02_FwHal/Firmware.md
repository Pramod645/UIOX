00_Firmware/
├── include/
│   ├── uiox_fw_types.h        # Base types, error codes, magic numbers
│   ├── uiox_fw_hw.h           # HW HAL: MMIO, vtable, platform ops
│   ├── uiox_fw_irq.h          # IRQ controller abstraction (GIC/8259A/LAPIC)
│   ├── uiox_fw_uart.h         # UART driver (PL011 / 16550)
│   ├── uiox_fw_timer.h        # Timer driver (SP804 / PIT / ARM generic)
│   ├── uiox_fw_gpio.h         # GPIO abstraction
│   ├── uiox_fw_clock.h        # Clock / PLL management
│   ├── uiox_fw_power.h        # Power management (PSCI / ACPI S-states)
│   ├── uiox_fw_mem.h          # Physical memory map, MPU/MMU early init
│   ├── uiox_fw_storage.h      # Block storage HAL (SD/eMMC/NVMe/SATA)
│   ├── uiox_fw_net.h          # Network MAC/PHY HAL
│   ├── uiox_fw_devsw.h        # Device switch table (char + block)
│   ├── uiox_fw_sensor.h       # Sensor driver interface
│   └── uiox_fw.h              # Master include
├── src/
│   ├── arch/
│   │   ├── arm64/
│   │   │   ├── uiox_fw_arch_arm64.c   # ARM64 HW ops implementation
│   │   │   └── uiox_fw_arch_arm64.h   # ARM64 register defs
│   │   ├── arm32/
│   │   │   ├── uiox_fw_arch_arm32.c   # ARM32 HW ops implementation
│   │   │   └── uiox_fw_arch_arm32.h   # ARM32 register defs
│   │   └── x86_64/
│   │       ├── uiox_fw_arch_x86.c     # x86_64 HW ops implementation
│   │       └── uiox_fw_arch_x86.h     # x86_64 register defs
│   ├── uiox_fw_irq.c
│   ├── uiox_fw_uart.c
│   ├── uiox_fw_timer.c
│   ├── uiox_fw_gpio.c
│   ├── uiox_fw_clock.c
│   ├── uiox_fw_power.c
│   ├── uiox_fw_mem.c
│   ├── uiox_fw_storage.c
│   ├── uiox_fw_net.c
│   ├── uiox_fw_devsw.c
│   ├── uiox_fw_sensor.c
│   └── uiox_fw_main.c         # 8-stage firmware init pipeline
└── Makefile
===================================
File	Layer	Maps to UIOX repo
uiox_fw_types.h	Types	Shared base — mirrors uiox_boot_types.h
uiox_fw_hw.h/.c + arch/*/uiox_fw_arch_*.c	HAL (18-op vtable)	10_Arch/arm64,arm32,x86_64
uiox_fw_irq.h/.c	IRQ manager	10_Arch/*/arch_init.c GIC/8259A/LAPIC
uiox_fw_uart.h/.c	UART driver	10_Arch/*/arch_defs.h PL011/16550
uiox_fw_timer.h/.c	Timer driver	10_Arch/*/arch_defs.h SP804/PIT/ARM-GT
uiox_fw_gpio.h/.c	GPIO driver	30_DeviceDrivers/04_NonSecnsors
uiox_fw_clock.h	Clock/PLL	70_build_config/70_CPU_SoC
uiox_fw_power.h/.c	Power management	PSCI/ACPI — 10_Arch/*/arch_init.c
uiox_fw_mem.h/.c	Memory map	80_linker/uiox_arm64.ld physical layout
uiox_fw_storage.h/.c	Block storage	30_DeviceDrivers/01_block/BlockDrivers.h
uiox_fw_net.h	Network HAL	30_DeviceDrivers/04_NonSecnsors
uiox_fw_devsw.h/.c	Device switch	30_DeviceDrivers/iossytem.md devsw.h
uiox_fw_sensor.h/.c	Sensor HAL	30_DeviceDrivers/03_Sensors / 20_DriverInterfaces/02_Sensors
uiox_fw_main.c	8-stage pipeline	main.c 8-stage integration in uiox.md
============================================================================
Module	File pair	What it adds
POST	uiox_fw_post.h/c	8 hardware self-tests run before boot: CPU, D-cache, RAM march, UART TX, timer, IRQ controller, storage presence, SHA-256 crypto self-test. Returns pass/fail per test; prints formatted report.
Secure Boot	uiox_fw_secboot.h/c	SHA-256 image integrity (full FIPS 180-4 implementation, no libgcc), Ed25519 signature verification (stub ready for Monocypher), trusted key ring, TPM PCR measurement extension, three security levels (OFF / HASH / SIGN / MEASURED).
TrustZone	uiox_fw_tz.h/c	EL3 probe, SCR_EL3 / CPTR_EL3 / ACTLR_EL3 configuration, TZASC memory region programming, GIC Group 0/1 assignment, uiox_fw_tz_drop_to_el1() for ERET into Normal World. Gracefully stubs on ARM32 and x86-64.
PSCI	uiox_fw_psci.h/c	Full PSCI v1.1 SMC dispatcher: PSCI_VERSION, CPU_ON, CPU_OFF, CPU_SUSPEND, AFFINITY_INFO, SYSTEM_OFF, SYSTEM_RESET, SYSTEM_RESET2, PSCI_FEATURES, MEM_PROTECT. Per-CPU affinity state table, SMC call counter, QEMU SYSTEM_OFF/RESET stubs.
================
File tree (new additions to 02_FwHal/)
02_FwHal/
├── include/
│   ├── uiox_fw_als.h        # Ambient Light Sensor (VEML7700/OPT3001)
│   ├── uiox_fw_bms.h        # Battery Management System (BQ27742/MAX17055)
│   ├── uiox_fw_bt.h         # Bluetooth HCI transport (UART/USB)
│   ├── uiox_fw_camera.h     # MIPI CSI-2 camera sensor (OV5640)
│   ├── uiox_fw_charger.h    # USB-C PD / barrel charger (BQ25895)
│   ├── uiox_fw_emmc.h       # eMMC 5.1 SDIO host (HS400)
│   ├── uiox_fw_eth.h        # Ethernet MAC/PHY (SMSC LAN9118/VirtIO)
│   ├── uiox_fw_fan.h        # PWM fan controller
│   ├── uiox_fw_gpu.h        # GPU / display engine (VirtIO-GPU/Mali)
│   ├── uiox_fw_hdmi.h       # HDMI transmitter (DesignWare HDMI)
│   ├── uiox_fw_keyboard.h   # PS/2 + USB HID keyboard
│   ├── uiox_fw_mic.h        # MEMS microphone (PDM/I2S)
│   ├── uiox_fw_monitor.h    # Display monitor (EDID / DDC)
│   ├── uiox_fw_mouse.h      # PS/2 + USB HID mouse
│   ├── uiox_fw_nvme.h       # NVMe M.2 PCIe (BAR0 MMIO)
│   ├── uiox_fw_pmic.h       # PMIC (DA9062 / RK808)
│   ├── uiox_fw_ramrtc.h     # Battery-backed SRAM + RTC (DS1307/MC146818)
│   ├── uiox_fw_sata.h       # SATA III AHCI controller
│   ├── uiox_fw_sd.h         # SD/SDHC/SDXC host (SDIO 4-bit)
│   ├── uiox_fw_speaker.h    # I2S audio DAC + amplifier
│   ├── uiox_fw_tb4.h        # Thunderbolt 4 / USB4 (JHL8540)
│   ├── uiox_fw_thermal.h    # Thermal sensor + trip points
│   ├── uiox_fw_touchpwd.h   # Fingerprint / touch password sensor
│   ├── uiox_fw_usb.h        # USB host controller (XHCI/EHCI)
│   └── uiox_fw_wifi.h       # Wi-Fi 6 (SDIO/PCIe)
└── src/
    ├── uiox_fw_als.c
    ├── uiox_fw_bms.c
    ├── uiox_fw_bt.c
    ├── uiox_fw_camera.c
    ├── uiox_fw_charger.c
    ├── uiox_fw_emmc.c
    ├── uiox_fw_eth.c
    ├── uiox_fw_fan.c
    ├── uiox_fw_gpu.c
    ├── uiox_fw_hdmi.c
    ├── uiox_fw_keyboard.c
    ├── uiox_fw_mic.c
    ├── uiox_fw_monitor.c
    ├── uiox_fw_mouse.c
    ├── uiox_fw_nvme.c
    ├── uiox_fw_pmic.c
    ├── uiox_fw_ramrtc.c
    ├── uiox_fw_sata.c
    ├── uiox_fw_sd.c
    ├── uiox_fw_speaker.c
    ├── uiox_fw_tb4.c
    ├── uiox_fw_thermal.c
    ├── uiox_fw_touchpwd.c
    ├── uiox_fw_usb.c
    └── uiox_fw_wifi.c
