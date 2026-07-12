/* ── HDMI ─── */
#include "../include/uiox_fw_hdmi.h"
#define OPS_HDMI(d) ((const uiox_hdmi_ops_t*)(d)->priv)
static uiox_fw_err_t hdmi_dw_init(uiox_hdmi_dev_t *d){d->connected=true;d->initialized=true;return UIOX_FW_OK;}
static void hdmi_dw_deinit(uiox_hdmi_dev_t *d){(void)d;}
static uiox_fw_err_t hdmi_dw_read_edid(uiox_hdmi_dev_t *d,uint8_t *buf)
{ if(!d->ddc_i2c)return UIOX_FW_ERR_NODEV;
  return uiox_fw_i2c_read_reg(d->ddc_i2c,HDMI_DDC_ADDR,0u,buf,128u); }
static uiox_fw_err_t hdmi_dw_set_mode(uiox_hdmi_dev_t *d,const uiox_hdmi_mode_t *m)
{ d->active_mode=*m;return UIOX_FW_OK; }
static bool hdmi_dw_hpd(uiox_hdmi_dev_t *d){return d->connected;}
static void hdmi_dw_isr(uiox_hdmi_dev_t *d){d->connected=true;}
static const uiox_hdmi_ops_t hdmi_dw_ops={hdmi_dw_init,hdmi_dw_deinit,hdmi_dw_read_edid,hdmi_dw_set_mode,hdmi_dw_hpd,hdmi_dw_isr};
uiox_fw_err_t uiox_fw_hdmi_init(uiox_hdmi_dev_t *d,const uiox_hdmi_ops_t *o)
{ if(!d||!o||!o->init)return UIOX_FW_ERR_INVAL;d->priv=(void*)o;return o->init(d); }
void uiox_fw_hdmi_deinit(uiox_hdmi_dev_t *d){if(d&&d->priv&&OPS_HDMI(d)->deinit)OPS_HDMI(d)->deinit(d);}
uiox_fw_err_t uiox_fw_hdmi_read_edid(uiox_hdmi_dev_t *d,uint8_t *buf)
{ if(!d||!d->priv||!OPS_HDMI(d)->read_edid)return UIOX_FW_ERR_INVAL;return OPS_HDMI(d)->read_edid(d,buf); }
uiox_fw_err_t uiox_fw_hdmi_set_mode(uiox_hdmi_dev_t *d,const uiox_hdmi_mode_t *m)
{ if(!d||!d->priv||!OPS_HDMI(d)->set_mode)return UIOX_FW_ERR_INVAL;return OPS_HDMI(d)->set_mode(d,m); }
bool uiox_fw_hdmi_hpd(uiox_hdmi_dev_t *d)
{ return(d&&d->priv&&OPS_HDMI(d)->hpd_status)?OPS_HDMI(d)->hpd_status(d):false; }
uiox_fw_err_t uiox_fw_hdmi_init_dw(uiox_hdmi_dev_t *d,uintptr_t base,uiox_i2c_dev_t *ddc)
{ if(!d)return UIOX_FW_ERR_INVAL;uint8_t *p=(uint8_t*)d;for(size_t i=0u;i<sizeof(*d);i++)p[i]=0u;
  d->base=base;d->ddc_i2c=ddc;return uiox_fw_hdmi_init(d,&hdmi_dw_ops); }
