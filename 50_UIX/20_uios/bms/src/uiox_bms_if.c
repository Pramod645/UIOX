/**
 * @file    uiox_bms_if.c
 * @brief   UIOX BMS interface driver implementation.
 * @date    2026-06-04
 */
// Layer 2
 #include "uiox_bms_if.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_bms_if_config(uiox_bms_if_t *bif, uiox_bms_hw_t *hw)
 {
     if (!bif || !hw) return -EINVAL;
     memset(bif, 0, sizeof(*bif));
     bif->hw      = hw;
     bif->adc_gain= 365u;   /* default 365 µV/LSB for BQ76940 */
     bif->primed  = true;
     uiox_bms_buf_init();
     return 0;
 }
 
 int uiox_bms_if_start(uiox_bms_if_t *bif)
 {
     if (!bif || !bif->primed) return -EINVAL;
 
     /* Read device ID and calibration */
     const uiox_bms_hw_ops_t *ops =
         (const uiox_bms_hw_ops_t *)bif->hw->priv;
 
     if (ops && ops->reg_read)
         ops->reg_read(bif->hw, UIOX_REG_BMS_DEVICE_ID, &bif->device_id);
 
     /* Read ADC gain registers */
     uint8_t gain1 = 0, gain2 = 0, offset = 0;
     if (ops && ops->reg_read) {
         ops->reg_read(bif->hw, UIOX_REG_BMS_ADCGAIN1, &gain1);
         ops->reg_read(bif->hw, UIOX_REG_BMS_ADCGAIN2, &gain2);
         ops->reg_read(bif->hw, UIOX_REG_BMS_ADCOFFSET, &offset);
         /* BQ76940: GAIN = 365 + (((ADCGAIN1 & 0x0C) << 1) |
          *                        ((ADCGAIN2 & 0xE0) >> 5)) µV/LSB */
         bif->adc_gain = (uint16_t)(365u +
                         ((((uint16_t)gain1 & 0x0Cu) << 1u) |
                          (((uint16_t)gain2 & 0xE0u) >> 5u)));
         bif->adc_offset = (int8_t)offset;
     }
 
     /* Enable ADC and coulomb counter */
     if (ops && ops->reg_write) {
         ops->reg_write(bif->hw, UIOX_REG_BMS_SYS_CTRL1, 0x18u); /* ADC_EN */
         ops->reg_write(bif->hw, UIOX_REG_BMS_SYS_CTRL2, 0x41u); /* CC_EN + CHG + DSG */
     }
 
     /* Enable both FETs */
     uiox_bms_hw_set_chg_fet(bif->hw, true);
     uiox_bms_hw_set_dsg_fet(bif->hw, true);
     return 0;
 }
 
 void uiox_bms_if_stop(uiox_bms_if_t *bif)
 {
     if (!bif) return;
     uiox_bms_hw_set_chg_fet(bif->hw, false);
     uiox_bms_hw_set_dsg_fet(bif->hw, false);
 }
 
 int uiox_bms_if_measure(uiox_bms_if_t *bif)
 {
     if (!bif || !bif->primed) return -EINVAL;
     bif->stats.measurements++;
     int rc = uiox_bms_hw_measure_cells(bif->hw);
     if (rc < 0) { bif->stats.comm_errors++; return rc; }
     rc = uiox_bms_hw_measure_current(bif->hw);
     if (rc < 0) { bif->stats.comm_errors++; return rc; }
     rc = uiox_bms_hw_measure_temp(bif->hw);
     if (rc < 0) { bif->stats.comm_errors++; }
     return rc;
 }
 
 int uiox_bms_if_irq_handle(uiox_bms_if_t *bif, uint32_t now_ms)
 {
     if (!bif) return -EINVAL;
     bif->stats.irq_count++;
 
     uint32_t flags = 0;
     int rc = uiox_bms_hw_fault_status(bif->hw, &flags);
     if (rc < 0 || !flags) return rc;
 
     uiox_bms_hw_fault_clear(bif->hw, flags);
 
     static const struct { uint32_t bit; uiox_bms_ev_t ev; }
     fault_map[] = {
         { UIOX_BMS_FAULT_OVP,     UIOX_BMS_EV_OVP      },
         { UIOX_BMS_FAULT_UVP,     UIOX_BMS_EV_UVP      },
         { UIOX_BMS_FAULT_OCP_CHG, UIOX_BMS_EV_OCP_CHG  },
         { UIOX_BMS_FAULT_OCP_DSG, UIOX_BMS_EV_OCP_DSG  },
         { UIOX_BMS_FAULT_SCP,     UIOX_BMS_EV_SCP      },
         { UIOX_BMS_FAULT_OTP,     UIOX_BMS_EV_OTP      },
         { UIOX_BMS_FAULT_UTP,     UIOX_BMS_EV_UTP      },
     };
     for (size_t i = 0; i < sizeof(fault_map)/sizeof(fault_map[0]); i++) {
         if (flags & fault_map[i].bit) {
             uiox_bms_event_t ev = {
                 .type        = fault_map[i].ev,
                 .ts_ms       = now_ms,
                 .pack_mv     = bif->hw->pack_mv,
                 .current_ma  = bif->hw->current_ma,
                 .temp_dc     = bif->hw->temp_dc[0],
                 .fault_flags = flags,
                 .valid       = true,
             };
             uiox_bms_event_push(&ev);
             bif->stats.fault_count++;
         }
     }
     return (int)flags;
 }
 
 int uiox_bms_if_telemetry(uiox_bms_if_t *bif,
                            uiox_bms_telem_t *out,
                            uint32_t now_ms,
                            uint8_t soc_pct, uint8_t soh_pct,
                            int32_t remain_mah, int32_t full_mah)
 {
     if (!bif || !out) return -EINVAL;
     out->ts_ms      = now_ms;
     out->pack_mv    = bif->hw->pack_mv;
     out->current_ma = bif->hw->current_ma;
     out->soc_pct    = soc_pct;
     out->soh_pct    = soh_pct;
     out->remain_mah = remain_mah;
     out->full_mah   = full_mah;
     out->fault_flags= bif->hw->fault_flags;
     memcpy(out->cell_mv, bif->hw->cell_mv,
            bif->hw->num_cells * sizeof(uint32_t));
     memcpy(out->temp_dc, bif->hw->temp_dc,
            bif->hw->num_temps * sizeof(int16_t));
     return 0;
 }
 
 void uiox_bms_if_stats_get(const uiox_bms_if_t *bif,
                             uiox_bms_if_stats_t *out)
 { if (!bif || !out) return; memcpy(out, &bif->stats, sizeof(*out)); }
 
 void uiox_bms_if_stats_reset(uiox_bms_if_t *bif)
 { if (!bif) return; memset(&bif->stats, 0, sizeof(bif->stats)); }
 