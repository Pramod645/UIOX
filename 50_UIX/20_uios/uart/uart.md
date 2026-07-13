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
┌─────────────────────────────────────────────────────────┐
│  Layer 3 — Kernel Subsystem                             │
│  uiox_uart_subsys.h / .c                                │
│  • Probes HW from uiox_fw_platform_t (caps + uart_base) │
│  • TTY line discipline: echo, LF→CRLF, backspace, strip │
│  • Kernel console: kputc / kputs / kprintf              │
│  • Syscall surface: open/close/read/write/ioctl/poll    │
├─────────────────────────────────────────────────────────┤
│  Layer 2 — Feature / Device Access                      │
│  uiox_uart_dev.h / .c                                   │
│  • Registers ports into the UIOX devsw table            │
│  • open / close / read / write / ioctl / poll ops       │
│  • IOCTL commands: SET_BAUD, GET_FMT, FLUSH_RX/TX,      │
│    GET_STATS, CLR_ERR                                   │
│  • Poll flags: RX_RDY, TX_RDY, ERR                      │
├─────────────────────────────────────────────────────────┤
│  Layer 1 — Software IF Driver                           │
│  uiox_uart_drv.h / .c                                   │
│  • 256-byte power-of-2 RX + TX ring buffers             │
│  • ISR entry point: drains HW FIFO → SW ring            │
│  • TX kick: re-enables TX interrupt when data enqueued  │
│  • Buffered read/write + polled putc/getc               │
│  • Error flags: OVERRUN, FRAME, PARITY, BREAK           │
├─────────────────────────────────────────────────────────┤
│  Layer 0 — Hardware Interface                           │
│  uiox_uart_hw.h / .c                                    │
│  • PL011: all register offsets, FR/LCR/CR/IMSC/ICR bits │
│  • 16550A: all register offsets, IER/FCR/LCR/LSR bits   │
│  • Baud calc: IBRD/FBRD (PL011) and DLL/DLM (16550)     │
│    — software divide, no __aeabi_uidiv / libgcc         │
│  • PL011: pl011_probe() checks PeriphID0 == 0x11        │
│  • 16550: loopback self-test (0xAE echo) in init        │
│  • x86 I/O: inline asm outb/inb; non-x86 stubs compile  │
├─────────────────────────────────────────────────────────┤
│  Firmware Extension (02_FwHal/src/)                     │
│  uiox_fw_uart_ext.c                                     │
│  • init_default()  — 115200 8N1 one-call setup          │
│  • puts()          — NUL-terminated string TX           │
│  • hex32()         — 32-bit hex dump + CRLF             │
│  • self_test()     — loopback regression (PL011 + 16550) │
└─────────────────────────────────────────────────────────┘
         ↑ fw_mmio_read32/write32  (uiox_fw_types.h)
         ↑ uintptr_t base from uiox_fw_platform_t
    PL011 MMIO (ARM32/ARM64)   16550A I/O ports (x86_64)
==============================================================
Decision	Rationale
Software divide (udiv32) in every .c	Matches udiv32_soft_uart pattern already in uiox_fw_uart.c — no __aeabi_uidiv / libgcc dependency
Ring size = 256, power-of-2	Wrap with & RING_MASK — no modulo, safe in ISR
TX interrupt re-armed on write	TX IRQ is masked when ring empties; re-enabled when drv_write() enqueues bytes — prevents spurious IRQ spin
#include "uiox_fw.h" in uiox_fw_uart_ext.c	Matches the umbrella-include convention in the existing uiox_fw_uart.c
PL011 PERIPHID0 == 0x11 probe	ARM DDI0183G §3.1 — lets subsys_init auto-detect real vs absent PL011
Non-x86 u16550_outb/inb stubs	ARM builds compile cleanly; linker DCE removes the dead stubs
