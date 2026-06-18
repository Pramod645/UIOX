/**
 * @file    uiox_mic_device.h
 * @brief   UIOX Microphone top-level application-facing device API.
 * @date    2026-06-03
 */

 #ifndef UIOX_MIC_DEVICE_H
 #define UIOX_MIC_DEVICE_H
 
 #include "uiox_mic_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_mic_hw_t             *hw;
     const uiox_mic_hw_ops_t   *hw_ops;
     uiox_mic_codec_type_t      codec_type;
     uint8_t                    codec_i2c;
     uiox_mic_audio_fmt_t       fmt;
     uiox_mic_dsp_cfg_t         dsp;
     uiox_mic_evt_cb_t          evt_cb;
     void                      *evt_ctx;
 } uiox_mic_open_params_t;
 
 typedef struct {
     uiox_mic_subsys_t  subsys;
     uiox_mic_hw_t     *hw;
     bool               open;
 } uiox_mic_device_t;
 
 int      uiox_mic_open      (uiox_mic_device_t           *dev,
                               const uiox_mic_open_params_t *p);
 int      uiox_mic_start     (uiox_mic_device_t *dev);
 void     uiox_mic_stop      (uiox_mic_device_t *dev);
 void     uiox_mic_close     (uiox_mic_device_t *dev);
 void     uiox_mic_tick      (uiox_mic_device_t *dev, uint32_t now_ms);
 uint32_t uiox_mic_read      (uiox_mic_device_t *dev,
                               int16_t *buf, uint32_t n_samples);
 int      uiox_mic_set_gain  (uiox_mic_device_t *dev, uint8_t gain_db);
 int      uiox_mic_set_mute  (uiox_mic_device_t *dev, bool mute);
 bool     uiox_mic_voice_active(const uiox_mic_device_t *dev);
 float    uiox_mic_energy_dbfs (const uiox_mic_device_t *dev);
 void     uiox_mic_print_stats (const uiox_mic_device_t *dev);
 const char *uiox_mic_state_name(uiox_mic_state_t s);
 const char *uiox_mic_evt_name  (uiox_mic_evt_t evt);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MIC_DEVICE_H */
 