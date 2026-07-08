/* ── uiox_fw_tb4.c ──────────────────────────────────────────── */
#include "../include/uiox_fw_tb4.h"
#define OPS_TB4(d) ((const uiox_tb4_ops_t*)(d)->priv)
static inline void tb_wr(uintptr_t b,uint32_t o,uint32_t v){*((volatile uint32_t*)(b+o))=v;}
static inline uint32_t tb_rd(uintptr_t b,uint32_t o){return *((volatile uint32_t*)(b+o));}

static uiox_fw_err_t tb4_nhi_init(uiox_tb4_dev_t *d)
{
    /* Force-power the TB4 controller */
    tb_wr(d->nhi_base, 0x39640u, 0x01u);  /* FRC_PWR */
    /* Wait controller ready: poll NHI_POWER_STATE */
    for(uint32_t i=0u;i<100000u;i++)
        if(tb_rd(d->nhi_base,0x39630u)&0x01u)break;
    d->initialized=true;
    return UIOX_FW_OK;
}
static void tb4_nhi_deinit(uiox_tb4_dev_t *d){(void)d;}
static uiox_fw_err_t tb4_power_on(uiox_tb4_dev_t *d){(void)d;return UIOX_FW_OK;}
static void tb4_power_off(uiox_tb4_dev_t *d){(void)d;}
static uiox_fw_err_t tb4_icm_send(uiox_tb4_dev_t *d,
                                     const uint32_t *msg,uint8_t n)
{
    for(uint8_t i=0u;i<n;i++)
        tb_wr(d->nhi_base, 0x38000u+i*4u, msg[i]);
    return UIOX_FW_OK;
}
static uiox_fw_err_t tb4_icm_recv(uiox_tb4_dev_t *d,
                                     uint32_t *msg,uint8_t max)
{
    for(uint8_t i=0u;i<max;i++)
        msg[i]=tb_rd(d->nhi_base, 0x38100u+i*4u);
    return UIOX_FW_OK;
}
static uiox_fw_err_t tb4_approve(uiox_tb4_dev_t *d,
                                    uint8_t route_hi,uint32_t route_lo)
{
    (void)d;(void)route_hi;(void)route_lo;return UIOX_FW_OK;
}
static bool tb4_hotplug(uiox_tb4_dev_t *d)
{ return !!(tb_rd(d->nhi_base,0x39620u)&0x01u); }
static void tb4_isr(uiox_tb4_dev_t *d){(void)d;}

static const uiox_tb4_ops_t tb4_jhl_ops={
    tb4_nhi_init,tb4_nhi_deinit,
    tb4_power_on,tb4_power_off,
    tb4_icm_send,tb4_icm_recv,
    tb4_approve,tb4_hotplug,tb4_isr
};
uiox_fw_err_t uiox_fw_tb4_init(uiox_tb4_dev_t *d,const uiox_tb4_ops_t *o)
{ if(!d||!o||!o->init)return UIOX_FW_ERR_INVAL;
  d->priv=(void*)o;return o->init(d); }
void uiox_fw_tb4_deinit(uiox_tb4_dev_t *d)
{ if(d&&d->priv&&OPS_TB4(d)->deinit)OPS_TB4(d)->deinit(d); }
uiox_fw_err_t uiox_fw_tb4_power_on(uiox_tb4_dev_t *d)
{ if(!d||!d->priv||!OPS_TB4(d)->power_on)return UIOX_FW_ERR_INVAL;
  return OPS_TB4(d)->power_on(d); }
void uiox_fw_tb4_power_off(uiox_tb4_dev_t *d)
{ if(d&&d->priv&&OPS_TB4(d)->power_off)OPS_TB4(d)->power_off(d); }
uiox_fw_err_t uiox_fw_tb4_approve_dev(uiox_tb4_dev_t *d,
                                         uint8_t route_hi,uint32_t route_lo)
{ if(!d||!d->priv||!OPS_TB4(d)->approve_dev)return UIOX_FW_ERR_INVAL;
  return OPS_TB4(d)->approve_dev(d,route_hi,route_lo); }
uiox_fw_err_t uiox_fw_tb4_init_jhl8540(uiox_tb4_dev_t *d,
                                          uintptr_t nhi_base,uint32_t irq)
{
    if(!d)return UIOX_FW_ERR_INVAL;
    uint8_t *p=(uint8_t*)d;for(size_t i=0u;i<sizeof(*d);i++)p[i]=0u;
    d->nhi_base=nhi_base;d->irq=irq;
    d->version=UIOX_TB4_VER_TB4;d->security=UIOX_TB4_SEC_USER;
    d->num_ports=2u;d->caps=UIOX_TB4_CAP_40GBPS|UIOX_TB4_CAP_PCIE|
                              UIOX_TB4_CAP_DP|UIOX_TB4_CAP_USB3|
                              UIOX_TB4_CAP_POWER_100W;
    const char *m="Intel JHL8540";
    for(int i=0;m[i]&&i<31;i++)d->model[i]=m[i];
    return uiox_fw_tb4_init(d,&tb4_jhl_ops);
}
