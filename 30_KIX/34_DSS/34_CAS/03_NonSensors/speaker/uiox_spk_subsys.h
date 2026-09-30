/**
 * @file    uiox_spk_subsys.h
 * @brief   UIOX Speaker subsystem — playback, mixing, EQ, volume.
 *
 * Full pipeline:
 *   write PCM → DSP → ring buffer → DMA refill → speaker
 *
 * Features:
 *   - Multi-source software mixer (up to 4 concurrent audio streams)
 *   - Master volume + per-stream volume
 *   - Hardware + software EQ
 *   - Pop-free mute/unmute (fade in/out)
 *   - Playback state machine (idle/playing/paused/stopping)
 *   - Event callbacks (underrun, start, stop, volume change)
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_SPK_SUBSYS_H
 #define UIOX_SPK_SUBSYS_H
 
 #include "uiox_spk_if.h"
 #include "uiox_spk_codec.h"
 #include "uiox_spk_dsp.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Mixer streams
  * ====================================================================== */
 
 #define UIOX_SPK_MAX_STREAMS    4
 
 typedef struct {
     const int16_t *data;       /**< Pointer to stereo int16 source data   */
     uint32_t       total;      /**< Total stereo sample pairs             */
     uint32_t       pos;        /**< Current read position                 */
     float          gain;       /**< Per-stream gain (0..1.0)              */
     bool           loop;       /**< Loop when end reached                 */
     bool           active;
     uint8_t        id;
 } uiox_spk_stream_t;
 
 /* =========================================================================
  * Subsystem events
  * ====================================================================== */
 
 typedef enum {
     UIOX_SPK_EVT_START = 0,
     UIOX_SPK_EVT_STOP,
     UIOX_SPK_EVT_UNDERRUN,
     UIOX_SPK_EVT_STREAM_END,
     UIOX_SPK_EVT_VOLUME_CHANGE,
     UIOX_SPK_EVT_FAULT,
 } uiox_spk_evt_t;
 
 typedef void (*uiox_spk_evt_cb_t)(uiox_spk_evt_t evt,
                                    uint8_t stream_id, void *ctx);
 
 /* =========================================================================
  * Playback state
  * ====================================================================== */
 
 typedef enum {
     UIOX_SPK_STATE_STOPPED = 0,
     UIOX_SPK_STATE_PLAYING,
     UIOX_SPK_STATE_PAUSED,
     UIOX_SPK_STATE_STOPPING,
 } uiox_spk_state_t;
 
 /* =========================================================================
  * Subsystem descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_spk_if_t          sif;
     uiox_spk_codec_t       codec;
     uiox_spk_dsp_t         dsp;
     uiox_spk_state_t       state;
     uiox_spk_stream_t      streams[UIOX_SPK_MAX_STREAMS];
     uint8_t                stream_count;
     uint8_t                master_vol;  /**< 0..100 %                     */
     uiox_spk_evt_cb_t      evt_cb;
     void                  *evt_ctx;
 
     /* Mixing scratch buffer */
     int16_t                mix_buf[UIOX_SPK_PCM_FRAME_SAMPLES * 2];
 
     /* Stats */
     uint64_t               total_frames;
     uint64_t               total_underruns;
 } uiox_spk_subsys_t;
 
 /* =========================================================================
  * Subsystem API
  * ====================================================================== */
 
 int  uiox_spk_subsys_init    (uiox_spk_subsys_t          *sys,
                                uiox_spk_hw_t              *hw,
                                uiox_spk_codec_type_t       codec_type,
                                uint8_t                     codec_i2c,
                                const uiox_spk_audio_fmt_t *fmt);
 
 int  uiox_spk_subsys_start   (uiox_spk_subsys_t *sys);
 void uiox_spk_subsys_stop    (uiox_spk_subsys_t *sys);
 void uiox_spk_subsys_pause   (uiox_spk_subsys_t *sys);
 void uiox_spk_subsys_resume  (uiox_spk_subsys_t *sys);
 
 /** Add a PCM audio stream to the mixer. Returns stream ID ≥ 0. */
 int  uiox_spk_subsys_add_stream(uiox_spk_subsys_t *sys,
                                  const int16_t *data,
                                  uint32_t n_stereo,
                                  float gain,
                                  bool loop);
 
 /** Stop a specific stream. */
 void uiox_spk_subsys_stop_stream(uiox_spk_subsys_t *sys, uint8_t id);
 
 /** Write raw PCM directly into the ring buffer. */
 uint32_t uiox_spk_subsys_write(uiox_spk_subsys_t *sys,
                                 const int16_t *samples,
                                 uint32_t n_stereo);
 
 /** Set master volume (0..100 %). */
 int  uiox_spk_subsys_set_vol (uiox_spk_subsys_t *sys, uint8_t vol_pct);
 
 /** Mute/unmute with pop suppression fade. */
 int  uiox_spk_subsys_mute    (uiox_spk_subsys_t *sys, bool mute);
 
 /** Set EQ band. */
 void uiox_spk_subsys_set_eq  (uiox_spk_subsys_t *sys, uint8_t band,
                                float hz, float gain_db, float q);
 
 /** Periodic tick — mix streams into ring buffer, trigger DMA refill. */
 void uiox_spk_subsys_tick    (uiox_spk_subsys_t *sys, uint32_t now_ms);
 
 void uiox_spk_subsys_set_cb  (uiox_spk_subsys_t *sys,
                                uiox_spk_evt_cb_t cb, void *ctx);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SPK_SUBSYS_H */
 