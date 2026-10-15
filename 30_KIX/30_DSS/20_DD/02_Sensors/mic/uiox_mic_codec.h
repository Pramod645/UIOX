/**
 * @file    uiox_mic_codec.h
 * @brief   UIOX Microphone codec/MEMS abstraction (I2C/SPI config).
 *
 * Supports: ICS-43434, SPH0645, MP34DT05, ADMP441, WM8731, TLV320AIC3x
 *
 * @date    2026-06-03
 */

 #ifndef UIOX_MIC_CODEC_H
 #define UIOX_MIC_CODEC_H
 
 #include "uiox_mic_hw.h"
 #include "uiox_klibc.h"

 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_MIC_CODEC_ICS43434 = 0,
     UIOX_MIC_CODEC_SPH0645,
     UIOX_MIC_CODEC_MP34DT05,
     UIOX_MIC_CODEC_ADMP441,
     UIOX_MIC_CODEC_WM8731,
     UIOX_MIC_CODEC_TLV320,
     UIOX_MIC_CODEC_NAU8822,
     UIOX_MIC_CODEC_CUSTOM,
 } uiox_mic_codec_type_t;
 
 typedef struct {
     uiox_mic_hw_t        *hw;
     uiox_mic_codec_type_t type;
     uint8_t               i2c_addr;
     uint8_t               gain_db;    /**< 0..40 dB                       */
     bool                  muted;
     bool                  detected;
     uint8_t               channels;
 } uiox_mic_codec_t;
 
 int  uiox_mic_codec_init    (uiox_mic_codec_t *codec,
                               uiox_mic_hw_t *hw,
                               uiox_mic_codec_type_t type,
                               uint8_t i2c_addr);
 int  uiox_mic_codec_detect  (uiox_mic_codec_t *codec);
 int  uiox_mic_codec_set_gain(uiox_mic_codec_t *codec, uint8_t gain_db);
 int  uiox_mic_codec_set_mute(uiox_mic_codec_t *codec, bool mute);
 int  uiox_mic_codec_set_fmt (uiox_mic_codec_t *codec,
                               const uiox_mic_audio_fmt_t *fmt);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MIC_CODEC_H */
 