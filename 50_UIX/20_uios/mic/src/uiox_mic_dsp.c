/**
 * @file    uiox_mic_dsp.c
 * @brief   UIOX Microphone DSP implementation.
 * @date    2026-06-03
 */

 #include "uiox_mic_dsp.h"
 #include <string.h>
 #include <math.h>
 #include <errno.h>
 
 #ifndef M_PI
 #define M_PI 3.14159265358979323846
 #endif
 
 /* -------------------------------------------------------------------------
  * High-pass biquad coefficient (Audio EQ Cookbook, 1st order HP)
  * ---------------------------------------------------------------------- */
 
 static void build_hp(float *b, float *a, float fc, float fs)
 {
     float w0    = 2.0f * (float)M_PI * fc / fs;
     float cw    = cosf(w0);
     float sw    = sinf(w0);
     float alpha = sw / (2.0f * 0.7071f);   /* Q = 1/√2 Butterworth */
     float a0    = 1.0f + alpha;
     b[0] =  (1.0f + cw) / 2.0f / a0;
     b[1] = -(1.0f + cw)        / a0;
     b[2] =  (1.0f + cw) / 2.0f / a0;
     a[0] =  1.0f;
     a[1] = -2.0f * cw          / a0;
     a[2] = (1.0f - alpha)      / a0;
 }
 
 static inline float biquad(float *b, float *a,
                              float *x, float *y, float xn)
 {
     float yn = b[0]*xn + b[1]*x[0] + b[2]*x[1]
                        - a[1]*y[0] - a[2]*y[1];
     x[1]=x[0]; x[0]=xn;
     y[1]=y[0]; y[0]=yn;
     return yn;
 }
 
 static inline float clampf(float v, float lo, float hi)
 { return v < lo ? lo : v > hi ? hi : v; }
 
 int uiox_mic_dsp_init(uiox_mic_dsp_t *dsp, const uiox_mic_dsp_cfg_t *cfg)
 {
     if (!dsp || !cfg || cfg->sample_rate == 0) return -EINVAL;
     memset(dsp, 0, sizeof(*dsp));
     memcpy(&dsp->cfg, cfg, sizeof(*cfg));
 
     dsp->agc_gain = 1.0f;
     float fs = (float)cfg->sample_rate;
 
     /* AGC time constants → per-sample coefficients */
     dsp->agc_attack_coef  = 1.0f - expf(-1.0f / (cfg->agc_attack_ms  * fs / 1000.0f));
     dsp->agc_release_coef = 1.0f - expf(-1.0f / (cfg->agc_release_ms * fs / 1000.0f));
 
     /* HP filter coefficients */
     float fc = (cfg->hp_cutoff_hz > 0.0f) ? cfg->hp_cutoff_hz : 80.0f;
     build_hp(dsp->hp_b, dsp->hp_a, fc, fs);
 
     dsp->noise_floor = 1.0f;
     dsp->dc_est      = 0.0f;
     return 0;
 }
 
 void uiox_mic_dsp_reset(uiox_mic_dsp_t *dsp)
 {
     if (!dsp) return;
     dsp->hp_x[0] = dsp->hp_x[1] = 0.0f;
     dsp->hp_y[0] = dsp->hp_y[1] = 0.0f;
     dsp->agc_gain = 1.0f;
     dsp->dc_est   = 0.0f;
     dsp->vad_energy_acc   = 0.0f;
     dsp->vad_sample_count = 0;
 }
 
 int uiox_mic_dsp_process(uiox_mic_dsp_t *dsp,
                           int16_t *samples, uint32_t n)
 {
     if (!dsp || !samples || n == 0) return -EINVAL;
 
     float   energy_sum = 0.0f;
     float   target_lin = powf(10.0f,
                               (float)UIOX_MIC_DSP_AGC_TARGET_DBFS / 20.0f);
 
     for (uint32_t i = 0; i < n; i++) {
         float s = (float)samples[i] / 32768.0f;
 
         /* 1. DC removal (leaky integrator) */
         dsp->dc_est += 0.001f * (s - dsp->dc_est);
         s -= dsp->dc_est;
 
         /* 2. High-pass filter */
         if (dsp->cfg.highpass)
             s = biquad(dsp->hp_b, dsp->hp_a, dsp->hp_x, dsp->hp_y, s);
 
         /* 3. Spectral subtraction noise gate (time-domain approximation) */
         if (dsp->cfg.noise_cancel) {
             if (!dsp->noise_estimated) {
                 dsp->noise_floor += (fabsf(s) - dsp->noise_floor) * 0.01f;
                 dsp->noise_frames_counted++;
                 if (dsp->noise_frames_counted >
                     (uint32_t)(UIOX_MIC_DSP_NOISE_FRAMES *
                                dsp->cfg.sample_rate / 1000u))
                     dsp->noise_estimated = true;
             } else {
                 float thresh = dsp->noise_floor * 1.5f;
                 if (fabsf(s) < thresh) s *= 0.1f;
                 else s -= (s > 0) ? thresh : -thresh;
             }
         }
 
         /* 4. AGC */
         if (dsp->cfg.agc_enabled) {
             float env = fabsf(s) * dsp->agc_gain;
             if (env > target_lin)
                 dsp->agc_gain -= dsp->agc_attack_coef  * dsp->agc_gain * 0.1f;
             else
                 dsp->agc_gain += dsp->agc_release_coef * 0.01f;
             dsp->agc_gain = clampf(dsp->agc_gain, 0.01f, 32.0f);
             s *= dsp->agc_gain;
         }
 
         /* 5. Accumulate energy for VAD */
         energy_sum += s * s;
 
         /* 6. Clamp and convert back */
         s = clampf(s, -1.0f, 1.0f);
         samples[i] = (int16_t)(s * 32767.0f);
     }
 
     /* VAD decision */
     if (dsp->cfg.vad_enabled) {
         float rms  = sqrtf(energy_sum / (float)n);
         float dbfs = (rms > 1e-9f) ? 20.0f * log10f(rms) : -120.0f;
         dsp->vad.energy_dbfs  = dbfs;
         dsp->vad.voice_active = (dbfs > dsp->cfg.vad_threshold_db);
         if (dsp->vad.voice_active) dsp->vad.active_frames++;
         else                        dsp->vad.silent_frames++;
     }
 
     return 0;
 }
 
 const uiox_mic_vad_t *uiox_mic_dsp_vad(const uiox_mic_dsp_t *dsp)
 { return dsp ? &dsp->vad : NULL; }
 
 float uiox_mic_dsp_agc_gain(const uiox_mic_dsp_t *dsp)
 { return dsp ? dsp->agc_gain : 1.0f; }
 