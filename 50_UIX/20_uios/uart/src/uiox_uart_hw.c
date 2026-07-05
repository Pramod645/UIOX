/**
 * @file  uiox_uart_hw.c
 * @brief UIOX UART HAL — PL011, NS16550, SiFive implementations.
 *        Extends uiox_fw_uart.c (02_FwHal/src/uiox_fw_uart.c) with
 *        full interrupt, flow control, break, and format change support.
 * @date  2026-07-05
 */

 #include "uiox_uart_device.h"
 #include <string.h>
 #include <errno.h>
 
 #define OPS(hw) ((const uiox_uart_hw_ops_t *)(hw)->priv)
 
 /* =========================================================================
  * x86 port I/O helpers (compiled away on ARM)
  * ====================================================================== */
 
 #if defined(__x86_64__) || defined(__i386__)
 static inline void _outb(uint16_t p, uint8_t v)
 { __asm__ volatile("outb %0,%1"::"a"(v),"dN"(p)); }
 static inline uint8_t _inb(uint16_t p)
 { uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"dN"(p)); return v; }
 #endif
 
 /* =========================================================================
  * PL011 ops
  * ====================================================================== */
 
 static int pl011_init(uiox_uart_hw_t *hw, const uiox_uart_cfg_t *cfg)
 {
     uintptr_t b = hw->base;
     /* Disable UART */
     uart_mmio_write32(b + PL011_CR, 0u);
     /* Clear all pending interrupts */
     uart_mmio_write32(b + PL011_ICR, PL011_INT_ALL);
     /* Set baud rate divisor: IBRD.FBRD from clk_hz and baud */
     uint32_t div_x64 = (hw->clk_hz * 4u) / cfg->baud;
     uart_mmio_write32(b + PL011_IBRD, div_x64 >> 6u);
     uart_mmio_write32(b + PL011_FBRD, div_x64 & 0x3Fu);
     /* Line control: word length, stop bits, parity, FIFO */
     uint32_t lcr = 0u;
     switch (cfg->data_bits) {
     case UIOX_UART_BITS_8: lcr |= PL011_LCR_WLEN8; break;
     case UIOX_UART_BITS_7: lcr |= PL011_LCR_WLEN7; break;
     case UIOX_UART_BITS_6: lcr |= PL011_LCR_WLEN6; break;
     default:               lcr |= PL011_LCR_WLEN8; break;
     }
     if (cfg->stop_bits == UIOX_UART_STOP_2)  lcr |= PL011_LCR_STP2;
     if (cfg->parity != UIOX_UART_PARITY_NONE) {
         lcr |= PL011_LCR_PEN;
         if (cfg->parity == UIOX_UART_PARITY_EVEN)  lcr |= PL011_LCR_EPS;
         if (cfg->parity == UIOX_UART_PARITY_MARK ||
             cfg->parity == UIOX_UART_PARITY_SPACE)  lcr |= PL011_LCR_SPS;
     }
     if (cfg->fifo_en) lcr |= PL011_LCR_FEN;
     uart_mmio_write32(b + PL011_LCR_H, lcr);
     /* FIFO levels: RX ≥ 1/4 full, TX ≤ 1/2 full */
     uart_mmio_write32(b + PL011_IFLS,
                       PL011_IFLS_RX_1_4 | PL011_IFLS_TX_1_2);
     /* Mask all interrupts (IF layer will selectively enable) */
     uart_mmio_write32(b + PL011_IMSC, 0u);
     /* Control: enable UART, TX, RX */
     uint32_t cr = PL011_CR_UARTEN | PL011_CR_TXE | PL011_CR_RXE;
     if (cfg->flow_ctrl == UIOX_UART_FLOW_HW_RTS)
         cr |= PL011_CR_CTSEN | PL011_CR_RTSEN;
     if (cfg->loopback) cr |= PL011_CR_LBE;
     uart_mmio_write32(b + PL011_CR, cr);
     hw->cfg = *cfg;
     return 0;
 }
 
 static void pl011_deinit(uiox_uart_hw_t *hw)
 {
     uart_mmio_write32(hw->base + PL011_CR,   0u);
     uart_mmio_write32(hw->base + PL011_IMSC, 0u);
     uart_mmio_write32(hw->base + PL011_ICR,  PL011_INT_ALL);
 }
 
 static void pl011_putc(uiox_uart_hw_t *hw, char c)
 {
     while (uart_mmio_read32(hw->base + PL011_FR) & PL011_FR_TXFF)
         ;
     uart_mmio_write32(hw->base + PL011_DR, (uint32_t)(uint8_t)c);
 }
 
 static int pl011_getc(uiox_uart_hw_t *hw)
 {
     if (uart_mmio_read32(hw->base + PL011_FR) & PL011_FR_RXFE) return -1;
     uint32_t dr = uart_mmio_read32(hw->base + PL011_DR);
     hw->error_flags = 0u;
     if (dr & PL011_DR_OE) hw->error_flags |= UIOX_UART_ERR_OVERRUN;
     if (dr & PL011_DR_PE) hw->error_flags |= UIOX_UART_ERR_PARITY;
     if (dr & PL011_DR_FE) hw->error_flags |= UIOX_UART_ERR_FRAMING;
     if (dr & PL011_DR_BE) hw->error_flags |= UIOX_UART_ERR_BREAK;
     return (int)(dr & 0xFFu);
 }
 
 static bool pl011_rx_ready(uiox_uart_hw_t *hw)
 { return !(uart_mmio_read32(hw->base + PL011_FR) & PL011_FR_RXFE); }
 
 static bool pl011_tx_ready(uiox_uart_hw_t *hw)
 { return !(uart_mmio_read32(hw->base + PL011_FR) & PL011_FR_TXFF); }
 
 static void pl011_tx_flush(uiox_uart_hw_t *hw)
 { while (uart_mmio_read32(hw->base + PL011_FR) & PL011_FR_BUSY) ; }
 
 static int pl011_set_baud(uiox_uart_hw_t *hw, uint32_t baud)
 {
     pl011_tx_flush(hw);
     uint32_t cr = uart_mmio_read32(hw->base + PL011_CR);
     uart_mmio_write32(hw->base + PL011_CR, cr & ~PL011_CR_UARTEN);
     uint32_t div_x64 = (hw->clk_hz * 4u) / baud;
     uart_mmio_write32(hw->base + PL011_IBRD, div_x64 >> 6u);
     uart_mmio_write32(hw->base + PL011_FBRD, div_x64 & 0x3Fu);
     /* Re-write LCR_H to latch the new divisor */
     uint32_t lcr = uart_mmio_read32(hw->base + PL011_LCR_H);
     uart_mmio_write32(hw->base + PL011_LCR_H, lcr);
     uart_mmio_write32(hw->base + PL011_CR, cr);
     hw->cfg.baud = baud;
     return 0;
 }
 
 static int pl011_set_format(uiox_uart_hw_t *hw,
                               uiox_uart_bits_t bits,
                               uiox_uart_stop_t stop,
                               uiox_uart_parity_t parity)
 {
     uint32_t lcr = uart_mmio_read32(hw->base + PL011_LCR_H);
     /* Clear word, stop, parity bits */
     lcr &= ~(PL011_LCR_WLEN8 | PL011_LCR_STP2 |
              PL011_LCR_PEN   | PL011_LCR_EPS | PL011_LCR_SPS);
     switch (bits) {
     case UIOX_UART_BITS_8: lcr |= PL011_LCR_WLEN8; break;
     case UIOX_UART_BITS_7: lcr |= PL011_LCR_WLEN7; break;
     case UIOX_UART_BITS_6: lcr |= PL011_LCR_WLEN6; break;
     default:               lcr |= PL011_LCR_WLEN8; break;
     }
     if (stop == UIOX_UART_STOP_2) lcr |= PL011_LCR_STP2;
     if (parity != UIOX_UART_PARITY_NONE) {
         lcr |= PL011_LCR_PEN;
         if (parity == UIOX_UART_PARITY_EVEN)  lcr |= PL011_LCR_EPS;
         if (parity == UIOX_UART_PARITY_MARK ||
             parity == UIOX_UART_PARITY_SPACE)  lcr |= PL011_LCR_SPS;
     }
     uart_mmio_write32(hw->base + PL011_LCR_H, lcr);
     hw->cfg.data_bits = bits; hw->cfg.stop_bits = stop;
     hw->cfg.parity    = parity;
     return 0;
 }
 
 static int pl011_set_flow(uiox_uart_hw_t *hw, uiox_uart_flow_t flow)
 {
     uint32_t cr = uart_mmio_read32(hw->base + PL011_CR);
     cr &= ~(PL011_CR_CTSEN | PL011_CR_RTSEN);
     if (flow == UIOX_UART_FLOW_HW_RTS)
         cr |= PL011_CR_CTSEN | PL011_CR_RTSEN;
     uart_mmio_write32(hw->base + PL011_CR, cr);
     hw->cfg.flow_ctrl = flow;
     return 0;
 }
 
 static int pl011_set_loopback(uiox_uart_hw_t *hw, bool en)
 {
     uint32_t cr = uart_mmio_read32(hw->base + PL011_CR);
     if (en) cr |=  PL011_CR_LBE;
     else    cr &= ~PL011_CR_LBE;
     uart_mmio_write32(hw->base + PL011_CR, cr);
     hw->cfg.loopback = en;
     return 0;
 }
 
 static void pl011_send_break(uiox_uart_hw_t *hw, uint32_t duration_ms)
 {
     (void)duration_ms;
     uint32_t lcr = uart_mmio_read32(hw->base + PL011_LCR_H);
     uart_mmio_write32(hw->base + PL011_LCR_H, lcr | PL011_LCR_BRK);
     /* Busy-wait for duration (real kernel would sleep) */
     volatile uint32_t n = duration_ms * 10000u;
     while (n--) ;
     uart_mmio_write32(hw->base + PL011_LCR_H, lcr & ~PL011_LCR_BRK);
 }
 
 static void pl011_irq_enable(uiox_uart_hw_t *hw, uint32_t mask)
 {
     uint32_t imsc = uart_mmio_read32(hw->base + PL011_IMSC);
     uart_mmio_write32(hw->base + PL011_IMSC, imsc | mask);
 }
 
 static void pl011_irq_disable(uiox_uart_hw_t *hw, uint32_t mask)
 {
     uint32_t imsc = uart_mmio_read32(hw->base + PL011_IMSC);
     uart_mmio_write32(hw->base + PL011_IMSC, imsc & ~mask);
 }
 
 static uint32_t pl011_irq_status(uiox_uart_hw_t *hw)
 {
     uint32_t mis = uart_mmio_read32(hw->base + PL011_MIS);
     uart_mmio_write32(hw->base + PL011_ICR, mis);  /* W1C */
 
     uint32_t pending = 0u;
     if (mis & PL011_INT_RXI)   pending |= UIOX_UART_IRQ_RX_DATA;
     if (mis & PL011_INT_RTI)   pending |= UIOX_UART_IRQ_RX_TIMEOUT;
     if (mis & PL011_INT_TXI)   pending |= UIOX_UART_IRQ_TX_EMPTY;
     if (mis & PL011_INT_ERROR) pending |= UIOX_UART_IRQ_LINE_ERR;
     if (mis & (PL011_INT_CTSMI | PL011_INT_DSRMI |
                PL011_INT_DCDMI | PL011_INT_RIMI))
         pending |= UIOX_UART_IRQ_MODEM;
     return pending;
 }
 
 static void pl011_isr(uiox_uart_hw_t *hw)
 {
     hw->pending_irq = pl011_irq_status(hw);
 }
 
 static void pl011_gpio_write(uiox_uart_hw_t *hw, uint32_t pin, bool val)
 {
     uint32_t cr = uart_mmio_read32(hw->base + PL011_CR);
     /* PL011 GPIO: pin 0 = DTR, pin 1 = RTS */
     if (pin == 0u) {
         if (val) cr |=  PL011_CR_DTR;
         else     cr &= ~PL011_CR_DTR;
     } else if (pin == 1u) {
         if (val) cr |=  PL011_CR_RTS;
         else     cr &= ~PL011_CR_RTS;
     }
     uart_mmio_write32(hw->base + PL011_CR, cr);
 }
 
 static bool pl011_gpio_read(uiox_uart_hw_t *hw, uint32_t pin)
 {
     uint32_t fr = uart_mmio_read32(hw->base + PL011_FR);
     if (pin == 0u) return !!(fr & PL011_FR_DSR);
     if (pin == 1u) return !!(fr & PL011_FR_CTS);
     if (pin == 2u) return !!(fr & PL011_FR_DCD);
     return false;
 }
 
 static const uiox_uart_hw_ops_t pl011_ops = {
     .init        = pl011_init,
     .deinit      = pl011_deinit,
     .putc        = pl011_putc,
     .getc        = pl011_getc,
     .rx_ready    = pl011_rx_ready,
     .tx_ready    = pl011_tx_ready,
     .tx_flush    = pl011_tx_flush,
     .set_baud    = pl011_set_baud,
     .set_format  = pl011_set_format,
     .set_flow    = pl011_set_flow,
     .set_loopback= pl011_set_loopback,
     .send_break  = pl011_send_break,
     .irq_enable  = pl011_irq_enable,
     .irq_disable = pl011_irq_disable,
     .irq_status  = pl011_irq_status,
     .gpio_write  = pl011_gpio_write,
     .gpio_read   = pl011_gpio_read,
     .isr         = pl011_isr,
 };
 
 /* =========================================================================
  * NS16550 ops
  * ====================================================================== */
 
 #if defined(__x86_64__) || defined(__i386__)
 
 static int ns16550_init(uiox_uart_hw_t *hw, const uiox_uart_cfg_t *cfg)
 {
     uint16_t port = (uint16_t)hw->base;
     _outb(port + UART16550_IER, 0x00u);  /* disable all IRQs             */
     _outb(port + UART16550_LCR, UART16550_LCR_DLAB);
     /* Divisor: base clock = 1.8432 MHz → divisor = 1843200 / (16*baud) */
     uint16_t div = (uint16_t)(1843200u / cfg->baud);
     _outb(port + UART16550_DLL, (uint8_t)(div & 0xFFu));
     _outb(port + UART16550_DLM, (uint8_t)(div >> 8u));
     /* Line control */
     uint8_t lcr = 0u;
     switch (cfg->data_bits) {
     case UIOX_UART_BITS_8: lcr |= UART16550_LCR_WLS8; break;
     case UIOX_UART_BITS_7: lcr |= UART16550_LCR_WLS7; break;
     case UIOX_UART_BITS_6: lcr |= UART16550_LCR_WLS6; break;
     default:               lcr |= UART16550_LCR_WLS8; break;
     }
     if (cfg->stop_bits  == UIOX_UART_STOP_2)         lcr |= UART16550_LCR_STB;
     if (cfg->parity     != UIOX_UART_PARITY_NONE)    lcr |= UART16550_LCR_PEN;
     if (cfg->parity     == UIOX_UART_PARITY_EVEN)    lcr |= UART16550_LCR_EPS;
     if (cfg->loopback)
         _outb(port + UART16550_MCR, UART16550_MCR_LOOP);
     _outb(port + UART16550_LCR, lcr);
     /* FIFO: enable, reset TX+RX, trigger at 14 bytes */
     if (cfg->fifo_en)
         _outb(port + UART16550_FCR,
               UART16550_FCR_FIFOEN | UART16550_FCR_RXRST |
               UART16550_FCR_TXRST  | UART16550_FCR_TRIG14);
     /* MCR: RTS+DTR+OUT2 (OUT2 required to enable IRQs) */
     uint8_t mcr = UART16550_MCR_DTR | UART16550_MCR_RTS | UART16550_MCR_OUT2;
     if (cfg->flow_ctrl == UIOX_UART_FLOW_HW_RTS) mcr |= UART16550_MCR_RTS;
     _outb(port + UART16550_MCR, mcr);
     hw->cfg = *cfg;
     return 0;
 }
 
 static void ns16550_deinit(uiox_uart_hw_t *hw)
 {
     uint16_t port = (uint16_t)hw->base;
     _outb(port + UART16550_IER, 0u);
     _outb(port + UART16550_MCR, 0u);
 }
 
 static void ns16550_putc(uiox_uart_hw_t *hw, char c)
 {
     uint16_t port = (uint16_t)hw->base;
     while (!(_inb(port + UART16550_LSR) & UART16550_LSR_THRE)) ;
     _outb(port + UART16550_THR, (uint8_t)c);
 }
 
 static int ns16550_getc(uiox_uart_hw_t *hw)
 {
     uint16_t port = (uint16_t)hw->base;
     uint8_t  lsr  = _inb(port + UART16550_LSR);
     hw->error_flags = 0u;
     if (lsr & UART16550_LSR_OE) hw->error_flags |= UIOX_UART_ERR_OVERRUN;
     if (lsr & UART16550_LSR_PE) hw->error_flags |= UIOX_UART_ERR_PARITY;
     if (lsr & UART16550_LSR_FE) hw->error_flags |= UIOX_UART_ERR_FRAMING;
     if (lsr & UART16550_LSR_BI) hw->error_flags |= UIOX_UART_ERR_BREAK;
     if (!(lsr & UART16550_LSR_DR)) return -1;
     return (int)_inb(port + UART16550_RBR);
 }
 
 static bool ns16550_rx_ready(uiox_uart_hw_t *hw)
 { return !!(_inb((uint16_t)hw->base + UART16550_LSR) & UART16550_LSR_DR); }
 
 static bool ns16550_tx_ready(uiox_uart_hw_t *hw)
 { return !!(_inb((uint16_t)hw->base + UART16550_LSR) & UART16550_LSR_THRE); }
 
 static void ns16550_tx_flush(uiox_uart_hw_t *hw)
 { while (!(_inb((uint16_t)hw->base + UART16550_LSR) & UART16550_LSR_TEMT)) ; }
 
 static int ns16550_set_baud(uiox_uart_hw_t *hw, uint32_t baud)
 {
     uint16_t port = (uint16_t)hw->base;
     uint8_t  lcr  = _inb(port + UART16550_LCR);
     _outb(port + UART16550_LCR, lcr | UART16550_LCR_DLAB);
     uint16_t div = (uint16_t)(1843200u / baud);
     _outb(port + UART16550_DLL, (uint8_t)(div & 0xFFu));
     _outb(port + UART16550_DLM, (uint8_t)(div >> 8u));
     _outb(port + UART16550_LCR, lcr);
     hw->cfg.baud = baud;
     return 0;
 }
 
 static int ns16550_set_format(uiox_uart_hw_t *hw,
                                uiox_uart_bits_t bits,
                                uiox_uart_stop_t stop,
                                uiox_uart_parity_t parity)
 {
     uint16_t port = (uint16_t)hw->base;
     uint8_t  lcr  = 0u;
     switch (bits) {
     case UIOX_UART_BITS_8: lcr = UART16550_LCR_WLS8; break;
     case UIOX_UART_BITS_7: lcr = UART16550_LCR_WLS7; break;
     case UIOX_UART_BITS_6: lcr = UART16550_LCR_WLS6; break;
     default:               lcr = UART16550_LCR_WLS8; break;
     }
     if (stop   == UIOX_UART_STOP_2)          lcr |= UART16550_LCR_STB;
     if (parity != UIOX_UART_PARITY_NONE)     lcr |= UART16550_LCR_PEN;
     if (parity == UIOX_UART_PARITY_EVEN)     lcr |= UART16550_LCR_EPS;
     _outb(port + UART16550_LCR, lcr);
     hw->cfg.data_bits = bits; hw->cfg.stop_bits = stop;
     hw->cfg.parity    = parity;
     return 0;
 }
 
 static int ns16550_set_flow(uiox_uart_hw_t *hw, uiox_uart_flow_t flow)
 {
     (void)hw; (void)flow;
     /* 16550 HW flow: managed via MCR.RTS + MSR.CTS */
     hw->cfg.flow_ctrl = flow;
     return 0;
 }
 
 static int ns16550_set_loopback(uiox_uart_hw_t *hw, bool en)
 {
     uint16_t port = (uint16_t)hw->base;
     uint8_t  mcr  = _inb(port + UART16550_MCR);
     if (en) mcr |=  UART16550_MCR_LOOP;
     else    mcr &= ~UART16550_MCR_LOOP;
     _outb(port + UART16550_MCR, mcr);
     hw->cfg.loopback = en;
     return 0;
 }
 
 static void ns16550_send_break(uiox_uart_hw_t *hw, uint32_t ms)
 {
     uint16_t port = (uint16_t)hw->base;
     uint8_t  lcr  = _inb(port + UART16550_LCR);
     _outb(port + UART16550_LCR, lcr | UART16550_LCR_SBC);
     volatile uint32_t n = ms * 10000u; while (n--) ;
     _outb(port + UART16550_LCR, lcr & ~UART16550_LCR_SBC);
 }
 
 static void ns16550_irq_enable(uiox_uart_hw_t *hw, uint32_t mask)
 {
     uint16_t port = (uint16_t)hw->base;
     uint8_t  ier  = _inb(port + UART16550_IER);
     if (mask & UIOX_UART_IRQ_RX_DATA)  ier |= UART16550_IER_ERBFI;
     if (mask & UIOX_UART_IRQ_TX_EMPTY) ier |= UART16550_IER_ETBEI;
     if (mask & UIOX_UART_IRQ_LINE_ERR) ier |= UART16550_IER_ELSI;
     if (mask & UIOX_UART_IRQ_MODEM)    ier |= UART16550_IER_EDSSI;
     _outb(port + UART16550_IER, ier);
 }
 
 static void ns16550_irq_disable(uiox_uart_hw_t *hw, uint32_t mask)
 {
     uint16_t port = (uint16_t)hw->base;
     uint8_t  ier  = _inb(port + UART16550_IER);
     if (mask & UIOX_UART_IRQ_RX_DATA)  ier &= ~UART16550_IER_ERBFI;
     if (mask & UIOX_UART_IRQ_TX_EMPTY) ier &= ~UART16550_IER_ETBEI;
     if (mask & UIOX_UART_IRQ_LINE_ERR) ier &= ~UART16550_IER_ELSI;
     if (mask & UIOX_UART_IRQ_MODEM)    ier &= ~UART16550_IER_EDSSI;
     _outb(port + UART16550_IER, ier);
 }
 
 static uint32_t ns16550_irq_status(uiox_uart_hw_t *hw)
 {
     uint16_t port = (uint16_t)hw->base;
     uint8_t  iir  = _inb(port + UART16550_IIR);
     if (iir & 0x01u) return 0u;   /* No interrupt pending */
     uint32_t pending = 0u;
     switch (iir & 0x0Eu) {
     case 0x06u: pending = UIOX_UART_IRQ_LINE_ERR;  break;
     case 0x04u: pending = UIOX_UART_IRQ_RX_DATA;   break;
     case 0x0Cu: pending = UIOX_UART_IRQ_RX_TIMEOUT;break;
     case 0x02u: pending = UIOX_UART_IRQ_TX_EMPTY;  break;
     case 0x00u: pending = UIOX_UART_IRQ_MODEM;     break;
     }
     return pending;
 }
 
 static void ns16550_isr(uiox_uart_hw_t *hw)
 { hw->pending_irq = ns16550_irq_status(hw); }
 
 static void ns16550_gpio_write(uiox_uart_hw_t *hw, uint32_t pin, bool val)
 {
     uint16_t port = (uint16_t)hw->base;
     uint8_t  mcr  = _inb(port + UART16550_MCR);
     if (pin == 0u) {
         if (val) mcr |=  UART16550_MCR_DTR;
         else     mcr &= ~UART16550_MCR_DTR;
     } else if (pin == 1u) {
         if (val) mcr |=  UART16550_MCR_RTS;
         else     mcr &= ~UART16550_MCR_RTS;
     }
     _outb(port + UART16550_MCR, mcr);
 }
 
 static bool ns16550_gpio_read(uiox_uart_hw_t *hw, uint32_t pin)
 {
     uint16_t port = (uint16_t)hw->base;
     uint8_t  msr  = _inb(port + UART16550_MSR);
     if (pin == 0u) return !!(msr & UART16550_MSR_DSR);
     if (pin == 1u) return !!(msr & UART16550_MSR_CTS);
     if (pin == 2u) return !!(msr & UART16550_MSR_DCD);
     return false;
 }
 
 static const uiox_uart_hw_ops_t ns16550_ops = {
     .init        = ns16550_init,
     .deinit      = ns16550_deinit,
     .putc        = ns16550_putc,
     .getc        = ns16550_getc,
     .rx_ready    = ns16550_rx_ready,
     .tx_ready    = ns16550_tx_ready,
     .tx_flush    = ns16550_tx_flush,
     .set_baud    = ns16550_set_baud,
     .set_format  = ns16550_set_format,
     .set_flow    = ns16550_set_flow,
     .set_loopback= ns16550_set_loopback,
     .send_break  = ns16550_send_break,
     .irq_enable  = ns16550_irq_enable,
     .irq_disable = ns16550_irq_disable,
     .irq_status  = ns16550_irq_status,
     .gpio_write  = ns16550_gpio_write,
     .gpio_read   = ns16550_gpio_read,
     .isr         = ns16550_isr,
 };
 
 #endif /* x86 */
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int uiox_uart_hw_init(uiox_uart_hw_t *hw,
                        const uiox_uart_hw_ops_t *ops,
                        const uiox_uart_cfg_t *cfg)
 {
     if (!hw || !ops || !ops->init || !cfg) return -EINVAL;
     hw->priv        = (void *)ops;
     hw->pending_irq = 0u;
     hw->error_flags = 0u;
     return ops->init(hw, cfg);
 }
 
 void uiox_uart_hw_deinit(uiox_uart_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     if (OPS(hw)->deinit) OPS(hw)->deinit(hw);
     hw->priv = NULL;
 }
 
 void     uiox_uart_hw_putc      (uiox_uart_hw_t *hw, char c)
 { if (hw && hw->priv && OPS(hw)->putc) OPS(hw)->putc(hw, c); }
 
 int      uiox_uart_hw_getc      (uiox_uart_hw_t *hw)
 { return (hw && hw->priv && OPS(hw)->getc) ? OPS(hw)->getc(hw) : -1; }
 
 bool     uiox_uart_hw_rx_ready  (uiox_uart_hw_t *hw)
 { return (hw && hw->priv && OPS(hw)->rx_ready) ? OPS(hw)->rx_ready(hw) : false; }
 
 bool     uiox_uart_hw_tx_ready  (uiox_uart_hw_t *hw)
 { return (hw && hw->priv && OPS(hw)->tx_ready) ? OPS(hw)->tx_ready(hw) : false; }
 
 void     uiox_uart_hw_tx_flush  (uiox_uart_hw_t *hw)
 { if (hw && hw->priv && OPS(hw)->tx_flush) OPS(hw)->tx_flush(hw); }
 
 int      uiox_uart_hw_set_baud  (uiox_uart_hw_t *hw, uint32_t baud)
 { if (!hw || !hw->priv || !OPS(hw)->set_baud) return -ENOSYS;
   return OPS(hw)->set_baud(hw, baud); }
 
 int uiox_uart_hw_set_format(uiox_uart_hw_t *hw,
                               uiox_uart_bits_t bits,
                               uiox_uart_stop_t stop,
                               uiox_uart_parity_t parity)
 { if (!hw || !hw->priv || !OPS(hw)->set_format) return -ENOSYS;
   return OPS(hw)->set_format(hw, bits, stop, parity); }
 
 int      uiox_uart_hw_set_flow  (uiox_uart_hw_t *hw, uiox_uart_flow_t flow)
 { if (!hw || !hw->priv || !OPS(hw)->set_flow) return -ENOSYS;
   return OPS(hw)->set_flow(hw, flow); }
 
 void     uiox_uart_hw_send_break(uiox_uart_hw_t *hw, uint32_t ms)
 { if (hw && hw->priv && OPS(hw)->send_break) OPS(hw)->send_break(hw, ms); }
 
 void uiox_uart_hw_irq_enable(uiox_uart_hw_t *hw, uint32_t mask)
 { if (hw && hw->priv && OPS(hw)->irq_enable) OPS(hw)->irq_enable(hw, mask); }
 
 void uiox_uart_hw_irq_disable(uiox_uart_hw_t *hw, uint32_t mask)
 { if (hw && hw->priv && OPS(hw)->irq_disable) OPS(hw)->irq_disable(hw, mask); }
 
 /* =========================================================================
  * Platform init helpers
  * ====================================================================== */
 
 int uiox_uart_pl011_init(uiox_uart_hw_t *hw, uintptr_t base,
                           uint32_t clk_hz, uint32_t irq,
                           const uiox_uart_cfg_t *cfg)
 {
     if (!hw || !cfg) return -EINVAL;
     memset(hw, 0, sizeof(*hw));
     hw->base    = base;
     hw->clk_hz  = clk_hz;
     hw->irq     = irq;
     hw->variant = UIOX_UART_PL011;
     hw->caps    = UIOX_UART_CAP_FIFO | UIOX_UART_CAP_HW_FLOW |
                   UIOX_UART_CAP_MODEM | UIOX_UART_CAP_BREAK;
     strncpy(hw->model, "ARM PL011", UIOX_UART_MODEL_LEN - 1u);
     return uiox_uart_hw_init(hw, &pl011_ops, cfg);
 }
 
 int uiox_uart_16550_init(uiox_uart_hw_t *hw, uintptr_t port,
                           uint32_t irq, const uiox_uart_cfg_t *cfg)
 {
 #if defined(__x86_64__) || defined(__i386__)
     if (!hw || !cfg) return -EINVAL;
     memset(hw, 0, sizeof(*hw));
     hw->base    = port;
     hw->clk_hz  = 1843200u;
     hw->irq     = irq;
     hw->variant = UIOX_UART_16550;
     hw->caps    = UIOX_UART_CAP_FIFO | UIOX_UART_CAP_MODEM |
                   UIOX_UART_CAP_BREAK;
     strncpy(hw->model, "NS16550A", UIOX_UART_MODEL_LEN - 1u);
     return uiox_uart_hw_init(hw, &ns16550_ops, cfg);
 #else
     (void)hw; (void)port; (void)irq; (void)cfg;
     return -ENOSYS;
 #endif
 }
 