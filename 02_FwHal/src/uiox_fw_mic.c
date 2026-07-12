/* ── Mic ─── */
#include "../include/uiox_fw_mic.h"
#define OPS_MIC(d) ((const uiox_mic_ops_t*)(d)->priv)
static uiox_fw_err_t mic_i2s_init(uiox_mic_dev_t *d,uint32_t r,uint8_t ch,uint8_t bits)
{ d->sample_rate_hz=r;d->channels=ch;d->bits_per_sample=bits;d->initialized=true;return UIOX_FW_OK; }
static void mic_i2s_deinit(uiox_mic_dev_t *d){(void)d;}
static uiox_fw_err_t mic_start(uiox_mic_dev_t *d){(void)d;return UIOX_FW_OK;}
static void mic_stop(uiox_mic_dev_t *d){(void)d;}
static uiox_fw_err_t mic_read(uiox_mic_dev_t *d,int16_t *buf,uint32_t frames)
{ for(uint32_t i=0u;i<frames*(uint32_t)d->channels;i++)buf[i]=0;return UIOX_FW_OK; }
static const uiox_mic_ops_t mic_i2s_ops={mic_i2s_init,mic_i2s_deinit,mic_start,mic_stop,mic_read};
uiox_fw_err_t uiox_fw_mic_init(uiox_mic_dev_t *d,const uiox_mic_ops_t *o,uint32_t r,uint8_t ch,uint8_t bits)
{ if(!d||!o||!o->init)return UIOX_FW_ERR_INVAL;d->priv=(void*)o;return o->init(d,r,ch,bits); }
void uiox_fw_mic_deinit(uiox_mic_dev_t *d){if(d&&d->priv&&OPS_MIC(d)->deinit)OPS_MIC(d)->deinit(d);}
uiox_fw_err_t uiox_fw_mic_start(uiox_mic_dev_t *d){if(!d||!d->priv||!OPS_MIC(d)->start)return UIOX_FW_ERR_INVAL;return OPS_MIC(d)->start(d);}
void uiox_fw_mic_stop(uiox_mic_dev_t *d){if(d&&d->priv&&OPS_MIC(d)->stop)OPS_MIC(d)->stop(d);}
uiox_fw_err_t uiox_fw_mic_read(uiox_mic_dev_t *d,int16_t *buf,uint32_t frames)
{ if(!d||!d->priv||!OPS_MIC(d)->read)return UIOX_FW_ERR_INVAL;return OPS_MIC(d)->read(d,buf,frames); }
