/**
 * @file  uiox_als_demo.c
 * @brief UIOX ALS stack demo — stub VEML7700 HAL + full stack exercise.
 * @date  2026-06-11
 */

 #include "uiox_als_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <errno.h>
 
 /* =========================================================================
  * Stub VEML7700 register bank
  * ====================================================================== */
 
 static uint16_t s_regs[8];
 
 /* Simulated ALS raw counts for different light conditions */
 static uint16_t s_sim_als   = 5000u;   /* ~336 lux at 1× 100ms           */
 static uint16_t s_sim_white = 5500u;
 static uint16_t s_sim_ir    =  300u;
 static bool     s_sim_dark        = false;
 static bool     s_sim_bright      = false;
 static bool     s_sim_saturate    = false;
 static bool     s_sim_thresh_high = false;
 static bool     s_sim_thresh_low  = false;
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int stub_init(uiox_als_hw_t *hw)
 {
     (void)hw;
     memset(s_regs, 0, sizeof(s_regs));
     /* ALS_CONF: gain=1×, itime=100ms, INT enabled */
     s_regs[VEML7700_REG_ALS_CONF] = VEML7700_GAIN_1X  |
                                      VEML7700_ITIME_100MS |
                                      VEML7700_INT_EN;
     printf("  [hal] init  %s  I2C=0x%02X  bus=%u  IRQ=%u\n",
            hw->model, hw->i2c_addr, hw->i2c_bus, hw->irq);
     return 0;
 }
 static void stub_deinit  (uiox_als_hw_t *hw) { (void)hw; }
 
 static int stub_power_on (uiox_als_hw_t *hw)
 {
     (void)hw;
     s_regs[VEML7700_REG_ALS_CONF] &= (uint16_t)~VEML7700_SD;
     printf("  [hal] power ON\n");
     return 0;
 }
 static void stub_power_off(uiox_als_hw_t *hw)
 {
     (void)hw;
     s_regs[VEML7700_REG_ALS_CONF] |= VEML7700_SD;
     printf("  [hal] power OFF\n");
 }
 
 static int stub_reg_read(uiox_als_hw_t *hw, uint8_t reg, uint16_t *val)
 {
     (void)hw;
     if (reg >= 7u) return -ERANGE;
     *val = s_regs[reg];
     return 0;
 }
 
 static int stub_reg_write(uiox_als_hw_t *hw, uint8_t reg, uint16_t val)
 {
     (void)hw;
     if (reg >= 7u) return -ERANGE;
     s_regs[reg] = val;
     printf("  [hal] reg_write [0x%02X] <- 0x%04X\n", reg, val);
     return 0;
 }
 
 static int stub_set_gain(uiox_als_hw_t *hw, uiox_als_gain_t g)
 {
     (void)hw;
     static const uint16_t gain_bits[] = {
         VEML7700_GAIN_1_8X, VEML7700_GAIN_1_4X,
         VEML7700_GAIN_1X,   VEML7700_GAIN_2X,
         VEML7700_GAIN_2X,   VEML7700_GAIN_2X, VEML7700_GAIN_2X,
     };
     s_regs[VEML7700_REG_ALS_CONF] =
         (uint16_t)((s_regs[VEML7700_REG_ALS_CONF] & ~VEML7700_GAIN_MASK)
                    | gain_bits[g < UIOX_ALS_GAIN_MAX ? g : 0u]);
     printf("  [hal] set_gain %s\n", uiox_als_gain_name(g));
     return 0;
 }
 
 static int stub_set_itime(uiox_als_hw_t *hw, uiox_als_itime_t t)
 {
     (void)hw;
     static const uint16_t itime_bits[] = {
         VEML7700_ITIME_25MS,  VEML7700_ITIME_50MS,
         VEML7700_ITIME_100MS, VEML7700_ITIME_200MS,
         VEML7700_ITIME_400MS, VEML7700_ITIME_800MS,
     };
     s_regs[VEML7700_REG_ALS_CONF] =
         (uint16_t)((s_regs[VEML7700_REG_ALS_CONF] & ~VEML7700_ITIME_MASK)
                    | itime_bits[t < UIOX_ALS_ITIME_MAX ? t : 0u]);
     printf("  [hal] set_itime %s\n", uiox_als_itime_name(t));
     return 0;
 }
 
 static int stub_read_als(uiox_als_hw_t *hw, uint16_t *als, uint16_t *white)
 {
     (void)hw;
     *als   = s_sim_saturate ? 0xFFFFu : s_sim_als;
     *white = s_sim_saturate ? 0xFFFFu : s_sim_white;
     s_regs[VEML7700_REG_ALS]   = *als;
     s_regs[VEML7700_REG_WHITE] = *white;
     return 0;
 }
 
 static int stub_read_ir(uiox_als_hw_t *hw, uint16_t *ir)
 {
     (void)hw;
     *ir = s_sim_ir;
     return 0;
 }
 
 static int stub_set_threshold(uiox_als_hw_t *hw,
                                uint16_t low, uint16_t high)
 {
     (void)hw;
     s_regs[VEML7700_REG_ALS_WL] = low;
     s_regs[VEML7700_REG_ALS_WH] = high;
     printf("  [hal] threshold  low=%u  high=%u\n", low, high);
     return 0;
 }
 
 static int stub_int_enable(uiox_als_hw_t *hw, bool en)
 {
     (void)hw;
     if (en) s_regs[VEML7700_REG_ALS_CONF] |=  VEML7700_INT_EN;
     else    s_regs[VEML7700_REG_ALS_CONF] &= (uint16_t)~VEML7700_INT_EN;
     printf("  [hal] int_enable=%d\n", (int)en);
     return 0;
 }
 
 static int stub_int_clear(uiox_als_hw_t *hw)
 {
     (void)hw;
     s_regs[VEML7700_REG_ALS_INT] = 0u;
     return 0;
 }
 
 static int stub_trigger(uiox_als_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] one-shot trigger\n");
     return 0;
 }
 
 static void stub_gpio_w(uiox_als_hw_t *hw, uint32_t p, bool v)
 { (void)hw; printf("  [hal] GPIO pin=%u val=%d\n", p, (int)v); }
 
 static bool stub_gpio_r(uiox_als_hw_t *hw, uint32_t p)
 { (void)hw; (void)p; return false; }
 
 static void stub_isr(uiox_als_hw_t *hw)
 {
     if (!hw) return;
     if (s_sim_thresh_high)
         hw->pending_irq |= UIOX_ALS_IRQ_THRESH_HIGH;
     else if (s_sim_thresh_low)
         hw->pending_irq |= UIOX_ALS_IRQ_THRESH_LOW;
     else
         hw->pending_irq |= UIOX_ALS_IRQ_DATA_READY;
     printf("  [hal] ISR  pending=0x%08X\n", hw->pending_irq);
 }
 
 static const uiox_als_hw_ops_t stub_ops = {
     .init          = stub_init,
     .deinit        = stub_deinit,
     .power_on      = stub_power_on,
     .power_off     = stub_power_off,
     .reg_read      = stub_reg_read,
     .reg_write     = stub_reg_write,
     .set_gain      = stub_set_gain,
     .set_itime     = stub_set_itime,
     .read_als      = stub_read_als,
     .read_ir       = stub_read_ir,
     .set_threshold = stub_set_threshold,
     .int_enable    = stub_int_enable,
     .int_clear     = stub_int_clear,
     .trigger       = stub_trigger,
     .gpio_write    = stub_gpio_w,
     .gpio_read     = stub_gpio_r,
     .isr           = stub_isr,
 };
 
 static uiox_als_hw_t s_hw = {
     .i2c_addr  = VEML7700_I2C_ADDR,
     .i2c_bus   = 0u,
     .irq       = 33u,
     .caps      = UIOX_ALS_CAP_ALS_CH      | UIOX_ALS_CAP_WHITE_CH |
                  UIOX_ALS_CAP_IR_CH        | UIOX_ALS_CAP_GAIN_CTRL |
                  UIOX_ALS_CAP_ITIME_CTRL   | UIOX_ALS_CAP_THRESHOLD_INT |
                  UIOX_ALS_CAP_PERSIST_FILTER | UIOX_ALS_CAP_POWER_SAVE,
     .ic_type   = UIOX_ALS_IC_VEML7700,
     .model     = "Vishay VEML7700 ALS",
     .gain      = UIOX_ALS_GAIN_1X,
     .itime     = UIOX_ALS_ITIME_100MS,
 };
 
 /* =========================================================================
  * Event callback
  * ====================================================================== */
 
 static void on_als_event(uiox_als_ev_t ev,
                           const uiox_als_sample_t *s, void *ctx)
 {
     (void)ctx;
     if (s)
         printf("  [event] %-16s  lux=%u.%03u  cct=%u K"
                "  als=%u  gain=%s  itime=%s\n",
                uiox_als_ev_name(ev),
                s->lux_milli / 1000u, s->lux_milli % 1000u,
                s->cct_k,
                s->raw_als,
                uiox_als_gain_name(s->gain),
                uiox_als_itime_name(s->itime));
     else
         printf("  [event] %-16s\n", uiox_als_ev_name(ev));
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX Ambient Light Sensor Stack Demo ===\n\n");
 
     /* ------------------------------------------------------------------ */
     printf("--- Open ---\n");
     uiox_als_device_t      dev;
     uiox_als_open_params_t p = {
         .hw         = &s_hw,
         .hw_ops     = &stub_ops,
         .coeff      = &uiox_als_coeff_veml7700,
         .continuous = true,
         .evt_cb     = on_als_event,
     };
     int rc = uiox_als_open(&dev, &p);
     if (rc < 0) { printf("[error] open: %d\n", rc); return 1; }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Start ---\n");
     rc = uiox_als_start(&dev);
     printf("  State: %s  rc=%d\n",
            uiox_als_state_name(dev.subsys.state), rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Device info ---\n");
     uiox_als_print_info(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Normal office light (~336 lux) tick ---\n");
     s_sim_als   = 5000u;
     s_sim_white = 5500u;
     s_sim_ir    =  300u;
     for (uint32_t t = 10u; t <= 30u; t += 10u)
         uiox_als_tick(&dev, t);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Manual gain set to 2× ---\n");
     rc = uiox_als_set_gain(&dev, UIOX_ALS_GAIN_2X);
     printf("  set_gain rc=%d\n", rc);
     uiox_als_tick(&dev, 40u);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Manual integration time set to 200 ms ---\n");
     rc = uiox_als_set_itime(&dev, UIOX_ALS_ITIME_200MS);
     printf("  set_itime rc=%d\n", rc);
     uiox_als_tick(&dev, 260u);  /* > 200 ms after last */
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Apply cover-glass trim (×1.15) ---\n");
     uiox_als_set_trim(&dev, 1150u);
     uiox_als_tick(&dev, 280u);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Set custom thresholds (5 lux low / 500 lux high) ---\n");
     uiox_als_set_thresh(&dev, 5000u, 500000u);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate dim room (8 lux) → DARK transition ---\n");
     s_sim_als   = 120u;
     s_sim_white = 130u;
     s_sim_ir    =  10u;
     s_sim_dark  = true;
     uiox_als_tick(&dev, 300u);
     s_sim_dark  = false;
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate bright sunlight → BRIGHT + auto-gain decrease ---\n");
     s_sim_als    = 62000u;
     s_sim_white  = 65000u;
     s_sim_ir     =  8000u;
     s_sim_bright = true;
     for (uint32_t t = 310u; t <= 330u; t += 10u)
         uiox_als_tick(&dev, t);
     s_sim_bright = false;
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate saturation (ADC clipped) ---\n");
     s_sim_saturate = true;
     uiox_als_tick(&dev, 340u);
     s_sim_saturate = false;
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate threshold-high IRQ ---\n");
     s_sim_thresh_high = true;
     stub_isr(&s_hw);
     uiox_als_tick(&dev, 350u);
     s_sim_thresh_high = false;
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate threshold-low IRQ ---\n");
     s_sim_als         = 50u;
     s_sim_thresh_low  = true;
     stub_isr(&s_hw);
     uiox_als_tick(&dev, 360u);
     s_sim_thresh_low  = false;
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Auto-gain enabled — low-light gain ramp ---\n");
     uiox_als_auto_gain(&dev, true);
     s_sim_als   = 10u;
     s_sim_white = 11u;
     s_sim_ir    =  1u;
     for (uint32_t t = 400u; t <= 450u; t += 10u)
         uiox_als_tick(&dev, t);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Direct register read-back ---\n");
     {
         uint16_t conf = 0u;
         uiox_als_hw_reg_read(&s_hw, VEML7700_REG_ALS_CONF, &conf);
         printf("  VEML7700_ALS_CONF = 0x%04X\n", conf);
         uint16_t wh = 0u, wl = 0u;
         uiox_als_hw_reg_read(&s_hw, VEML7700_REG_ALS_WH, &wh);
         uiox_als_hw_reg_read(&s_hw, VEML7700_REG_ALS_WL, &wl);
         printf("  Thresh high = %u  low = %u\n", wh, wl);
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Last sample ---\n");
     {
         uint32_t lux_milli = 0u, cct_k = 0u;
         uint16_t als = 0u, white = 0u, ir = 0u;
         uiox_als_get_lux(&dev, &lux_milli);
         uiox_als_get_cct(&dev, &cct_k);
         uiox_als_get_raw(&dev, &als, &white, &ir);
         printf("  Lux  : %u.%03u\n", lux_milli/1000u, lux_milli%1000u);
         printf("  CCT  : %u K\n",    cct_k);
         printf("  Raw  : als=%u  white=%u  ir=%u\n", als, white, ir);
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Tick loop (5 × 10 ms) ---\n");
     s_sim_als   = 3000u;
     s_sim_white = 3300u;
     s_sim_ir    =  200u;
     for (uint32_t t = 500u; t <= 540u; t += 10u)
         uiox_als_tick(&dev, t);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Statistics ---\n");
     uiox_als_print_stats(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Final device info ---\n");
     uiox_als_print_info(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Stop and close ---\n");
     uiox_als_stop(&dev);
     printf("  State: %s\n", uiox_als_state_name(dev.subsys.state));
     uiox_als_close(&dev);
     printf("  Device: CLOSED\n");
 
     printf("\n=== UIOX ALS Demo complete ===\n");
     return 0;
 }
 