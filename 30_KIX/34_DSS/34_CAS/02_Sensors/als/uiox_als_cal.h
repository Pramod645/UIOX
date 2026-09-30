/**
 * @file  uiox_als_cal.h
 * @brief UIOX ALS calibration — lux formula, CCT, auto-gain.
 * @date  2026-06-11
 */

 #ifndef UIOX_ALS_CAL_H
 #define UIOX_ALS_CAL_H
 
 #include "uiox_als_if.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Calibration coefficients
  * ====================================================================== */
 
 /**
  * struct uiox_als_coeff - Per-IC lux formula coefficients.
  *
  * VEML7700 application note formula (4th-order poly correction):
  *   lux_raw  = count × resolution
  *   lux_corr = 6.0135e-13 × lux_raw^4
  *             − 9.3924e-9  × lux_raw^3
  *             + 8.1488e-5  × lux_raw^2
  *             + 1.0023     × lux_raw
  *
  * For simpler ICs (OPT3001, BH1750) a single linear scale suffices:
  *   lux = count × scale_num / scale_den
  */
 typedef struct {
     /* Linear scale  (lux × 1000 = raw × scale_num / scale_den) */
     uint32_t scale_num;   /**< Numerator   (fixed-point × 1000)           */
     uint32_t scale_den;   /**< Denominator                                 */
     /* Gain correction factors (×1000) for each gain step */
     uint32_t gain_factor[UIOX_ALS_GAIN_MAX];
     /* Integration-time correction (×1000) for each itime step */
     uint32_t itime_factor[UIOX_ALS_ITIME_MAX];
     /* Auto-gain thresholds (raw counts) */
     uint16_t ag_saturate; /**< Counts above which gain must decrease       */
     uint16_t ag_too_dark; /**< Counts below which gain should increase     */
     /* CCT coefficients (Ohno 2014 simplified):
      *   CCT = ct_a × (ch_ir / ch_als)^2 + ct_b × (ch_ir / ch_als) + ct_c */
     int32_t  cct_a;       /**< ×1000                                       */
     int32_t  cct_b;       /**< ×1000                                       */
     int32_t  cct_c;       /**< K (base offset)                             */
 } uiox_als_coeff_t;
 
 /* Built-in coefficient tables */
 extern const uiox_als_coeff_t uiox_als_coeff_veml7700;
 extern const uiox_als_coeff_t uiox_als_coeff_opt3001;
 extern const uiox_als_coeff_t uiox_als_coeff_bh1750;
 extern const uiox_als_coeff_t uiox_als_coeff_tsl2591;
 
 /* =========================================================================
  * Calibration context
  * ====================================================================== */
 
 typedef struct {
     uiox_als_if_t          *aif;
     const uiox_als_coeff_t *coeff;
     /* User-supplied scale trim (default 1000 = 1.000×) */
     uint32_t                trim;
 } uiox_als_cal_t;
 
 /* =========================================================================
  * Calibration API
  * ====================================================================== */
 
 int  uiox_als_cal_init      (uiox_als_cal_t *cal,
                               uiox_als_if_t *aif,
                               const uiox_als_coeff_t *coeff);
 
 /* Convert raw counts → lux × 1000 */
 uint32_t uiox_als_cal_to_lux(const uiox_als_cal_t *cal,
                                uint16_t raw_als,
                                uiox_als_gain_t gain,
                                uiox_als_itime_t itime);
 
 /* Estimate CCT (Kelvin) from als + ir counts */
 uint32_t uiox_als_cal_to_cct(const uiox_als_cal_t *cal,
                                uint16_t raw_als,
                                uint16_t raw_ir);
 
 /* Auto-gain: suggest new gain/itime based on last raw count.
  * Returns true if settings changed. */
 bool uiox_als_cal_auto_gain (uiox_als_cal_t *cal,
                                uint16_t raw_als,
                                uiox_als_gain_t *gain,
                                uiox_als_itime_t *itime);
 
 /* Set user trim (e.g. 1200 = ×1.200 to compensate cover glass) */
 void uiox_als_cal_set_trim  (uiox_als_cal_t *cal, uint32_t trim);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_ALS_CAL_H */
 