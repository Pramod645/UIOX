/**
 * @file  uiox_als_cal.c
 * @brief UIOX ALS calibration — lux formula, CCT, auto-gain.
 * @date  2026-06-11
 */

 #include "uiox_als_cal.h"
 #include <string.h>
 #include <errno.h>
 
 /* =========================================================================
  * Built-in coefficient tables
  * ====================================================================== */
 
 /*
  * VEML7700 @ gain=1×, itime=100 ms:
  *   resolution = 0.0672 lux/count  → scale = 672/10000
  *   gain_factor[1x]=1000, [2x]=500, [1/8x]=8000, [1/4x]=4000
  *   itime_factor[100ms]=1000, [200ms]=500, [400ms]=250, [800ms]=125
  *   auto-gain: saturate >45000, too_dark <100
  */
 const uiox_als_coeff_t uiox_als_coeff_veml7700 = {
     .scale_num          = 672u,
     .scale_den          = 10000u,
     .gain_factor        = { 8000u, 4000u, 1000u, 500u, 0u, 0u, 0u },
     .itime_factor       = { 4000u, 2000u, 1000u, 500u, 250u, 125u },
     .ag_saturate        = 45000u,
     .ag_too_dark        = 100u,
     .cct_a              =  449000,
     .cct_b              = -3525000,
     .cct_c              =  6823,
 };
 
 /*
  * OPT3001: result register encodes lux directly (mantissa × 2^exp × 0.01)
  * We use scale 10/1 (raw already in 0.01 lux units after decode).
  */
 const uiox_als_coeff_t uiox_als_coeff_opt3001 = {
     .scale_num          = 10u,
     .scale_den          = 1u,
     .gain_factor        = { 1000u, 1000u, 1000u, 1000u, 0u, 0u, 0u },
     .itime_factor       = { 0u, 0u, 1000u, 0u, 0u, 1000u },
     .ag_saturate        = 60000u,
     .ag_too_dark        = 50u,
     .cct_a              = 0, .cct_b = 0, .cct_c = 5000,
 };
 
 /*
  * BH1750: 1 count = 1 lux (high-resolution mode)
  */
 const uiox_als_coeff_t uiox_als_coeff_bh1750 = {
     .scale_num          = 1000u,
     .scale_den          = 1u,
     .gain_factor        = { 1000u, 1000u, 1000u, 1000u, 0u, 0u, 0u },
     .itime_factor       = { 0u, 0u, 1000u, 0u, 0u, 0u },
     .ag_saturate        = 65000u,
     .ag_too_dark        = 10u,
     .cct_a              = 0, .cct_b = 0, .cct_c = 5000,
 };
 
 /*
  * TSL2591: CH0 = visible+IR, CH1 = IR only
  *   lux = (CH0 − CH1) × (CH0 / (CH0 − CH1)) × gain_scale
  *   Simplified: lux ≈ (CH0 − 1.7×CH1) × 408 / (gain × itime_ms)
  */
 const uiox_als_coeff_t uiox_als_coeff_tsl2591 = {
     .scale_num          = 408u,
     .scale_den          = 1u,
     .gain_factor        = { 8000u, 4000u, 1000u, 500u,
                             125u,   63u,  0u },
     .itime_factor       = { 4000u, 2000u, 1000u, 500u, 250u, 125u },
     .ag_saturate        = 36000u,
     .ag_too_dark        = 50u,
     .cct_a              =  449000,
     .cct_b              = -3525000,
     .cct_c              =  6823,
 };
 
 /* =========================================================================
  * Calibration API
  * ====================================================================== */
 
 int uiox_als_cal_init(uiox_als_cal_t *cal,
                        uiox_als_if_t *aif,
                        const uiox_als_coeff_t *coeff)
 {
     if (!cal || !aif || !coeff) return -EINVAL;
     cal->aif   = aif;
     cal->coeff = coeff;
     cal->trim  = 1000u;  /* 1.000× default */
     return 0;
 }
 
 void uiox_als_cal_set_trim(uiox_als_cal_t *cal, uint32_t trim)
 { if (cal) cal->trim = (trim > 0u) ? trim : 1000u; }
 
 uint32_t uiox_als_cal_to_lux(const uiox_als_cal_t *cal,
                                uint16_t raw_als,
                                uiox_als_gain_t gain,
                                uiox_als_itime_t itime)
 {
     if (!cal || !cal->coeff) return 0u;
     const uiox_als_coeff_t *c = cal->coeff;
 
     if (c->scale_den == 0u) return 0u;
 
     /* Step 1: apply scale  (result in milli-lux) */
     uint32_t lux_milli = ((uint32_t)raw_als * c->scale_num * 1000u)
                          / c->scale_den;
 
     /* Step 2: correct for gain (factor stored ×1000) */
     if (gain < UIOX_ALS_GAIN_MAX && c->gain_factor[gain] > 0u)
         lux_milli = lux_milli * c->gain_factor[gain] / 1000u;
 
     /* Step 3: correct for integration time (factor stored ×1000) */
     if (itime < UIOX_ALS_ITIME_MAX && c->itime_factor[itime] > 0u)
         lux_milli = lux_milli * c->itime_factor[itime] / 1000u;
 
     /* Step 4: apply user trim */
     lux_milli = lux_milli * cal->trim / 1000u;
 
     return lux_milli;
 }
 
 uint32_t uiox_als_cal_to_cct(const uiox_als_cal_t *cal,
                                uint16_t raw_als, uint16_t raw_ir)
 {
     if (!cal || !cal->coeff || raw_als == 0u) return 0u;
     const uiox_als_coeff_t *c = cal->coeff;
 
     if (c->cct_a == 0 && c->cct_b == 0)
         return (uint32_t)c->cct_c;   /* fixed CCT (no IR channel) */
 
     /* ratio = ir / als  ×1000 to avoid floats */
     uint32_t ratio = ((uint32_t)raw_ir * 1000u) / (uint32_t)raw_als;
 
     /*
      * CCT = cct_a × ratio^2 + cct_b × ratio + cct_c
      * (cct_a, cct_b stored ×1000; ratio ×1000)
      */
     int64_t cct = (int64_t)c->cct_a * (int64_t)ratio * (int64_t)ratio
                   / 1000000LL
                 + (int64_t)c->cct_b * (int64_t)ratio
                   / 1000LL
                 + (int64_t)c->cct_c;
 
     return (cct > 0) ? (uint32_t)cct : 2700u; /* floor: warm white */
 }
 
 bool uiox_als_cal_auto_gain(uiox_als_cal_t *cal,
                               uint16_t raw_als,
                               uiox_als_gain_t *gain,
                               uiox_als_itime_t *itime)
 {
     if (!cal || !gain || !itime) return false;
     const uiox_als_coeff_t *c = cal->coeff;
     bool changed = false;
 
     if (raw_als >= c->ag_saturate) {
         /* Too bright: decrease gain */
         if (*gain > UIOX_ALS_GAIN_1_8X) {
             (*gain)--;
             changed = true;
         } else if (*itime > UIOX_ALS_ITIME_25MS) {
             /* Decrease integration time if already at min gain */
             (*itime)--;
             changed = true;
         }
     } else if (raw_als <= c->ag_too_dark) {
         /* Too dark: increase gain */
         if (*gain < UIOX_ALS_GAIN_48X) {
             (*gain)++;
             changed = true;
         } else if (*itime < UIOX_ALS_ITIME_800MS) {
             (*itime)++;
             changed = true;
         }
     }
     return changed;
 }
 