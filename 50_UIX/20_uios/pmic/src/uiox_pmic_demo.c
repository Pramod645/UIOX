/**
 * @file    uiox_pmic_demo.c
 * @brief   UIOX PMIC stack end-to-end demonstration.
 * @date    2026-06-04
 */

 #include "uiox_pmic_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <errno.h>
 
 /* Simulated register file */
 static uint8_t s_regs[0x200];
 static uint32_t s_adc_vals[UIOX_PMIC_ADC_MAX] =
     { 3800,5000,35,28,500,5050 }; /* vbat,vsys,die,ntc,ibat,vbus */
 static uint32_t s_irq_flags = 0;
 static uint32_t s_wdt_kicks = 0;
 
 static int  stub_init   (uiox_pmic_hw_t *hw)
 { (void)hw; printf("  [hal] init  DA9063  I2C=0x%02X\n", hw->i2c_addr);
   s_regs[UIOX_REG_DEVICE_ID]  = 0x61u;
   s_regs[UIOX_REG_VARIANT_ID] = 0x01u;
   return 0; }
 static void stub_deinit (uiox_pmic_hw_t *hw) { (void)hw; }
 static int  stub_enable (uiox_pmic_hw_t *hw, bool on)
 { (void)hw; printf("  [hal] PMIC %s\n", on?"ON":"OFF"); return 0; }
 static int  stub_reset  (uiox_pmic_hw_t *hw)
 { (void)hw; printf("  [hal] reset\n"); return 0; }
 static int  stub_reg_r  (uiox_pmic_hw_t *hw, uint16_t r, uint8_t *v)
 { (void)hw; *v=(r<0x200u)?s_regs[r]:0; return 0; }
 static int  stub_reg_w  (uiox_pmic_hw_t *hw, uint16_t r, uint8_t v)
 { (void)hw; if(r<0x200u) s_regs[r]=v;
   printf("  [hal] reg[0x%03X] ← 0x%02X\n",r,v); return 0; }
 static int  stub_reg_u  (uiox_pmic_hw_t *hw, uint16_t r, uint8_t m, uint8_t v)
 { (void)hw; if(r<0x200u) s_regs[r]=(s_regs[r]&~m)|v; return 0; }
 static int  stub_bulk_r (uiox_pmic_hw_t *hw, uint16_t r,
                           uint8_t *b, uint16_t l)
 { (void)hw; for(uint16_t i=0;i<l&&(r+i)<0x200u;i++) b[i]=s_regs[r+i]; return 0; }
 static int  stub_bulk_w (uiox_pmic_hw_t *hw, uint16_t r,
                           const uint8_t *b, uint16_t l)
 { (void)hw; for(uint16_t i=0;i<l&&(r+i)<0x200u;i++) s_regs[r+i]=b[i]; return 0; }
 static int  stub_adc    (uiox_pmic_hw_t *hw,
                           uiox_pmic_adc_ch_t ch, uint32_t *v)
 { (void)hw; *v=s_adc_vals[ch<UIOX_PMIC_ADC_MAX?ch:0]; return 0; }
 static int  stub_wdt_kick(uiox_pmic_hw_t *hw)
 { (void)hw; s_wdt_kicks++; printf("  [hal] WDT kick #%u\n",s_wdt_kicks); return 0;}
 static int  stub_wdt_en (uiox_pmic_hw_t *hw, bool en, uint32_t ms)
 { (void)hw; printf("  [hal] WDT %s  timeout=%u ms\n",en?"ON":"OFF",ms); return 0;}
 static int  stub_irq_st (uiox_pmic_hw_t *hw, uint32_t *f)
 { (void)hw; *f=s_irq_flags; return 0; }
 static int  stub_irq_cl (uiox_pmic_hw_t *hw, uint32_t f)
 { (void)hw; s_irq_flags &= ~f; return 0; }
 static int  stub_irq_mk (uiox_pmic_hw_t *hw, uint32_t m)
 { (void)hw; (void)m; return 0; }
 static int  stub_irq_um (uiox_pmic_hw_t *hw, uint32_t m)
 { (void)hw; (void)m; return 0; }
 static bool stub_gpio_r (uiox_pmic_hw_t *hw, uint32_t p)
 { (void)hw; (void)p; return false; }
 static void stub_gpio_w (uiox_pmic_hw_t *hw, uint32_t p, bool v)
 { (void)hw; printf("  [hal] GPIO pin=%u val=%d\n",p,(int)v); }
 static void stub_isr    (uiox_pmic_hw_t *hw) { (void)hw; }
 
 static const uiox_pmic_hw_ops_t stub_ops = {
     .init=stub_init, .deinit=stub_deinit, .enable=stub_enable,
     .reset=stub_reset, .reg_read=stub_reg_r, .reg_write=stub_reg_w,
     .reg_update=stub_reg_u, .bulk_read=stub_bulk_r, .bulk_write=stub_bulk_w,
     .adc_read=stub_adc, .wdt_kick=stub_wdt_kick, .wdt_enable=stub_wdt_en,
     .irq_status=stub_irq_st, .irq_clear=stub_irq_cl,
     .irq_mask=stub_irq_mk, .irq_unmask=stub_irq_um,
     .gpio_read=stub_gpio_r, .gpio_write=stub_gpio_w, .isr=stub_isr,
 };
 
 static uiox_pmic_hw_t s_hw = {
     .i2c_base=0x40005400uL, .i2c_addr=0x58u, .irq=42,
     .caps=UIOX_PMIC_CAP_BUCK|UIOX_PMIC_CAP_LDO|UIOX_PMIC_CAP_OTP|
           UIOX_PMIC_CAP_OCP|UIOX_PMIC_CAP_OVP|UIOX_PMIC_CAP_WATCHDOG|
           UIOX_PMIC_CAP_ADC|UIOX_PMIC_CAP_DVFS,
     .bus=UIOX_PMIC_BUS_I2C, .model="DA9063",
     .num_bucks=6, .num_ldos=11, .num_switches=2,
     .en_pin=8, .rst_pin=9, .int_pin=10, .pgood_pin=11,
 };
 
 static void on_pmic_event(uiox_pmic_ev_t evt, uint8_t rail_id, void *ctx)
 { (void)ctx; printf("  [event] %-14s  rail=0x%02X\n",
                     uiox_pmic_ev_name(evt), rail_id); }
 
 int main(void)
 {
     printf("=== UIOX PMIC Stack Demo ===\n\n");
 
     printf("--- Open ---\n");
     uiox_pmic_device_t dev;
     uiox_pmic_open_params_t p = {
         .hw=&s_hw, .hw_ops=&stub_ops,
         .evt_cb=on_pmic_event,
     };
     int rc = uiox_pmic_open(&dev, &p);
     if (rc<0) { printf("[error] open: %d\n",rc); return 1; }
 
     printf("\n--- Register rails ---\n");
     static const uiox_pmic_rail_t rails[] = {
         { "VCORE", UIOX_PMIC_RAIL_BUCK, 0, 0x029u,0x020u,0,0x7Fu,
           600u,1200u,25u,1000u,1000u,5000u,false,false,0 },
         { "VMEM",  UIOX_PMIC_RAIL_BUCK, 1, 0x02Bu,0x022u,0,0x7Fu,
           1000u,1200u,25u,1100u,1100u,3000u,false,false,0 },
         { "VIO",   UIOX_PMIC_RAIL_LDO,  2, 0x0A2u,0x0A3u,0,0x1Fu,
           1800u,3300u,100u,1800u,1800u,500u,false,false,0 },
         { "VRTC",  UIOX_PMIC_RAIL_LDO,  3, 0x0A4u,0x0A5u,0,0x1Fu,
           1200u,3300u,100u,1800u,1800u,50u,false,true,0 },
     };
     for (size_t i=0; i<sizeof(rails)/sizeof(rails[0]); i++) {
         uiox_pmic_rail_add(&dev, &rails[i]);
         printf("  Registered: %s\n", rails[i].name);
     }
 
     printf("\n--- Add OPPs ---\n");
     uiox_pmic_add_opp(&dev,  600u,  850u,  800u);
     uiox_pmic_add_opp(&dev, 1200u,  950u, 2000u);
     uiox_pmic_add_opp(&dev, 1800u, 1050u, 4500u);
     uiox_pmic_add_opp(&dev, 2400u, 1100u, 7000u);
     printf("  4 OPPs added\n");
 
     printf("\n--- Start ---\n");
     rc = uiox_pmic_start(&dev);
     printf("  State: %s  rc=%d\n",
            uiox_pmic_state_name(dev.subsys.state), rc);
 
     printf("\n--- Enable rails ---\n");
     uiox_pmic_rail_on(&dev, "VCORE");
     uiox_pmic_rail_on(&dev, "VMEM");
     uiox_pmic_rail_on(&dev, "VIO");
     uiox_pmic_rail_on(&dev, "VRTC");
 
     printf("\n--- DVFS: set power state ACTIVE ---\n");
     uiox_pmic_set_ps(&dev, UIOX_PMIC_PS_ACTIVE);
     uint32_t vcore_mv=0;
     uiox_pmic_rail_read_mv(&dev, "VCORE", &vcore_mv);
     printf("  VCORE = %u mV  OPP=%u\n", vcore_mv, dev.subsys.policy.cur_opp);
 
     printf("\n--- DVFS: voltage adjustment ---\n");
     uiox_pmic_rail_voltage(&dev, "VCORE", 1050u);
     uiox_pmic_rail_read_mv(&dev, "VCORE", &vcore_mv);
     printf("  VCORE set 1050 mV → actual %u mV\n", vcore_mv);
 
     printf("\n--- ADC readings ---\n");
     uint32_t val=0;
     uiox_pmic_adc_read(&dev, UIOX_PMIC_ADC_VSYS,     &val);
     printf("  VSYS     = %u mV\n", val);
     uiox_pmic_adc_read(&dev, UIOX_PMIC_ADC_VBAT,     &val);
     printf("  VBAT     = %u mV\n", val);
     uiox_pmic_adc_read(&dev, UIOX_PMIC_ADC_IBAT,     &val);
     printf("  IBAT     = %u mA\n", val);
     uiox_pmic_adc_read(&dev, UIOX_PMIC_ADC_TEMP_DIE, &val);
     printf("  TEMP_DIE = %d°C\n", (int8_t)val);
 
     printf("\n--- Telemetry snapshot ---\n");
     uiox_pmic_telem_t snap;
     uiox_pmic_get_telemetry(&dev, &snap, 100u);
     printf("  vsys=%u mV  vbat=%u mV  ibat=%u mA  die=%d°C\n",
            snap.vsys_mv, snap.vbat_mv, snap.ibat_ma, snap.die_temp_c);
 
     printf("\n--- Load-based DVFS (5 ticks) ---\n");
     uint32_t loads[] = {10,50,85,30,90};
     for (int i=0; i<5; i++) {
         uiox_pmic_update_load(&dev, loads[i], (uint32_t)((i+1)*10));
         uiox_pmic_tick(&dev, (uint32_t)((i+1)*10));
         printf("  load=%3u%%  OPP=%u  %u MHz  %u mV\n",
                loads[i],
                dev.subsys.policy.cur_opp,
                dev.subsys.policy.opps[dev.subsys.policy.cur_opp].cpu_freq_mhz,
                dev.subsys.policy.opps[dev.subsys.policy.cur_opp].vcore_mv);
     }
 
     printf("\n--- Simulate OCP fault ---\n");
     s_irq_flags = UIOX_PMIC_FAULT_OCP;
     uiox_pmic_tick(&dev, 100u);
 
     printf("\n--- Sleep / Wake ---\n");
     uiox_pmic_set_ps(&dev, UIOX_PMIC_PS_SLEEP);
     printf("  Power state: %s\n", uiox_pmic_ps_name(dev.subsys.policy.current_ps));
     uiox_pmic_policy_wake(&dev.subsys.policy);
     printf("  Woke up\n");
 
     printf("\n--- Watchdog kick ---\n");
     uiox_pmic_wdt_kick(&dev);
 
     printf("\n--- Statistics ---\n");
     uiox_pmic_print_stats(&dev);
 
     printf("\n--- Event log ---\n");
     uiox_pmic_print_events();
 
     printf("\n--- Disable rails + stop ---\n");
     uiox_pmic_rail_off(&dev, "VIO");
     uiox_pmic_rail_off(&dev, "VCORE");
     uiox_pmic_rail_off(&dev, "VMEM");
     uiox_pmic_stop(&dev);
     uiox_pmic_close(&dev);
     printf("  Device: CLOSED\n");
     printf("\n=== UIOX PMIC Demo complete ===\n");
     return 0;
 }
 