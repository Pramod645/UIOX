/* ── uiox_fw_charger.c ──────────────────────────────────────── */
#include "../include/uiox_fw_charger.h"
#define OPS_CHG(d) ((const uiox_chg_ops_t *)(d)->priv)
static uiox_fw_err_t bq_chg_init(uiox_chg_dev_t *dev)
{
    uint8_t v = 0u;
    /* Read device ID register to confirm chip */
    uiox_fw_i2c_read_reg(dev->i2c, dev->addr, BQ25895_REG0E, &v, 1u);
    return UIOX_FW_OK;
}
static void bq_chg_deinit(uiox_chg_dev_t *dev) { (void)dev; }
static uiox_fw_err_t bq_chg_status(uiox_chg_dev_t *dev,
                                     uiox_chg_stat_t *stat,
                                     uiox_chg_src_t  *src)
{
    uint8_t r08 = 0u;
    uiox_fw_i2c_read_reg(dev->i2c, dev->addr, BQ25895_REG08, &r08, 1u);
    *src  = (uiox_chg_src_t) ((r08 >> BQ25895_VBUS_STAT_SHIFT) & 0x7u);
    *stat = (uiox_chg_stat_t)((r08 >> BQ25895_CHRG_STAT_SHIFT) & 0x3u);
    dev->src  = *src;
    dev->stat = *stat;
    return UIOX_FW_OK;
}
static uiox_fw_err_t bq_set_ichg(uiox_chg_dev_t *dev, uint32_t ma)
{
    uint8_t v = (uint8_t)((ma / 64u) & 0x7Fu);
    return uiox_fw_i2c_write_reg(dev->i2c, dev->addr, BQ25895_REG02, &v, 1u);
}
static uiox_fw_err_t bq_set_vchg(uiox_chg_dev_t *dev, uint32_t mv)
{
    uint8_t v = (uint8_t)(((mv > 3840u ? mv - 3840u : 0u) / 16u) & 0x3Fu);
    v = (uint8_t)(v << 2u);
    return uiox_fw_i2c_write_reg(dev->i2c, dev->addr, BQ25895_REG04, &v, 1u);
}
static uiox_fw_err_t bq_set_iin(uiox_chg_dev_t *dev, uint32_t ma)
{
    uint8_t v = (uint8_t)((ma > 100u ? (ma-100u)/50u : 0u) & 0x3Fu);
    return uiox_fw_i2c_write_reg(dev->i2c, dev->addr, BQ25895_REG00, &v, 1u);
}
static uiox_fw_err_t bq_read_adc(uiox_chg_dev_t *dev,
                                    uint32_t *vbus, uint32_t *vbat,
                                    uint32_t *ichg)
{
    uint8_t v = 0u;
    /* Enable ADC conversion */
    uiox_fw_i2c_read_reg(dev->i2c, dev->addr, BQ25895_REG02, &v, 1u);
    v |= 0x80u;
    uiox_fw_i2c_write_reg(dev->i2c, dev->addr, BQ25895_REG02, &v, 1u);
    /* Read VBUS: 2600 + reg × 100 mV */
    uiox_fw_i2c_read_reg(dev->i2c, dev->addr, BQ25895_REG0E, &v, 1u);
    *vbus = 2600u + (uint32_t)(v & 0x7Fu) * 100u;
    /* Read VBAT: 2304 + reg × 20 mV */
    uiox_fw_i2c_read_reg(dev->i2c, dev->addr, BQ25895_REG0F, &v, 1u);
    *vbat = 2304u + (uint32_t)(v & 0x7Fu) * 20u;
    /* Read ICHG: reg × 50 mA */
    uiox_fw_i2c_read_reg(dev->i2c, dev->addr, BQ25895_REG13, &v, 1u);
    *ichg = (uint32_t)(v & 0x7Fu) * 50u;
    return UIOX_FW_OK;
}
static const uiox_chg_ops_t bq25895_ops = {
    .init       = bq_chg_init,
    .deinit     = bq_chg_deinit,
    .get_status = bq_chg_status,
    .set_ichg   = bq_set_ichg,
    .set_vchg   = bq_set_vchg,
    .set_iin_lim= bq_set_iin,
    .read_adc   = bq_read_adc,
    .enable_otg = NULL,
    .isr        = NULL,
};
uiox_fw_err_t uiox_fw_chg_init(uiox_chg_dev_t *dev, const uiox_chg_ops_t *ops)
{ if(!dev||!ops||!ops->init) return UIOX_FW_ERR_INVAL;
  dev->priv=(void*)ops; return ops->init(dev); }
void uiox_fw_chg_deinit(uiox_chg_dev_t *dev)
{ if(dev&&dev->priv&&OPS_CHG(dev)->deinit) OPS_CHG(dev)->deinit(dev); }
uiox_fw_err_t uiox_fw_chg_get_status(uiox_chg_dev_t *dev,
                                        uiox_chg_stat_t *stat,
                                        uiox_chg_src_t  *src)
{ if(!dev||!dev->priv||!OPS_CHG(dev)->get_status) return UIOX_FW_ERR_INVAL;
  return OPS_CHG(dev)->get_status(dev,stat,src); }
uiox_fw_err_t uiox_fw_chg_set_ichg(uiox_chg_dev_t *dev, uint32_t ma)
{ if(!dev||!dev->priv||!OPS_CHG(dev)->set_ichg) return UIOX_FW_ERR_INVAL;
  return OPS_CHG(dev)->set_ichg(dev,ma); }
uiox_fw_err_t uiox_fw_chg_set_vchg(uiox_chg_dev_t *dev, uint32_t mv)
{ if(!dev||!dev->priv||!OPS_CHG(dev)->set_vchg) return UIOX_FW_ERR_INVAL;
  return OPS_CHG(dev)->set_vchg(dev,mv); }
uiox_fw_err_t uiox_fw_chg_read_adc(uiox_chg_dev_t *dev,
                                      uint32_t *vbus, uint32_t *vbat,
                                      uint32_t *ichg)
{ if(!dev||!dev->priv||!OPS_CHG(dev)->read_adc) return UIOX_FW_ERR_INVAL;
  return OPS_CHG(dev)->read_adc(dev,vbus,vbat,ichg); }
uiox_fw_err_t uiox_fw_chg_init_bq25895(uiox_chg_dev_t *dev,
                                          uiox_i2c_dev_t *i2c)
{
    if(!dev||!i2c) return UIOX_FW_ERR_INVAL;
    uint8_t *p=(uint8_t*)dev; for(size_t i=0u;i<sizeof(*dev);i++) p[i]=0u;
    dev->i2c=i2c; dev->addr=BQ25895_ADDR; dev->chip=UIOX_CHG_IC_BQ25895;
    return uiox_fw_chg_init(dev, &bq25895_ops);
}