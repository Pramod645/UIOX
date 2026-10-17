/**
 * @file    uiox_mic_subsys.h
 * @brief   UIOX Microphone subsystem — capture, DSP pipeline, VAD events.
 * @date    2026-06-03
 */

 #ifndef UIOX_MIC_SUBSYS_H
 #define UIOX_MIC_SUBSYS_H
 
 #include "uiox_mic_if.h"
 #include "uiox_mic_codec.h"
 #include "uiox_mic_dsp.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_MIC_EVT_STARTED = 0,
     UIOX_MIC_EVT_STOPPED,
     UIOX_MIC_EVT_VOICE_START,
     UIOX_MIC_EVT_VOICE_END,
     UIOX_MIC_EVT_OVERRUN,
     UIOX_MIC_EVT_GAIN_CHANGE,
 } uiox_mic_evt_t;
 
 typedef void (*uiox_mic_evt_cb_t)(uiox_mic_evt_t evt, void *ctx);
 
 typedef enum {
     UIOX_MIC_STATE_STOPPED = 0,
     UIOX_MIC_STATE_RUNNING,
     UIOX_MIC_STATE_MUTED,
     UIOX_MIC_STATE_ERROR,
 } uiox_mic_state_t;
 
 typedef struct {
     uiox_mic_if_t        mif;
     uiox_mic_codec_t     codec;
     uiox_mic_dsp_t       dsp;
     uiox_mic_state_t     state;
     uiox_mic_evt_cb_t    evt_cb;
     void                *evt_ctx;
     bool                 last_vad;
     uint64_t             total_frames;
     uint64_t             total_overruns;
     uint32_t             tick_count;
 } uiox_mic_subsys_t;
 
 int  uiox_mic_subsys_init    (uiox_mic_subsys_t          *sys,
                                uiox_mic_hw_t              *hw,
                                uiox_mic_codec_type_t       codec_type,
                                uint8_t                     codec_i2c,
                                const uiox_mic_audio_fmt_t *fmt,
                                const uiox_mic_dsp_cfg_t   *dsp_cfg);
 
 int  uiox_mic_subsys_start   (uiox_mic_subsys_t *sys);
 void uiox_mic_subsys_stop    (uiox_mic_subsys_t *sys);
 void uiox_mic_subsys_tick    (uiox_mic_subsys_t *sys, uint32_t now_ms);
 
 int      uiox_mic_subsys_set_gain(uiox_mic_subsys_t *sys, uint8_t gain_db);
 int      uiox_mic_subsys_set_mute(uiox_mic_subsys_t *sys, bool mute);
 uint32_t uiox_mic_subsys_read    (uiox_mic_subsys_t *sys,
                                    int16_t *buf, uint32_t n);
 void     uiox_mic_subsys_set_cb  (uiox_mic_subsys_t *sys,
                                    uiox_mic_evt_cb_t cb, void *ctx);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MIC_SUBSYS_H */
 