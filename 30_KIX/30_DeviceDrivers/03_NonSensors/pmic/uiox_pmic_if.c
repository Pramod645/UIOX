/**
 * @file    uiox_pmic_if.c
 * @brief   UIOX PMIC interface driver implementation.
 * @date    2026-06-04
 */

 #include "uiox_pmic_if.h"
 
 int uiox_pmic_if_config(uiox_pmic_if_t *pif, uiox_pmic_hw_t *hw)
 {
     if (!pif || !hw) return -EINVAL;
     memset(pif, 0, sizeof(*pif));
     pif->hw     = hw;
     pif->primed = true;
     uiox_pmic_buf_init();
     return 0;
 }
 
 int uiox_pmic_if_start(uiox_pmic_if_t *pif)
 {
     if (!pif || !pif->primed) return -EINVAL;
 
     /* Read device/variant IDs */
     uiox_pmic_hw_reg_read(pif->hw, UIOX_REG_DEVICE_ID,  &pif->device_id);
     uiox_pmic_hw_reg_read(pif->hw, UIOX_REG_VARIANT_ID, &pif->variant_id);
 
     /* Enable PMIC */
     int rc = uiox_pmic_hw_enable(pif->hw, true);
     if (rc < 0) return rc;
 
     /* Unmask critical IRQs (OTP, OCP, OVP, WDT) */
     uiox_pmic_hw_irq_clear(pif->hw, 0xFFFFFFFFu);
     const uiox_pmic_hw_ops_t *ops =
         (const uiox_pmic_hw_ops_t *)pif->hw->priv;
     if (ops && ops->irq_unmask)
         ops->irq_unmask(pif->hw,
                         UIOX_PMIC_FAULT_OTP | UIOX_PMIC_FAULT_OCP |
                         UIOX_PMIC_FAULT_OVP | UIOX_PMIC_FAULT_WDT);
 
     /* Enable watchdog */
     if (ops && ops->wdt_enable)
         ops->wdt_enable(pif->hw, true, 10000u); /* 10 s timeout */
 
     return 0;
 }
 
 void uiox_pmic_if_stop(uiox_pmic_if_t *pif)
 {
     if (!pif) return;
     uiox_pmic_hw_enable(pif->hw, false);
 }
 
 int uiox_pmic_if_field_wr(uiox_pmic_if_t *pif,
                            uint16_t reg, uint8_t mask,
                            uint8_t shift, uint8_t val)
 {
     if (!pif) return -EINVAL;
     uint8_t field = (uint8_t)(val << shift) & mask;
     int rc = uiox_pmic_hw_reg_update(pif->hw, reg, mask, field);
     if (rc == 0) pif->stats.reg_writes++;
     return rc;
 }
 
 int uiox_pmic_if_irq_handle(uiox_pmic_if_t *pif, uint32_t now_ms)
 {
     if (!pif) return -EINVAL;
     pif->stats.irq_count++;
 
     uint32_t flags = 0;
     int rc = uiox_pmic_hw_irq_status(pif->hw, &flags);
     if (rc < 0 || !flags) return rc;
 
     uiox_pmic_hw_irq_clear(pif->hw, flags);
 
     /* Push events for each active fault */
     static const struct { uint32_t bit; uiox_pmic_ev_t ev; }
     fault_map[] = {
         { UIOX_PMIC_FAULT_OTP,   UIOX_PMIC_EV_OTP   },
         { UIOX_PMIC_FAULT_OCP,   UIOX_PMIC_EV_OCP   },
         { UIOX_PMIC_FAULT_OVP,   UIOX_PMIC_EV_OVP   },
         { UIOX_PMIC_FAULT_UVP,   UIOX_PMIC_EV_UVP   },
         { UIOX_PMIC_FAULT_WDT,   UIOX_PMIC_EV_WDT   },
         { UIOX_PMIC_FAULT_PGOOD, UIOX_PMIC_EV_PGOOD_LOST },
     };
 
     for (size_t i = 0; i < sizeof(fault_map)/sizeof(fault_map[0]); i++) {
         if (flags & fault_map[i].bit) {
             uiox_pmic_event_t ev = {
                 .type        = fault_map[i].ev,
                 .rail_id     = 0xFFu,
                 .ts_ms       = now_ms,
                 .fault_flags = flags,
                 .valid       = true,
             };
             uiox_pmic_event_push(&ev);
             pif->stats.fault_count++;
             pif->hw->fault = true;
         }
     }
     return (int)flags;
 }
 
 int uiox_pmic_if_telemetry(uiox_pmic_if_t *pif,
                             uiox_pmic_telem_t *out, uint32_t now_ms)
 {
     if (!pif || !out) return -EINVAL;
     out->ts_ms = now_ms;
 
     uiox_pmic_hw_adc_read(pif->hw, UIOX_PMIC_ADC_VSYS,     &out->vsys_mv);
     uiox_pmic_hw_adc_read(pif->hw, UIOX_PMIC_ADC_VBAT,     &out->vbat_mv);
     uiox_pmic_hw_adc_read(pif->hw, UIOX_PMIC_ADC_IBAT,     &out->ibat_ma);
     uiox_pmic_hw_adc_read(pif->hw, UIOX_PMIC_ADC_VBUS,     &out->vbus_mv);
 
     uint32_t temp_raw = 0;
     uiox_pmic_hw_adc_read(pif->hw, UIOX_PMIC_ADC_TEMP_DIE, &temp_raw);
     out->die_temp_c = (int8_t)(temp_raw & 0xFFu);
     pif->hw->die_temp_c = out->die_temp_c;
 
     uint32_t ntc_raw = 0;
     uiox_pmic_hw_adc_read(pif->hw, UIOX_PMIC_ADC_TEMP_NTC, &ntc_raw);
     out->ntc_temp_c = (int8_t)(ntc_raw & 0xFFu);
     return 0;
 }
 
 void uiox_pmic_if_stats_get(const uiox_pmic_if_t *pif,
                              uiox_pmic_if_stats_t *out)
 { if (!pif || !out) return; memcpy(out, &pif->stats, sizeof(*out)); }
 
 void uiox_pmic_if_stats_reset(uiox_pmic_if_t *pif)
 { if (!pif) return; memset(&pif->stats, 0, sizeof(pif->stats)); }
 