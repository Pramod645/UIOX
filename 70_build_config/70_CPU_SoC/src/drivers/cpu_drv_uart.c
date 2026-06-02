/*
 * cpu_drv_uart.c - Early-boot UART driver (PL011 / 16550 / SiFive)
 */
#include "../../include/drivers/cpu_drv_uart.h"
#include "../../include/cpu_regs.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

cpu_uart_ctx_t g_uart;

int cpu_uart_init(cpu_uart_type_t type, cpu_addr_t base,
                   cpu_u32_t baud, cpu_u32_t clock_hz)
{
    g_uart.type     = type;
    g_uart.base     = base;
    g_uart.baud     = baud;
    g_uart.clock_hz = clock_hz;

    switch (type) {
        case UART_PL011: {
            /* disable UART */
            cpu_mmio_write32(base + PL011_CR, 0u);
            /* set baud rate: ibrd + fbrd */
            cpu_u32_t div  = (clock_hz * 4u) / baud;
            cpu_u32_t ibrd = div >> 6;
            cpu_u32_t fbrd = div & 0x3Fu;
            cpu_mmio_write32(base + PL011_IBRD, ibrd);
            cpu_mmio_write32(base + PL011_FBRD, fbrd);
            /* 8-bit, FIFO enable, no parity */
            cpu_mmio_write32(base + PL011_LCR_H, 0x70u);
            /* enable TX, RX, UART */
            cpu_mmio_write32(base + PL011_CR, 0x301u);
            break;
        }
        case UART_16550: {
            cpu_addr_t b = base;
            /* disable interrupts */
            cpu_mmio_write32(b + UART16550_IER, 0u);
            /* enable DLAB, set divisor */
            cpu_mmio_write32(b + UART16550_LCR, 0x80u);
            cpu_u32_t div = clock_hz / (16u * baud);
            cpu_mmio_write8(b + UART16550_DLL,
                             (cpu_u8_t)(div & 0xFF));
            cpu_mmio_write8(b + UART16550_DLH,
                             (cpu_u8_t)(div >> 8));
            /* 8N1 */
            cpu_mmio_write32(b + UART16550_LCR, 0x03u);
            /* enable FIFO */
            cpu_mmio_write32(b + UART16550_FCR, 0xC7u);
            break;
        }
        case UART_SIFIVE: {
            /* enable TX/RX */
            cpu_mmio_write32(base + SIFIVE_UART_TXCTRL, 0x1u);
            cpu_mmio_write32(base + SIFIVE_UART_RXCTRL, 0x1u);
            /* set divisor for baud */
            cpu_u32_t div = clock_hz / baud - 1u;
            cpu_mmio_write32(base + SIFIVE_UART_DIV, div);
            break;
        }
    }
    return CPU_OK;
}

void cpu_uart_putc(char c)
{
    if (c == '\n') cpu_uart_putc('\r');

    switch (g_uart.type) {
        case UART_PL011:
            while (cpu_mmio_read32(g_uart.base + PL011_FR) &
                   PL011_FR_TXFF);
            cpu_mmio_write32(g_uart.base + PL011_DR, (cpu_u32_t)c);
            break;
        case UART_16550:
            while (!(cpu_mmio_read32(g_uart.base + UART16550_LSR) &
                     UART16550_LSR_THRE));
            cpu_mmio_write32(g_uart.base + UART16550_THR, (cpu_u32_t)c);
            break;
        case UART_SIFIVE:
            while (cpu_mmio_read32(g_uart.base + SIFIVE_UART_TXDATA) &
                   SIFIVE_UART_TXFULL);
            cpu_mmio_write32(g_uart.base + SIFIVE_UART_TXDATA,
                              (cpu_u32_t)c);
            break;
    }
}

void cpu_uart_puts(const char *s)
{ while (*s) cpu_uart_putc(*s++); }

int cpu_uart_getc(void)
{
    cpu_u32_t val = 0;
    switch (g_uart.type) {
        case UART_PL011:
            while (cpu_mmio_read32(g_uart.base + PL011_FR) &
                   PL011_FR_RXFE);
            val = cpu_mmio_read32(g_uart.base + PL011_DR) & 0xFF;
            break;
        case UART_16550:
            while (!(cpu_mmio_read32(g_uart.base + UART16550_LSR) &
                     UART16550_LSR_DR));
            val = cpu_mmio_read32(g_uart.base + UART16550_RBR) & 0xFF;
            break;
        case UART_SIFIVE: {
            cpu_u32_t r;
            do { r = cpu_mmio_read32(g_uart.base + SIFIVE_UART_RXDATA); }
            while (r & SIFIVE_UART_RXEMPTY);
            val = r & 0xFF;
            break;
        }
    }
    return (int)val;
}

int cpu_uart_poll_rx(void)
{
    switch (g_uart.type) {
        case UART_PL011:
            return !(cpu_mmio_read32(g_uart.base + PL011_FR) &
                     PL011_FR_RXFE);
        case UART_16550:
            return !!(cpu_mmio_read32(g_uart.base + UART16550_LSR) &
                      UART16550_LSR_DR);
        case UART_SIFIVE:
            return !(cpu_mmio_read32(g_uart.base + SIFIVE_UART_RXDATA) &
                     SIFIVE_UART_RXEMPTY);
    }
    return 0;
}

void cpu_uart_flush(void)
{
    switch (g_uart.type) {
        case UART_PL011:
            while (cpu_mmio_read32(g_uart.base + PL011_FR) &
                   PL011_FR_TXFF);
            break;
        case UART_16550:
            while (!(cpu_mmio_read32(g_uart.base + UART16550_LSR) &
                     UART16550_LSR_THRE));
            break;
        default: break;
    }
}

void cpu_uart_printf(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    cpu_uart_puts(buf);
}
