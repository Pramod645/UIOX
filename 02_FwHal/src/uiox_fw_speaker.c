/* ── uiox_fw_speaker.c ──────────────────────────────────────── */
#include "../include/uiox_fw_speaker.h"
#define OPS_SPK(d) ((const uiox_spk_ops_t*)(d)->priv)
static uiox_fw_err_t spk_i2s_init(uiox_spk_dev_t *d,uint32_t r,uint8_t ch,uint8_t bits)
{ d->sample_rate_hz=r;d->channels=ch;d->bits_per_sample=bits;
  d->volume_pct=80u;d->muted=false;d->initialized=true;return UIOX_FW_OK; }
static void spk_i2s_deinit(uiox_spk_dev_t *d){(void)d;}
static uiox_fw_err_t spk_play(uiox_spk_dev_t *d,const int16_t *buf,uint32_t frames)
{ (void)d;(void)buf;(void)frames;return UIOX_FW_OK; }
static void spk_stop(uiox_spk_dev_t *d){(void)d;}
static uiox_fw_err_t spk_set_volume(uiox_spk_dev_t *d,uint8_t pct)
{ d->volume_pct=(pct>100u?100u:pct);return UIOX_FW_OK; }
static void spk_mute(uiox_spk_dev_t *d,bool en){d->muted=en;}
static const uiox_spk_ops_t spk_i2s_ops={spk_i2s_init,spk_i2s_deinit,
    spk_play,spk_stop,spk_set_volume,spk_mute};
uiox_fw_err_t uiox_fw_spk_init(uiox_spk_dev_t *d,const uiox_spk_ops_t *o,
                                  uint32_t r,uint8_t ch,uint8_t bits)
{ if(!d||!o||!o->init)return UIOX_FW_ERR_INVAL;
  d->priv=(void*)o;return o->init(d,r,ch,bits); }
void uiox_fw_spk_deinit(uiox_spk_dev_t *d)
{ if(d&&d->priv&&OPS_SPK(d)->deinit)OPS_SPK(d)->deinit(d); }
uiox_fw_err_t uiox_fw_spk_play(uiox_spk_dev_t *d,const int16_t *buf,uint32_t frames)
{ if(!d||!d->priv||!OPS_SPK(d)->play)return UIOX_FW_ERR_INVAL;
  return OPS_SPK(d)->play(d,buf,frames); }
void uiox_fw_spk_stop(uiox_spk_dev_t *d)
{ if(d&&d->priv&&OPS_SPK(d)->stop)OPS_SPK(d)->stop(d); }
uiox_fw_err_t uiox_fw_spk_set_volume(uiox_spk_dev_t *d,uint8_t pct)
{ if(!d||!d->priv||!OPS_SPK(d)->set_volume)return UIOX_FW_ERR_INVAL;
  return OPS_SPK(d)->set_volume(d,pct); }
void uiox_fw_spk_mute(uiox_spk_dev_t *d,bool en)
{ if(d&&d->priv&&OPS_SPK(d)->mute)OPS_SPK(d)->mute(d,en); }
