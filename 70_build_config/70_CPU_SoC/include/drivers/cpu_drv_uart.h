#ifndef CPU_DRV_UART_H
#define CPU_DRV_UART_H
/*
 * cpu_drv_uart.h - Early-boot UART driver
 * Supports PL011 (ARM), 16550 (x86/RISC-V)
 */
#include "../cpu_types.h"

typedef enum cpu_uart_type {
    UART_PL011  = 0,   /* ARM PL011                              */
    UART_16550  = 1,   /* 16550 MMIO or port-I/O                 */
    UART_SIFIVE = 2,   /* SiFive UART                            */
} cpu_uart_type_t;

/* -- PL011 register offsets --------------------------------- */
#define PL011_DR        0x000u
#define PL011_RSR       0x004u
#define PL011_FR        0x018u
#define PL011_IBRD      0x024u
#define PL011_FBRD      0x028u
#define PL011_LCR_H     0x02Cu
#define PL011_CR        0x030u
#define PL011_IMSC      0x038u
#define PL011_ICR       0x044u
#define PL011_FR_TXFF   (1u << 5)
#define PL011_FR_RXFE   (1u << 4)

/* -- 16550 register offsets (MMIO, 4-byte stride) ----------- */
#define UART16550_RBR   0x00u   /* RX buffer (read)              */
#define UART16550_THR   0x00u   /* TX holding (write)            */
#define UART16550_IER   0x04u
#define UART16550_FCR   0x08u
#define UART16550_LCR   0x0Cu
#define UART16550_MCR   0x10u
#define UART16550_LSR   0x14u
#define UART16550_DLL   0x00u   /* divisor latch lo (DLAB=1)     */
#define UART16550_DLH   0x04u   /* divisor latch hi (DLAB=1)     */
#define UART16550_LSR_DR    (1u << 0)  /* data ready               */
#define UART16550_LSR_THRE  (1u << 5)  /* TX holding reg empty     */

/* -- SiFive UART register offsets --------------------------- */
#define SIFIVE_UART_TXDATA  0x00u
#define SIFIVE_UART_RXDATA  0x04u
#define SIFIVE_UART_TXCTRL  0x08u
#define SIFIVE_UART_RXCTRL  0x0Cu
#define SIFIVE_UART_IE      0x10u
#define SIFIVE_UART_IP      0x14u
#define SIFIVE_UART_DIV     0x18u
#define SIFIVE_UART_TXFULL  (1u << 31)
#define SIFIVE_UART_RXEMPTY (1u << 31)

/* -- UART context ------------------------------------------- */
typedef struct cpu_uart_ctx {
    cpu_uart_type_t type;
    cpu_addr_t      base;
    cpu_u32_t       baud;
    cpu_u32_t       clock_hz;
    cpu_u32_t       irq;
    cpu_bool_t      use_port_io;   /* x86 port I/O vs MMIO       */
    cpu_u16_t       port;          /* x86 COM port base          */
} cpu_uart_ctx_t;

extern cpu_uart_ctx_t g_uart;

/* -- API ---------------------------------------------------- */
int  cpu_uart_init    (cpu_uart_type_t type, cpu_addr_t base,
                        cpu_u32_t baud, cpu_u32_t clock_hz);
void cpu_uart_putc    (char c);
void cpu_uart_puts    (const char *s);
int  cpu_uart_getc    (void);
int  cpu_uart_poll_rx (void);
void cpu_uart_flush   (void);
void cpu_uart_printf  (const char *fmt, ...);

#endif /* CPU_DRV_UART_H */

