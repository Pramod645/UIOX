/**
 * @file    uiox_spk_dsp.h
 * @brief   UIOX Speaker DSP: EQ, resampler, limiter, crossover.
 *
 * Processes PCM audio before DMA submission:
 *   - 5-band parametric EQ (biquad IIR per band)
 *   - Soft limiter (prevent clipping)
 *   - Simple linear resampler
 *   - Stereo to mono downmix / mono to stereo upmix
 *   - Volume fade-in / fade-out (pop suppression)
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_SPK_DSP_H
 #define UIOX_SPK_DSP_H
 
 #include "uiox_spk_buf.h"
 #include <stdint.h>
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_SPK_DSP_EQ_BANDS   5
 
 typedef struct {
     float b[3];   /**< Numerator coefficients                             */
     float a[3];   /**< Denominator coefficients                           */
     float x[2];   /**< Input delay line                                   */
     float y[2];   /**< Output delay line                                  */
 } uiox_spk_biquad_t;
 
 typedef struct {
     float     center_hz;   /**< Band centre frequency (Hz)               */
     float     gain_db;     /**< Band gain (−12..+12 dB)                  */
     float     q;           /**< Quality factor (bandwidth)               */
 } uiox_spk_eq_band_t;
 
 typedef struct {
     uiox_spk_eq_band_t   bands[UIOX_SPK_DSP_EQ_BANDS];
     uiox_spk_biquad_t    biquads_l[UIOX_SPK_DSP_EQ_BANDS];
     uiox_spk_biquad_t    biquads_r[UIOX_SPK_DSP_EQ_BANDS];
     float                master_gain;   /**< Linear gain (0..2.0)        */
     float                limiter_thresh;/**< Normalised threshold (0..1)  */
     bool                 eq_enabled;
     bool                 limiter_enabled;
     uint32_t             sample_rate;
     uint8_t              channels;
     /* Fade state */
     float                fade_gain;     /**< Current fade gain            */
     float                fade_step;     /**< Per-sample gain step         */
     bool                 fading;
 } uiox_spk_dsp_t;
 
 int  uiox_spk_dsp_init       (uiox_spk_dsp_t *dsp,
                                uint32_t sample_rate, uint8_t channels);
 void uiox_spk_dsp_set_eq_band(uiox_spk_dsp_t *dsp, uint8_t band,
                                float center_hz, float gain_db, float q);
 void uiox_spk_dsp_rebuild_eq (uiox_spk_dsp_t *dsp);
 void uiox_spk_dsp_set_volume (uiox_spk_dsp_t *dsp, float gain_linear);
 void uiox_spk_dsp_fade       (uiox_spk_dsp_t *dsp,
                                float target_gain, uint32_t ms);
 
 /** Process stereo int16 in-place. @return 0 on success. */
 int  uiox_spk_dsp_process    (uiox_spk_dsp_t *dsp,
                                int16_t *samples, uint32_t n_stereo);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SPK_DSP_H */
 