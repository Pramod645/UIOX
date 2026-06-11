/**
 * @file  uiox_rtc_demo.c
 * @brief UIOX RTC stack demo — stub HAL + full stack exercise.
 *        Mirrors uiox_tb4_demo.c in structure.
 * @date  2026-06-10
 */

 #include "uiox_rtc_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <errno.h>
 
 /* =========================================================================
  * Stub CMOS register bank
  * ====================================================================== */
 
 static uint8_t s_cmos[128];
 static bool    s_alarm_sim   = false;
 static bool    s_periodic_sim= false;
 static bool    s_bat_low_sim = false;
 
 /* Pre-load CMOS with 2026-06-10 09:30:45, BCD */
 static void stub_cmos_preset(void)
 {
     memset(s_cmos, 0, sizeof(s_cmos));
     /* time: 09:30:45 */
     s_cmos[RTC_REG_SECONDS] = 0x45u;   /* BCD 45 */
     s_cmos[RTC_REG_MINUTES] = 0x30u;
     s_cmos[RTC_REG_HOURS]   = 0x09u;
     /* date: 2026-06-10 */
     s_cmos[RTC_REG_DAY]     = 0x10u;
     s_cmos[RTC_REG_MONTH]   = 0x06u;
     s_cmos[RTC_REG_YEAR]    = 0x26u;   /* BCD year in century */
     s_cmos[RTC_REG_CENTURY] = 0x20u;   /* BCD century 20xx   */
     /* Ctrl B: BCD, 24h */
     s_cmos[RTC_REG_B]       = RTC_REG_B_24H;
     /* Ctrl D: battery good */
     s_cmos[RTC_REG_D]       = RTC_REG_D_VRT;
 }
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int stub_init(uiox_rtc_hw_t *hw)
 {
     (void)hw;
     stub_cmos_preset();
     printf("  [hal] init  %s  ports=0x%03X/0x%03X  IRQ=%u\n",
            hw->model, hw->index_port, hw->data_port, hw->irq);
     return 0;
 }
 static void stub_deinit(uiox_rtc_hw_t *hw) { (void)hw; }
 
 static uint8_t stub_reg_read(uiox_rtc_hw_t *hw, uint8_t reg)
 {
     (void)hw;
     /* Reading Reg C clears interrupt flags */
     if (reg == RTC_REG_C) {
         uint8_t val = s_cmos[RTC_REG_C];
         s_cmos[RTC_REG_C] = 0u;
         return val;
     }
     return s_cmos[reg & 0x7Fu];
 }
 
 static void stub_reg_write(uiox_rtc_hw_t *hw, uint8_t reg, uint8_t val)
 {
     (void)hw;
     s_cmos[reg & 0x7Fu] = val;
     if (reg == RTC_REG_B)
         printf("  [hal] Reg B <- 0x%02X\n", val);
 }
 
 static uiox_rtc_bat_t stub_bat_check(uiox_rtc_hw_t *hw)
 {
     (void)hw;
     if (s_bat_low_sim) {
         s_cmos[RTC_REG_D] &= (uint8_t)~RTC_REG_D_VRT;
         return UIOX_RTC_BAT_LOW;
     }
     s_cmos[RTC_REG_D] |= RTC_REG_D_VRT;
     return UIOX_RTC_BAT_GOOD;
 }
 
 /* BCD helpers */
 static uint8_t b2d(uint8_t b) { return (uint8_t)((b>>4)*10u + (b&0x0Fu)); }
 static uint8_t d2b(uint8_t d) { return (uint8_t)(((d/10u)<<4)|(d%10u)); }
 
 static int stub_time_read(uiox_rtc_hw_t *hw,
                            uint8_t *s, uint8_t *m, uint8_t *h,
                            uint8_t *md, uint8_t *mo, uint16_t *yr)
 {
     (void)hw;
     *s  = b2d(s_cmos[RTC_REG_SECONDS]);
     *m  = b2d(s_cmos[RTC_REG_MINUTES]);
     *h  = b2d(s_cmos[RTC_REG_HOURS]);
     *md = b2d(s_cmos[RTC_REG_DAY]);
     *mo = b2d(s_cmos[RTC_REG_MONTH]);
     *yr = (uint16_t)(b2d(s_cmos[RTC_REG_CENTURY]) * 100u
                     + b2d(s_cmos[RTC_REG_YEAR]));
     printf("  [hal] time_read  %04u-%02u-%02u %02u:%02u:%02u\n",
            *yr, *mo, *md, *h, *m, *s);
     return 0;
 }
 
 static int stub_time_write(uiox_rtc_hw_t *hw,
                             uint8_t s, uint8_t m, uint8_t h,
                             uint8_t md, uint8_t mo, uint16_t yr)
 {
     (void)hw;
     /* Simulate SET bit protocol */
     s_cmos[RTC_REG_B] |= RTC_REG_B_SET;
     s_cmos[RTC_REG_SECONDS] = d2b(s);
     s_cmos[RTC_REG_MINUTES] = d2b(m);
     s_cmos[RTC_REG_HOURS]   = d2b(h);
     s_cmos[RTC_REG_DAY]     = d2b(md);
     s_cmos[RTC_REG_MONTH]   = d2b(mo);
     s_cmos[RTC_REG_YEAR]    = d2b((uint8_t)(yr % 100u));
     s_cmos[RTC_REG_CENTURY] = d2b((uint8_t)(yr / 100u));
     s_cmos[RTC_REG_B] &= (uint8_t)~RTC_REG_B_SET;
     printf("  [hal] time_write %04u-%02u-%02u %02u:%02u:%02u\n",
            yr, mo, md, h, m, s);
     return 0;
 }
 
 static int stub_alarm_read(uiox_rtc_hw_t *hw,
                             uint8_t *s, uint8_t *m, uint8_t *h)
 {
     (void)hw;
     *s = s_cmos[RTC_REG_SEC_ALARM];
     *m = s_cmos[RTC_REG_MIN_ALARM];
     *h = s_cmos[RTC_REG_HR_ALARM];
     printf("  [hal] alarm_read  %02X:%02X:%02X\n", *h, *m, *s);
     return 0;
 }
 
 static int stub_alarm_write(uiox_rtc_hw_t *hw,
                              uint8_t s, uint8_t m, uint8_t h)
 {
     (void)hw;
     s_cmos[RTC_REG_SEC_ALARM] = s;
     s_cmos[RTC_REG_MIN_ALARM] = m;
     s_cmos[RTC_REG_HR_ALARM]  = h;
     printf("  [hal] alarm_write %02X:%02X:%02X\n", h, m, s);
     return 0;
 }
 
 static int stub_alarm_enable(uiox_rtc_hw_t *hw, bool en)
 {
     (void)hw;
     if (en) s_cmos[RTC_REG_B] |=  RTC_REG_B_AIE;
     else    s_cmos[RTC_REG_B] &= (uint8_t)~RTC_REG_B_AIE;
     printf("  [hal] alarm_enable=%d\n", (int)en);
     return 0;
 }
 
 static int stub_periodic_set(uiox_rtc_hw_t *hw, uint8_t rs)
 {
     (void)hw;
     s_cmos[RTC_REG_A] = (uint8_t)((s_cmos[RTC_REG_A] & ~RTC_REG_A_RS_MASK)
                                   | (rs & RTC_REG_A_RS_MASK));
     printf("  [hal] periodic rate_sel=0x%X\n", rs);
     return 0;
 }
 
 static int stub_nvram_read(uiox_rtc_hw_t *hw, uint8_t off, uint8_t *val)
 {
     (void)hw;
     if (off >= RTC_NVRAM_LEN) return -ERANGE;
     *val = s_cmos[RTC_NVRAM_START + off];
     return 0;
 }
 
 static int stub_nvram_write(uiox_rtc_hw_t *hw, uint8_t off, uint8_t val)
 {
     (void)hw;
     if (off >= RTC_NVRAM_LEN) return -ERANGE;
     s_cmos[RTC_NVRAM_START + off] = val;
     return 0;
 }
 
 static void stub_gpio_w(uiox_rtc_hw_t *hw, uint32_t p, bool v)
 { (void)hw; printf("  [hal] GPIO pin=%u val=%d\n", p, (int)v); }
 static bool stub_gpio_r(uiox_rtc_hw_t *hw, uint32_t p)
 { (void)hw; (void)p; return false; }
 static void stub_isr(uiox_rtc_hw_t *hw)
 { (void)hw; printf("  [hal] ISR fired\n"); }
 
 static const uiox_rtc_hw_ops_t stub_ops = {
     .init         = stub_init,
     .deinit       = stub_deinit,
     .reg_read     = stub_reg_read,
     .reg_write    = stub_reg_write,
     .bat_check    = stub_bat_check,
     .time_read    = stub_time_read,
     .time_write   = stub_time_write,
     .alarm_read   = stub_alarm_read,
     .alarm_write  = stub_alarm_write,
     .alarm_enable = stub_alarm_enable,
     .periodic_set = stub_periodic_set,
     .nvram_read   = stub_nvram_read,
     .nvram_write  = stub_nvram_write,
     .gpio_write   = stub_gpio_w,
     .gpio_read    = stub_gpio_r,
     .isr          = stub_isr,
 };
 
 static uiox_rtc_hw_t s_hw = {
     .index_port   = RTC_PORT_INDEX,
     .data_port    = RTC_PORT_DATA,
     .irq          = RTC_IRQ_LINE,
     .caps         = UIOX_RTC_CAP_BCD   | UIOX_RTC_CAP_24H    |
                     UIOX_RTC_CAP_ALARM  | UIOX_RTC_CAP_PERIODIC |
                     UIOX_RTC_CAP_UPDATE_IRQ | UIOX_RTC_CAP_CENTURY_REG |
                     UIOX_RTC_CAP_NVRAM  | UIOX_RTC_CAP_BATTERY |
                     UIOX_RTC_CAP_NMI_DISABLE,
     .version      = UIOX_RTC_VER_MC146818,
     .model        = "MC146818A CMOS RTC (CR2032)",
     .century_reg  = RTC_REG_CENTURY,
     .use_nmi_mask = true,
     .use_bcd      = true,
     .use_24h      = true,
 };
 
 /* =========================================================================
  * Event callback
  * ====================================================================== */
 
 static void on_rtc_event(uiox_rtc_ev_t ev,
                           const uiox_rtc_tm_t *tm, void *ctx)
 {
     (void)ctx;
     if (tm)
         printf("  [event] %-22s  %04d-%02d-%02d %02d:%02d:%02d\n",
                uiox_rtc_ev_name(ev),
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                tm->tm_hour, tm->tm_min, tm->tm_sec);
     else
         printf("  [event] %-22s  (no time)\n", uiox_rtc_ev_name(ev));
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX RTC CR2032 Stack Demo ===\n\n");
 
     /* --- Open --- */
     printf("--- Open ---\n");
     uiox_rtc_device_t      dev;
     uiox_rtc_open_params_t p = {
         .hw      = &s_hw,
         .hw_ops  = &stub_ops,
         .evt_cb  = on_rtc_event,
     };
     int rc = uiox_rtc_open(&dev, &p);
     if (rc < 0) { printf("[error] open: %d\n", rc); return 1; }
 
     /* --- Start --- */
     printf("\n--- Start ---\n");
     rc = uiox_rtc_start(&dev);
     printf("  State: %s  rc=%d\n",
            uiox_rtc_state_name(dev.subsys.state), rc);
 
     /* --- Info --- */
     printf("\n--- Device info ---\n");
     uiox_rtc_print_info(&dev);
 
     /* --- Read time --- */
     printf("\n--- Read time ---\n");
     uiox_rtc_tm_t tm;
     rc = uiox_rtc_get_time(&dev, &tm);
     printf("  %04d-%02d-%02d %02d:%02d:%02d  rc=%d\n",
            tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec, rc);
 
     /* --- Epoch conversion --- */
     printf("\n--- Epoch conversion ---\n");
     int64_t epoch = uiox_rtc_tm_to_epoch(&tm);
     printf("  Epoch : %lld\n", (long long)epoch);
     uiox_rtc_tm_t tm2;
     uiox_rtc_epoch_to_tm(epoch, &tm2);
     printf("  Round-trip: %04d-%02d-%02d %02d:%02d:%02d\n",
            tm2.tm_year+1900, tm2.tm_mon+1, tm2.tm_mday,
            tm2.tm_hour, tm2.tm_min, tm2.tm_sec);
 
     /* --- Set time --- */
     printf("\n--- Set time (2026-06-10 12:00:00) ---\n");
     uiox_rtc_tm_t new_tm = {
         .tm_sec=0, .tm_min=0, .tm_hour=12,
         .tm_mday=10, .tm_mon=5, .tm_year=126
     };
     rc = uiox_rtc_set_time(&dev, &new_tm);
     printf("  Set rc=%d\n", rc);
 
     /* --- Set alarm --- */
     printf("\n--- Set alarm (12:00:30) ---\n");
     uiox_rtc_alarm_t alm = {
         .time = { .tm_sec=30, .tm_min=0, .tm_hour=12,
                   .tm_mday=-1, .tm_mon=-1 },
         .enabled = true
     };
     rc = uiox_rtc_set_alarm(&dev, &alm);
     printf("  Alarm set rc=%d\n", rc);
 
     /* --- NVRAM write/read --- */
     printf("\n--- NVRAM write / read ---\n");
     rc = uiox_rtc_hw_nvram_write(&s_hw, 0u, 0xA5u);
     uint8_t nval = 0u;
     rc = uiox_rtc_hw_nvram_read(&s_hw, 0u, &nval);
     printf("  NVRAM[0] = 0x%02X  rc=%d\n", nval, rc);
 
     /* --- Simulate alarm IRQ --- */
     printf("\n--- Simulate alarm IRQ ---\n");
     s_alarm_sim = true;
     s_cmos[RTC_REG_C] = RTC_REG_C_IRQF | RTC_REG_C_AF;
     for (uint32_t t = 10u; t <= 30u; t += 10u)
         uiox_rtc_tick(&dev, t);
 
     /* --- Simulate periodic IRQ --- */
     printf("\n--- Simulate periodic IRQ ---\n");
     s_cmos[RTC_REG_C] = RTC_REG_C_IRQF | RTC_REG_C_PF;
     uiox_rtc_tick(&dev, 40u);
 
     /* --- Simulate battery low --- */
     printf("\n--- Simulate CR2032 battery low ---\n");
     s_bat_low_sim = true;
     s_cmos[RTC_REG_C] = RTC_REG_C_IRQF | RTC_REG_C_UF; /* update tick */
     uiox_rtc_tick(&dev, 50u);
     s_bat_low_sim = false;
 
     /* --- Tick loop --- */
     printf("\n--- Tick loop (3 × 10 ms) ---\n");
     for (uint32_t t = 100u; t <= 120u; t += 10u)
         uiox_rtc_tick(&dev, t);
 
     /* --- Statistics --- */
     printf("\n--- Statistics ---\n");
     uiox_rtc_print_stats(&dev);
 
     /* --- Stop / close --- */
     printf("\n--- Stop and close ---\n");
     uiox_rtc_stop(&dev);
     printf("  State: %s\n", uiox_rtc_state_name(dev.subsys.state));
     uiox_rtc_close(&dev);
     printf("  Device: CLOSED\n");
 
     printf("\n=== UIOX RTC CR2032 Demo complete ===\n");
     return 0;
 }
 