/**
 * @file    uiox_therm_sensor.c
 * @brief   UIOX Thermal Sensor abstraction implementation.
 * @date    2026-06-05
 */

#include "uiox_therm_sensor.h"

/* -------------------------------------------------------------------------
 * Freestanding natural log approximation (no <math.h>).
 *
 * Uses the identity: ln(x) = 2 * atanh((x-1)/(x+1))
 * with a 4-term Taylor series for atanh(u) = u + u³/3 + u⁵/5 + u⁷/7.
 * Accurate to < 0.5 % for x in [0.1, 10] — sufficient for NTC Beta eq.
 * ---------------------------------------------------------------------- */
static float uiox_ln_approx(float x)
{
    if (x <= 0.0f) return -1e30f;   /* guard: return large negative */

    /* Reduce x into [0.5, 2) using ln(x) = n*ln(2) + ln(x/2^n) */
    int   n  = 0;
    float xr = x;
    while (xr >= 2.0f) { xr *= 0.5f;  n++; }
    while (xr <  0.5f) { xr *= 2.0f;  n--; }

    /* atanh series on u = (xr-1)/(xr+1), |u| < 0.333 after reduction */
    float u  = (xr - 1.0f) / (xr + 1.0f);
    float u2 = u * u;
    float s  = u * (1.0f + u2 * (1.0f/3.0f +
                    u2 * (1.0f/5.0f +
                    u2 *  1.0f/7.0f)));

    /* ln(2) ≈ 0.693147 */
    return 2.0f * s + (float)n * 0.693147f;
}

/* NTC: Simplified Beta equation */
int16_t uiox_therm_ntc_convert(uint16_t raw,
                                 const uiox_therm_ntc_cfg_t *cfg)
{
    if (!cfg || raw == 0) return INT16_MIN;
    uint32_t adc_max = (1u << cfg->adc_bits) - 1u;
    if (raw >= adc_max) return INT16_MIN;

    /* Voltage divider: R_ntc = R_series × raw / (adc_max - raw) */
    float r_ntc = cfg->r_series * (float)raw / (float)(adc_max - raw);
    if (r_ntc <= 0.0f) return INT16_MIN;

    /* Beta equation: 1/T = 1/T_nom + (1/Beta) × ln(R/R_nom) */
    float t_nom_k = cfg->t_nominal + 273.15f;
    float inv_t   = (1.0f / t_nom_k) +
                    (1.0f / cfg->beta) * uiox_ln_approx(r_ntc / cfg->r_nominal);
    if (inv_t <= 0.0f) return INT16_MIN;

    float t_celsius = (1.0f / inv_t) - 273.15f;
    return (int16_t)(t_celsius * 10.0f);
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
