/**
 * @file    uiox_tpwd_demo.c
 * @brief   UIOX Touch-Password stack end-to-end demonstration.
 *
 * Demonstrates:
 *   HAL init → power → PIN enrolment → correct verify →
 *   wrong verify × 3 → lockout → pattern enrolment →
 *   pattern verify → session → logout → audit log
 *
 * @date    2026-06-01
 */

 #include "uiox_tpwd_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <errno.h>
 
 /* =========================================================================
  * Simulated touch sequence injector
  * Each entry: { x, y, pressed, at_tick }
  * ====================================================================== */
 
 typedef struct { uint16_t x; uint16_t y; bool pressed; uint32_t at_tick; }
     sim_touch_t;
 
 /* Panel: 240×320 px, 3×4 grid → cell width=80, cell height=80
  * Cell centres:
  *  col0=40, col1=120, col2=200
  *  row0=40,  row1=120, row2=200, row3=280
  * Digit map: 1=r0c0, 2=r0c1, 3=r0c2
  *            4=r1c0, 5=r1c1, 6=r1c2
  *            7=r2c0, 8=r2c1, 9=r2c2
  *            *=r3c0, 0=r3c1, #=r3c2
  * PIN "1234#":
  */
 
 static const sim_touch_t s_pin_enrol[] = {
     /* '1' */   { 40,  40, true,  2},  { 40,  40, false, 4},
     /* '2' */   {120,  40, true,  6},  {120,  40, false, 8},
     /* '3' */   {200,  40, true, 10},  {200,  40, false,12},
     /* '4' */   { 40, 120, true, 14},  { 40, 120, false,16},
     /* '#' */   {200, 280, true, 18},  {200, 280, false,20},
 };
 
 static const sim_touch_t s_pin_correct[] = {
     { 40,  40, true, 2}, { 40,  40, false, 4},
     {120,  40, true, 6}, {120,  40, false, 8},
     {200,  40, true,10}, {200,  40, false,12},
     { 40, 120, true,14}, { 40, 120, false,16},
     {200, 280, true,18}, {200, 280, false,20},
 };
 
 static const sim_touch_t s_pin_wrong[] = {
     { 40,  40, true, 2}, { 40,  40, false, 4},  /* '1' */
     { 40,  40, true, 6}, { 40,  40, false, 8},  /* '1' (wrong) */
     { 40,  40, true,10}, { 40,  40, false,12},  /* '1' (wrong) */
     { 40,  40, true,14}, { 40,  40, false,16},  /* '1' (wrong) */
     {200, 280, true,18}, {200, 280, false,20},  /* '#' submit  */
 };
 
 static const sim_touch_t s_pattern_enrol[] = {
     /* nodes: 0,1,2,4,5,6 (top row + middle row) */
     { 40,  40, true,  2},   /* node 0 */
     {120,  40, true,  4},   /* node 1 */
     {200,  40, true,  6},   /* node 2 */
     { 40, 120, true,  8},   /* node 3 (=4 in 0-indexed) */
     {120, 120, true, 10},   /* node 4 */
     {200, 120, true, 12},   /* node 5 */
     { 40,  40, false,14},   /* lift → complete */
 };
 
 static const sim_touch_t *s_seq       = NULL;
 static size_t              s_seq_len  = 0;
 static uint32_t            s_tick_now = 0;
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int stub_init(uiox_tpwd_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] init  FT6336  I2C=0x%02X  panel=%ux%u\n",
            hw->i2c_addr, hw->panel.width, hw->panel.height);
     return 0;
 }
 
 static void stub_deinit(uiox_tpwd_hw_t *hw) { (void)hw; }
 
 static int stub_power(uiox_tpwd_hw_t *hw, bool on)
 {
     (void)hw;
     printf("  [hal] power %s\n", on ? "ON" : "OFF");
     return 0;
 }
 
 static int stub_reset(uiox_tpwd_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] reset\n");
     return 0;
 }
 
 static int stub_i2c_read(uiox_tpwd_hw_t *hw, uint8_t reg,
                           uint8_t *buf, uint16_t len)
 { (void)hw; (void)reg; memset(buf, 0, len); return 0; }
 
 static int stub_i2c_write(uiox_tpwd_hw_t *hw, uint8_t reg,
                            const uint8_t *buf, uint16_t len)
 { (void)hw; (void)reg; (void)buf; (void)len; return 0; }
 
 static int stub_read_touch(uiox_tpwd_hw_t *hw, uiox_tpwd_raw_evt_t *evt)
 {
     (void)hw;
     memset(evt, 0, sizeof(*evt));
 
     if (!s_seq || s_seq_len == 0) return 0;
 
     /* Find active entry for current tick */
     for (size_t i = 0; i < s_seq_len; i++) {
         if (s_seq[i].at_tick == s_tick_now) {
             if (s_seq[i].pressed) {
                 evt->pts[0].x       = s_seq[i].x;
                 evt->pts[0].y       = s_seq[i].y;
                 evt->pts[0].active  = true;
                 evt->pts[0].pressure= 128u;
                 evt->num_points     = 1;
                 evt->ts_ns          = (uint64_t)s_tick_now * 10000000ULL;
                 return 1;
             } else {
                 evt->num_points = 0;
                 return 0;
             }
         }
     }
     return 0;
 }
 
 static int stub_set_sensitivity(uiox_tpwd_hw_t *hw, uint8_t lvl)
 { (void)hw; (void)lvl; return 0; }
 
 static int stub_set_backlight(uiox_tpwd_hw_t *hw, uint8_t lvl)
 {
     (void)hw;
     if (lvl > 0)
         printf("  [hal] backlight ON  level=%u\n", lvl);
     else
         printf("  [hal] backlight OFF\n");
     return 0;
 }
 
 static void stub_delay_us(uiox_tpwd_hw_t *hw, uint32_t us)
 { (void)hw; (void)us; }
 
 static void stub_isr(uiox_tpwd_hw_t *hw) { (void)hw; }
 
 static const uiox_tpwd_hw_ops_t stub_ops = {
     .init            = stub_init,
     .deinit          = stub_deinit,
     .power           = stub_power,
     .reset           = stub_reset,
     .i2c_read        = stub_i2c_read,
     .i2c_write       = stub_i2c_write,
     .read_touch      = stub_read_touch,
     .set_sensitivity = stub_set_sensitivity,
     .set_backlight   = stub_set_backlight,
     .delay_us        = stub_delay_us,
     .isr             = stub_isr,
 };
 
 /* =========================================================================
  * Hardware device instance
  * ====================================================================== */
 
 static uiox_tpwd_hw_t s_hw = {
     .i2c_base   = 0x40005400uL,
     .i2c_addr   = 0x38u,
     .irq        = 40,
     .rst_pin    = 5,
     .int_pin    = 6,
     .bl_pin     = 7,
     .caps       = UIOX_TPWD_CAP_SINGLE   |
                   UIOX_TPWD_CAP_BACKLIGHT |
                   UIOX_TPWD_CAP_WAKEUP,
     .chip       = UIOX_TPWD_CHIP_FT6336,
     .panel      = { .width=240, .height=320,
                     .rotation=UIOX_TPWD_ROT_0,
                     .flip_x=false, .flip_y=false },
     .sensitivity= 5u,
 };
 
 /* =========================================================================
  * Event callback
  * ====================================================================== */
 
 static void on_event(uiox_tpwd_evt_t evt,
                       uint8_t attempts_remaining, void *ctx)
 {
     (void)ctx;
     printf("  [event] %-12s  attempts_left=%u\n",
            uiox_tpwd_evt_name(evt), attempts_remaining);
 }
 
 /* =========================================================================
  * Run a simulated sequence through tick loop
  * ====================================================================== */
 
 static void run_sequence(uiox_tpwd_device_t *dev,
                           const sim_touch_t  *seq,
                           size_t              seq_len,
                           const char         *label)
 {
     printf("\n  [sim] %s  (%zu events)\n", label, seq_len);
     s_seq     = seq;
     s_seq_len = seq_len;
 
     /* Find max tick in sequence */
     uint32_t max_tick = 0;
     for (size_t i = 0; i < seq_len; i++)
         if (seq[i].at_tick > max_tick) max_tick = seq[i].at_tick;
 
     for (s_tick_now = 1; s_tick_now <= max_tick + 5u; s_tick_now++) {
         uint32_t now_ms = s_tick_now * 10u;   /* 10 ms per tick */
         uint32_t now_s  = now_ms / 1000u;
         uiox_tpwd_tick(dev, now_ms, now_s);
     }
 
     s_seq = NULL; s_seq_len = 0;
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX Touch-Password Stack Demo ===\n\n");
 
     /* ------------------------------------------------------------------ */
     /* 1. Open device                                                      */
     /* ------------------------------------------------------------------ */
 
     printf("--- Open ---\n");
     uiox_tpwd_device_t dev;
     uiox_tpwd_open_params_t p;
     memset(&p, 0, sizeof(p));
 
     p.hw     = &s_hw;
     p.hw_ops = &stub_ops;
 
     /* Gesture config: PIN mode, 4..8 digits, 10-second timeout */
     p.gesture.mode            = UIOX_TPWD_MODE_PIN;
     p.gesture.min_pin_len     = 4u;
     p.gesture.max_pin_len     = 8u;
     p.gesture.min_pattern_pts = 4u;
     p.gesture.entry_timeout_ms= 10000u;
     p.gesture.mask_input      = true;
 
     /* Interface timings */
     p.debounce_ms = 20u;
     p.hold_ms     = 800u;
     p.timeout_ms  = 3000u;
 
     /* Security */
     p.rng_seed = 0xC0FFEE42u;
 
     /* Event callback */
     p.evt_cb  = on_event;
     p.evt_ctx = NULL;
 
     int rc = uiox_tpwd_open(&dev, &p);
     if (rc < 0) {
         printf("[error] uiox_tpwd_open failed: %d\n", rc);
         return 1;
     }
 
     /* ------------------------------------------------------------------ */
     /* 2. Start (power on + reset)                                         */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Start ---\n");
     rc = uiox_tpwd_start(&dev);
     printf("  Touch controller: ACTIVE  rc=%d\n", rc);
     printf("  Panel: %ux%u  grid: %dx%d\n",
            s_hw.panel.width, s_hw.panel.height,
            UIOX_TPWD_GRID_COLS, UIOX_TPWD_GRID_ROWS);
 
     /* ------------------------------------------------------------------ */
     /* 3. Enrol PIN "1234" for user "alice"                               */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- PIN Enrolment (user: alice, PIN: 1234) ---\n");
     rc = uiox_tpwd_enrol(&dev, "alice", 10u);
     printf("  Enrol start  rc=%d  state=%s\n",
            rc, uiox_tpwd_state_name(uiox_tpwd_state(&dev)));
 
     run_sequence(&dev, s_pin_enrol,
                  sizeof(s_pin_enrol)/sizeof(s_pin_enrol[0]),
                  "PIN enrol sequence");
 
     printf("  State after enrol: %s\n",
            uiox_tpwd_state_name(uiox_tpwd_state(&dev)));
 
     /* ------------------------------------------------------------------ */
     /* 4. Verify correct PIN                                               */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- PIN Verification (correct: 1234) ---\n");
     rc = uiox_tpwd_verify(&dev, "alice", 10u);
     printf("  Verify start  rc=%d\n", rc);
 
     run_sequence(&dev, s_pin_correct,
                  sizeof(s_pin_correct)/sizeof(s_pin_correct[0]),
                  "Correct PIN sequence");
 
     printf("  State: %s\n", uiox_tpwd_state_name(uiox_tpwd_state(&dev)));
     printf("  Authenticated: %s\n",
            uiox_tpwd_authenticated(&dev, 0u) ? "YES" : "NO");
 
     /* ------------------------------------------------------------------ */
     /* 5. Logout                                                           */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Logout ---\n");
     uiox_tpwd_logout(&dev);
     printf("  State: %s\n", uiox_tpwd_state_name(uiox_tpwd_state(&dev)));
     printf("  Authenticated: %s\n",
            uiox_tpwd_authenticated(&dev, 0u) ? "YES" : "NO");
 
     /* ------------------------------------------------------------------ */
     /* 6. Verify wrong PIN × 3 (approach lockout)                         */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Wrong PIN attempts (trigger lockout) ---\n");
     for (int attempt = 1; attempt <= 3; attempt++) {
         rc = uiox_tpwd_verify(&dev, "alice", (uint32_t)(attempt * 100));
         if (rc == -EPERM) {
             printf("  Attempt %d: LOCKED OUT\n", attempt);
             break;
         }
         printf("  Attempt %d start  rc=%d  attempts_left=%u\n",
                attempt, rc,
                uiox_tpwd_attempts_left(&dev, "alice"));
 
         run_sequence(&dev, s_pin_wrong,
                      sizeof(s_pin_wrong)/sizeof(s_pin_wrong[0]),
                      "Wrong PIN");
 
         printf("  State: %s  attempts_left=%u\n",
                uiox_tpwd_state_name(uiox_tpwd_state(&dev)),
                uiox_tpwd_attempts_left(&dev, "alice"));
     }
 
     /* ------------------------------------------------------------------ */
     /* 7. Delete alice and enrol with pattern mode                        */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Delete 'alice' credential ---\n");
     rc = uiox_tpwd_delete(&dev, "alice");
     printf("  Delete rc=%d\n", rc);
 
     /* Switch to pattern mode */
     printf("\n--- Pattern Enrolment (user: bob) ---\n");
     dev.subsys.gesture.cfg.mode            = UIOX_TPWD_MODE_PATTERN;
     dev.subsys.gesture.cfg.min_pattern_pts = 4u;
     dev.subsys.gesture.cfg.entry_timeout_ms= 15000u;
 
     rc = uiox_tpwd_enrol(&dev, "bob", 200u);
     printf("  Enrol start  rc=%d  state=%s\n",
            rc, uiox_tpwd_state_name(uiox_tpwd_state(&dev)));
 
     run_sequence(&dev, s_pattern_enrol,
                  sizeof(s_pattern_enrol)/sizeof(s_pattern_enrol[0]),
                  "Pattern enrol sequence");
 
     printf("  State after enrol: %s\n",
            uiox_tpwd_state_name(uiox_tpwd_state(&dev)));
 
     /* ------------------------------------------------------------------ */
     /* 8. Verify pattern for bob                                           */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Pattern Verification (user: bob) ---\n");
     rc = uiox_tpwd_verify(&dev, "bob", 300u);
     printf("  Verify start  rc=%d\n", rc);
 
     run_sequence(&dev, s_pattern_enrol,  /* same sequence = correct */
                  sizeof(s_pattern_enrol)/sizeof(s_pattern_enrol[0]),
                  "Pattern verify sequence");
 
     printf("  State: %s\n", uiox_tpwd_state_name(uiox_tpwd_state(&dev)));
     printf("  Authenticated: %s\n",
            uiox_tpwd_authenticated(&dev, 300u) ? "YES" : "NO");
 
     /* ------------------------------------------------------------------ */
     /* 9. Backlight control                                               */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Backlight control ---\n");
     uiox_tpwd_set_backlight(&dev, 255u);
     uiox_tpwd_set_backlight(&dev, 128u);
     uiox_tpwd_set_backlight(&dev, 0u);
 
     /* ------------------------------------------------------------------ */
     /* 10. Statistics                                                      */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Statistics ---\n");
     uiox_tpwd_print_stats(&dev);
 
     /* ------------------------------------------------------------------ */
     /* 11. Audit log                                                       */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Audit log ---\n");
     uiox_tpwd_print_audit(&dev);
 
     /* ------------------------------------------------------------------ */
     /* 12. Stop and close                                                  */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Stop and close ---\n");
     uiox_tpwd_logout(&dev);
     uiox_tpwd_stop(&dev);
     printf("  Device: STOPPED\n");
     uiox_tpwd_close(&dev);
     printf("  Device: CLOSED  (security context zeroed)\n");
 
     printf("\n=== UIOX Touch-Password Demo complete ===\n");
     return 0;
 }
 
 