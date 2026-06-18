/**
 * @file    uiox_spk_device.h
 * @brief   UIOX Speaker top-level application-facing device API.
 * @date    2026-06-01
 */

 #ifndef UIOX_SPK_DEVICE_H
 #define UIOX_SPK_DEVICE_H
 
 #include "uiox_spk_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_spk_hw_t             *hw;
     const uiox_spk_hw_ops_t   *hw_ops;
     uiox_spk_codec_type_t      codec_type;
     uint8_t                    codec_i2c;
     uiox_spk_audio_fmt_t       fmt;
     uiox_spk_evt_cb_t          evt_cb;
     void                      *evt_ctx;
 } uiox_spk_open_params_t;
 
 typedef struct {
     uiox_spk_subsys_t  subsys;
     uiox_spk_hw_t     *hw;
     bool               open;
 } uiox_spk_device_t;
 
 int  uiox_spk_open        (uiox_spk_device_t           *dev,
                             const uiox_spk_open_params_t *p);
 int  uiox_spk_start       (uiox_spk_device_t *dev);
 void uiox_spk_stop        (uiox_spk_device_t *dev);
 void uiox_spk_pause       (uiox_spk_device_t *dev);
 void uiox_spk_resume      (uiox_spk_device_t *dev);
 void uiox_spk_close       (uiox_spk_device_t *dev);
 void uiox_spk_tick        (uiox_spk_device_t *dev, uint32_t now_ms);
 
 int      uiox_spk_play    (uiox_spk_device_t *dev,
                             const int16_t *data, uint32_t n_stereo,
                             float gain, bool loop);
 void     uiox_spk_stop_stream(uiox_spk_device_t *dev, uint8_t id);
 uint32_t uiox_spk_write   (uiox_spk_device_t *dev,
                             const int16_t *samples, uint32_t n_stereo);
 
 int  uiox_spk_set_volume  (uiox_spk_device_t *dev, uint8_t vol_pct);
 int  uiox_spk_set_mute    (uiox_spk_device_t *dev, bool mute);
 void uiox_spk_set_eq      (uiox_spk_device_t *dev, uint8_t band,
                             float hz, float gain_db, float q);
 void uiox_spk_set_bass    (uiox_spk_device_t *dev, int8_t db);
 void uiox_spk_set_treble  (uiox_spk_device_t *dev, int8_t db);
 
 uiox_spk_state_t uiox_spk_state(const uiox_spk_device_t *dev);
 void uiox_spk_print_stats (const uiox_spk_device_t *dev);
 const char *uiox_spk_state_name(uiox_spk_state_t s);
 const char *uiox_spk_evt_name  (uiox_spk_evt_t evt);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SPK_DEVICE_H */
 