/**
 * @file  uiox_uart_device.c
 * @brief UIOX UART application device API.
 * @date  2026-07-05
 */

 #include "uiox_uart_device.h"
 #include <string.h>
 #include <stdarg.h>
 #include <stdio.h>
 #include <errno.h>
 
 int uiox_uart_open(uiox_uart_device_t *dev,
                     const uiox_uart_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
     int rc  = uiox_uart_hw_init(p->hw, p->hw_ops, &p->cfg);
     if (rc < 0) return rc;
     rc = uiox_uart_subsys_init(&dev->subsys, p->hw, p->is_console);
     if (rc < 0) return rc;
     if (p->evt_cb)
         uiox_uart_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
     dev->open = true;
     return 0;
 }
 
 int  uiox_uart_start(uiox_uart_device_t *dev)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_uart_subsys_start(&dev->subsys); }
 
 void uiox_uart_stop(uiox_uart_device_t *dev)
 { if (!dev || !dev->open) return;
   uiox_uart_subsys_stop(&dev->subsys); }
 
 void uiox_uart_close(uiox_uart_device_t *dev)
 { if (!dev || !dev->open) return;
   uiox_uart_stop(dev);
   uiox_uart_hw_deinit(dev->hw);
   dev->open = false; }
 
 void uiox_uart_tick(uiox_uart_device_t *dev, uint32_t now_ms)
 { if (!dev || !dev->open) return;
   uiox_uart_subsys_tick(&dev->subsys, now_ms); }
 
 int uiox_uart_putc(uiox_uart_device_t *dev, char c)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_uart_subsys_putc(&dev->subsys, c); }
 
 int uiox_uart_puts(uiox_uart_device_t *dev, const char *s)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_uart_subsys_puts(&dev->subsys, s); }
 
 int uiox_uart_write(uiox_uart_device_t *dev,
                      const uint8_t *buf, uint32_t len)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_uart_subsys_write(&dev->subsys, buf, len); }
 
 int uiox_uart_getc(uiox_uart_device_t *dev)
 { if (!dev || !dev->open) return -1;
   return uiox_uart_subsys_getc(&dev->subsys); }
 
 int uiox_uart_read(uiox_uart_device_t *dev, uint8_t *buf, uint32_t len)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_uart_subsys_read(&dev->subsys, buf, len); }
 
 int uiox_uart_printf(uiox_uart_device_t *dev, const char *fmt, ...)
 {
     if (!dev || !dev->open) return -EINVAL;
     char buf[512];
     va_list ap;
     va_start(ap, fmt);
     int n = vsnprintf(buf, sizeof(buf), fmt, ap);
     va_end(ap);
     uiox_uart_subsys_puts(&dev->subsys, buf);
     return n;
 }
 
 int uiox_uart_set_baud(uiox_uart_device_t *dev, uint32_t baud)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_uart_if_set_baud(&dev->subsys.uif, baud); }
 
 int uiox_uart_set_format(uiox_uart_device_t *dev,
                           uiox_uart_bits_t bits,
                           uiox_uart_stop_t stop,
                           uiox_uart_parity_t parity)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_uart_if_set_format(&dev->subsys.uif, bits, stop, parity); }
 
 int uiox_uart_set_flow(uiox_uart_device_t *dev, uiox_uart_flow_t flow)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_uart_if_set_flow(&dev->subsys.uif, flow); }
 
 int uiox_uart_set_ldisc(uiox_uart_device_t *dev, uiox_uart_ldisc_t mode)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_uart_proto_set_ldisc(&dev->subsys.proto, mode); }
 
 void uiox_uart_set_echo(uiox_uart_device_t *dev, bool echo)
 { if (dev && dev->open) uiox_uart_proto_set_echo(&dev->subsys.proto, echo); }
 
 void uiox_uart_send_break(uiox_uart_device_t *dev, uint32_t ms)
 { if (dev && dev->open) uiox_uart_if_send_break(&dev->subsys.uif, ms); }
 
 void uiox_uart_print_info(const uiox_uart_device_t *dev)
 {
     if (!dev) return;
     const uiox_uart_hw_t *hw = dev->hw;
     const uiox_uart_subsys_t *s = &dev->subsys;
     printf("  Variant     : %s\n", uiox_uart_variant_name(hw->variant));
     printf("  Model       : %s\n", hw->model);
     printf("  Base        : 0x%08lx\n", (unsigned long)hw->base);
     printf("  IRQ         : %u\n", hw->irq);
     printf("  State       : %s\n", uiox_uart_state_name(s->state));
     printf("  Console     : %s\n", s->is_console ? "YES" : "NO");
     printf("  Baud        : %u\n", hw->cfg.baud);
     printf("  Format      : %u%c%u\n",
            (uint32_t)hw->cfg.data_bits,
            hw->cfg.parity == UIOX_UART_PARITY_NONE ? 'N' :
            hw->cfg.parity == UIOX_UART_PARITY_ODD  ? 'O' : 'E',
            hw->cfg.stop_bits == UIOX_UART_STOP_2 ? 2u : 1u);
     printf("  Flow ctrl   : %s\n",
            hw->cfg.flow_ctrl == UIOX_UART_FLOW_NONE   ? "None" :
            hw->cfg.flow_ctrl == UIOX_UART_FLOW_HW_RTS ? "RTS/CTS" : "XON/XOFF");
     printf("  Line disc   : %s\n",
            s->proto.ldisc == UIOX_UART_LDISC_RAW    ? "raw" :
            s->proto.ldisc == UIOX_UART_LDISC_COOKED ? "cooked" : "cbreak");
     printf("  Echo        : %s\n", s->proto.echo ? "ON" : "OFF");
     printf("  RX buf free : %u\n",
            uiox_uart_ring_free(&s->uif.rx_ring));
     printf("  TX buf free : %u\n",
            uiox_uart_ring_free(&s->uif.tx_ring));
 }
 
 void uiox_uart_print_stats(uiox_uart_device_t *dev)
 {
     if (!dev) return;
     const uiox_uart_subsys_t *s = &dev->subsys;
     uiox_uart_if_stats_t st;
     uiox_uart_if_stats_get(&s->uif, &st);
     printf("  Uptime      : %llu ms\n", (unsigned long long)s->uptime_ms);
     printf("  Ticks       : %u\n",  s->tick_count);
     printf("  RX events   : %u\n",  s->rx_event_count);
     printf("  Errors      : %u\n",  s->error_count);
     printf("  TX bytes    : %llu\n", (unsigned long long)st.tx_bytes);
     printf("  RX bytes    : %llu\n", (unsigned long long)st.rx_bytes);
     printf("  TX IRQs     : %u\n",  st.tx_irqs);
     printf("  RX IRQs     : %u\n",  st.rx_irqs);
     printf("  Err IRQs    : %u\n",  st.err_irqs);
     printf("  Overruns    : %u\n",  st.overruns);
     printf("  Parity errs : %u\n",  st.parity_errs);
     printf("  Frame errs  : %u\n",  st.framing_errs);
     printf("  Break events: %u\n",  st.break_events);
     printf("  ANSI seqs   : %u\n",  s->proto.ansi_sequences);
     printf("  Lines recv  : %u\n",  s->proto.lines_received);
     printf("  Evt pool    : %u / %u\n",
            uiox_uart_evt_free_cnt(), UIOX_UART_EVT_POOL_SIZE);
 }
 