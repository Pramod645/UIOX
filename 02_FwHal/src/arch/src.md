File	Category	What It Implements
uiox_fw_uart.c	Peripheral	PL011 baud divisor, 16550A port-IO init, putc/getc, RX ISR
uiox_fw_spi.c	Peripheral	PL022 SSP CR0/CR1, CPOL/CPHA, CS assert/release, transfer loop
uiox_fw_timer.c	Peripheral	SP804 load/ctrl, PIT 8254 divisor, ARM Generic Timer CSR, CLINT set_timer
uiox_fw_sd.c	Peripheral	SD CMD sequence, card init, block read/write
uiox_fw_net.c	Peripheral	VirtIO-net send/receive, SMSC LAN9118 register access
uiox_fw_nvme.c	Peripheral	NVMe Admin queue, identify, IO read/write submit/complete
uiox_fw_pcie.c	Peripheral	ECAM config read/write, BAR probe and assign, device scan
uiox_fw_usb.c	Peripheral	xHCI USBCMD reset, port init, control transfer
uiox_fw_sata.c	Peripheral	AHCI GHC enable, port detect, FIS construct, DMA read/write
uiox_fw_storage.c	Peripheral	Unified block ops — dispatches to SD/eMMC/NVMe/SATA backend
uiox_fw_thermal.c	Peripheral	I2C LM75/NCT7802 temp read, trip-point callback
uiox_fw_wdt.c	Peripheral	SP805 load/enable/kick, iTCO unlock+set
uiox_fw_pmic.c	Peripheral	DA9062/RK808 I2C rail voltage set, power sequencing
uiox_fw_sensor.c	Peripheral	BMI270/TSL2591/BMP390 init, read, interrupt config
uiox_fw_wifi.c	Peripheral	Firmware load, scan, connect/disconnect, RX callback
uiox_fw_speaker.c	Peripheral	I2S DAC init, volume, mute, play buffer
uiox_fw_mic.c	Peripheral	PDM/I2S start/stop, frame read
uiox_fw_mouse.c	Peripheral	PS/2 byte decode, USB HID report parse, event callback
uiox_fw_ramrtc.c	Peripheral	DS1307/MC146818 time read/write, NVRAM access
uiox_fw_gpio.c	Peripheral	Direction set, read/write, IRQ enable/ISR dispatch
uiox_fw_dma.c	Peripheral	PL080/PL330 channel alloc, transfer submit, ISR