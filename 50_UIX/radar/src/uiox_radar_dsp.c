/**
 * @file    uiox_radar_dsp.c
 * @brief   UIOX Radar DSP chain: range FFT, Doppler FFT, CFAR, angle.
 * @date    2026-05-26
 */

 #include "uiox_radar_dsp.h"
 #include <string.h>
 #include <stdlib.h>
 #include <math.h>
 #include <errno.h>
 
 #ifndef M_PI
 #define M_PI 3.14159265358979323846
 #endif
 
 /* =========================================================================
  * Window coefficient generation
  * ====================================================================== */
 
 static void gen_window(float *coeff, uint16_t N, uiox_radar_window_t type)
 {
     for (uint16_t n = 0; n < N; n++) {
         float x = (float)n / (float)(N - 1);
         switch (type) {
         case UIOX_RADAR_WIN_HANN:
             coeff[n] = 0.5f * (1.0f - cosf(2.0f * (float)M_PI * x));
             break;
         case UIOX_RADAR_WIN_HAMMING:
             coeff[n] = 0.54f - 0.46f * cosf(2.0f * (float)M_PI * x);
             break;
         case UIOX_RADAR_WIN_BLACKMAN:
             coeff[n] = 0.42f
                      - 0.50f * cosf(2.0f * (float)M_PI * x)
                      + 0.08f * cosf(4.0f * (float)M_PI * x);
             break;
         case UIOX_RADAR_WIN_NONE:
         default:
             coeff[n] = 1.0f;
             break;
         }
     }
 }
 
 /* =========================================================================
  * Twiddle factor generation  W_N^k = e^{-j2πk/N}
  * ====================================================================== */
 
 static void gen_twiddle(uiox_cplx32f_t *tw, uint16_t N)
 {
     for (uint16_t k = 0; k < N; k++) {
         float angle = -2.0f * (float)M_PI * (float)k / (float)N;
         tw[k].i = cosf(angle);
         tw[k].q = sinf(angle);
     }
 }
 
 /* =========================================================================
  * In-place Cooley-Tukey Radix-2 DIT FFT (float complex)
  * N must be a power of 2.
  * ====================================================================== */
 
 static void fft_r2dit(uiox_cplx32f_t *x, uint16_t N,
                       const uiox_cplx32f_t *twiddle)
 {
     /* Bit-reversal permutation */
     uint16_t j = 0;
     for (uint16_t i = 1; i < N; i++) {
         uint16_t bit = N >> 1;
         for (; j & bit; bit >>= 1) j ^= bit;
         j ^= bit;
         if (i < j) {
             uiox_cplx32f_t tmp = x[i];
             x[i] = x[j];
             x[j] = tmp;
         }
     }
 
     /* Butterfly stages */
     for (uint16_t len = 2; len <= N; len <<= 1) {
         uint16_t half  = len >> 1;
         uint16_t step  = N / len;
         for (uint16_t i = 0; i < N; i += len) {
             for (uint16_t k = 0; k < half; k++) {
                 const uiox_cplx32f_t *w = &twiddle[k * step];
                 uiox_cplx32f_t *u = &x[i + k];
                 uiox_cplx32f_t *v = &x[i + k + half];
 
                 /* v' = W × v */
                 float vri = w->i * v->i - w->q * v->q;
                 float vrq = w->i * v->q + w->q * v->i;
 
                 v->i = u->i - vri;
                 v->q = u->q - vrq;
                 u->i = u->i + vri;
                 u->q = u->q + vrq;
             }
         }
     }
 }
 
 /* =========================================================================
  * Magnitude squared  (avoids sqrt for CFAR thresholding)
  * ====================================================================== */
 
 static inline float mag2(const uiox_cplx32f_t *c)
 {
     return c->i * c->i + c->q * c->q;
 }
 
 /* =========================================================================
  * DSP init / deinit
  * ====================================================================== */
 
 int uiox_radar_dsp_init(uiox_radar_dsp_t *dsp,
                          const uiox_radar_dsp_cfg_t *cfg)
 {
     if (!dsp || !cfg) return -EINVAL;
     memset(dsp, 0, sizeof(*dsp));
     memcpy(&dsp->cfg, cfg, sizeof(*cfg));
 
     /* Twiddle tables */
     dsp->range_twiddle = malloc(cfg->range_fft_size *
                                 sizeof(uiox_cplx32f_t));
     dsp->doppler_twiddle = malloc(cfg->doppler_fft_size *
                                   sizeof(uiox_cplx32f_t));
     dsp->angle_twiddle = malloc(cfg->angle_fft_size *
                                 sizeof(uiox_cplx32f_t));
 
     /* Window tables */
     dsp->range_win_coeff   = malloc(cfg->range_fft_size   * sizeof(float));
     dsp->doppler_win_coeff = malloc(cfg->doppler_fft_size * sizeof(float));
 
     if (!dsp->range_twiddle   || !dsp->doppler_twiddle ||
         !dsp->angle_twiddle   || !dsp->range_win_coeff ||
         !dsp->doppler_win_coeff) {
         uiox_radar_dsp_deinit(dsp);
         return -ENOMEM;
     }
 
     gen_twiddle(dsp->range_twiddle,   cfg->range_fft_size);
     gen_twiddle(dsp->doppler_twiddle, cfg->doppler_fft_size);
     gen_twiddle(dsp->angle_twiddle,   cfg->angle_fft_size);
 
     gen_window(dsp->range_win_coeff,   cfg->range_fft_size,
                cfg->range_win);
     gen_window(dsp->doppler_win_coeff, cfg->doppler_fft_size,
                cfg->doppler_win);
 
     return 0;
 }
 
 void uiox_radar_dsp_deinit(uiox_radar_dsp_t *dsp)
 {
     if (!dsp) return;
     free(dsp->range_twiddle);
     free(dsp->doppler_twiddle);
     free(dsp->angle_twiddle);
     free(dsp->range_win_coeff);
     free(dsp->doppler_win_coeff);
     memset(dsp, 0, sizeof(*dsp));
 }
 
 /* =========================================================================
  * Stage 1 — Range FFT
  * Input : raw->vaddr   layout [num_rx][num_chirps][num_samples] uiox_cplx16_t
  * Output: range_cube   layout [num_rx][num_chirps][range_fft_size] uiox_cplx32f_t
  * ====================================================================== */
 
 int uiox_radar_dsp_range_fft(uiox_radar_dsp_t *dsp,
                               const uiox_radar_frame_t *raw,
                               uiox_cplx32f_t *range_cube)
 {
     if (!dsp || !raw || !range_cube) return -EINVAL;
 
     const uiox_radar_dsp_cfg_t *cfg = &dsp->cfg;
     const uiox_cplx16_t *adc =
         (const uiox_cplx16_t *)raw->vaddr;
 
     uint16_t N  = cfg->range_fft_size;
     uint16_t S  = raw->num_samples;
     uint16_t C  = raw->num_chirps;
     uint16_t R  = raw->num_rx;
 
     for (uint16_t rx = 0; rx < R; rx++) {
         for (uint16_t chirp = 0; chirp < C; chirp++) {
 
             /* Pointer into output range_cube row */
             uiox_cplx32f_t *row =
                 &range_cube[(rx * C + chirp) * N];
 
             /* Zero-pad and convert ADC samples to float */
             memset(row, 0, N * sizeof(uiox_cplx32f_t));
             const uiox_cplx16_t *src =
                 &adc[(rx * C + chirp) * S];
 
             uint16_t copy = (S < N) ? S : N;
             for (uint16_t n = 0; n < copy; n++) {
                 /* Apply range window + int16 → float conversion */
                 float win = dsp->range_win_coeff[n];
                 row[n].i  = (float)src[n].i * win;
                 row[n].q  = (float)src[n].q * win;
             }
 
             /* In-place FFT */
             fft_r2dit(row, N, dsp->range_twiddle);
         }
     }
     return 0;
 }
 
 /* =========================================================================
  * Stage 2 — Doppler FFT
  * Input : range_cube  [num_rx][num_chirps][range_fft_size]
  * Output: rdmap       [num_rx][doppler_fft_size][range_fft_size]  (power)
  * ====================================================================== */
 
 int uiox_radar_dsp_doppler_fft(uiox_radar_dsp_t *dsp,
                                 uiox_cplx32f_t   *range_cube,
                                 uint16_t          num_rx,
                                 uint16_t          num_chirps,
                                 uint16_t          num_range_bins,
                                 float            *rdmap)
 {
     if (!dsp || !range_cube || !rdmap) return -EINVAL;
 
     const uiox_radar_dsp_cfg_t *cfg = &dsp->cfg;
     uint16_t D = cfg->doppler_fft_size;
 
     /* Temporary chirp column buffer */
     uiox_cplx32f_t *col = malloc(D * sizeof(uiox_cplx32f_t));
     if (!col) return -ENOMEM;
 
     for (uint16_t rx = 0; rx < num_rx; rx++) {
         for (uint16_t rb = 0; rb < num_range_bins; rb++) {
 
             /* Extract chirp dimension for this (rx, range_bin) */
             memset(col, 0, D * sizeof(uiox_cplx32f_t));
             uint16_t copy = (num_chirps < D) ? num_chirps : D;
 
             for (uint16_t c = 0; c < copy; c++) {
                 float win = dsp->doppler_win_coeff[c];
                 uiox_cplx32f_t s =
                     range_cube[(rx * num_chirps + c) * num_range_bins + rb];
                 col[c].i = s.i * win;
                 col[c].q = s.q * win;
             }
 
             /* Doppler FFT */
             fft_r2dit(col, D, dsp->doppler_twiddle);
 
             /* Store power (mag²) into rdmap */
             for (uint16_t d = 0; d < D; d++) {
                 rdmap[(rx * D + d) * num_range_bins + rb] = mag2(&col[d]);
             }
         }
     }
 
     free(col);
     return 0;
 }
 
 /* =========================================================================
  * Stage 3 — CA-CFAR (2D) detection
  * Input : rdmap  [num_rx][doppler_bins][range_bins]  (power, linear)
  * Output: fills det_frame->detections[]
  * ====================================================================== */
 
 int uiox_radar_dsp_cfar(uiox_radar_dsp_t     *dsp,
                          const float          *rdmap,
                          uint16_t              num_range_bins,
                          uint16_t              num_doppler_bins,
                          uiox_radar_det_frame_t *out)
 {
     if (!dsp || !rdmap || !out) return -EINVAL;
 
     const uiox_radar_dsp_cfg_t *cfg = &dsp->cfg;
     uint8_t  G  = cfg->cfar_guard_cells;
     uint8_t  T  = cfg->cfar_train_cells;
     float    th = powf(10.0f, cfg->cfar_threshold_db / 10.0f);
 
     out->num_detections = 0;
 
     for (uint16_t d = 0; d < num_doppler_bins; d++) {
         for (uint16_t r = 0; r < num_range_bins; r++) {
 
             float cell_power = rdmap[d * num_range_bins + r];
 
             /* Accumulate training cells (range dimension CA-CFAR) */
             float noise_sum = 0.0f;
             int   count     = 0;
 
             for (int tr = -(T + G); tr <= (T + G); tr++) {
                 if (tr == 0) continue;
                 if (abs(tr) <= G) continue;  /* skip guard cells */
                 int idx = (int)r + tr;
                 if (idx < 0 || idx >= num_range_bins) continue;
                 noise_sum += rdmap[d * num_range_bins + idx];
                 count++;
             }
 
             if (count == 0) continue;
             float noise_est = noise_sum / (float)count;
 
             /* Detection test */
             if (cell_power > th * noise_est) {
                 if (out->num_detections >= UIOX_RADAR_MAX_DETECTIONS)
                     goto done;
 
                 uiox_radar_detection_t *det =
                     &out->detections[out->num_detections++];
                 det->range_bin   = r;
                 det->doppler_bin = d;
                 /* Range/velocity resolved in subsystem with sensor perf */
                 det->range_m         = 0.0f; /* filled by subsystem */
                 det->velocity_mps    = 0.0f;
                 det->azimuth_deg     = 0.0f;
                 det->elevation_deg   = 0.0f;
                 det->snr_db = 10.0f * log10f(cell_power / noise_est);
                 det->rcs_dbsm = det->snr_db; /* placeholder */
             }
         }
     }
 done:
     return 0;
 }
 
 /* =========================================================================
  * Stage 4 — Angle estimation (FFT-based beamforming across RX array)
  * ====================================================================== */
 
 int uiox_radar_dsp_angle(uiox_radar_dsp_t       *dsp,
                           const uiox_cplx32f_t   *range_cube,
                           uiox_radar_det_frame_t  *dets,
                           uint16_t                 num_rx,
                           uint16_t                 num_range_bins)
 {
     if (!dsp || !range_cube || !dets) return -EINVAL;
 
     const uiox_radar_dsp_cfg_t *cfg = &dsp->cfg;
     uint16_t A = cfg->angle_fft_size;
 
     uiox_cplx32f_t *avec = malloc(A * sizeof(uiox_cplx32f_t));
     if (!avec) return -ENOMEM;
 
     for (uint16_t di = 0; di < dets->num_detections; di++) {
         uiox_radar_detection_t *det = &dets->detections[di];
         uint16_t rb = det->range_bin;
 
         /* Build steering vector from RX samples at this range bin
          * (chirp 0, all RX channels) */
         memset(avec, 0, A * sizeof(uiox_cplx32f_t));
         for (uint16_t rx = 0; rx < num_rx && rx < A; rx++) {
             /* range_cube layout: [rx][chirp][range_bin] */
             avec[rx] = range_cube[rx * num_range_bins + rb];
         }
 
         /* Angle FFT */
         fft_r2dit(avec, A, dsp->angle_twiddle);
 
         /* Find peak bin */
         float peak = 0.0f;
         uint16_t peak_bin = 0;
         for (uint16_t a = 0; a < A; a++) {
             float m = mag2(&avec[a]);
             if (m > peak) { peak = m; peak_bin = a; }
         }
 
         /* Map bin to angle: sin(θ) = (bin - A/2) / (A/2) */
         float sin_theta = ((float)peak_bin - (float)(A / 2)) /
                            (float)(A / 2);
         /* Clamp to [-1, 1] */
         if (sin_theta >  1.0f) sin_theta =  1.0f;
         if (sin_theta < -1.0f) sin_theta = -1.0f;
         det->azimuth_deg = (180.0f / (float)M_PI) * asinf(sin_theta);
         det->elevation_deg = 0.0f; /* single-row array: no elevation */
     }
 
     free(avec);
     return 0;
 }
 
 /* =========================================================================
  * Full pipeline
  * ====================================================================== */
 
 int uiox_radar_dsp_process(uiox_radar_dsp_t         *dsp,
                             const uiox_radar_frame_t *raw,
                             uiox_radar_det_frame_t   *out)
 {
     if (!dsp || !raw || !out) return -EINVAL;
 
     const uiox_radar_dsp_cfg_t *cfg = &dsp->cfg;
     uint16_t R  = raw->num_rx;
     uint16_t C  = raw->num_chirps;
     uint16_t NR = cfg->range_fft_size;
     uint16_t ND = cfg->doppler_fft_size;
 
     /* Allocate temporary processing buffers */
     size_t range_cube_sz = (size_t)R * C * NR * sizeof(uiox_cplx32f_t);
     size_t rdmap_sz      = (size_t)R * ND * NR * sizeof(float);
 
     uiox_cplx32f_t *range_cube = malloc(range_cube_sz);
     float          *rdmap       = malloc(rdmap_sz);
 
     if (!range_cube || !rdmap) {
         free(range_cube);
         free(rdmap);
         return -ENOMEM;
     }
 
     int rc;
 
     /* Stage 1: Range FFT */
     rc = uiox_radar_dsp_range_fft(dsp, raw, range_cube);
     if (rc < 0) goto done;
 
     /* Stage 2: Doppler FFT */
     rc = uiox_radar_dsp_doppler_fft(dsp, range_cube, R, C, NR, rdmap);
     if (rc < 0) goto done;
 
     /* Stage 3: CFAR (on RX0 Doppler map for simplicity) */
     out->num_detections = 0;
     out->frame_id       = raw->frame_id;
     out->ts_ns          = raw->ts_ns;
     rc = uiox_radar_dsp_cfar(dsp, rdmap, NR, ND, out);
     if (rc < 0) goto done;
 
     /* Stage 4: Angle estimation */
     if (cfg->do_angle_est && out->num_detections > 0)
         rc = uiox_radar_dsp_angle(dsp, range_cube, out, R, NR);
 
 done:
     free(range_cube);
     free(rdmap);
     return rc;
 }
 