## 4_RISCV_Map

| Peripheral | Block / Function | Base Address | End Address | Size (hex) | IRQ (PLIC source#) | Bus | Access Width | Clock Source | Source File | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| CLINT | Core-Local Interruptor | 0x02000000 | 0x0200FFFF | 0x10000 | — | AXI | 64 | — | cpu_hw / DTB_rv | clint_base field; mtime@0xBFF8; mtimecmp0@0x4000; IPI msip@0x0 |
| CLINT msip0 | Machine SW IRQ hart 0 | 0x02000000 | 0x02000003 | 0x4 | — | AXI | 32 | — | cpu_hw | Write 1→trigger MIP.MSIP (software IPI); ipi_send vtable |
| CLINT msip1 | Machine SW IRQ hart 1 | 0x02000004 | 0x02000007 | 0x4 | — | AXI | 32 | — | cpu_hw |  |
| CLINT msip2 | Machine SW IRQ hart 2 | 0x02000008 | 0x0200000B | 0x4 | — | AXI | 32 | — | cpu_hw |  |
| CLINT msip3 | Machine SW IRQ hart 3 | 0x0200000C | 0x0200000F | 0x4 | — | AXI | 32 | — | cpu_hw |  |
| CLINT mtimecmp0 | Machine timer compare h0 | 0x02004000 | 0x02004007 | 0x8 | — | AXI | 64 | — | cpu_hw | isr_timer vtable fires when mtime >= mtimecmp |
| CLINT mtimecmp1 | Machine timer compare h1 | 0x02004008 | 0x0200400F | 0x8 | — | AXI | 64 | — | cpu_hw |  |
| CLINT mtimecmp2 | Machine timer compare h2 | 0x02004010 | 0x02004017 | 0x8 | — | AXI | 64 | — | cpu_hw |  |
| CLINT mtimecmp3 | Machine timer compare h3 | 0x02004018 | 0x0200401F | 0x8 | — | AXI | 64 | — | cpu_hw |  |
| CLINT mtime | Global machine timer | 0x0200BFF8 | 0x0200BFFF | 0x8 | — | AXI | 64 | 1 MHz ref | cpu_hw | timebase-frequency=1000000; uiox_cpu_csr_read(mtime) |
| PLIC | Platform-Level Interrupt Ctrl | 0x0C000000 | 0x0FFFFFFF | 0x4000000 | — | AXI | 32 | — | cpu_hw / DTB_rv | gic_base field used for PLIC on RV64; 64 sources; 4 harts × 2 ctxs |
| PLIC Priority base | Source priority regs | 0x0C000000 | 0x0C0000FF | 0x100 | — | AXI | 32 | — | cpu_hw | 4 B per source; 0=disabled; 1..7 priority |
| PLIC Pending base | Pending bits | 0x0C001000 | 0x0C00107F | 0x80 | — | AXI | 32 | — | cpu_hw | Read-only; bit n=source n pending |
| PLIC Enable ctx0 (M) | Hart0 M-mode enables | 0x0C002000 | 0x0C00207F | 0x80 | — | AXI | 32 | — | cpu_hw | Bit n=enable source n for this ctx |
| PLIC Enable ctx1 (S) | Hart0 S-mode enables | 0x0C002080 | 0x0C0020FF | 0x80 | — | AXI | 32 | — | cpu_hw |  |
| PLIC Threshold ctx0 | Hart0 M threshold | 0x0C200000 | 0x0C200003 | 0x4 | — | AXI | 32 | — | cpu_hw |  |
| PLIC Claim/Complete ctx0 | Hart0 M claim/cmp | 0x0C200004 | 0x0C200007 | 0x4 | — | AXI | 32 | — | cpu_hw | Read=highest-prio IRQ; Write=complete |
| UART0 (SiFive) | Primary console | 0x10010000 | 0x10010FFF | 0x1000 | PLIC 4 | APB | 32 | periph_clk (125 MHz) | DTB_rv | 115200 n8; txdata@0; rxdata@4; txctrl@8; rxctrl@C; ie@10; ip@14; div@18 |
| UART1 (SiFive) | Secondary | 0x10011000 | 0x10011FFF | 0x1000 | PLIC 5 | APB | 32 | periph_clk | DTB_rv | disabled |
| SPI0 / QSPI | QSPI NOR flash + Wi-Fi SPI | 0x10040000 | 0x10040FFF | 0x1000 | PLIC 51 | APB | 32 | periph_clk | wifi_hw / DTB_rv | CS0: SPI NOR (KRL/mlog); sifive,spi0 compat; 50 MHz max |
| SDMMC0 | eMMC / SD + CYW43xx SDIO | 0x10050000 | 0x1005FFFF | 0x10000 | PLIC 52 | AHB | 32 | bus_clk (500 MHz) | wifi_hw / DTB_rv | 4-bit SDIO IRQ; cap-sdio-irq; CYW43xx vtable |
| I2C0 | 400 kHz; thermal sensor | 0x10060000 | 0x10060FFF | 0x1000 | PLIC 53 | APB | 32 | periph_clk | therm_hw / DTB_rv | Sensor @ 0x48; sifive,i2c0 |
| GPIO0 | 16-bit GPIO + IRQ | 0x10070000 | 0x10070FFF | 0x1000 | PLIC 7–22 | APB | 32 | periph_clk | DTB_rv | sifive,gpio0; 16 GPIO lines; IRQ per pin |
| Watchdog | HW watchdog | 0x10080000 | 0x10080FFF | 0x1000 | PLIC 61 | APB | 32 | periph_clk | DTB_rv | sifive,wdog0; 30 s timeout |
| RTC | Real-time clock | 0x10090000 | 0x10090FFF | 0x1000 | PLIC 62 | APB | 32 | periph_clk | DTB_rv | uiox,uiox-rtc |
| USB3 xHCI (OTG) | USB 3.0 Host+OTG | 0x10100000 | 0x1010FFFF | 0x10000 | PLIC 54 | AHB | 32 | bus_clk | usb_hw / DTB_rv | FS/HS/SS; DMA TX/RX rings; EP table; VBUS; usb.md |
| Crypto Engine | SHA-256/384, RSA, ECDSA | 0x10200000 | 0x1020FFFF | 0x10000 | PLIC 60 | AHB | 32 | bus_clk | ksign_img / DTB_rv | ksign-hw-accel flag; alg IDs 1-4 (uiox_ks_alg_t) |
| PCIe Host | PCIe 3.0 ×4; AX200 + TB4 | 0x30000000 | 0x3FFFFFFF | 0x10000000 | PLIC 56,57 | AXI | 32 | — | DTB_rv | sifive,fu740-pcie; I/O@0x60000000; MEM@0x61000000 |
| PCIe I/O window | Translated PCIe I/O | 0x60000000 | 0x60FFFFFF | 0x1000000 | — | PCIe | 32 | — | DTB_rv |  |
| PCIe MEM window | Translated PCIe memory | 0x61000000 | 0x7FFFFFFF | 0x1F000000 | — | PCIe | 32 | — | DTB_rv | TB4 + Intel AX200 |
| CSR mstatus | Machine status | CSR 0x300 | — | 64-bit | — | CSR | 64 | — | cpu_hw | MIE/MPIE/MPP bits; uiox_cpu_csr_read/write |
| CSR mie | Machine interrupt enable | CSR 0x304 | — | 64-bit | — | CSR | 64 | — | cpu_hw | MSIE/MTIE/MEIE bits; isr_timer/isr_ipi hooks |
| CSR mip | Machine interrupt pending | CSR 0x344 | — | 64-bit | — | CSR | 64 | — | cpu_hw | Read: pending sources |
| CSR mhartid | Hardware thread ID | CSR 0xF14 | — | 64-bit | — | CSR | 64 | — | cpu_hw | uiox_cpu_this_id() = csr_read(mhartid)&0xFF |
| CSR mcycle | Machine cycle counter | CSR 0xB00 | — | 64-bit | — | CSR | 64 | — | cpu_feat | PMU event; uiox_cpu_hw_timestamp() |
| CSR minstret | Machine instr-retired counter | CSR 0xB02 | — | 64-bit | — | CSR | 64 | — | cpu_feat | UIOX_PMU_MAX_COUNTERS=8 includes this |
| CSR satp | Supervisor Addr Trans. & Prot. | CSR 0x180 | — | 64-bit | — | CSR | 64 | — | cpu_hw | MODE=8 → SV39; MODE=9 → SV48; ASID + PPN |
