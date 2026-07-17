File	Category	What It Defines
uiox_fw_uart.h	Peripheral	PL011 / 16550A / NS16550A UART: baud, parity, FIFO, RTS/CTS, uiox_fw_uart_t, uiox_fw_uart_init/putc/getc/irq()
uiox_fw_spi.h	Peripheral	PL022 SSP / Intel ICH SPI: CPOL/CPHA modes, CS0–3, uiox_spi_dev_t, uiox_fw_spi_init/transfer/write/read()
uiox_fw_i2c.h	Peripheral	DesignWare APB / Intel SMBus: 100K/400K/1M/3.4M speeds, uiox_i2c_dev_t, uiox_fw_i2c_write/read/transfer()
uiox_fw_timer.h	Peripheral	SP804 / PIT 8254 / ARM Generic Timer / RISC-V CLINT: uiox_fw_timer_t, uiox_fw_timer_init/start/stop/ticks()
uiox_fw_gpio.h	Peripheral	GPIO controller: direction/pull/IRQ modes, 128 pins, uiox_fw_gpio_t, uiox_fw_gpio_set_dir/write/read/irq_en()
uiox_fw_wdt.h	Peripheral	Watchdog timer SP805/iTCO: load/kick/disable, timeout formula, hardware reset on expiry
uiox_fw_pcie.h	Peripheral	PCIe ECAM: config space read/write, BAR assignment, device scan, uiox_pcie_ctrl_t, uiox_pcie_dev_t
uiox_fw_usb.h	Peripheral	xHCI/EHCI/OHCI host: capability registers, USBCMD/USBSTS, port reset, enumerate, ctrl/bulk transfer
uiox_fw_eth.h	Peripheral	Ethernet MAC: VirtIO-net / SMSC LAN9118 / RTL8139, uiox_eth_dev_t, send/receive/ISR
uiox_fw_net.h	Peripheral	Network HAL: VirtIO-net / SMSC / e1000, MAC frame, RX callback, link status
uiox_fw_sd.h	Peripheral	SD/SDHC/SDXC SDIO host: CMD0/8/17/24/ACMD41, 4-bit bus, card detect, uiox_sd_dev_t
uiox_fw_emmc.h	Peripheral	eMMC 5.1 HS400/HS200: HC registers, boot/RPMB partitions, 8-bit bus, uiox_emmc_dev_t
uiox_fw_sata.h	Peripheral	SATA III AHCI: GHC/PI/IS registers, port map, ATA_CMD_IDENTIFY/READ_DMA/WRITE_DMA
uiox_fw_nvme.h	Peripheral	NVMe M.2 PCIe BAR0: CAP/CC/CSTS registers, Admin+IO queues, identify/read/write/trim
uiox_fw_storage.h	Peripheral	Block storage HAL: unified interface over SD/eMMC/NVMe/SATA
uiox_fw_wifi.h	Peripheral	Wi-Fi 6 SDIO/PCIe: 802.11n/ac/ax, firmware load, scan/connect/disconnect, uiox_wifi_dev_t
uiox_fw_bt.h	Peripheral	Bluetooth HCI transport: UART/USB, firmware patch load, HCI send/receive, GPIO reset/pwren pins
uiox_fw_gpu.h	Peripheral	VirtIO-GPU / Mali framebuffer: resource create, scanout, flush, uiox_gpu_dev_t
uiox_fw_hdmi.h	Peripheral	HDMI output: DDC/EDID read, mode set, HPD interrupt
uiox_fw_monitor.h	Peripheral	Display monitor EDID/DDC over I2C: uiox_monitor_dev_t, read_edid, get preferred mode
uiox_fw_camera.h	Peripheral	MIPI CSI-2 camera: OV5640/IMX219 I2C config, lane enable, uiox_camera_dev_t
uiox_fw_sensor.h	Peripheral	Sensor HAL: BMI270 IMU, TSL2591 ALS, BMP390 pressure — I2C/SPI, uiox_fw_sens_type_t
uiox_fw_als.h	Peripheral	Ambient light sensor: VEML7700/OPT3001 over I2C, lux×1000, auto-gain, interrupt threshold
uiox_fw_thermal.h	Peripheral	Thermal sensor + trip points: LM75/NCT7802 over I2C, zone descriptors, fan PWM
uiox_fw_fan.h	Peripheral	PWM fan + tach RPM: timer PWM output, duty cycle 0–100%, min_rpm fault
uiox_fw_pmic.h	Peripheral	PMIC over I2C: DA9062/RK808/ACT8865, BUCK/LDO rails, voltage set, power sequencing
uiox_fw_charger.h	Peripheral	USB-C PD / barrel charger: BQ25895/FUSB302, VBUS/VBAT/ICHG ADC, OTG enable
uiox_fw_bms.h	Peripheral	Battery management: BQ27742 over I2C, SOC%, remaining capacity, avg current
uiox_fw_ramrtc.h	Peripheral	Battery-backed RTC + NVRAM: DS1307/DS3231/MC146818, time read/write, 56-byte NVRAM
uiox_fw_speaker.h	Peripheral	I2S audio DAC + amplifier: BCLK/LRCK/DATA pins, sample rate, volume, mute
uiox_fw_mic.h	Peripheral	MEMS microphone PDM/I2S: CLK/DATA pins, sample rate, channel count
uiox_fw_mouse.h	Peripheral	PS/2 / USB HID mouse: dx/dy/dz event, button bits, IRQ handler
uiox_fw_keyboard.h	Peripheral	PS/2 / USB HID keyboard: scan codes, key event callback
uiox_fw_touchpwd.h	Peripheral	Fingerprint sensor over I2C: FPC1020/Goodix, enroll/verify, 10 template slots, INT/RESET GPIO
uiox_fw_tb4.h	Peripheral	Thunderbolt 4 / USB4 (Intel JHL8540): NHI BAR0, ICM messaging, device approve, hotplug
uiox_fw_nvme.h	Peripheral	(listed above)
uiox_fw_devsw.h	Peripheral	Device switch / power button: GPIO debounce, long-press detect, callback