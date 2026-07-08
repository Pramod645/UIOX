/* ── uiox_fw_touchpwd.c ─────────────────────────────────────── */
#include "../include/uiox_fw_touchpwd.h"
#define OPS_FP(d) ((const uiox_fp_ops_t*)(d)->priv)
static uiox_fw_err_t fpc_init(uiox_fp_dev_t *d)
{
    uint8_t id=0u;
    uiox_fw_i2c_read_reg(d->i2c,d->addr,0x00u,&id,1u);
    d->initialized=true;return UIOX_FW_OK;
}
static void fpc_deinit(uiox_fp_dev_t *d){(void)d;}
static uiox_fw_err_t fpc_enroll_start(uiox_fp_dev_t *d,uint8_t slot)
{ (void)d;(void)slot;return UIOX_FW_OK; }
static uiox_fw_err_t fpc_enroll_capture(uiox_fp_dev_t *d)
{ (void)d;return UIOX_FW_OK; }
static uiox_fw_err_t fpc_enroll_commit(uiox_fp_dev_t *d,uint8_t slot)
{ d->num_templates++;(void)slot;return UIOX_FW_OK; }
static uiox_fw_err_t fpc_verify(uiox_fp_dev_t *d,uint8_t *match_id)
{ (void)d;*match_id=0u;return UIOX_FW_OK; }
static uiox_fw_err_t fpc_delete(uiox_fp_dev_t *d,uint8_t slot)
{ if(d->num_templates)d->num_templates--;(void)slot;return UIOX_FW_OK; }
static void fpc_isr(uiox_fp_dev_t *d)
{ if(d->ev_cb)d->ev_cb(UIOX_FP_EVT_TOUCH,0u,d->ev_priv); }
static const uiox_fp_ops_t fpc1020_ops={
    fpc_init,fpc_deinit,fpc_enroll_start,fpc_enroll_capture,
    fpc_enroll_commit,fpc_verify,fpc_delete,fpc_isr
};
uiox_fw_err_t uiox_fw_fp_init(uiox_fp_dev_t *d,const uiox_fp_ops_t *o)
{ if(!d||!o||!o->init)return UIOX_FW_ERR_INVAL;
  d->priv=(void*)o;return o->init(d); }
void uiox_fw_fp_deinit(uiox_fp_dev_t *d)
{ if(d&&d->priv&&OPS_FP(d)->deinit)OPS_FP(d)->deinit(d); }
uiox_fw_err_t uiox_fw_fp_verify(uiox_fp_dev_t *d,uint8_t *match_id)
{ if(!d||!d->priv||!OPS_FP(d)->verify)return UIOX_FW_ERR_INVAL;
  return OPS_FP(d)->verify(d,match_id); }
uiox_fw_err_t uiox_fw_fp_enroll(uiox_fp_dev_t *d,uint8_t slot,
                                   uint8_t num_captures)
{
    if(!d||!d->priv)return UIOX_FW_ERR_INVAL;
    OPS_FP(d)->enroll_start(d,slot);
    for(uint8_t i=0u;i<num_captures;i++)
        OPS_FP(d)->enroll_capture(d);
    return OPS_FP(d)->enroll_commit(d,slot);
}
void uiox_fw_fp_set_cb(uiox_fp_dev_t *d,uiox_fp_cb_t cb,void *p)
{ if(d){d->ev_cb=cb;d->ev_priv=p;} }
uiox_fw_err_t uiox_fw_fp_init_fpc1020(uiox_fp_dev_t *d,uiox_i2c_dev_t *i2c,
                                         uint32_t gpio_int,uint32_t gpio_reset)
{
    if(!d||!i2c)return UIOX_FW_ERR_INVAL;
    uint8_t *p=(uint8_t*)d;for(size_t i=0u;i<sizeof(*d);i++)p[i]=0u;
    d->i2c=i2c;d->addr=0x60u;d->chip=UIOX_FP_FPC1020;
    d->gpio_int=gpio_int;d->gpio_reset=gpio_reset;
    return uiox_fw_fp_init(d,&fpc1020_ops);
}
