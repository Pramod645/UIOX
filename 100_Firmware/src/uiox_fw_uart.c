/**
 * @file  uiox_fw_uart.c
 * @brief UIOX Firmware — PL011 / 16550 UART driver.
 * @date  2026-06-21
 */

 #include "uiox_fw.h"

 /* x86 port I/O helpers (compiled out on ARM) */
 #if defined(__x86_64__) || defined(__i386__)
 static inline void _outb(uint16_t port, uint8_t v)
 { __asm__ volatile("outb %0,%1"::"a"(v),"dN"(port)); }
 static inline uint8_t _inb(uint16_t port)
 { uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"dN"(port)); return v; }
 #endif
 
 /* ── PL011 ─────────────────────────────────────────────────────────── */
 
 static void pl011_init(uiox_fw_uart_t *u)
 {
     uintptr_t b = u->base;
     fw_mmio_write32(b + PL011_CR, 0u);          /* disable              */
     /* baud: IBRD + FBRD from clock / (16 × baud) */
     uint32_t div16 = u->clock_hz / u->cfg.baud; /* × 16 fixed-point    */
     fw_mmio_write32(b + PL011_IBRD, div16 >> 4u);
     fw_mmio_write32(b + PL011_FBRD, div16 & 0xFu);
     fw_mmio_write32(b + PL011_LCR_H, PL011_LCR_WLEN8 |
                     (u->cfg.fifo_en ? PL011_LCR_FEN : 0u));
     fw_mmio_write32(b + PL011_IMSC, 0u);        /* mask all             */
     fw_mmio_write32(b + PL011_CR,
                     PL011_CR_UARTEN | PL011_CR_TXE | PL011_CR_RXE);
 }
 
 static void pl011_putc(uiox_fw_uart_t *u, char c)
 {
     while (fw_mmio_read32(u->base + PL011_FR) & PL011_FR_TXFF) ;
     fw_mmio_write32(u->base + PL011_DR, (uint32_t)(uint8_t)c);
     u->tx_bytes++;
 }
 
 static int pl011_getc(uiox_fw_uart_t *u)
 {
     if (fw_mmio_read32(u->base + PL011_FR) & PL011_FR_RXFE)
         return -1;
     u->rx_bytes++;
     return (int)(fw_mmio_read32(u->base + PL011_DR) & 0xFFu);
 }
 
 static bool pl011_rx_rdy(uiox_fw_uart_t *u)
 { return !(fw_mmio_read32(u->base + PL011_FR) & PL011_FR_RXFE); }
 
 /* ── 16550 ─────────────────────────────────────────────────────────── */
 
 #if defined(__x86_64__) || defined(__i386__)
 static void uart16550_init(uiox_fw_uart_t *u)
 {
     uint16_t port = (uint16_t)u->base;
     _outb(port + UART16550_IER, 0x00u);
     _outb(port + UART16550_LCR, 0x80u);    /* DLAB=1                   */
     uint16_t div = (uint16_t)(115200u / u->cfg.baud);
     _outb(port + UART16550_DLL, (uint8_t)(div & 0xFFu));
     _outb(port + UART16550_DLM, (uint8_t)(div >> 8u));
     _outb(port + UART16550_LCR, 0x03u);    /* 8N1, DLAB=0              */
     _outb(port + UART16550_FCR, 0xC7u);    /* FIFO enable              */
     _outb(port + UART16550_MCR, 0x0Bu);    /* RTS+DTR+OUT2             */
 }
 
 static void uart16550_putc(uiox_fw_uart_t *u, char c)
 {
     uint16_t port = (uint16_t)u->base;
     while (!(_inb(port + UART16550_LSR) & UART16550_LSR_THRE)) ;
     _outb(port + UART16550_THR, (uint8_t)c);
     u->tx_bytes++;
 }
 
 static int uart16550_getc(uiox_fw_uart_t *u)
 {
     uint16_t port = (uint16_t)u->base;
     if (!(_inb(port + UART16550_LSR) & UART16550_LSR_DR)) return -1;
     u->rx_bytes++;
     return (int)_inb(port + UART16550_RBR);
 }
 
 static bool uart16550_rx_rdy(uiox_fw_uart_t *u)
 { return !!(_inb((uint16_t)u->base + UART16550_LSR) & UART16550_LSR_DR); }
 #endif
 
 /* ── Public API ─────────────────────────────────────────────────────── */
 
 uiox_fw_err_t uiox_fw_uart_init(uiox_fw_uart_t *u,
                                   uintptr_t base, bool is_pl011,
                                   uint32_t irq,
                                   const uiox_fw_uart_cfg_t *cfg)
 {
     if (!u || !cfg) return UIOX_FW_ERR_INVAL;
     u->base       = base;
     u->irq        = irq;
     u->is_pl011   = is_pl011;
     u->cfg        = *cfg;
     u->clock_hz   = 24000000u;   /* 24 MHz default (QEMU PL011)       */
     u->tx_bytes   = 0u;
     u->rx_bytes   = 0u;
     u->errors     = 0u;
     u->rx_cb      = NULL;
     u->rx_priv    = NULL;
     if (is_pl011) pl011_init(u);
 #if defined(__x86_64__) || defined(__i386__)
     else          uart16550_init(u);
 #endif
     return UIOX_FW_OK;
 }
 
 void uiox_fw_uart_putc(uiox_fw_uart_t *u, char c)
 {
     if (!u) return;
     if (c == '\n') uiox_fw_uart_putc(u, '\r');
     if (u->is_pl011) pl011_putc(u, c);
 #if defined(__x86_64__) || defined(__i386__)
     else             uart16550_putc(u, c);
 #endif
 }
 
 void uiox_fw_uart_puts(uiox_fw_uart_t *u, const char *s)
 { if (s) while (*s) uiox_fw_uart_putc(u, *s++); }
 
 int uiox_fw_uart_getc(uiox_fw_uart_t *u)
 {
     if (!u) return -1;
     if (u->is_pl011) return pl011_getc(u);
 #if defined(__x86_64__) || defined(__i386__)
     return uart16550_getc(u);
 #else
     return -1;
 #endif
 }
 
 bool uiox_fw_uart_rx_rdy(uiox_fw_uart_t *u)
 {
     if (!u) return false;
     if (u->is_pl011) return pl011_rx_rdy(u);
 #if defined(__x86_64__) || defined(__i386__)
     return uart16550_rx_rdy(u);
 #else
     return false;
 #endif
 }
 
 void uiox_fw_uart_irq(uiox_fw_uart_t *u)
 {
     while (uiox_fw_uart_rx_rdy(u)) {
         int c = uiox_fw_uart_getc(u);
         if (c >= 0 && u->rx_cb)
             u->rx_cb((char)c, u->rx_priv);
     }
 }
 
 void uiox_fw_uart_set_rx_cb(uiox_fw_uart_t *u,
                               uiox_fw_uart_rx_cb_t cb, void *priv)
 { if (u) { u->rx_cb = cb; u->rx_priv = priv; } }
 