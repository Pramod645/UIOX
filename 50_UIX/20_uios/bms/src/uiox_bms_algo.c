/**
 * @file    uiox_bms_algo.c
 * @brief   UIOX BMS algorithms implementation.
 * @date    2026-06-04
 */

 #include "uiox_bms_algo.h"
 #include <string.h>
 #include <errno.h>
 
 /* OCV-SoC lookup table for NMC 3.5 Ah cell at 25°C
  * Format: { ocv_mv, soc_pct × 10 } sorted ascending by OCV */
 static const struct { uint32_t ocv_mv; uint16_t soc_x10; }
 s_ocv_table[UIOX_BMS_OCV_TABLE_SIZE] = {
     { 2800u,   0u }, { 3100u,  50u }, { 3300u, 100u },
     { 3450u, 150u }, { 3550u, 200u }, { 3620u, 250u },
     { 3680u, 300u }, { 3720u, 350u }, { 3760u, 400u },
     { 3790u, 450u }, { 3820u, 500u }, { 3850u, 550u },
     { 3880u, 600u }, { 3910u, 650u }, { 3940u, 700u },
     { 3970u, 750u }, { 4010u, 800u }, { 4060u, 850u },
     { 4110u, 900u }, { 4160u, 950u }, { 4200u,1000u },
 };
 
 int uiox_bms_algo_init(uiox_bms_algo_t *algo, const uiox_bms_batt_t *batt)
 {
     if (!algo || !batt) return -EINVAL;
     memset(algo, 0, sizeof(*algo));
     memcpy(&algo->batt, batt, sizeof(*batt));
     algo->remain_mah  = (int32_t)batt->full_mah;
     algo->soc_pct     = 100u;
     algo->soh_pct     = 100u;
     algo->tte_min     = -1;
     algo->ttf_min     = -1;
     return 0;
 }
 
 void uiox_bms_algo_update_soc(uiox_bms_algo_t *algo,
                                int32_t current_ma,
                                uint32_t pack_mv,
                                uint32_t dt_ms)
 {
     if (!algo) return;
 
     /* Coulomb integration: ΔQ = I × Δt / 3600000 (mAh) */
     int32_t delta_uah = (current_ma * (int32_t)dt_ms) / 3600;
     algo->remain_mah += delta_uah / 1000;
 
     /* Clamp */
     if (algo->remain_mah < 0)
         algo->remain_mah = 0;
     if (algo->remain_mah > (int32_t)algo->batt.full_mah)
         algo->remain_mah = (int32_t)algo->batt.full_mah;
 
     /* SoC from coulomb counter */
     uint8_t soc_cc = (algo->batt.full_mah > 0) ?
         (uint8_t)((uint32_t)algo->remain_mah * 100u / algo->batt.full_mah) :
         0u;
 
     /* OCV cross-check at low current (< 100 mA) */
     uint8_t soc_ocv = algo->soc_pct;
     if (current_ma > -100 && current_ma < 100) {
         uint32_t cell_ocv = pack_mv / algo->batt.design_mah; /* simplified */
         soc_ocv = uiox_bms_algo_ocv_to_soc(pack_mv / 4u);  /* 4S example */
         /* Blend: 70% coulomb, 30% OCV */
         soc_cc = (uint8_t)((uint32_t)soc_cc * 7u / 10u +
                             (uint32_t)soc_ocv * 3u / 10u);
         (void)cell_ocv;
     }
 
     algo->soc_pct = soc_cc;
     algo->charging = (current_ma > 50);
 }
 
 void uiox_bms_algo_update_soh(uiox_bms_algo_t *algo,
                                int32_t measured_full_mah)
 {
     if (!algo || !algo->batt.design_mah || measured_full_mah <= 0) return;
     algo->batt.full_mah = (uint32_t)measured_full_mah;
     algo->soh_pct = (uint8_t)((uint32_t)measured_full_mah * 100u /
                                algo->batt.design_mah);
     if (algo->soh_pct > 100u) algo->soh_pct = 100u;
 }
 
 void uiox_bms_algo_update_tte(uiox_bms_algo_t *algo, int32_t current_ma)
 {
     if (!algo) return;
     if (current_ma < -10) {
         /* Discharging */
         uint32_t abs_ma = (uint32_t)(-current_ma);
         algo->tte_min = (int32_t)((uint32_t)algo->remain_mah * 60u / abs_ma);
         algo->ttf_min = -1;
     } else if (current_ma > 10) {
         /* Charging */
         int32_t to_full = (int32_t)algo->batt.full_mah - algo->remain_mah;
         if (to_full > 0)
             algo->ttf_min = (int32_t)((uint32_t)to_full * 60u /
                                        (uint32_t)current_ma);
         else
             algo->ttf_min = 0;
         algo->tte_min = -1;
     } else {
         algo->tte_min = -1;
         algo->ttf_min = -1;
     }
 }
 
 uint8_t uiox_bms_algo_ocv_to_soc(uint32_t ocv_mv)
 {
     /* Below minimum */
     if (ocv_mv <= s_ocv_table[0].ocv_mv) return 0u;
     /* Above maximum */
     if (ocv_mv >= s_ocv_table[UIOX_BMS_OCV_TABLE_SIZE-1].ocv_mv)
         return 100u;
 
     /* Linear interpolation */
     for (int i = 0; i < UIOX_BMS_OCV_TABLE_SIZE - 1; i++) {
         if (ocv_mv >= s_ocv_table[i].ocv_mv &&
             ocv_mv <= s_ocv_table[i+1].ocv_mv) {
             uint32_t dV  = s_ocv_table[i+1].ocv_mv - s_ocv_table[i].ocv_mv;
             uint32_t dS  = s_ocv_table[i+1].soc_x10 - s_ocv_table[i].soc_x10;
             uint32_t soc_x10 = s_ocv_table[i].soc_x10 +
                                 dS * (ocv_mv - s_ocv_table[i].ocv_mv) / dV;
             return (uint8_t)(soc_x10 / 10u);
         }
     }
     return 0u;
 }
 
 bool uiox_bms_algo_check_full(uiox_bms_algo_t *algo,
                                uint32_t pack_mv, int32_t current_ma)
 {
     if (!algo) return false;
     /* Full = voltage at max AND taper current < C/20 */
     uint32_t c20 = algo->batt.nominal_mah / 20u;
     bool v_full = (pack_mv >= algo->batt.vfull_mv);
     bool i_taper = (current_ma >= 0 && (uint32_t)current_ma < c20);
     if (v_full && i_taper && !algo->full_detected) {
         algo->full_detected  = true;
         algo->last_full_mv   = pack_mv;
         algo->remain_mah     = (int32_t)algo->batt.full_mah;
         algo->soc_pct        = 100u;
     }
     return algo->full_detected;
 }
 