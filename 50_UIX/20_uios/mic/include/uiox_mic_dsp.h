/**
 * @file    uiox_mic_dsp.h
 * @brief   UIOX Microphone DSP: AGC, VAD, noise cancel, beamform.
 *
 * Processes captured PCM before delivery to application:
 *   - DC offset removal
 *   - Bandpass filter (IIR biquad, 100 Hz – 8 kHz)
 *   - Automatic Gain Control (AGC)
 *   - Voice Activity Detection (VAD) — energy threshold
 *   - Spectral subtraction noise reduction
 *   - High-pass filter (wind / handling noise)
 *
 * @date    2026-06-03
 */

 #ifndef UIOX_MIC_DSP_H
 #define UIOX_MIC_DSP_H
 
 #include "uiox_mic_buf.h"
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_MIC_DSP_AGC_TARGET_DBFS  (-12)   /**< AGC target level       */
 #define UIOX_MIC_DSP_VAD_FRAME_MS     10       /**< VAD decision per 10 ms */
 #define UIOX_MIC_DSP_NOISE_FRAMES     20       /**< Noise estimate frames  */
 
 /* =========================================================================
  * DSP configuration
  * ====================================================================== */
 
 typedef struct {
     bool      agc_enabled;
     bool      vad_enabled;
     bool      noise_cancel;
     bool      highpass;
     bool      bandpass;
     float     agc_attack_ms;    /**< AGC attack time (ms)                 */
     float     agc_release_ms;   /**< AGC release time (ms)                */
     float     vad_threshold_db; /**< VAD energy threshold (dBFS)          */
     float     hp_cutoff_hz;     /**< High-pass cutoff (default 80 Hz)     */
     uint32_t  sample_rate;
 } uiox_mic_dsp_cfg_t;
 
 /* =========================================================================
  * VAD result
  * ====================================================================== */
 
 typedef struct {
     bool     voice_active;
     float    energy_dbfs;
     uint32_t active_frames;
     uint32_t silent_frames;
 } uiox_mic_vad_t;
 
 /* =========================================================================
  * DSP context
  * ====================================================================== */
 
 typedef struct {
     uiox_mic_dsp_cfg_t cfg;
 
     /* AGC state */
     float  agc_gain;          /**< Current linear gain (1.0 = 0 dB)      */
     float  agc_attack_coef;
     float  agc_release_coef;
 
     /* Biquad IIR state (HP filter) */
     float  hp_b[3], hp_a[3];
     float  hp_x[2], hp_y[2];
 
     /* Noise estimate (spectral subtraction) */
     float  noise_floor;
     uint32_t noise_frames_counted;
     bool   noise_estimated;
 
     /* VAD */
     uiox_mic_vad_t vad;
     float  vad_energy_acc;
     uint32_t vad_sample_count;
 
     /* DC offset */
     float  dc_est;
 } uiox_mic_dsp_t;
 
 /* =========================================================================
  * DSP API
  * ====================================================================== */
 
 int  uiox_mic_dsp_init    (uiox_mic_dsp_t *dsp,
                             const uiox_mic_dsp_cfg_t *cfg);
 void uiox_mic_dsp_reset   (uiox_mic_dsp_t *dsp);
 
 /**
  * @brief  Process mono int16 samples in-place.
  *         Chain: DC remove → HP filter → noise cancel → AGC → VAD
  */
 int  uiox_mic_dsp_process (uiox_mic_dsp_t *dsp,
                             int16_t *samples, uint32_t n);
 
 /** Get last VAD result. */
 const uiox_mic_vad_t *uiox_mic_dsp_vad(const uiox_mic_dsp_t *dsp);
 
 /** Get current AGC gain (linear). */
 float uiox_mic_dsp_agc_gain(const uiox_mic_dsp_t *dsp);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MIC_DSP_H */
 