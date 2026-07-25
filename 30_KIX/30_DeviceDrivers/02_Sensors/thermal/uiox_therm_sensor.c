/**
 * @file    uiox_therm_sensor.c
 * @brief   UIOX Thermal Sensor abstraction implementation.
 * @date    2026-06-05
 *
 * FIX: replaced all float arithmetic with fixed-point integer math.
 * On x86_64 with -mno-sse/-mno-sse2, float return values require SSE
 * registers which are disabled in the freestanding kernel build.
 * All intermediate values are scaled integers (× 1000 or × 65536).
 */

 #include "uiox_therm_sensor.h"

 /* -------------------------------------------------------------------------
  * Fixed-point natural log approximation — no float, no SSE.
  *
  * Returns ln(x) × 65536  (Q16.16 fixed point).
  * Input:  x as a Q16.16 fixed-point value  (i.e. real_x × 65536).
  * Accurate to < 1% for x in [0.1 × 65536 … 10 × 65536].
  *
  * Method: range-reduce to [0.5, 2) then atanh series on
  *         u = (xr-1)/(xr+1), same as the float version.
  * ---------------------------------------------------------------------- */
 static int32_t uiox_ln_q16(int32_t x_q16)
 {
     if (x_q16 <= 0) return (int32_t)0xC0000000;  /* large negative */
 
     /* ln(2) in Q16.16 = 0.693147 × 65536 ≈ 45426 */
     const int32_t LN2_Q16 = 45426;
 
     /* Range-reduce: bring x_q16 into [32768, 131072) = [0.5, 2) × 65536 */
     int32_t n   = 0;
     int32_t xr  = x_q16;
     while (xr >= 131072) { xr >>= 1; n++; }    /* xr /= 2, n++ */
     while (xr <  32768)  { xr <<= 1; n--; }    /* xr *= 2, n-- */
 
     /*
      * u = (xr - 65536) / (xr + 65536)  in Q16.16
      * Both numerator and denominator are in the same Q16 scale,
      * so we divide: u_q16 = ((xr - 65536) << 16) / (xr + 65536)
      */
     int32_t num  = xr - 65536;
     int32_t den  = xr + 65536;
     int32_t u_q16 = (int32_t)(((int64_t)num << 16) / den);
 
     /* u2 = u² in Q16.16 */
     int32_t u2_q16 = (int32_t)(((int64_t)u_q16 * u_q16) >> 16);
 
     /*
      * atanh(u) ≈ u × (1 + u²/3 + u⁴/5 + u⁶/7)
      * s_q16 = u × (65536 + u²×21845/65536 + u⁴×13107/65536 + u⁶×9362/65536)
      * where 21845 ≈ 65536/3, 13107 ≈ 65536/5, 9362 ≈ 65536/7
      */
     int32_t u4_q16 = (int32_t)(((int64_t)u2_q16 * u2_q16) >> 16);
     int32_t u6_q16 = (int32_t)(((int64_t)u4_q16 * u2_q16) >> 16);
 
     int32_t inner = 65536
                   + (int32_t)(((int64_t)u2_q16 * 21845) >> 16)
                   + (int32_t)(((int64_t)u4_q16 * 13107) >> 16)
                   + (int32_t)(((int64_t)u6_q16 *  9362) >> 16);
 
     int32_t s_q16 = (int32_t)(((int64_t)u_q16 * inner) >> 16);
 
     /* ln(xr) = 2 × atanh(u);  ln(x) = ln(xr) + n × ln(2) */
     return (2 * s_q16) + (n * LN2_Q16);
 }
 
 /* -------------------------------------------------------------------------
  * NTC: Simplified Beta equation — fixed-point, no float.
  *
  * Returns temperature in tenths of a degree Celsius (°C × 10).
  * e.g. 250 = 25.0 °C,  -100 = -10.0 °C
  *
  * Beta equation: 1/T = 1/T_nom + (1/Beta) × ln(R/R_nom)
  *
  * All temperatures in the cfg struct are in °C × 10 (int16_t).
  * cfg->beta, cfg->r_series, cfg->r_nominal are plain uint32_t.
  * ---------------------------------------------------------------------- */
 int16_t uiox_therm_ntc_convert(uint16_t raw,
                                 const uiox_therm_ntc_cfg_t *cfg)
 {
     if (!cfg || raw == 0) return INT16_MIN;
 
     uint32_t adc_max = (1u << cfg->adc_bits) - 1u;
     if (raw >= adc_max) return INT16_MIN;
 
     /*
      * R_ntc = R_series × raw / (adc_max - raw)
      * Compute as Q16.16: r_ntc_q16 = (r_series × raw << 16) / (adc_max - raw)
      */
     uint32_t denom = adc_max - raw;
     int32_t  r_ntc_q16 = (int32_t)(((uint64_t)cfg->r_series * raw << 16) / denom);
     if (r_ntc_q16 <= 0) return INT16_MIN;
 
     /*
      * ln(R_ntc / R_nom) = ln(r_ntc_q16 / r_nominal_q16)
      * r_nominal as Q16.16: r_nom_q16 = cfg->r_nominal << 16 / 1 … just scale ratio
      * ratio_q16 = (r_ntc_q16 << 16) / (cfg->r_nominal << 0)
      * But r_ntc_q16 is already Q16, so:
      * ratio_q16 = r_ntc_q16 × 65536 / cfg->r_nominal
      */
     int32_t ratio_q16 = (int32_t)(((int64_t)r_ntc_q16 << 16) /
                                    (int32_t)cfg->r_nominal);
     int32_t ln_r_q16  = uiox_ln_q16(ratio_q16);
 
     /*
      * T_nom in Kelvin × 65536 (Q16):
      * cfg->t_nominal is °C × 10, so T_nom_K = cfg->t_nominal/10 + 273.15
      * In Q16: t_nom_q16 = ((cfg->t_nominal * 6554) + 17904742)
      *   where 6554 ≈ 65536/10  and  17904742 ≈ 273.15 × 65536
      */
     int32_t t_nom_q16 = (int32_t)((int32_t)cfg->t_nominal * 6554 + 17904742);
     if (t_nom_q16 <= 0) return INT16_MIN;
 
     /*
      * 1/T = 1/T_nom + (1/Beta) × ln(R/R_nom)
      *
      * Represent as Q16 reciprocals (×65536):
      *   inv_t_nom_q16 = (65536 × 65536) / t_nom_q16
      *   delta_q16     = ln_r_q16 / beta  (Q16/scalar = Q16)
      *   inv_t_q16     = inv_t_nom_q16 + delta_q16
      */
     int32_t inv_t_nom_q16 = (int32_t)(((int64_t)65536 * 65536) / t_nom_q16);
     int32_t delta_q16     = (int32_t)((int64_t)ln_r_q16 / (int32_t)cfg->beta);
     int32_t inv_t_q16     = inv_t_nom_q16 + delta_q16;
 
     if (inv_t_q16 <= 0) return INT16_MIN;
 
     /*
      * T_kelvin_q16 = (65536 × 65536) / inv_t_q16
      * T_celsius × 10 = (T_kelvin_q16 / 65536 - 273.15) × 10
      *                = T_kelvin_q16 × 10 / 65536 - 2731
      *   where 2731 = 273.1 × 10 (rounds to nearest 0.1 °C)
      */
     int32_t t_k_q16    = (int32_t)(((int64_t)65536 * 65536) / inv_t_q16);
     int32_t t_celsius10 = (int32_t)(((int64_t)t_k_q16 * 10) >> 16) - 2731;
 
     if (t_celsius10 < INT16_MIN || t_celsius10 > INT16_MAX) return INT16_MIN;
     return (int16_t)t_celsius10;
 }
 
 int uiox_therm_sensor_init(uiox_therm_sensor_mgr_t *mgr,
                             uiox_therm_if_t *tif)
 {
     if (!mgr || !tif) return -EINVAL;
     memset(mgr, 0, sizeof(*mgr));
     mgr->tif = tif;
     return 0;
 }
 
 int uiox_therm_sensor_register(uiox_therm_sensor_mgr_t *mgr,
                                 const uiox_therm_sensor_t *s)
 {
     if (!mgr || !s) return -EINVAL;
     if (mgr->num_sensors >= UIOX_THERM_MAX_SENSORS) return -ENOSPC;
     memcpy(&mgr->sensors[mgr->num_sensors++], s, sizeof(*s));
     return 0;
 }
 