/**
 * @file    uiox_us_dsp.c
 * @brief   UIOX Ultrasonic DSP implementation.
 * @date    2026-05-26
 */

 #include "uiox_us_dsp.h"
 #include <string.h>
 #include <math.h>
 #include <stdlib.h>
 #include <errno.h>
 
 #ifndef M_PI
 #define M_PI 3.14159265358979323846
 #endif
 
 /* =========================================================================
  * Biquad bandpass coefficient computation (peaking EQ → bandpass form)
  * Using Audio EQ Cookbook bilinear transform.
  * ====================================================================== */
 
 static void compute_biquad_bp(float *b, float *a,
                                float fc, float bw, float fs)
 {
     /* Bandwidth → Q factor */
     float Q  = fc / bw;
     float w0 = 2.0f * (float)M_PI * fc / fs;
     float cw = cosf(w0);
     float sw = sinf(w0);
     float alpha = sw / (2.0f * Q);
 
     float a0 = 1.0f + alpha;
 
     b[0] =  alpha / a0;
     b[1] =  0.0f;
     b[2] = -alpha / a0;
     a[0] =  1.0f;
     a[1] = -2.0f * cw / a0;
     a[2] = (1.0f - alpha) / a0;
 }
 
 /* =========================================================================
  * Single-pole IIR low-pass coefficient
  *   alpha = 1 - exp(-2π × fc / fs)
  * ====================================================================== */
 
 static float compute_lp_alpha(float fc, float fs)
 {
     return 1.0f - expf(-2.0f * (float)M_PI * fc / fs);
 }
 
 /* =========================================================================
  * DSP init
  * ====================================================================== */
 
 int uiox_us_dsp_init(uiox_us_dsp_t           *dsp,
                       const uiox_us_dsp_cfg_t *cfg,
                       uint32_t                 sample_rate_hz)
 {
     if (!dsp || !cfg || sample_rate_hz == 0) return -EINVAL;
     memset(dsp, 0, sizeof(*dsp));
     memcpy(&dsp->cfg, cfg, sizeof(*cfg));
 
     float fs = (float)sample_rate_hz;
 
     /* Bandpass filter coefficients */
     compute_biquad_bp(dsp->bp_b, dsp->bp_a,
                       cfg->bp_center_hz, cfg->bp_bandwidth_hz, fs);
 
     /* Envelope LP alpha */
     dsp->env_alpha = compute_lp_alpha(cfg->env_cutoff_hz, fs);
 
     /* Clamp median window to valid range */
     if (dsp->cfg.median_window < 1) dsp->cfg.median_window = 1;
     if (dsp->cfg.median_window > UIOX_US_MEDIAN_MAX)
         dsp->cfg.median_window = UIOX_US_MEDIAN_MAX;
     /* Ensure odd */
     if ((dsp->cfg.median_window & 1u) == 0)
         dsp->cfg.median_window++;
 
     return 0;
 }
 
 void uiox_us_dsp_reset(uiox_us_dsp_t *dsp)
 {
     if (!dsp) return;
     dsp->bp_x[0] = dsp->bp_x[1] = 0.0f;
     dsp->bp_y[0] = dsp->bp_y[1] = 0.0f;
     dsp->env_state = 0.0f;
 }
 
 /* =========================================================================
  * Biquad filter tick
  * ====================================================================== */
 
 static inline float biquad_tick(uiox_us_dsp_t *dsp, float x_n)
 {
     float y_n = dsp->bp_b[0] * x_n
               + dsp->bp_b[1] * dsp->bp_x[0]
               + dsp->bp_b[2] * dsp->bp_x[1]
               - dsp->bp_a[1] * dsp->bp_y[0]
               - dsp->bp_a[2] * dsp->bp_y[1];
 
     dsp->bp_x[1] = dsp->bp_x[0]; dsp->bp_x[0] = x_n;
     dsp->bp_y[1] = dsp->bp_y[0]; dsp->bp_y[0] = y_n;
     return y_n;
 }
 
 /* =========================================================================
  * Envelope detector tick: |y| → LP filter
  * ====================================================================== */
 
 static inline float envelope_tick(uiox_us_dsp_t *dsp, float y_n)
 {
     float rect = fabsf(y_n);
     dsp->env_state += dsp->env_alpha * (rect - dsp->env_state);
     return dsp->env_state;
 }
 
 /* =========================================================================
  * Median filter (insertion sort on small window)
  * ====================================================================== */
 
 static float median_filter(uiox_us_dsp_t *dsp, float val)
 {
     uint8_t W = dsp->cfg.median_window;
     dsp->median_buf[dsp->median_idx % W] = val;
     dsp->median_idx++;
     if (dsp->median_count < W) dsp->median_count++;
 
     /* Copy window and sort */
     float tmp[UIOX_US_MEDIAN_MAX];
     uint8_t n = dsp->median_count;
     for (uint8_t i = 0; i < n; i++) tmp[i] = dsp->median_buf[i];
     for (uint8_t i = 1; i < n; i++) {
         float key = tmp[i];
         int j = (int)i - 1;
         while (j >= 0 && tmp[j] > key) { tmp[j+1] = tmp[j]; j--; }
         tmp[j+1] = key;
     }
     return tmp[n / 2];
 }
 
 /* =========================================================================
  * Sub-sample peak interpolation (parabolic)
  * Finds sub-sample peak between samples [p-1, p, p+1]
  * ====================================================================== */
 
 static float parabolic_peak(float ym1, float y0, float yp1)
 {
     float denom = 2.0f * (2.0f * y0 - ym1 - yp1);
     if (fabsf(denom) < 1e-9f) return 0.0f;
     return (yp1 - ym1) / denom;
 }
 
 /* =========================================================================
  * Process raw ADC frame
  * ====================================================================== */
 
  int uiox_us_dsp_process_raw(uiox_us_dsp_t          *dsp,
    const uiox_us_frame_t  *raw,
    const uiox_us_sensor_t *sensor,
    uiox_us_result_t       *out)
{
if (!dsp || !raw || !sensor || !out) return -EINVAL;
if (raw->num_samples == 0)           return -EINVAL;

memset(out, 0, sizeof(*out));
out->channel  = raw->channel;
out->meas_id  = raw->meas_id;
out->ts_ns    = raw->ts_ns;
out->sos_mps  = uiox_us_sensor_sos(sensor);
out->temp_celsius = (float)sensor->temp.temp_mc / 1000.0f;

const int16_t *samples = (const int16_t *)raw->vaddr;
uint32_t       N       = raw->num_samples;
uint32_t       blank   = dsp->cfg.blanking_samples;
if (blank >= N) blank = 0;

/* ------------------------------------------------------------------ */
/* Step 1 — DC offset removal (mean subtraction)                      */
/* ------------------------------------------------------------------ */

float dc = 0.0f;
for (uint32_t i = 0; i < N; i++) dc += (float)samples[i];
dc /= (float)N;

/* ------------------------------------------------------------------ */
/* Step 2 + 3 — Bandpass filter + envelope detection                  */
/* ------------------------------------------------------------------ */

uiox_us_dsp_reset(dsp);

float *env = malloc(N * sizeof(float));
if (!env) return -ENOMEM;

float peak_amp  = 0.0f;
uint32_t peak_i = 0;

for (uint32_t i = 0; i < N; i++) {
float x_n    = (float)samples[i] - dc;
float y_bp   = biquad_tick(dsp, x_n);
float y_env  = envelope_tick(dsp, y_bp);
env[i]       = (i < blank) ? 0.0f : y_env;
if (env[i] > peak_amp) { peak_amp = env[i]; peak_i = i; }
}

/* ------------------------------------------------------------------ */
/* Step 4 — Threshold detection → first crossing above threshold      */
/* ------------------------------------------------------------------ */

float threshold = dsp->cfg.threshold_pct * peak_amp;
uint32_t tof_i  = 0;
bool     found  = false;

for (uint32_t i = blank; i < N; i++) {
if (env[i] >= threshold) {
tof_i = i;
found = true;
break;
}
}

if (!found) {
free(env);
out->valid = false;
return 0;
}

/* ------------------------------------------------------------------ */
/* Step 5 — Sub-sample peak interpolation                             */
/* ------------------------------------------------------------------ */

float sub_sample = 0.0f;
if (dsp->cfg.do_interpolate && tof_i > 0 && tof_i < N - 1)
sub_sample = parabolic_peak(env[tof_i - 1],
          env[tof_i],
          env[tof_i + 1]);

free(env);

/* ------------------------------------------------------------------ */
/* Step 6 — ToF → distance                                            */
/* ------------------------------------------------------------------ */

float tof_samples  = (float)tof_i + sub_sample;
float tof_s        = tof_samples / (float)raw->sample_rate_hz;
float tof_us       = tof_s * 1e6f;
float dist_m       = (tof_s * out->sos_mps) / 2.0f;

/* ------------------------------------------------------------------ */
/* Step 7 — Median filter                                             */
/* ------------------------------------------------------------------ */

float dist_filtered = median_filter(dsp, dist_m);

/* ------------------------------------------------------------------ */
/* Step 8 — Range validation                                          */
/* ------------------------------------------------------------------ */

if (dist_filtered < sensor->pulse.min_range_m ||
dist_filtered > sensor->pulse.max_range_m) {
out->valid = false;
return 0;
}

/* ------------------------------------------------------------------ */
/* SNR estimate: peak / noise floor (std dev of blanking region)      */
/* ------------------------------------------------------------------ */

float noise_sum  = 0.0f;
uint32_t n_blank = blank > 0 ? blank : 1u;
for (uint32_t i = 0; i < n_blank; i++)
noise_sum += (float)samples[i] * (float)samples[i];
float noise_rms = sqrtf(noise_sum / (float)n_blank);
if (noise_rms < 1.0f) noise_rms = 1.0f;
out->snr_db = 20.0f * log10f(peak_amp / noise_rms);

out->distance_m    = dist_filtered;
out->tof_us        = tof_us;
out->peak_amplitude = peak_amp;
out->valid         = true;
return 0;
}

/* =========================================================================
* GPIO pulse-width path (no ADC)
* ====================================================================== */

int uiox_us_dsp_process_ticks(uiox_us_dsp_t          *dsp,
      int64_t                 ticks,
      const uiox_us_sensor_t *sensor,
      const uiox_us_hw_t     *hw,
      uiox_us_result_t       *out)
{
if (!dsp || !sensor || !hw || !out) return -EINVAL;
memset(out, 0, sizeof(*out));

if (ticks <= 0) { out->valid = false; return 0; }

out->sos_mps      = uiox_us_sensor_sos(sensor);
out->temp_celsius = (float)sensor->temp.temp_mc / 1000.0f;

float dist_m = uiox_us_sensor_ticks_to_m(sensor, hw, ticks);
if (dist_m < 0.0f) { out->valid = false; return 0; }

float tof_s   = (float)ticks / (float)hw->timer_freq_hz;
out->tof_us   = tof_s * 1e6f;

/* Median filter */
float dist_filtered = median_filter(dsp, dist_m);

if (dist_filtered < sensor->pulse.min_range_m ||
dist_filtered > sensor->pulse.max_range_m) {
out->valid = false;
return 0;
}

out->distance_m     = dist_filtered;
out->peak_amplitude = 1.0f;  /* not available in GPIO mode */
out->snr_db         = 0.0f;  /* not available in GPIO mode */
out->valid          = true;
return 0;
}
 