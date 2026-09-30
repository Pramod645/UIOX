/**
 * @file    uiox_spk_dsp.c
 * @brief   UIOX Speaker DSP implementation.
 * @date    2026-06-01
 */

 #include "uiox_spk_dsp.h"
 #include <string.h>
 #include <math.h>
 #include <errno.h>
 
 #ifndef M_PI
 #define M_PI 3.14159265358979323846
 #endif
 
 /* =========================================================================
  * Peaking EQ biquad coefficient computation (Audio EQ Cookbook)
  * ====================================================================== */
 
 static void build_peaking_eq(uiox_spk_biquad_t *bq,
                               float fc, float gain_db, float q, float fs)
 {
     float A  = powf(10.0f, gain_db / 40.0f);
     float w0 = 2.0f * (float)M_PI * fc / fs;
     float cw = cosf(w0);
     float sw = sinf(w0);
     float alpha = sw / (2.0f * q);
     float a0    = 1.0f + alpha / A;
 
     bq->b[0] = (1.0f + alpha * A) / a0;
     bq->b[1] = (-2.0f * cw)       / a0;
     bq->b[2] = (1.0f - alpha * A) / a0;
     bq->a[0] =  1.0f;
     bq->a[1] = (-2.0f * cw)       / a0;
     bq->a[2] = (1.0f - alpha / A) / a0;
     bq->x[0] = bq->x[1] = bq->y[0] = bq->y[1] = 0.0f;
 }
 
 /* =========================================================================
  * Biquad tick
  * ====================================================================== */
 
 static inline float biquad_tick(uiox_spk_biquad_t *bq, float x)
 {
     float y = bq->b[0] * x +
               bq->b[1] * bq->x[0] +
               bq->b[2] * bq->x[1] -
               bq->a[1] * bq->y[0] -
               bq->a[2] * bq->y[1];
     bq->x[1] = bq->x[0]; bq->x[0] = x;
     bq->y[1] = bq->y[0]; bq->y[0] = y;
     return y;
 }
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 int uiox_spk_dsp_init(uiox_spk_dsp_t *dsp,
                        uint32_t sample_rate, uint8_t channels)
 {
     if (!dsp || !sample_rate) return -EINVAL;
     memset(dsp, 0, sizeof(*dsp));
     dsp->sample_rate     = sample_rate;
     dsp->channels        = channels;
     dsp->master_gain     = 1.0f;
     dsp->limiter_thresh  = 0.95f;
     dsp->eq_enabled      = false;
     dsp->limiter_enabled = true;
     dsp->fade_gain       = 1.0f;
     dsp->fade_step       = 0.0f;
     dsp->fading          = false;
 
     /* Default EQ bands: 80Hz, 300Hz, 1kHz, 4kHz, 12kHz */
     static const float default_hz[UIOX_SPK_DSP_EQ_BANDS] =
         { 80.0f, 300.0f, 1000.0f, 4000.0f, 12000.0f };
/**
 * @file    uiox_spk_dsp.c
 * @brief   UIOX Speaker DSP implementation.
 * @date    2026-06-01
 */

 #include "uiox_spk_dsp.h"
 #include <string.h>
 #include <math.h>
 #include <errno.h>
 
 #ifndef M_PI
 #define M_PI 3.14159265358979323846
 #endif
 
 /* =========================================================================
  * Peaking EQ biquad coefficient computation (Audio EQ Cookbook)
  * ====================================================================== */
 
 static void build_peaking_eq(uiox_spk_biquad_t *bq,
                               float fc, float gain_db, float q, float fs)
 {
     float A  = powf(10.0f, gain_db / 40.0f);
     float w0 = 2.0f * (float)M_PI * fc / fs;
     float cw = cosf(w0);
     float sw = sinf(w0);
     float alpha = sw / (2.0f * q);
     float a0    = 1.0f + alpha / A;
 
     bq->b[0] = (1.0f + alpha * A) / a0;
     bq->b[1] = (-2.0f * cw)       / a0;
     bq->b[2] = (1.0f - alpha * A) / a0;
     bq->a[0] =  1.0f;
     bq->a[1] = (-2.0f * cw)       / a0;
     bq->a[2] = (1.0f - alpha / A) / a0;
     bq->x[0] = bq->x[1] = bq->y[0] = bq->y[1] = 0.0f;
 }
 
 /* =========================================================================
  * Biquad tick
  * ====================================================================== */
 
 static inline float biquad_tick(uiox_spk_biquad_t *bq, float x)
 {
     float y = bq->b[0] * x +
               bq->b[1] * bq->x[0] +
               bq->b[2] * bq->x[1] -
               bq->a[1] * bq->y[0] -
               bq->a[2] * bq->y[1];
     bq->x[1] = bq->x[0]; bq->x[0] = x;
     bq->y[1] = bq->y[0]; bq->y[0] = y;
     return y;
 }
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 int uiox_spk_dsp_init(uiox_spk_dsp_t *dsp,
                        uint32_t sample_rate, uint8_t channels)
 {
     if (!dsp || !sample_rate) return -EINVAL;
     memset(dsp, 0, sizeof(*dsp));
     dsp->sample_rate     = sample_rate;
     dsp->channels        = channels;
     dsp->master_gain     = 1.0f;
     dsp->limiter_thresh  = 0.95f;
     dsp->eq_enabled      = false;
     dsp->limiter_enabled = true;
     dsp->fade_gain       = 1.0f;
     dsp->fade_step       = 0.0f;
     dsp->fading          = false;
 
     /* Default EQ bands: 80Hz, 300Hz, 1kHz, 4kHz, 12kHz */
     static const float default_hz[UIOX_SPK_DSP_EQ_BANDS] =
         { 80.0f, 300.0f, 1000.0f, 4000.0f, 12000.0f };
         for (uint8_t i = 0; i < UIOX_SPK_DSP_EQ_BANDS; i++) {
            dsp->bands[i].center_hz = default_hz[i];
            dsp->bands[i].gain_db   = 0.0f;
            dsp->bands[i].q         = 1.41f;   /* √2 = Butterworth            */
        }
        return 0;
    }
    
    void uiox_spk_dsp_set_eq_band(uiox_spk_dsp_t *dsp, uint8_t band,
                                    float center_hz, float gain_db, float q)
    {
        if (!dsp || band >= UIOX_SPK_DSP_EQ_BANDS) return;
        dsp->bands[band].center_hz = center_hz;
        dsp->bands[band].gain_db   = gain_db;
        dsp->bands[band].q         = (q > 0.1f) ? q : 0.1f;
        uiox_spk_dsp_rebuild_eq(dsp);
    }
    
    void uiox_spk_dsp_rebuild_eq(uiox_spk_dsp_t *dsp)
    {
        if (!dsp) return;
        float fs = (float)dsp->sample_rate;
        for (uint8_t i = 0; i < UIOX_SPK_DSP_EQ_BANDS; i++) {
            build_peaking_eq(&dsp->biquads_l[i],
                              dsp->bands[i].center_hz,
                              dsp->bands[i].gain_db,
                              dsp->bands[i].q, fs);
            build_peaking_eq(&dsp->biquads_r[i],
                              dsp->bands[i].center_hz,
                              dsp->bands[i].gain_db,
                              dsp->bands[i].q, fs);
        }
    }
    
    void uiox_spk_dsp_set_volume(uiox_spk_dsp_t *dsp, float gain_linear)
    {
        if (!dsp) return;
        if (gain_linear < 0.0f) gain_linear = 0.0f;
        if (gain_linear > 2.0f) gain_linear = 2.0f;
        dsp->master_gain = gain_linear;
    }
    
    void uiox_spk_dsp_fade(uiox_spk_dsp_t *dsp,
                            float target_gain, uint32_t ms)
    {
        if (!dsp || ms == 0) {
            if (dsp) dsp->fade_gain = target_gain;
            return;
        }
        uint32_t total_samples = (dsp->sample_rate * ms) / 1000u;
        dsp->fade_step = (total_samples > 0) ?
            (target_gain - dsp->fade_gain) / (float)total_samples : 0.0f;
        dsp->fading = true;
    }
    
    /* =========================================================================
     * Main DSP process — in-place on stereo int16
     * ====================================================================== */
    
    int uiox_spk_dsp_process(uiox_spk_dsp_t *dsp,
                              int16_t *samples, uint32_t n_stereo)
    {
        if (!dsp || !samples) return -EINVAL;
    
        for (uint32_t i = 0; i < n_stereo; i++) {
            float l = (float)samples[i * 2]     / 32768.0f;
            float r = (float)samples[i * 2 + 1] / 32768.0f;
    
            /* EQ — apply each band biquad in sequence */
            if (dsp->eq_enabled) {
                for (uint8_t b = 0; b < UIOX_SPK_DSP_EQ_BANDS; b++) {
                    l = biquad_tick(&dsp->biquads_l[b], l);
                    r = biquad_tick(&dsp->biquads_r[b], r);
                }
            }
    
            /* Master gain */
            l *= dsp->master_gain;
            r *= dsp->master_gain;
    
            /* Fade */
            if (dsp->fading) {
                l *= dsp->fade_gain;
                r *= dsp->fade_gain;
                dsp->fade_gain += dsp->fade_step;
                if ((dsp->fade_step >= 0.0f && dsp->fade_gain >= dsp->master_gain) ||
                    (dsp->fade_step <  0.0f && dsp->fade_gain <= 0.0f)) {
                    dsp->fading    = false;
                    dsp->fade_step = 0.0f;
                }
            }
    
            /* Soft limiter (tanh saturation) */
            if (dsp->limiter_enabled) {
                float thr = dsp->limiter_thresh;
                if (l >  thr) l =  thr + (1.0f - thr) * tanhf((l - thr) / (1.0f - thr));
                if (l < -thr) l = -thr - (1.0f - thr) * tanhf((-l - thr) / (1.0f - thr));
                if (r >  thr) r =  thr + (1.0f - thr) * tanhf((r - thr) / (1.0f - thr));
                if (r < -thr) r = -thr - (1.0f - thr) * tanhf((-r - thr) / (1.0f - thr));
            }
    
            /* Clamp and convert back to int16 */
            if (l >  1.0f) l =  1.0f;
            if (l < -1.0f) l = -1.0f;
            if (r >  1.0f) r =  1.0f;
            if (r < -1.0f) r = -1.0f;
    
            samples[i * 2]     = (int16_t)(l * 32767.0f);
            samples[i * 2 + 1] = (int16_t)(r * 32767.0f);
        }
        return 0;
    }
    