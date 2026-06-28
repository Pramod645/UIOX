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