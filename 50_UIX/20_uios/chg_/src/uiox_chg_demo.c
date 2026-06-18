/**
 * @file  uiox_chg_demo.c
 * @brief UIOX Charger stack demo — stub HAL + full stack exercise.
 *        Mirrors uiox_rtc_demo.c / uiox_tb4_demo.c in structure.
 * @date  2026-06-11
 */

 #include "uiox_chg_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <errno.h>
 
 /* =========================================================================
  * Stub register bank
  * ====================================================================== */
 
 static uint8_t s_regs[0x20];
 
 /* Simulated ADC values */
 static int32_t s_adc[UIOX_CHG_ADC_MAX] = {
     [UIOX_CHG_ADC_VBUS] = 20000,   /* 20 V USB-C PD          */
     [UIOX_CHG_ADC_VBAT] =  3800,   /* 3.8 V LiPo             */
     [UIOX_CHG_ADC_IBAT] =  2000,   /* 2 A charge current      */
     [UIOX_CHG_ADC_VSYS] =  4200,   /* 4.2 V system rail       */
     [UIOX_CHG_ADC_TDIE] = 350000,  /* 35.0 °C die temp (m°C)  */
     [UIOX_CHG_ADC_NTC]  = 250000,  /* 25.0 °C NTC  (m°C)      */
 };
 
 /* State simulation flags */
 static bool s_usbc_pd_connected = false;
 static bool s_barrel_connected  = false;
 static bool s_fault_sim         = false;
 static bool s_thermal_sim       = false;
 static bool s_pd_caps_ready     = false;
 static bool s_pd_accept_ready   = false;
 
 /* Canned PD source caps (5 V/3 A + 9 V/3 A + 20 V/5 A) */
 static const uint8_t s_pd_caps_msg[] = {
     PD_MSG_CAPABILITIES,
     /* PDO0: 5 V / 3 A */
     (uint8_t)((( 5000u/50u) >> 0u) & 0xFFu),
     (uint8_t)((( 5000u/50u) >> 8u) | ((3000u/10u << 2u) & 0xFCu)),
     (uint8_t)(((3000u/10u) >> 6u) & 0xFFu),
     0x00u,
     /* PDO1: 9 V / 3 A */
     (uint8_t)((( 9000u/50u) >> 0u) & 0xFFu),
     (uint8_t)((( 9000u/50u) >> 8u) | ((3000u/10u << 2u) & 0xFCu)),
     (uint8_t)(((3000u/10u) >> 6u) & 0xFFu),
     0x00u,
     /* PDO2: 20 V / 5 A */
     (uint8_t)(((20000u/50u) >> 0u) & 0xFFu),
     (uint8_t)(((20000u/50u) >> 8u) | ((5000u/10u << 2u) & 0xFCu)),
     (uint8_t)(((5000u/10u)  >> 6u) & 0xFFu),
     0x00u,
 };
 
 static uint8_t s_pd_rx_buf[32];
 static uint8_t s_pd_rx_len = 0u;
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int stub_init(uiox_chg_hw_t *hw)
 {
     (void)hw;
     memset(s_regs, 0, sizeof(s_regs));
     /* Pre-set part number reg */
     s_regs[BQ25895_REG0B] = 0x23u;    /* BQ25895 device ID */
     /* Power-good, no charge initially */
     s_regs[BQ25895_REG08] = 0x00u;
     printf("  [hal] init  %s  I2C=0x%02X  IRQ=%u\n",
            hw->model, hw->i2c_addr, hw->irq);
     return 0;
 }
 static void stub_deinit(uiox_chg_hw_t *hw) { (void)hw; }
 
 static int stub_reg_read(uiox_chg_hw_t *hw, uint8_t reg, uint8_t *val)
 {
     (void)hw;
     if (reg >= sizeof(s_regs)) return -ERANGE;
     *val = s_regs[reg];
     return 0;
 }
 
 static int stub_reg_write(uiox_chg_hw_t *hw, uint8_t reg, uint8_t val)
 {
     (void)hw;
     if (reg >= sizeof(s_regs)) return -ERANGE;
     s_regs[reg] = val;
     printf("  [hal] reg_write  [0x%02X] <- 0x%02X\n", reg, val);
     return 0;
 }
 
 static int stub_reg_rmw(uiox_chg_hw_t *hw,
                          uint8_t reg, uint8_t mask, uint8_t bits)
 {
     (void)hw;
     if (reg >= sizeof(s_regs)) return -ERANGE;
     s_regs[reg] = (uint8_t)((s_regs[reg] & ~mask) | (bits & mask));
     return 0;
 }
 
 static int stub_adc_read(uiox_chg_hw_t *hw,
                           uiox_chg_adc_ch_t ch, int32_t *val_mv)
 {
     (void)hw;
     if (ch >= UIOX_CHG_ADC_MAX) return -ERANGE;
     /* Simulate thermal fault */
     if (s_thermal_sim && ch == UIOX_CHG_ADC_TDIE)
         *val_mv = 850000;  /* 85 °C — above throttle threshold */
     else
         *val_mv = s_adc[ch];
     return 0;
 }
 
 static int stub_set_ichg(uiox_chg_hw_t *hw, uint32_t ma)
 {
     (void)hw;
     /* REG02 bits [6:0] = ICHG; 64 mA steps, offset 0 */
     s_regs[BQ25895_REG02] = (uint8_t)((ma / 64u) & 0x7Fu);
     printf("  [hal] set_ichg  %u mA  reg=0x%02X\n",
            ma, s_regs[BQ25895_REG02]);
     return 0;
 }
 
 static int stub_set_vchg(uiox_chg_hw_t *hw, uint32_t mv)
 {
     (void)hw;
     /* REG04 bits [7:2] = VREG; 16 mV steps, offset 3840 mV */
     uint32_t code = (mv > 3840u) ? (mv - 3840u) / 16u : 0u;
     s_regs[BQ25895_REG04] = (uint8_t)((code & 0x3Fu) << 2u);
     printf("  [hal] set_vchg  %u mV  reg=0x%02X\n",
            mv, s_regs[BQ25895_REG04]);
     return 0;
 }
 
 static int stub_set_iin_lim(uiox_chg_hw_t *hw, uint32_t ma)
 {
     (void)hw;
     /* REG00 bits [5:0] = IINLIM; 50 mA steps, offset 100 mA */
     uint32_t code = (ma > 100u) ? (ma - 100u) / 50u : 0u;
     s_regs[BQ25895_REG00] = (uint8_t)(code & 0x3Fu);
     printf("  [hal] set_iin_lim  %u mA  reg=0x%02X\n",
            ma, s_regs[BQ25895_REG00]);
     return 0;
 }
 
 static int stub_set_vindpm(uiox_chg_hw_t *hw, uint32_t mv)
 {
     (void)hw;
     s_regs[BQ25895_REG0D] = (uint8_t)((mv / 100u) & 0x7Fu);
     printf("  [hal] set_vindpm  %u mV\n", mv);
     return 0;
 }
 
 static int stub_charge_enable(uiox_chg_hw_t *hw, bool en)
 {
     (void)hw;
     printf("  [hal] charge_enable=%d\n", (int)en);
     if (en) s_regs[BQ25895_REG03] |=  0x10u;  /* CHG_CONFIG */
     else    s_regs[BQ25895_REG03] &= (uint8_t)~0x10u;
     return 0;
 }
 
 static int stub_otg_enable(uiox_chg_hw_t *hw, bool en)
 {
     (void)hw;
     printf("  [hal] otg_enable=%d\n", (int)en);
     if (en) s_regs[BQ25895_REG03] |=  0x20u;
     else    s_regs[BQ25895_REG03] &= (uint8_t)~0x20u;
     return 0;
 }
 
 static int stub_get_status(uiox_chg_hw_t *hw,
                             uiox_chg_chrg_t *chrg,
                             uiox_chg_src_t  *src,
                             uint32_t        *faults)
 {
     (void)hw;
     /* Build simulated REG08 */
     uint8_t r08 = 0u;
 
     if (s_usbc_pd_connected) {
         r08 |= (0x03u << BQ_VBUS_STAT_SHIFT);  /* VBUS_STAT = USB-C */
         r08 |= BQ_PG_STAT;
         r08 |= ((uint8_t)BQ_CHRG_STAT_FAST << BQ_CHRG_STAT_SHIFT);
         *src  = UIOX_CHG_SRC_USBC_PD;
         *chrg = UIOX_CHG_CHRG_FAST;
     } else if (s_barrel_connected) {
         r08 |= (0x05u << BQ_VBUS_STAT_SHIFT);  /* VBUS_STAT = adapter */
         r08 |= BQ_PG_STAT;
         r08 |= ((uint8_t)BQ_CHRG_STAT_FAST << BQ_CHRG_STAT_SHIFT);
         *src  = UIOX_CHG_SRC_BARREL;
         *chrg = UIOX_CHG_CHRG_FAST;
     } else {
         *src  = UIOX_CHG_SRC_NONE;
         *chrg = UIOX_CHG_CHRG_IDLE;
     }
 
     s_regs[BQ25895_REG08] = r08;
 
     /* Fault register */
     *faults = UIOX_CHG_FAULT_NONE;
     if (s_fault_sim) {
         s_regs[BQ25895_REG09] = BQ_FAULT_BAT_OVP;
         *faults = UIOX_CHG_FAULT_BAT_OVP;
         *chrg   = UIOX_CHG_CHRG_FAULT;
     } else {
         s_regs[BQ25895_REG09] = 0x00u;
     }
     return 0;
 }
 
 static int stub_wdog_reset(uiox_chg_hw_t *hw)
 {
     (void)hw;
     /* Writing 1 to REG03 bit 6 kicks the watchdog */
     s_regs[BQ25895_REG03] |= 0x40u;
     return 0;
 }
 
 static int stub_pd_tx(uiox_chg_hw_t *hw,
                        const uint8_t *buf, uint8_t len)
 {
     (void)hw;
     printf("  [hal] PD TX  opcode=0x%02X  len=%u\n", buf[0], len);
     /* Simulate source response */
    /* Simulate source response */
    if (buf[0] == PD_MSG_GET_CAPABILITIES) {
        s_pd_caps_ready = true;
        /* Schedule caps response into RX buffer */
        memcpy(s_pd_rx_buf, s_pd_caps_msg, sizeof(s_pd_caps_msg));
        s_pd_rx_len = (uint8_t)sizeof(s_pd_caps_msg);
        /* Signal PD message pending IRQ */
        hw->pending_irq |= UIOX_CHG_IRQ_PD_MSG;
    } else if (buf[0] == PD_MSG_REQUEST) {
        s_pd_accept_ready = true;
        /* Schedule ACCEPT response */
        s_pd_rx_buf[0] = PD_MSG_ACCEPT;
        s_pd_rx_len    = 1u;
        hw->pending_irq |= UIOX_CHG_IRQ_PD_MSG;
    }
    return 0;
}

static int stub_pd_rx(uiox_chg_hw_t *hw,
                       uint8_t *buf, uint8_t max_len)
{
    (void)hw;
    if (!s_pd_rx_len) return 0;
    uint8_t n = (s_pd_rx_len < max_len) ? s_pd_rx_len : max_len;
    memcpy(buf, s_pd_rx_buf, n);
    printf("  [hal] PD RX  opcode=0x%02X  len=%u\n", buf[0], n);
    s_pd_rx_len = 0u;
    return (int)n;
}

static void stub_gpio_w(uiox_chg_hw_t *hw, uint32_t pin, bool val)
{ (void)hw; printf("  [hal] GPIO pin=%u val=%d\n", pin, (int)val); }

static bool stub_gpio_r(uiox_chg_hw_t *hw, uint32_t pin)
{ (void)hw; (void)pin; return false; }

static void stub_isr(uiox_chg_hw_t *hw)
{
    if (!hw) return;
    hw->pending_irq |= UIOX_CHG_IRQ_STATUS;
    printf("  [hal] ISR fired  pending=0x%08X\n", hw->pending_irq);
}

/* =========================================================================
 * Stub ops table
 * ====================================================================== */

static const uiox_chg_hw_ops_t stub_ops = {
    .init           = stub_init,
    .deinit         = stub_deinit,
    .reg_read       = stub_reg_read,
    .reg_write      = stub_reg_write,
    .reg_rmw        = stub_reg_rmw,
    .adc_read       = stub_adc_read,
    .set_ichg       = stub_set_ichg,
    .set_vchg       = stub_set_vchg,
    .set_iin_lim    = stub_set_iin_lim,
    .set_vindpm     = stub_set_vindpm,
    .charge_enable  = stub_charge_enable,
    .otg_enable     = stub_otg_enable,
    .get_status     = stub_get_status,
    .wdog_reset     = stub_wdog_reset,
    .gpio_write     = stub_gpio_w,
    .gpio_read      = stub_gpio_r,
    .pd_tx_msg      = stub_pd_tx,
    .pd_rx_msg      = stub_pd_rx,
    .isr            = stub_isr,
};

/* =========================================================================
 * Hardware descriptor (BQ25895, 20 V / 5 A)
 * ====================================================================== */

static uiox_chg_hw_t s_hw = {
    .i2c_addr    = BQ25895_I2C_ADDR,
    .i2c_bus     = 0u,
    .irq         = 42u,
    .caps        = UIOX_CHG_CAP_USBC_PD   | UIOX_CHG_CAP_USBC_PPS  |
                   UIOX_CHG_CAP_BARREL     | UIOX_CHG_CAP_BC12       |
                   UIOX_CHG_CAP_OTG        | UIOX_CHG_CAP_FAST_CHARGE|
                   UIOX_CHG_CAP_ADC_VBUS   | UIOX_CHG_CAP_ADC_VBAT   |
                   UIOX_CHG_CAP_ADC_IBAT   | UIOX_CHG_CAP_ADC_TDIE   |
                   UIOX_CHG_CAP_ADC_NTC    | UIOX_CHG_CAP_BATFET      |
                   UIOX_CHG_CAP_WATCHDOG   | UIOX_CHG_CAP_VINDPM      |
                   UIOX_CHG_CAP_IINDPM,
    .ic_type     = UIOX_CHG_IC_BQ25895,
    .model       = "TI BQ25895 USB-C PD + Barrel Jack",
    .vbus_max_mv = 20000u,
    .vbat_max_mv =  4200u,
    .ibat_max_ma =  5000u,
    .iin_max_ma  =  5000u,
};

/* =========================================================================
 * Default charge profile (4.2 V LiPo, 2 A fast charge)
 * ====================================================================== */

static const uiox_chg_profile_t s_profile = {
    .precharge_ma   =  128u,
    .fast_charge_ma = 2000u,
    .cv_mv          = 4200u,
    .taper_ma       =  200u,
    .iin_lim_ma     = 3000u,
    .vindpm_mv      = 4500u,
};

/* =========================================================================
 * Event callback
 * ====================================================================== */

static void on_chg_event(uiox_chg_ev_t ev,
                          const uiox_chg_evt_t *data, void *ctx)
{
    (void)ctx;
    printf("  [event] %-24s  src=%-12s  chrg=%-12s",
           uiox_chg_ev_name(ev),
           data ? uiox_chg_src_name(data->src)   : "-",
           data ? uiox_chg_chrg_name(data->chrg)  : "-");
    if (data)
        printf("  vbus=%d mV  ibat=%d mA  faults=0x%02X",
               data->vbus_mv, data->ibat_ma, data->fault_flags);
    printf("\n");
}

/* =========================================================================
 * main
 * ====================================================================== */

int main(void)
{
    printf("=== UIOX AC Adapter / Charger IC Stack Demo ===\n\n");

    /* ------------------------------------------------------------------ */
    printf("--- Open ---\n");
    uiox_chg_device_t      dev;
    uiox_chg_open_params_t p = {
        .hw      = &s_hw,
        .hw_ops  = &stub_ops,
        .profile = &s_profile,
        .evt_cb  = on_chg_event,
    };
    int rc = uiox_chg_open(&dev, &p);
    if (rc < 0) { printf("[error] open: %d\n", rc); return 1; }

    /* ------------------------------------------------------------------ */
    printf("\n--- Start ---\n");
    rc = uiox_chg_start(&dev);
    printf("  State: %s  rc=%d\n",
           uiox_chg_state_name(dev.subsys.state), rc);

    /* ------------------------------------------------------------------ */
    printf("\n--- Device info ---\n");
    uiox_chg_print_info(&dev);

    /* ------------------------------------------------------------------ */
    printf("\n--- ADC snapshot (no source) ---\n");
    for (int ch = 0; ch < (int)UIOX_CHG_ADC_MAX; ch++) {
        int32_t v = 0;
        uiox_chg_get_adc(&dev, (uiox_chg_adc_ch_t)ch, &v);
        static const char *names[] = {
            "VBUS","VBAT","IBAT","VSYS","TDIE","NTC"
        };
        printf("  %-6s = %d\n", names[ch], v);
    }

    /* ------------------------------------------------------------------ */
    printf("\n--- Simulate USB-C PD plug-in ---\n");
    s_usbc_pd_connected = true;
    s_hw.pending_irq   |= UIOX_CHG_IRQ_VBUS | UIOX_CHG_IRQ_STATUS;
    /* Tick 1: plug detected, GET_CAPS sent */
    uiox_chg_tick(&dev, 10u);

    /* ------------------------------------------------------------------ */
    printf("\n--- Simulate PD source capabilities received ---\n");
    /* stub_pd_tx already loaded s_pd_rx_buf with caps; pending_irq set.
     * Tick 2: policy receives caps, selects 20 V / 5 A, sends REQUEST  */
    uiox_chg_tick(&dev, 20u);

    /* ------------------------------------------------------------------ */
    printf("\n--- Simulate PD ACCEPT received ---\n");
    /* stub_pd_tx (REQUEST path) loaded ACCEPT into s_pd_rx_buf          */
    uiox_chg_tick(&dev, 30u);

    /* ------------------------------------------------------------------ */
    printf("\n--- Status after PD contract ---\n");
    {
        uiox_chg_chrg_t chrg;
        uiox_chg_src_t  src;
        uint32_t        faults;
        uiox_chg_get_status(&dev, &chrg, &src, &faults);
        printf("  Source : %s\n", uiox_chg_src_name(src));
        printf("  Charge : %s\n", uiox_chg_chrg_name(chrg));
        printf("  Faults : 0x%08X\n", faults);
        printf("  VBUS   : %u mV\n",
               uiox_chg_policy_vbus_mv(&dev.subsys.policy));
        printf("  IBAT   : %u mA\n",
               uiox_chg_policy_ibat_ma(&dev.subsys.policy));
    }

    /* ------------------------------------------------------------------ */
    printf("\n--- Runtime profile update (3 A fast charge) ---\n");
    uiox_chg_profile_t new_profile = s_profile;
    new_profile.fast_charge_ma = 3000u;
    rc = uiox_chg_set_profile(&dev, &new_profile);
    printf("  set_profile rc=%d\n", rc);

    /* ------------------------------------------------------------------ */
    printf("\n--- Simulate USB-C PD unplug, barrel jack plug-in ---\n");
    s_usbc_pd_connected = false;
    s_barrel_connected  = true;
    s_hw.pending_irq   |= UIOX_CHG_IRQ_VBUS | UIOX_CHG_IRQ_ACOK |
                           UIOX_CHG_IRQ_STATUS;
    uiox_chg_tick(&dev, 40u);  /* unplug event */
    uiox_chg_tick(&dev, 50u);  /* barrel plug event */

    /* ------------------------------------------------------------------ */
    printf("\n--- Simulate OTG boost enable ---\n");
    s_barrel_connected = false;   /* remove input first */
    s_hw.pending_irq  |= UIOX_CHG_IRQ_STATUS;
    uiox_chg_tick(&dev, 60u);
    rc = uiox_chg_otg_enable(&dev, true);
    printf("  OTG enable rc=%d\n", rc);

    /* ------------------------------------------------------------------ */
    printf("\n--- Simulate OTG disable ---\n");
    rc = uiox_chg_otg_enable(&dev, false);
    printf("  OTG disable rc=%d\n", rc);

    /* ------------------------------------------------------------------ */
    printf("\n--- Simulate battery OVP fault ---\n");
    s_usbc_pd_connected = true;
    s_fault_sim         = true;
    s_hw.pending_irq   |= UIOX_CHG_IRQ_FAULT | UIOX_CHG_IRQ_STATUS;
    uiox_chg_tick(&dev, 70u);
    printf("  State after fault: %s\n",
           uiox_chg_state_name(dev.subsys.state));

    /* ------------------------------------------------------------------ */
    printf("\n--- Simulate fault cleared ---\n");
    s_fault_sim       = false;
    s_hw.pending_irq |= UIOX_CHG_IRQ_FAULT | UIOX_CHG_IRQ_STATUS;
    uiox_chg_tick(&dev, 80u);
    printf("  State after clear: %s\n",
           uiox_chg_state_name(dev.subsys.state));

    /* ------------------------------------------------------------------ */
    printf("\n--- Simulate thermal throttle (die temp 85 °C) ---\n");
    s_thermal_sim = true;
    for (uint32_t t = 90u; t <= 110u; t += 10u)
        uiox_chg_tick(&dev, t);

    /* ------------------------------------------------------------------ */
    printf("\n--- Simulate thermal resume (die temp back to 35 °C) ---\n");
    s_thermal_sim = false;
    uiox_chg_tick(&dev, 120u);

    /* ------------------------------------------------------------------ */
    printf("\n--- Tick loop (5 × 10 ms, watchdog exercise) ---\n");
    for (uint32_t t = 200u; t <= 240u; t += 10u)
        uiox_chg_tick(&dev, t);

    /* ------------------------------------------------------------------ */
    printf("\n--- NVRAM / register read-back ---\n");
    {
        uint8_t dev_id = 0u;
        uiox_chg_hw_reg_read(&s_hw, BQ25895_REG0B, &dev_id);
        printf("  BQ25895_REG0B (device ID) = 0x%02X\n", dev_id);
        uint8_t ichg_reg = 0u;
        uiox_chg_hw_reg_read(&s_hw, BQ25895_REG02, &ichg_reg);
        printf("  BQ25895_REG02 (ICHG)      = 0x%02X  (~%u mA)\n",
               ichg_reg, (uint32_t)(ichg_reg & 0x7Fu) * 64u);
    }

    /* ------------------------------------------------------------------ */
    printf("\n--- Statistics ---\n");
    uiox_chg_print_stats(&dev);

    /* ------------------------------------------------------------------ */
    printf("\n--- Final device info ---\n");
    uiox_chg_print_info(&dev);

    /* ------------------------------------------------------------------ */
    printf("\n--- Stop and close ---\n");
    uiox_chg_stop(&dev);
    printf("  State: %s\n", uiox_chg_state_name(dev.subsys.state));
    uiox_chg_close(&dev);
    printf("  Device: CLOSED\n");

    printf("\n=== UIOX AC Adapter / Charger IC Demo complete ===\n");
    return 0;
}
