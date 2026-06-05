/**
 * @file    uiox_mic_codec.c
 * @brief   UIOX Microphone codec abstraction implementation.
 * @date    2026-06-03
 */

 #include "uiox_mic_codec.h"
 #include <string.h>
 #include <errno.h>
 
 #define REG_DEVICE_ID   0x00u
 #define REG_POWER       0x01u
 #define REG_GAIN_LEFT   0x00u
 #define REG_GAIN_RIGHT  0x01u
 #define REG_MUTE        0x09u
 #define REG_FORMAT      0x07u
 #define REG_SAMPLE_RATE 0x08u
 
 static inline int wr(uiox_mic_codec_t *c, uint8_t reg, uint8_t val)
 {
     const uiox_mic_hw_ops_t *ops =
         (const uiox_mic_hw_ops_t *)c->hw->priv;
     if (!ops || !ops->i2c_write) return -ENOSYS;
     return ops->i2c_write(c->hw, c->i2c_addr, reg, &val, 1u);
 }
 
 static inline int rd(uiox_mic_codec_t *c, uint8_t reg, uint8_t *val)
 {
     const uiox_mic_hw_ops_t *ops =
         (const uiox_mic_hw_ops_t *)c->hw->priv;
     if (!ops || !ops->i2c_read) return -ENOSYS;
     return ops->i2c_read(c->hw, c->i2c_addr, reg, val, 1u);
 }
 
 int uiox_mic_codec_init(uiox_mic_codec_t *codec, uiox_mic_hw_t *hw,
                          uiox_mic_codec_type_t type, uint8_t i2c_addr)
 {
     if (!codec || !hw) return -EINVAL;
     memset(codec, 0, sizeof(*codec));
     codec->hw       = hw;
     codec->type     = type;
     codec->i2c_addr = i2c_addr;
     codec->gain_db  = 20u;
     codec->channels = (hw->caps & UIOX_MIC_CAP_STEREO) ? 2u : 1u;
     wr(codec, REG_POWER, 0x00u);  /* power on */
     return 0;
 }
 
 int uiox_mic_codec_detect(uiox_mic_codec_t *codec)
 {
     if (!codec) return -EINVAL;
     uint8_t id = 0;
     int rc = rd(codec, REG_DEVICE_ID, &id);
     codec->detected = (rc == 0);
     return rc;
 }
 
 int uiox_mic_codec_set_gain(uiox_mic_codec_t *codec, uint8_t gain_db)
 {
     if (!codec) return -EINVAL;
     if (gain_db > 40u) gain_db = 40u;
     codec->gain_db = gain_db;
     /* Map 0..40 dB → register 0x00..0x3F */
     uint8_t reg_val = (uint8_t)(gain_db * 63u / 40u);
     wr(codec, REG_GAIN_LEFT,  reg_val);
     wr(codec, REG_GAIN_RIGHT, reg_val);
     return 0;
 }
 
 int uiox_mic_codec_set_mute(uiox_mic_codec_t *codec, bool mute)
 {
     if (!codec) return -EINVAL;
     codec->muted = mute;
     return wr(codec, REG_MUTE, mute ? 0x03u : 0x00u);
 }
 
 int uiox_mic_codec_set_fmt(uiox_mic_codec_t *codec,
                             const uiox_mic_audio_fmt_t *fmt)
 {
     if (!codec || !fmt) return -EINVAL;
     uint8_t fmt_reg = 0x02u;  /* I2S default */
     if (fmt->bit_depth == 24u) fmt_reg |= 0x08u;
     if (fmt->bit_depth == 32u) fmt_reg |= 0x18u;
     wr(codec, REG_FORMAT, fmt_reg);
     /* Sample rate */
     uint8_t sr_reg = (fmt->sample_rate == 48000u) ? 0x00u :
                      (fmt->sample_rate == 44100u) ? 0x11u :
                      (fmt->sample_rate == 16000u) ? 0x44u : 0x44u;
     return wr(codec, REG_SAMPLE_RATE, sr_reg);
 }
 