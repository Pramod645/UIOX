/* ── uiox_fw_thermal.c ──────────────────────────────────────── */
#include "../include/uiox_fw_thermal.h"
#define OPS_THERM(d) ((const uiox_therm_ops_t*)(d)->priv)

static uiox_fw_err_t thm_mmio_init(uiox_therm_dev_t *d)
{ d->num_zones=1u;d->initialized=true;return UIOX_FW_OK; }
static void thm_mmio_deinit(uiox_therm_dev_t *d){(void)d;}

static uiox_fw_err_t thm_mmio_read(uiox_therm_dev_t *d,
                                      uint8_t zone,int32_t *temp_dc)
{
    if(zone>=d->num_zones)return UIOX_FW_ERR_INVAL;
    /* Read temperature from MMIO sensor register (placeholder: 45.0 °C) */
    if(d->mmio_base)
        *temp_dc=(int32_t)(*((volatile uint32_t*)d->mmio_base)&0xFFFFu);
    else
        *temp_dc=450;   /* 45.0 °C stub */
    d->zones[zone].temp_dc=*temp_dc;
    return UIOX_FW_OK;
}

static uiox_fw_err_t thm_set_trip(uiox_therm_dev_t *d,uint8_t zone,
                                     const uiox_trip_t *trip)
{
    if(zone>=d->num_zones)return UIOX_FW_ERR_INVAL;
    uint8_t idx=d->zones[zone].num_trips;
    if(idx>=UIOX_THERM_MAX_TRIPS)return UIOX_FW_ERR_OVERFLOW;
    d->zones[zone].trips[idx]=*trip;
    d->zones[zone].num_trips++;
    return UIOX_FW_OK;
}

static void thm_tick(uiox_therm_dev_t *d)
{
    for(uint8_t z=0u;z<d->num_zones;z++){
        int32_t temp=0;
        thm_mmio_read(d,z,&temp);
        for(uint8_t t=0u;t<d->zones[z].num_trips;t++){
            if(temp>=(int32_t)d->zones[z].trips[t].temp_dc
               && d->trip_cb)
                d->trip_cb(z,&d->zones[z].trips[t],temp,d->trip_priv);
        }
    }
}

static const uiox_therm_ops_t thm_mmio_ops={
    thm_mmio_init,thm_mmio_deinit,thm_mmio_read,thm_set_trip,thm_tick
};

uiox_fw_err_t uiox_fw_therm_init(uiox_therm_dev_t *d,
                                    const uiox_therm_ops_t *o)
{ if(!d||!o||!o->init)return UIOX_FW_ERR_INVAL;
  d->priv=(void*)o;return o->init(d); }
void uiox_fw_therm_deinit(uiox_therm_dev_t *d)
{ if(d&&d->priv&&OPS_THERM(d)->deinit)OPS_THERM(d)->deinit(d); }
uiox_fw_err_t uiox_fw_therm_read_temp(uiox_therm_dev_t *d,
                                         uint8_t zone,int32_t *temp_dc)
{ if(!d||!d->priv||!OPS_THERM(d)->read_temp)return UIOX_FW_ERR_INVAL;
  return OPS_THERM(d)->read_temp(d,zone,temp_dc); }
void uiox_fw_therm_tick(uiox_therm_dev_t *d)
{ if(d&&d->priv&&OPS_THERM(d)->tick)OPS_THERM(d)->tick(d); }
void uiox_fw_therm_set_cb(uiox_therm_dev_t *d,uiox_trip_cb_t cb,void *p)
{ if(d){d->trip_cb=cb;d->trip_priv=p;} }
uiox_fw_err_t uiox_fw_therm_add_trip(uiox_therm_dev_t *d,uint8_t zone,
                                        uint32_t temp_dc,uiox_trip_type_t type)
{ uiox_trip_t t={temp_dc,type};return thm_set_trip(d,zone,&t); }
uiox_fw_err_t uiox_fw_therm_init_mmio(uiox_therm_dev_t *d,uintptr_t base)
{
    if(!d)return UIOX_FW_ERR_INVAL;
    uint8_t *p=(uint8_t*)d;for(size_t i=0u;i<sizeof(*d);i++)p[i]=0u;
    d->mmio_base=base;
    return uiox_fw_therm_init(d,&thm_mmio_ops);
}
