/**
 * @file    uiox_kbd_demo.c
 * @brief   UIOX Keyboard stack end-to-end demonstration.
 *
 * Demonstrates: HAL init → matrix scan → debounce → keymap →
 *   event processing → shortcut dispatch → LED sync → statistics.
 *
 * Uses stub HAL ops — replace with real GPIO/timer driver.
 * @date    2026-05-27
 */
//Demo Application
 #include "uiox_kbd_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 
 /* =========================================================================
  * Simulated key sequence
  * Each entry: { row, col, pressed, at_scan }
  * ====================================================================== */
 
 typedef struct { uint8_t row, col; bool pressed; uint32_t at_scan; } sim_key_t;
 
 static const sim_key_t s_sim[] = {
     { 0, 0, true,  1  },   /* row0,col0 = 'A' press    at scan 1  */
     { 0, 0, false, 3  },   /* 'A' release               at scan 3  */
     { 0, 3, true,  4  },   /* 'D' press                 at scan 4  */
     { 3, 6, true,  4  },   /* LSHIFT press (row3,col6)  at scan 4  */
     { 0, 3, false, 6  },   /* 'D' release               at scan 6  */
     { 3, 6, false, 6  },   /* LSHIFT release            at scan 6  */
     { 0, 2, true,  7  },   /* 'C' press (Ctrl+C shortcut test) */
     { 2, 0, true,  7  },   /* LCTRL press (row2,col0)  */
     { 0, 2, false, 9  },   /* 'C' release               */
     { 2, 0, false, 9  },   /* LCTRL release             */
     { 3, 9, true,  10 },   /* CAPSLOCK press (row3,col9)*/
     { 3, 9, false, 11 },   /* CAPSLOCK release          */
     { 0, 0, true,  12 },   /* 'A' press again (caps = 'A') */
     { 0, 0, false, 14 },   /* 'A' release               */
 };
 
 #define SIM_COUNT  (sizeof(s_sim) / sizeof(s_sim[0]))
 
 static uint32_t s_scan_no = 0;
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int stub_init(uiox_kbd_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] init  matrix %ux%u  timer_clk=%u Hz\n",
            hw->matrix.num_rows, hw->matrix.num_cols, hw->clk_hz);
     return 0;
 }
 
 static void stub_deinit(uiox_kbd_hw_t *hw)
 { (void)hw; printf("  [hal] deinit\n"); }
 
 static int stub_scan_row(uiox_kbd_hw_t *hw, uint8_t row, uint16_t *cols_out)
 {
     (void)hw;
     *cols_out = 0u;
     /* Apply simulated key presses for this scan cycle */
     for (size_t i = 0; i < SIM_COUNT; i++) {
         if (s_sim[i].row == row &&
             s_sim[i].at_scan <= s_scan_no &&
             s_sim[i].pressed) {
             /* Check if it was released by now */
             bool still_pressed = true;
             for (size_t j = 0; j < SIM_COUNT; j++) {
                 if (s_sim[j].row == row &&
                     s_sim[j].col == s_sim[i].col &&
                     !s_sim[j].pressed &&
                     s_sim[j].at_scan <= s_scan_no) {
                     still_pressed = false;
                     break;
                 }
             }
             if (still_pressed)
                 *cols_out |= (uint16_t)(1u << s_sim[i].col);
         }
     }
     return 0;
 }
 
 static int stub_read_direct(uiox_kbd_hw_t *hw, uint32_t *keys_out)
 { (void)hw; *keys_out = 0u; return 0; }
 
 static int stub_set_leds(uiox_kbd_hw_t *hw, uint8_t led_mask)
 {
     (void)hw;
     printf("  [hal] LEDs  ");
     if (led_mask & UIOX_KBD_LED_CAPSLOCK)   printf("CAPS ");
     if (led_mask & UIOX_KBD_LED_NUMLOCK)    printf("NUM ");
     if (led_mask & UIOX_KBD_LED_SCROLLLOCK) printf("SCROLL ");
     if (!led_mask)                           printf("(all off)");
     printf("\n");
     return 0;
 }
 
 static int stub_set_backlight(uiox_kbd_hw_t *hw, uint8_t level)
 {
     (void)hw;
     printf("  [hal] backlight level=%u\n", level);
     return 0;
 }
 
 static void stub_delay_us(uiox_kbd_hw_t *hw, uint32_t us)
 { (void)hw; (void)us; }
 
 static void stub_isr(uiox_kbd_hw_t *hw) { (void)hw; }
 
 static int stub_i2c_read (uiox_kbd_hw_t *hw, uint8_t reg, uint8_t *val)
 { (void)hw; (void)reg; *val = 0; return 0; }
 
 static int stub_i2c_write(uiox_kbd_hw_t *hw, uint8_t reg, uint8_t val)
 { (void)hw; (void)reg; (void)val; return 0; }
 
 static int stub_ps2_send(uiox_kbd_hw_t *hw, uint8_t cmd)
 { (void)hw; (void)cmd; return 0; }
 
 static int stub_ps2_recv(uiox_kbd_hw_t *hw, uint8_t *val, uint32_t tms)
 { (void)hw; (void)tms; *val = 0; return -1; }
 
 static int stub_spi_transfer(uiox_kbd_hw_t *hw,
                               const uint8_t *tx, uint8_t *rx, uint16_t len)
 { (void)hw; (void)tx; (void)rx; (void)len; return 0; }
 
 static const uiox_kbd_hw_ops_t stub_ops = {
     .init          = stub_init,
     .deinit        = stub_deinit,
     .scan_row      = stub_scan_row,
     .read_direct   = stub_read_direct,
     .i2c_read      = stub_i2c_read,
     .i2c_write     = stub_i2c_write,
     .spi_transfer  = stub_spi_transfer,
     .ps2_send      = stub_ps2_send,
     .ps2_recv      = stub_ps2_recv,
     .set_leds      = stub_set_leds,
     .set_backlight = stub_set_backlight,
     .delay_us      = stub_delay_us,
     .isr           = stub_isr,
 };
 
 /* =========================================================================
  * Hardware device instance — 8×8 matrix keyboard
  * ====================================================================== */
 
 static uiox_kbd_hw_t s_hw = {
     .base_addr  = 0x40010000uL,
     .irq        = 35,
     .caps       = UIOX_KBD_CAP_MATRIX   |
                   UIOX_KBD_CAP_LED      |
                   UIOX_KBD_CAP_BACKLIGHT|
                   UIOX_KBD_CAP_IRQ      |
                   UIOX_KBD_CAP_WAKEUP,
     .if_type    = UIOX_KBD_IF_MATRIX,
     .clk_hz     = 48000000u,
 
     .matrix = {
         /* Row GPIO pins: PA0..PA7 */
         .row_pins  = { 0,1,2,3,4,5,6,7 },
         /* Col GPIO pins: PB0..PB7 */
         .col_pins  = { 16,17,18,19,20,21,22,23 },
         .num_rows  = 8,
         .num_cols  = 8,
         .active_low     = false,
         .scan_period_us = 5000u,  /* 5 ms scan period */
         .settle_us      = 5u,
     },
 
     /* LED pins: PC0..PC2 */
     .led_pins = { 32, 33, 34, 0, 0, 35 },
 };
 
 /* =========================================================================
  * Shortcut callbacks
  * ====================================================================== */
 
 static void on_ctrl_c(void *ctx)
 {
     (void)ctx;
     printf("  [shortcut] Ctrl+C — interrupt signal!\n");
 }
 
 static void on_ctrl_z(void *ctx)
 {
     (void)ctx;
     printf("  [shortcut] Ctrl+Z — suspend signal!\n");
 }
 
 static void on_switch_qwerty(void *ctx)
 {
     (void)ctx;
     uiox_kbd_map_set_layout(UIOX_KBD_LAYOUT_QWERTY);
     printf("  [shortcut] Layout → QWERTY-US\n");
 }
 
 static void on_switch_qwertz(void *ctx)
 {
     (void)ctx;
     uiox_kbd_map_set_layout(UIOX_KBD_LAYOUT_QWERTZ);
     printf("  [shortcut] Layout → QWERTZ-DE\n");
 }
 
 /* =========================================================================
  * Event callback — prints all key events
  * ====================================================================== */
 
 static const char *ev_type_name(uint8_t t)
 {
     switch (t) {
     case UIOX_KBD_EV_PRESS:   return "PRESS  ";
     case UIOX_KBD_EV_RELEASE: return "RELEASE";
     case UIOX_KBD_EV_REPEAT:  return "REPEAT ";
     case UIOX_KBD_EV_SPECIAL: return "SPECIAL";
     default:                   return "???????";
     }
 }
 
 static void on_key_event(const uiox_kbd_event_t *ev, void *ctx)
 {
     (void)ctx;
     char unicode_str[8] = {0};
     if (ev->unicode >= 0x20u && ev->unicode < 0x7Fu)
         unicode_str[0] = (char)ev->unicode;
     else if (ev->unicode)
         snprintf(unicode_str, sizeof(unicode_str), "U+%04X",
                  (unsigned)ev->unicode);
     else
         snprintf(unicode_str, sizeof(unicode_str), "---");
 
     printf("  [event] %s  sc=0x%02X  kc=0x%04X  char='%s'"
            "  mod=0x%02X  ts=%.3f ms\n",
            ev_type_name(ev->ev_type),
            ev->scancode,
            ev->keycode,
            unicode_str,
            ev->modifiers,
            (double)ev->ts_ns / 1e6);
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX Keyboard Stack Demo ===\n\n");
 
     /* ------------------------------------------------------------------ */
     /* 1. Open device                                                      */
     /* ------------------------------------------------------------------ */
 
     uiox_kbd_device_t dev;
     uiox_kbd_open_params_t p;
     memset(&p, 0, sizeof(p));
 
     p.hw      = &s_hw;
     p.hw_ops  = &stub_ops;
     p.if_type = UIOX_KBD_IF_MATRIX;
     p.layout  = UIOX_KBD_LAYOUT_QWERTY;
 
     /* Event processing config */
     p.event_cfg.debounce_ms        = UIOX_KBD_DEBOUNCE_MS;
     p.event_cfg.repeat_delay_ms    = UIOX_KBD_REPEAT_DELAY_MS;
     p.event_cfg.repeat_interval_ms = UIOX_KBD_REPEAT_INTERVAL_MS;
     p.event_cfg.repeat_enabled     = true;
 
     /* Backlight dim after 5 seconds idle */
     p.dim_timeout_ms   = 5000u;
     p.active_backlight = 200u;
     p.dim_backlight    = 20u;
 
     printf("--- Open ---\n");
     int rc = uiox_kbd_open(&dev, &p);
     if (rc < 0) {
         printf("[error] uiox_kbd_open failed: %d\n", rc);
         return 1;
     }
     printf("  Layout     : %s\n", uiox_kbd_layout_name());
     printf("  Interface  : Matrix %ux%u\n",
            s_hw.matrix.num_rows, s_hw.matrix.num_cols);
     printf("  Scan period: %u µs\n", s_hw.matrix.scan_period_us);
 
     /* ------------------------------------------------------------------ */
     /* 2. Register shortcuts                                               */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Register shortcuts ---\n");
     uiox_kbd_add_shortcut(&dev,
         UIOX_KBD_MOD_LCTRL, UIOX_KEY_C,
         on_ctrl_c, NULL, "Ctrl+C");
     uiox_kbd_add_shortcut(&dev,
         UIOX_KBD_MOD_LCTRL, UIOX_KEY_Z,
         on_ctrl_z, NULL, "Ctrl+Z");
     uiox_kbd_add_shortcut(&dev,
         UIOX_KBD_MOD_LCTRL | UIOX_KBD_MOD_LSHIFT, UIOX_KEY_F1,
         on_switch_qwerty, NULL, "Ctrl+Shift+F1 = QWERTY");
     uiox_kbd_add_shortcut(&dev,
         UIOX_KBD_MOD_LCTRL | UIOX_KBD_MOD_LSHIFT, UIOX_KEY_F2,
         on_switch_qwertz, NULL, "Ctrl+Shift+F2 = QWERTZ");
     printf("  Registered 4 shortcuts\n");
 
     /* ------------------------------------------------------------------ */
     /* 3. Register event callback                                          */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Register event callback ---\n");
     uiox_kbd_add_callback(&dev, on_key_event, NULL, 0u);
     printf("  Registered 1 global event callback\n");
 
     /* ------------------------------------------------------------------ */
     /* 4. Set backlight                                                    */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Backlight ---\n");
     uiox_kbd_set_backlight(&dev, 200u);
 
     /* ------------------------------------------------------------------ */
     /* 5. Start device                                                     */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Start ---\n");
     rc = uiox_kbd_start(&dev);
     printf("  Keyboard streaming: ACTIVE  rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     /* 6. Simulated scan loop                                              */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Scan loop (%zu scans) ---\n", SIM_COUNT + 5u);
 
     uint32_t now_ms = 0u;
 
     for (s_scan_no = 0; s_scan_no < (uint32_t)(SIM_COUNT + 5u); s_scan_no++) {
         now_ms += 10u;  /* 10 ms per tick */
         uint64_t ts_ns = (uint64_t)now_ms * 1000000ULL;
 
         printf("\n  [scan %02u  t=%u ms]\n", s_scan_no, now_ms);
 
         /* Tick processes scan + debounce + keymap + shortcuts */
         uiox_kbd_tick(&dev, now_ms, ts_ns);
 
         /* Application poll loop */
         uiox_kbd_event_t ev;
         while (uiox_kbd_poll(&dev, &ev)) {
             /* Events already printed by callback — show poll confirm */
             (void)ev;
         }
     }
 
     /* ------------------------------------------------------------------ */
     /* 7. Mid-stream layout switch                                        */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Layout switch to QWERTZ-DE ---\n");
     rc = uiox_kbd_set_layout(&dev, UIOX_KBD_LAYOUT_QWERTZ);
     printf("  Layout: %s  rc=%d\n", uiox_kbd_layout_name(), rc);
 
     /* One more tick to demonstrate QWERTZ */
     s_scan_no++;
     now_ms += 10u;
     uiox_kbd_tick(&dev, now_ms, (uint64_t)now_ms * 1000000ULL);
 
     /* Switch back */
     uiox_kbd_set_layout(&dev, UIOX_KBD_LAYOUT_QWERTY);
     printf("  Layout restored: %s\n", uiox_kbd_layout_name());
 
     /* ------------------------------------------------------------------ */
     /* 8. LED state query                                                  */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Lock key state ---\n");
     printf("  CapsLock   : %s\n",
            uiox_kbd_lock_state(&dev, UIOX_KBD_LED_CAPSLOCK)
            ? "ON" : "OFF");
     printf("  NumLock    : %s\n",
            uiox_kbd_lock_state(&dev, UIOX_KBD_LED_NUMLOCK)
            ? "ON" : "OFF");
     printf("  ScrollLock : %s\n",
            uiox_kbd_lock_state(&dev, UIOX_KBD_LED_SCROLLLOCK)
            ? "ON" : "OFF");
     printf("  Modifiers  : 0x%02X\n", uiox_kbd_modifiers(&dev));
 
     /* ------------------------------------------------------------------ */
     /* 9. Statistics                                                       */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Statistics ---\n");
     uiox_kbd_print_stats(&dev);
 
     /* ------------------------------------------------------------------ */
     /* 10. Stop and close                                                  */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Stop and close ---\n");
     uiox_kbd_stop(&dev);
     printf("  Keyboard  : STOPPED\n");
     uiox_kbd_close(&dev);
     printf("  Device    : CLOSED\n");
 
     printf("\n=== UIOX Keyboard Demo complete ===\n");
     return 0;
 } 
 