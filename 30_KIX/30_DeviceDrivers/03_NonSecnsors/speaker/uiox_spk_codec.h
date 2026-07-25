/**
 * @file    uiox_spk_codec.h
 * @brief   UIOX Speaker codec/amplifier abstraction (I2C/SPI config).
 *
 * Supports: TAS5756, MAX98357, NAU8822, WM8960, TLV320AIC3x
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_SPK_CODEC_H
 #define UIOX_SPK_CODEC_H
 
 #include "uiox_spk_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_SPK_CODEC_TAS5756  = 0,
     UIOX_SPK_CODEC_MAX98357,
     UIOX_SPK_CODEC_NAU8822,
     UIOX_SPK_CODEC_WM8960,
     UIOX_SPK_CODEC_TLV320,
     UIOX_SPK_CODEC_CUSTOM,
 } uiox_spk_codec_type_t;
 
 typedef struct {
     uiox_spk_hw_t        *hw;
     uiox_spk_codec_type_t type;
     uint8_t               i2c_addr;
     uint8_t               volume;       /**< 0..100 %                      */
     int8_t                bass_db;      /**< Bass boost dB (−12..+12)      */
     int8_t                treble_db;    /**< Treble boost dB (−12..+12)    */
     bool                  muted;
     bool                  detected;
 } uiox_spk_codec_t;
 
 int  uiox_spk_codec_init    (uiox_spk_codec_t *codec,
                               uiox_spk_hw_t *hw,
                               uiox_spk_codec_type_t type,
                               uint8_t i2c_addr);
 int  uiox_spk_codec_detect  (uiox_spk_codec_t *codec);
 int  uiox_spk_codec_set_vol (uiox_spk_codec_t *codec, uint8_t vol_pct);
 int  uiox_spk_codec_set_mute(uiox_spk_codec_t *codec, bool mute);
 int  uiox_spk_codec_set_bass(uiox_spk_codec_t *codec, int8_t db);
 int  uiox_spk_codec_set_treble(uiox_spk_codec_t *codec, int8_t db);
 int  uiox_spk_codec_set_fmt (uiox_spk_codec_t *codec,
                               const uiox_spk_audio_fmt_t *fmt);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SPK_CODEC_H */
 