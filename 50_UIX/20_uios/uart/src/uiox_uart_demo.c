/**
 * @file  uiox_uart_demo.c
 * @brief UIOX UART stack demo — stub HAL + full stack exercise.
 *        Mirrors uiox_rtc_demo.c / uiox_chg_demo.c structure.
 * @date  2026-07-05
 */

 #include "uiox_uart_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <errno.h>
 
 /* =========================================================================
  * Stub register bank and simulation flags
  * ====================================================================== */
 
 static uint32_t s_pl011_regs[0x50 / 4];
 static char     s_rx_inject[256];
 static uint32_t s_rx_inject_len  = 0u;
 static uint32_t s_rx_inject_pos  = 0u;
 static bool     s_sim_break      = false;
 static bool     s_sim_overrun    = false;
 static bool     s_sim_parity_err = false;
 
 static void sim_inject_string(const char *s)
 {
     uint32_t n = 0u;
     while (*s && n < sizeof(s_rx_inject) - 1u) {
         s_rx_inject[n++] = *s++;
     }
     s_rx_inject[n]   = '\0';
     s_rx_inject_len  = n;
     s_rx_inject_pos  = 0u;
 }
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int stub_init(uiox_uart_hw_t *hw, const uiox_uart_cfg_t *cfg)
 {
     memset(s_pl011_regs, 0, sizeof(s_pl011_regs));
     /* Simulate FR: TX not full, RX empty initially */
     s_pl011_regs[PL011_FR / 4] = PL011_FR_TXFE | PL011_FR_RXFE;
     printf("  [hal] init  %s  base=0x%08lx  baud=%u\n",
            hw->model, (unsigned long)hw->base, cfg->baud);
     return 0;
 }
 
 static void stub_deinit(uiox_uart_hw_t *hw) { (void)hw; }
 
 static void stub_putc(uiox_uart_hw_t *hw, char c)
 {
     (void)hw;
     /* Write to simulated DR — captured by test harness */
     s_pl011_regs[PL011_DR / 4] = (uint32_t)(uint8_t)c;
     /* Print to host stdout for demo visibility */
     if (c != '\r') putchar(c);
 }
 
 static int stub_getc(uiox_uart_hw_t *hw)
 {
     (void)hw;
     hw->error_flags = 0u;
     if (s_sim_overrun)    hw->error_flags |= UIOX_UART_ERR_OVERRUN;
     if (s_sim_parity_err) hw->error_flags |= UIOX_UART_ERR_PARITY;
     if (s_sim_break)      hw->error_flags |= UIOX_UART_ERR_BREAK;
     if (s_rx_inject_pos < s_rx_inject_len)
         return (int)(uint8_t)s_rx_inject[s_rx_inject_pos++];
     return -1;
 }
 
 static bool stub_rx_ready(uiox_uart_hw_t *hw)
 { (void)hw; return s_rx_inject_pos < s_rx_inject_len; }
 
 static bool stub_tx_ready(uiox_uart_hw_t *hw)
 { (void)hw; return true; }
 
 static void stub_tx_flush(uiox_uart_hw_t *hw) { (void)hw; }
 
 static int stub_set_baud(uiox_uart_hw_t *hw, uint32_t baud)
 { printf("  [hal] set_baud  %u\n", baud); hw->cfg.baud = baud; return 0; }
 
 static int stub_set_format(uiox_uart_hw_t *hw,
                              uiox_uart_bits_t bits,
                              uiox_uart_stop_t stop,
                              uiox_uart_parity_t parity)
 {
     printf("  [hal] set_format  %u%c%u\n",
            (uint32_t)bits,
            parity == UIOX_UART_PARITY_NONE ? 'N' :
            parity == UIOX_UART_PARITY_ODD  ? 'O' : 'E',
            stop == UIOX_UART_STOP_2 ? 2u : 1u);
     hw->cfg.data_bits = bits;
     hw->cfg.stop_bits = stop;
     hw->cfg.parity    = parity;
     return 0;
 }
 
 static int stub_set_flow(uiox_uart_hw_t *hw, uiox_uart_flow_t flow)
 { printf("  [hal] set_flow  %u\n", (uint32_t)flow);
   hw->cfg.flow_ctrl = flow; return 0; }
 
 static int stub_set_loopback(uiox_uart_hw_t *hw, bool en)
 { printf("  [hal] loopback  %d\n", (int)en); hw->cfg.loopback = en; return 0; }
 
 static void stub_send_break(uiox_uart_hw_t *hw, uint32_t ms)
 { (void)hw; printf("  [hal] BREAK %u ms\n", ms); }
 
 static void stub_irq_enable(uiox_uart_hw_t *hw, uint32_t mask)
 { (void)hw; printf("  [hal] irq_enable  0x%08x\n", mask); }
 
 static void stub_irq_disable(uiox_uart_hw_t *hw, uint32_t mask)
 { (void)hw; printf("  [hal] irq_disable 0x%08x\n", mask); }
 
 static uint32_t stub_irq_status(uiox_uart_hw_t *hw)
 { return hw->pending_irq; }
 
 static void stub_isr(uiox_uart_hw_t *hw)
 {
     if (s_sim_break || s_sim_overrun || s_sim_parity_err)
         hw->pending_irq |= UIOX_UART_IRQ_LINE_ERR;
     else if (s_rx_inject_pos < s_rx_inject_len)
         hw->pending_irq |= UIOX_UART_IRQ_RX_DATA;
     else
         hw->pending_irq |= UIOX_UART_IRQ_TX_EMPTY;
     printf("  [hal] ISR  pending=0x%08x\n", hw->pending_irq);
 }
 
 static void stub_gpio_write(uiox_uart_hw_t *hw, uint32_t pin, bool val)
 { (void)hw; printf("  [hal] GPIO pin=%u val=%d\n", pin, (int)val); }
 
 static bool stub_gpio_read(uiox_uart_hw_t *hw, uint32_t pin)
 { (void)hw; (void)pin; return false; }
 
 static const uiox_uart_hw_ops_t stub_ops = {
     .init         = stub_init,
     .deinit       = stub_deinit,
     .putc         = stub_putc,
     .getc         = stub_getc,
     .rx_ready     = stub_rx_ready,
     .tx_ready     = stub_tx_ready,
     .tx_flush     = stub_tx_flush,
     .set_baud     = stub_set_baud,
     .set_format   = stub_set_format,
     .set_flow     = stub_set_flow,
     .set_loopback = stub_set_loopback,
     .send_break   = stub_send_break,
     .irq_enable   = stub_irq_enable,
     .irq_disable  = stub_irq_disable,
     .irq_status   = stub_irq_status,
     .gpio_write   = stub_gpio_write,
     .gpio_read    = stub_gpio_read,
     .isr          = stub_isr,
 };
 
 static uiox_uart_hw_t s_hw = {
     .base    = UIOX_UART_ARM64_BASE,
     .clk_hz  = UIOX_UART_ARM64_CLK,
     .irq     = 33u,
     .variant = UIOX_UART_PL011,
     .caps    = UIOX_UART_CAP_FIFO | UIOX_UART_CAP_HW_FLOW |
                UIOX_UART_CAP_MODEM | UIOX_UART_CAP_BREAK,
     .model   = "ARM PL011 (stub)",
 };
 
 /* =========================================================================
  * Event callback
  * ====================================================================== */
 
 static void on_uart_event(uiox_uart_ev_t ev,
                            uiox_uart_evt_t *data, void *ctx)
 {
     (void)ctx;
     printf("  [event] %-14s  rx_count=%u  err=0x%02x\n",
            uiox_uart_ev_name(ev),
            data ? data->rx_count   : 0u,
            data ? data->error_flags : 0u);
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX UART Stack Demo ===\n\n");
 
     /* ------------------------------------------------------------------ */
     printf("--- Open (PL011, 115200 8N1) ---\n");
     uiox_uart_device_t     dev;
     uiox_uart_cfg_t        cfg = UIOX_UART_CFG_DEFAULT;
     uiox_uart_open_params_t p = {
         .hw         = &s_hw,
         .hw_ops     = &stub_ops,
         .cfg        = cfg,
         .is_console = true,
         .evt_cb     = on_uart_event,
     };
     int rc = uiox_uart_open(&dev, &p);
     printf("  open rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Start ---\n");
     rc = uiox_uart_start(&dev);
     printf("  start rc=%d  state=%s\n",
            rc, uiox_uart_state_name(dev.subsys.state));
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Device info ---\n");
     uiox_uart_print_info(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- puts: Hello UIOX UART ---\n");
     uiox_uart_puts(&dev, "Hello UIOX UART\n");
 
     /* ------------------------------------------------------------------ */
     printf("\n--- printf: version banner ---\n");
     uiox_uart_printf(&dev, "UIOX UART v%u.%u.%u on %s\n",
                      1u, 0u, 0u, s_hw.model);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Reconfigure: 9600 baud, 7E2 ---\n");
     uiox_uart_set_baud(&dev, 9600u);
     uiox_uart_set_format(&dev, UIOX_UART_BITS_7,
                           UIOX_UART_STOP_2, UIOX_UART_PARITY_EVEN);
     uiox_uart_print_info(&dev);
 
     /* Restore 115200 8N1 */
     uiox_uart_set_baud(&dev, 115200u);
     uiox_uart_set_format(&dev, UIOX_UART_BITS_8,
                           UIOX_UART_STOP_1, UIOX_UART_PARITY_NONE);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Enable HW flow control ---\n");
     uiox_uart_set_flow(&dev, UIOX_UART_FLOW_HW_RTS);
     uiox_uart_set_flow(&dev, UIOX_UART_FLOW_NONE);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Switch to cooked line discipline ---\n");
     uiox_uart_set_ldisc(&dev, UIOX_UART_LDISC_COOKED);
     uiox_uart_set_echo(&dev, true);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate RX: inject 'ls -l\\n' ---\n");
     sim_inject_string("ls -l\n");
     stub_isr(&s_hw);   /* Trigger RX IRQ */
     for (uint32_t t = 10u; t <= 30u; t += 10u)
         uiox_uart_tick(&dev, t);
     /* Check if line is ready */
     if (uiox_uart_proto_line_ready(&dev.subsys.proto)) {
         char line[128];
         uiox_uart_proto_read_line(&dev.subsys.proto, line, sizeof(line));
         printf("  Line received: '%s'\n", line);
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate RX with backspace editing ---\n");
     sim_inject_string("helo\b\bllo\n");   /* type 'helo', fix to 'hello' */
     stub_isr(&s_hw);
     for (uint32_t t = 40u; t <= 60u; t += 10u)
         uiox_uart_tick(&dev, t);
     if (uiox_uart_proto_line_ready(&dev.subsys.proto)) {
         char line[128];
         uiox_uart_proto_read_line(&dev.subsys.proto, line, sizeof(line));
         printf("  Line received: '%s'\n", line);
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate ANSI escape sequence ---\n");
     sim_inject_string("\033[1;32mGreen Bold\033[0m\n");
     stub_isr(&s_hw);
     for (uint32_t t = 70u; t <= 90u; t += 10u)
         uiox_uart_tick(&dev, t);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- ANSI cursor movement TX test ---\n");
     uiox_uart_proto_set_colour(&dev.subsys.proto, 3u, 0u);  /* yellow fg */
     uiox_uart_proto_puts(&dev.subsys.proto, "UIOX> ");
     uiox_uart_proto_reset_attr(&dev.subsys.proto);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Switch to raw mode ---\n");
     uiox_uart_set_ldisc(&dev, UIOX_UART_LDISC_RAW);
     sim_inject_string("raw");
     stub_isr(&s_hw);
     uiox_uart_tick(&dev, 100u);
     uint8_t raw_buf[8];
     int n = uiox_uart_read(&dev, raw_buf, sizeof(raw_buf));
     printf("  Raw read: %d bytes: ", n);
     for (int i = 0; i < n; i++) printf("%02x ", raw_buf[i]);
     printf("\n");
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate parity error ---\n");
     s_sim_parity_err = true;
     stub_isr(&s_hw);
     uiox_uart_tick(&dev, 110u);
     s_sim_parity_err = false;
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate BREAK condition ---\n");
     s_sim_break = true;
     stub_isr(&s_hw);
     uiox_uart_tick(&dev, 120u);
     s_sim_break = false;
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Send BREAK (250 ms) ---\n");
     uiox_uart_send_break(&dev, 250u);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- GPIO: assert RTS, read CTS ---\n");
     if (dev.hw->priv) {
         ((const uiox_uart_hw_ops_t *)dev.hw->priv)->gpio_write(
             dev.hw, 1u, true);   /* RTS = 1 */
         bool cts = ((const uiox_uart_hw_ops_t *)dev.hw->priv)->gpio_read(
             dev.hw, 1u);         /* read CTS */
         printf("  CTS = %d\n", (int)cts);
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- TX write burst (64 bytes) ---\n");
     static const uint8_t tx_data[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz.";
     n = uiox_uart_write(&dev, tx_data, sizeof(tx_data));
     printf("  Wrote %d bytes\n", n);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Tick loop (5 × 10 ms) ---\n");
     for (uint32_t t = 200u; t <= 240u; t += 10u)
         uiox_uart_tick(&dev, t);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Statistics ---\n");
     uiox_uart_print_stats(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Final device info ---\n");
     uiox_uart_print_info(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Stop and close ---\n");
     uiox_uart_stop(&dev);
     printf("  State: %s\n", uiox_uart_state_name(dev.subsys.state));
     uiox_uart_close(&dev);
     printf("  Device: CLOSED\n");
 
     printf("\n=== UIOX UART Demo complete ===\n");
     return 0;
 }
 