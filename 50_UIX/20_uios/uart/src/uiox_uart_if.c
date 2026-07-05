/**
 * @file  uiox_uart_if.c
 * @brief UIOX UART Interface driver — framing, ring buffers, flow, IRQ.
 * @date  2026-07-05
 */

 #include "uiox_uart_device.h"
 #include <string.h>
 #include <errno.h>
 
 /* XON / XOFF characters */
 #define XON_CHAR    0x11u  /* ^Q */
 #define XOFF_CHAR   0x13u  /* ^S */
 /* SW flow control thresholds */
 #define XOFF_THRESHOLD  (UIOX_UART_RX_BUF_SIZE * 3u / 4u)
 #define XON_THRESHOLD   (UIOX_UART_RX_BUF_SIZE / 4u)
 
 int uiox_uart_if_config(uiox_uart_if_t *uif, uiox_uart_hw_t *hw)
 {
     if (!uif || !hw) return -EINVAL;
     memset(uif, 0, sizeof(*uif));
     uif->hw        = hw;
     uif->primed    = true;
     uif->xon_char  = XON_CHAR;
     uif->xoff_char = XOFF_CHAR;
     uif->xoff_sent = false;
     /* Initialise ring buffers */
     uiox_uart_ring_init(&uif->tx_ring, UIOX_UART_TX_BUF_SIZE);
     uiox_uart_ring_init(&uif->rx_ring, UIOX_UART_RX_BUF_SIZE);
     /* Point rings at their backing memory */
     memcpy(uif->tx_ring.data, uif->tx_buf_mem, 0u); /* just init sizes */
     uif->tx_ring.size = UIOX_UART_TX_BUF_SIZE;
     uif->rx_ring.size = UIOX_UART_RX_BUF_SIZE;
     uiox_uart_buf_init();
     return 0;
 }
 
 int uiox_uart_if_start(uiox_uart_if_t *uif)
 {
     if (!uif || !uif->primed) return -EINVAL;
     /* Enable RX data + RX timeout + error interrupts */
     uiox_uart_hw_irq_enable(uif->hw,
                              UIOX_UART_IRQ_RX_DATA    |
                              UIOX_UART_IRQ_RX_TIMEOUT |
                              UIOX_UART_IRQ_LINE_ERR);
     return 0;
 }
 
 void uiox_uart_if_stop(uiox_uart_if_t *uif)
 {
     if (!uif) return;
     /* Flush TX and disable all IRQs */
     uiox_uart_hw_tx_flush(uif->hw);
     uiox_uart_hw_irq_disable(uif->hw, 0xFFFFFFFFu);
     uiox_uart_ring_flush(&uif->tx_ring);
     uiox_uart_ring_flush(&uif->rx_ring);
 }
 
 /* ── TX path ───────────────────────────────────────────────────── */
 
 static void if_tx_drain(uiox_uart_if_t *uif)
 {
     /* Drain SW TX ring into HW FIFO while HW is ready */
     uint8_t c;
     while (uiox_uart_hw_tx_ready(uif->hw) &&
            uiox_uart_ring_get(&uif->tx_ring, &c) == 0) {
         uiox_uart_hw_putc(uif->hw, (char)c);
         uif->stats.tx_bytes++;
     }
     /* Enable TX-empty IRQ if more data is waiting */
     if (!uiox_uart_ring_empty(&uif->tx_ring))
         uiox_uart_hw_irq_enable(uif->hw, UIOX_UART_IRQ_TX_EMPTY);
     else
         uiox_uart_hw_irq_disable(uif->hw, UIOX_UART_IRQ_TX_EMPTY);
 }
 
 int uiox_uart_if_putc(uiox_uart_if_t *uif, char c)
 {
     if (!uif) return -EINVAL;
     /* SW XON/XOFF: wait if peer sent XOFF */
     if (uif->hw->cfg.flow_ctrl == UIOX_UART_FLOW_SW_XON &&
         uif->xoff_sent)
         return -EAGAIN;
 
     if (uiox_uart_ring_put(&uif->tx_ring, (uint8_t)c) != 0)
         return -ENOSPC;
     if_tx_drain(uif);
     return 0;
 }
 
 int uiox_uart_if_puts(uiox_uart_if_t *uif, const char *s)
 {
     if (!uif || !s) return -EINVAL;
     int sent = 0;
     while (*s) {
         if (uiox_uart_if_putc(uif, *s++) < 0) break;
         sent++;
     }
     return sent;
 }
 
 int uiox_uart_if_write(uiox_uart_if_t *uif,
                         const uint8_t *buf, uint32_t len)
 {
     if (!uif || !buf) return -EINVAL;
     uint32_t n = 0u;
     for (; n < len; n++) {
         if (uiox_uart_if_putc(uif, (char)buf[n]) < 0) break;
     }
     return (int)n;
 }
 
 /* ── RX path ───────────────────────────────────────────────────── */
 
 int uiox_uart_if_getc(uiox_uart_if_t *uif)
 {
     if (!uif) return -1;
     uint8_t c;
     if (uiox_uart_ring_get(&uif->rx_ring, &c) != 0) return -1;
 
     /* SW XON/XOFF: send XON if buffer drained below threshold */
     if (uif->hw->cfg.flow_ctrl == UIOX_UART_FLOW_SW_XON &&
         uif->xoff_sent &&
         uiox_uart_ring_avail(&uif->rx_ring) < XON_THRESHOLD) {
         uiox_uart_hw_putc(uif->hw, (char)uif->xon_char);
         uif->xoff_sent = false;
     }
     return (int)c;
 }
 
 int uiox_uart_if_read(uiox_uart_if_t *uif, uint8_t *buf, uint32_t len)
 {
     if (!uif || !buf) return -EINVAL;
     uint32_t n = 0u;
     while (n < len) {
         int c = uiox_uart_if_getc(uif);
         if (c < 0) break;
         buf[n++] = (uint8_t)c;
     }
     return (int)n;
 }
 
 uint32_t uiox_uart_if_rx_avail(const uiox_uart_if_t *uif)
 { return uif ? uiox_uart_ring_avail(&uif->rx_ring) : 0u; }
 
 /* ── Configuration pass-throughs ──────────────────────────────── */
 
 int uiox_uart_if_set_baud(uiox_uart_if_t *uif, uint32_t baud)
 { return uif ? uiox_uart_hw_set_baud(uif->hw, baud) : -EINVAL; }
 
 int uiox_uart_if_set_format(uiox_uart_if_t *uif,
                               uiox_uart_bits_t bits,
                               uiox_uart_stop_t stop,
                               uiox_uart_parity_t parity)
 { return uif ? uiox_uart_hw_set_format(uif->hw, bits, stop, parity) : -EINVAL; }
 
 int uiox_uart_if_set_flow(uiox_uart_if_t *uif, uiox_uart_flow_t flow)
 {
     if (!uif) return -EINVAL;
     uif->hw_flow_en = (flow == UIOX_UART_FLOW_HW_RTS);
     return uiox_uart_hw_set_flow(uif->hw, flow);
 }
 
 void uiox_uart_if_flush(uiox_uart_if_t *uif)
 {
     if (!uif) return;
     if_tx_drain(uif);
     uiox_uart_hw_tx_flush(uif->hw);
 }
 
 void uiox_uart_if_send_break(uiox_uart_if_t *uif, uint32_t ms)
 { if (uif) uiox_uart_hw_send_break(uif->hw, ms); }
 
 /* ── IRQ handler ──────────────────────────────────────────────── */
 
 uiox_uart_evt_t *uiox_uart_if_irq_handle(uiox_uart_if_t *uif,
                                            uint32_t now_ms)
 {
     if (!uif) return NULL;
     uint32_t pending = uif->hw->pending_irq;
     uif->hw->pending_irq = 0u;
     if (!pending) return NULL;
 
     uiox_uart_evt_t *e = uiox_uart_evt_alloc();
     if (!e) { uif->stats.overruns++; return NULL; }
 
     e->timestamp_ms = now_ms;
     e->error_flags  = 0u;
 
     /* ── RX data ready / RX timeout ─────────────────────────── */
     if (pending & (UIOX_UART_IRQ_RX_DATA | UIOX_UART_IRQ_RX_TIMEOUT)) {
         uint32_t count = 0u;
         while (uiox_uart_hw_rx_ready(uif->hw)) {
             int c = uiox_uart_hw_getc(uif->hw);
             if (c < 0) break;
             /* SW XOFF: intercept XON/XOFF from peer */
             if (uif->hw->cfg.flow_ctrl == UIOX_UART_FLOW_SW_XON) {
                 if ((uint8_t)c == uif->xoff_char) {
                     uif->xoff_sent = true; continue;
                 }
                 if ((uint8_t)c == uif->xon_char)  {
                     uif->xoff_sent = false; continue;
                 }
             }
             if (uiox_uart_ring_put(&uif->rx_ring, (uint8_t)c) != 0) {
                 uif->stats.overruns++;
                 e->error_flags |= UIOX_UART_ERR_OVERRUN;
             }
             uif->stats.rx_bytes++;
             count++;
         }
         e->type     = (pending & UIOX_UART_IRQ_RX_TIMEOUT)
                       ? UIOX_UART_EVT_RX_TIMEOUT : UIOX_UART_EVT_RX_DATA;
         e->rx_count = count;
         uif->stats.rx_irqs++;
 
         /* SW XON/XOFF: send XOFF if RX buffer nearly full */
         if (uif->hw->cfg.flow_ctrl == UIOX_UART_FLOW_SW_XON &&
             !uif->xoff_sent &&
             uiox_uart_ring_avail(&uif->rx_ring) > XOFF_THRESHOLD) {
             uiox_uart_hw_putc(uif->hw, (char)uif->xoff_char);
             uif->xoff_sent = true;
         }
     }
 
     /* ── TX empty ──────────────────────────────────────────── */
     else if (pending & UIOX_UART_IRQ_TX_EMPTY) {
         if_tx_drain(uif);
         e->type = UIOX_UART_EVT_TX_DONE;
         uif->stats.tx_irqs++;
     }
 
     /* ── Line error ─────────────────────────────────────────── */
     else if (pending & UIOX_UART_IRQ_LINE_ERR) {
         e->type        = UIOX_UART_EVT_LINE_ERR;
         e->error_flags = uif->hw->error_flags;
         if (uif->hw->error_flags & UIOX_UART_ERR_BREAK)
             e->type = UIOX_UART_EVT_BREAK_DETECT;
         if (uif->hw->error_flags & UIOX_UART_ERR_OVERRUN)
             uif->stats.overruns++;
         if (uif->hw->error_flags & UIOX_UART_ERR_PARITY)
             uif->stats.parity_errs++;
         if (uif->hw->error_flags & UIOX_UART_ERR_FRAMING)
             uif->stats.framing_errs++;
         if (uif->hw->error_flags & UIOX_UART_ERR_BREAK)
             uif->stats.break_events++;
         uif->stats.err_irqs++;
     }
 
     /* ── Modem status ───────────────────────────────────────── */
     else if (pending & UIOX_UART_IRQ_MODEM) {
         e->type = UIOX_UART_EVT_MODEM_CHANGE;
     }
 
     else {
         e->type = UIOX_UART_EVT_NONE;
     }
 
     return e;
 }
 
 void uiox_uart_if_stats_get(const uiox_uart_if_t *uif,
                               uiox_uart_if_stats_t *out)
 { if (uif && out) memcpy(out, &uif->stats, sizeof(*out)); }
 