uiox-uart/
├── include/
│   ├── uiox_uart_hw.h        # Layer 1 — HAL: PL011/16550 registers, vtable
│   ├── uiox_uart_buf.h       # TX/RX ring buffer pool
│   ├── uiox_uart_if.h        # Layer 2 — Interface: framing, parity, flow ctrl
│   ├── uiox_uart_proto.h     # Layer 3 — Protocol: line discipline, ANSI, break
│   ├── uiox_uart_subsys.h    # Layer 4 — Subsystem: TTY, console, events
│   └── uiox_uart_device.h    # Layer 5 — Application-facing API
└── src/
    ├── uiox_uart_hw.c
    ├── uiox_uart_buf.c
    ├── uiox_uart_if.c
    ├── uiox_uart_proto.c
    ├── uiox_uart_subsys.c
    ├── uiox_uart_device.c
    └── uiox_uart_demo.c
=======================================================
LAyer Map:
File	Layer	Mirrors
uiox_uart_hw.h/.c	Hardware — PL011/16550/SiFive registers, 16-op vtable, baud calc, break	uiox_fw_uart.c (extended)
uiox_uart_buf.h/.c	Buffer pool — TX/RX ring buffers, event pool	uiox_rtc_buf pattern
uiox_uart_if.h/.c	Interface driver — SW rings, XON/XOFF, FIFO drain, IRQ dispatch	uiox_chg_if pattern
uiox_uart_proto.h/.c	Protocol — ANSI escape parser, line discipline, echo, ^C/^U/^D	uiox_emmc_proto pattern
uiox_uart_subsys.h/.c	Subsystem — tick loop, event dispatch, console flag	uiox_chg_subsys pattern
uiox_uart_device.h/.c	Application API — open/start/stop/tick/puts/gets/printf/config	uiox_rtc_device pattern
uiox_uart_demo.c	Demo — stub PL011 HAL, 14-scenario full stack exercise	uiox_rtc_demo pattern
=============================================================
