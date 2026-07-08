/* ── uiox_fw_ramrtc.c ───────────────────────────────────────── */
#include "../include/uiox_fw_ramrtc.h"
#define OPS_RTC(d) ((const uiox_ramrtc_ops_t*)(d)->priv)

/* DS1307 — BCD encoded */
static uint8_t to_bcd(uint8_t v){ return (uint8_t)(((v/10u)<<4u)|(v%10u)); }
static uint8_t from_bcd(uint8_t b){ return (uint8_t)((b>>4u)*10u+(b&0x0Fu)); }

static uiox_fw_err_t ds1307_init(uiox_ramrtc_dev_t *d)
{
    /* Clear CH bit (bit7 of seconds register) to start oscillator */
    uint8_t v = 0u;
    uiox_fw_i2c_read_reg(d->i2c, d->addr, DS1307_REG_SEC, &v, 1u);
    v &= 0x7Fu;
    uiox_fw_i2c_write_reg(d->i2c, d->addr, DS1307_REG_SEC, &v, 1u);
    d->battery_ok  = true;
    d->initialized = true;
    return UIOX_FW_OK;
}
static void ds1307_deinit(uiox_ramrtc_dev_t *d) { (void)d; }

static uiox_fw_err_t ds1307_read_time(uiox_ramrtc_dev_t *d,
                                        uiox_rtc_time_t *t)
{
    uint8_t buf[7];
    uiox_fw_err_t rc = uiox_fw_i2c_read_reg(d->i2c, d->addr,
                                               DS1307_REG_SEC, buf, 7u);
    if (rc != UIOX_FW_OK) return rc;
    t->sec   = from_bcd(buf[0] & 0x7Fu);
    t->min   = from_bcd(buf[1]);
    t->hr    = from_bcd(buf[2] & 0x3Fu);
    t->dow   = buf[3];
    t->date  = from_bcd(buf[4]);
    t->month = from_bcd(buf[5]);
    t->year  = (uint16_t)(2000u + from_bcd(buf[6]));
    return UIOX_FW_OK;
}

static uiox_fw_err_t ds1307_write_time(uiox_ramrtc_dev_t *d,
                                         const uiox_rtc_time_t *t)
{
    uint8_t buf[7] = {
        to_bcd(t->sec),  to_bcd(t->min),
        to_bcd(t->hr),   t->dow,
        to_bcd(t->date), to_bcd(t->month),
        to_bcd((uint8_t)(t->year % 100u))
    };
    return uiox_fw_i2c_write_reg(d->i2c, d->addr, DS1307_REG_SEC, buf, 7u);
}

static uiox_fw_err_t ds1307_read_nvram(uiox_ramrtc_dev_t *d,
                                          uint8_t off, uint8_t *buf,
                                          uint8_t len)
{
    return uiox_fw_i2c_read_reg(d->i2c, d->addr,
                                  DS1307_REG_NVRAM + off, buf, len);
}

static uiox_fw_err_t ds1307_write_nvram(uiox_ramrtc_dev_t *d,
                                           uint8_t off, const uint8_t *buf,
                                           uint8_t len)
{
    return uiox_fw_i2c_write_reg(d->i2c, d->addr,
                                   DS1307_REG_NVRAM + off, buf, len);
}

static bool ds1307_bat_ok(uiox_ramrtc_dev_t *d) { return d->battery_ok; }

static const uiox_ramrtc_ops_t ds1307_rtc_ops = {
    ds1307_init, ds1307_deinit,
    ds1307_read_time, ds1307_write_time,
    ds1307_read_nvram, ds1307_write_nvram,
    ds1307_bat_ok
};

/* MC146818 (x86 CMOS RTC) via MMIO */
static uiox_fw_err_t mc_init(uiox_ramrtc_dev_t *d)
{
    /* Ensure oscillator running: REG_A bits 6:4 = 010 */
    volatile uint8_t *idx  = (volatile uint8_t *)d->mmio_base;
    volatile uint8_t *data = (volatile uint8_t *)(d->mmio_base + 1u);
    *idx = 0x0Au; uint8_t a = *data;
    if ((a & 0x70u) != 0x20u) { *idx=0x0Au; *data=(a&0x8Fu)|0x20u; }
    d->battery_ok  = true;
    d->initialized = true;
    return UIOX_FW_OK;
}
static void mc_deinit(uiox_ramrtc_dev_t *d) { (void)d; }

static uiox_fw_err_t mc_read_time(uiox_ramrtc_dev_t *d, uiox_rtc_time_t *t)
{
    volatile uint8_t *idx  = (volatile uint8_t *)d->mmio_base;
    volatile uint8_t *data = (volatile uint8_t *)(d->mmio_base + 1u);
    /* Wait for UIP=0 */
    for (uint32_t i=0u;i<100000u;i++) {
        *idx=0x0Au; if (!(*data & 0x80u)) break;
    }
    *idx=0x00u; t->sec   = from_bcd(*data);
    *idx=0x02u; t->min   = from_bcd(*data);
    *idx=0x04u; t->hr    = from_bcd(*data & 0x3Fu);
    *idx=0x06u; t->dow   = *data;
    *idx=0x07u; t->date  = from_bcd(*data);
    *idx=0x08u; t->month = from_bcd(*data);
    *idx=0x09u; t->year  = (uint16_t)(2000u + from_bcd(*data));
    return UIOX_FW_OK;
}

static uiox_fw_err_t mc_write_time(uiox_ramrtc_dev_t *d,
                                     const uiox_rtc_time_t *t)
{
    volatile uint8_t *idx  = (volatile uint8_t *)d->mmio_base;
    volatile uint8_t *data = (volatile uint8_t *)(d->mmio_base + 1u);
    /* Set SET bit in REG_B to halt update */
    *idx=0x0Bu; uint8_t b = *data; *idx=0x0Bu; *data = (uint8_t)(b|0x80u);
    *idx=0x00u; *data=to_bcd(t->sec);
    *idx=0x02u; *data=to_bcd(t->min);
    *idx=0x04u; *data=to_bcd(t->hr);
    *idx=0x06u; *data=t->dow;
    *idx=0x07u; *data=to_bcd(t->date);
    *idx=0x08u; *data=to_bcd(t->month);
    *idx=0x09u; *data=to_bcd((uint8_t)(t->year%100u));
    /* Clear SET bit */
    *idx=0x0Bu; *data = (uint8_t)(b & ~0x80u);
    return UIOX_FW_OK;
}

static uiox_fw_err_t mc_read_nvram(uiox_ramrtc_dev_t *d,
                                     uint8_t off, uint8_t *buf, uint8_t len)
{
    volatile uint8_t *idx  = (volatile uint8_t *)d->mmio_base;
    volatile uint8_t *data = (volatile uint8_t *)(d->mmio_base + 1u);
    for (uint8_t i=0u;i<len;i++) {
        *idx = (uint8_t)(0x0Eu + off + i);
        buf[i] = *data;
    }
    return UIOX_FW_OK;
}

static uiox_fw_err_t mc_write_nvram(uiox_ramrtc_dev_t *d,
                                      uint8_t off, const uint8_t *buf,
                                      uint8_t len)
{
    volatile uint8_t *idx  = (volatile uint8_t *)d->mmio_base;
    volatile uint8_t *data = (volatile uint8_t *)(d->mmio_base + 1u);
    for (uint8_t i=0u;i<len;i++) {
        *idx  = (uint8_t)(0x0Eu + off + i);
        *data = buf[i];
    }
    return UIOX_FW_OK;
}

static const uiox_ramrtc_ops_t mc146818_rtc_ops = {
    mc_init, mc_deinit,
    mc_read_time, mc_write_time,
    mc_read_nvram, mc_write_nvram,
    ds1307_bat_ok   /* reuse — same logic */
};

uiox_fw_err_t uiox_fw_ramrtc_init(uiox_ramrtc_dev_t *d,
                                     const uiox_ramrtc_ops_t *o)
{ if(!d||!o||!o->init)return UIOX_FW_ERR_INVAL;
  d->priv=(void*)o; return o->init(d); }
void uiox_fw_ramrtc_deinit(uiox_ramrtc_dev_t *d)
{ if(d&&d->priv&&OPS_RTC(d)->deinit)OPS_RTC(d)->deinit(d); }
uiox_fw_err_t uiox_fw_ramrtc_read_time(uiox_ramrtc_dev_t *d,
                                          uiox_rtc_time_t *t)
{ if(!d||!d->priv||!OPS_RTC(d)->read_time)return UIOX_FW_ERR_INVAL;
  return OPS_RTC(d)->read_time(d,t); }
uiox_fw_err_t uiox_fw_ramrtc_write_time(uiox_ramrtc_dev_t *d,
                                           const uiox_rtc_time_t *t)
{ if(!d||!d->priv||!OPS_RTC(d)->write_time)return UIOX_FW_ERR_INVAL;
  return OPS_RTC(d)->write_time(d,t); }
uiox_fw_err_t uiox_fw_ramrtc_read_nvram(uiox_ramrtc_dev_t *d,
                                           uint8_t off,uint8_t *buf,uint8_t len)
{ if(!d||!d->priv||!OPS_RTC(d)->read_nvram)return UIOX_FW_ERR_INVAL;
  return OPS_RTC(d)->read_nvram(d,off,buf,len); }
bool uiox_fw_ramrtc_bat_ok(uiox_ramrtc_dev_t *d)
{ return(d&&d->priv&&OPS_RTC(d)->bat_ok)?OPS_RTC(d)->bat_ok(d):false; }

uiox_fw_err_t uiox_fw_ramrtc_init_ds1307(uiox_ramrtc_dev_t *d,
                                            uiox_i2c_dev_t *i2c)
{
    if(!d||!i2c)return UIOX_FW_ERR_INVAL;
    uint8_t *p=(uint8_t*)d;for(size_t i=0u;i<sizeof(*d);i++)p[i]=0u;
    d->i2c=i2c; d->addr=DS1307_ADDR; d->chip=UIOX_RAMRTC_DS1307;
    return uiox_fw_ramrtc_init(d,&ds1307_rtc_ops);
}
uiox_fw_err_t uiox_fw_ramrtc_init_mc146818(uiox_ramrtc_dev_t *d,
                                              uintptr_t base)
{
    if(!d)return UIOX_FW_ERR_INVAL;
    uint8_t *p=(uint8_t*)d;for(size_t i=0u;i<sizeof(*d);i++)p[i]=0u;
    d->mmio_base=base; d->chip=UIOX_RAMRTC_MC146818;
    return uiox_fw_ramrtc_init(d,&mc146818_rtc_ops);
}
