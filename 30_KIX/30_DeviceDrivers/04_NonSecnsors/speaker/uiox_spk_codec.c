/**
 * @file    uiox_spk_codec.c
 * @brief   UIOX Speaker codec abstraction implementation.
 * @date    2026-06-01
 */

 #include "uiox_spk_codec.h"
 #include <string.h>
 #include <errno.h>
 
 /* -------------------------------------------------------------------------
  * Codec register address stubs (TAS5756-style as example)
  * ---------------------------------------------------------------------- */
 
 #define CODEC_REG_POWER_CTRL   0x02u
 #define CODEC_REG_MUTE_CTRL    0x03u
 #define CODEC_REG_VOL_LEFT     0x3Du
 #define CODEC_REG_VOL_RIGHT    0x3Eu
 #define CODEC_REG_BASS_CTRL    0x22u
 #define CODEC_REG_TREBLE_CTRL  0x23u
 #define CODEC_REG_FORMAT       0x28u
 #define CODEC_REG_DEVICE_ID    0x00u
 
 static inline int wr(uiox_spk_codec_t *c, uint8_t reg, uint8_t val)
 {
     const uiox_spk_hw_ops_t *ops =
         (const uiox_spk_hw_ops_t *)c->hw->priv;
     if (!ops || !ops->i2c_write) return -ENOSYS;
     return ops->i2c_write(c->hw, c->i2c_addr, reg, &val, 1u);
 }
 
 static inline int rd(uiox_spk_codec_t *c, uint8_t reg, uint8_t *val)
 {
     const uiox_spk_hw_ops_t *ops =
         (const uiox_spk_hw_ops_t *)c->hw->priv;
     if (!ops || !ops->i2c_read) return -ENOSYS;
     return ops->i2c_read(c->hw, c->i2c_addr, reg, val, 1u);
 }
 
 int uiox_spk_codec_init(uiox_spk_codec_t *codec,
                          uiox_spk_hw_t *hw,
                          uiox_spk_codec_type_t type,
                          uint8_t i2c_addr)
 {
     if (!codec || !hw) return -EINVAL;
     memset(codec, 0, sizeof(*codec));
     codec->hw       = hw;
     codec->type     = type;
     codec->i2c_addr = i2c_addr;
     codec->volume   = 80u;
     /* Power on codec */
     wr(codec, CODEC_REG_POWER_CTRL, 0x00u);  /* normal operation */
     return 0;
 }
 
 int uiox_spk_codec_detect(uiox_spk_codec_t *codec)
 {
     if (!codec) return -EINVAL;
     uint8_t id = 0;
     int rc = rd(codec, CODEC_REG_DEVICE_ID, &id);
     codec->detected = (rc == 0);
     return rc;
 }
 
 int uiox_spk_codec_set_vol(uiox_spk_codec_t *codec, uint8_t vol_pct)
 {
     if (!codec) return -EINVAL;
     if (vol_pct > 100u) vol_pct = 100u;
     codec->volume = vol_pct;
     /* Map 0..100% → codec register 0xFF(mute)..0x30(max) */
     uint8_t reg_val = (uint8_t)(0xFF - (vol_pct * 0xCFu / 100u));
     wr(codec, CODEC_REG_VOL_LEFT,  reg_val);
     wr(codec, CODEC_REG_VOL_RIGHT, reg_val);
     return 0;
 }
 
 int uiox_spk_codec_set_mute(uiox_spk_codec_t *codec, bool mute)
 {
     if (!codec) return -EINVAL;
     codec->muted = mute;
     return wr(codec, CODEC_REG_MUTE_CTRL, mute ? 0x11u : 0x00u);
 }
 
 int uiox_spk_codec_set_bass(uiox_spk_codec_t *codec, int8_t db)
 {
     if (!codec) return -EINVAL;
     if (db < -12) db = -12;
     if (db >  12) db =  12;
     codec->bass_db = db;
     uint8_t reg_val = (uint8_t)((db + 12) & 0xFFu);
     return wr(codec, CODEC_REG_BASS_CTRL, reg_val);
 }
 
 int uiox_spk_codec_set_treble(uiox_spk_codec_t *codec, int8_t db)
 {
     if (!codec) return -EINVAL;
     if (db < -12) db = -12;
     if (db >  12) db =  12;
     codec->treble_db = db;
     uint8_t reg_val = (uint8_t)((db + 12) & 0xFFu);
     return wr(codec, CODEC_REG_TREBLE_CTRL, reg_val);
 }
 
 int uiox_spk_codec_set_fmt(uiox_spk_codec_t *codec,
                             const uiox_spk_audio_fmt_t *fmt)
 {
     if (!codec || !fmt) return -EINVAL;
     uint8_t fmt_reg = 0x00u;
     if (fmt->bit_depth == 24u) fmt_reg |= 0x08u;
     if (fmt->bit_depth == 32u) fmt_reg |= 0x18u;
     if (fmt->channels > 2u)   fmt_reg |= 0x04u;
     return wr(codec, CODEC_REG_FORMAT, fmt_reg);
 }
 