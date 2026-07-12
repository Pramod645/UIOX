#include "../include/uiox_fw_bms.h"
#define OPS_BMS(d) ((const uiox_bms_ops_t *)(d)->priv)

static uint8_t bcd2b(uint8_t b) { return (uint8_t)((b>>4)*10u+(b&0xFu)); }

/* Read a 16-bit LE register from BQ27742 */
static uiox_fw_err_t bq_rd16(uiox_bms_dev_t *dev,
                               uint8_t reg, int16_t *val)
{
    uint8_t buf[2];
    uiox_fw_err_t rc = uiox_fw_i2c_read_reg(dev->i2c, dev->addr,
                                               reg, buf, 2u);
    if (rc != UIOX_FW_OK) return rc;
    *val = (int16_t)((uint16_t)buf[1] << 8u | buf[0]);
    return UIOX_FW_OK;
}

static uiox_fw_err_t bq_init(uiox_bms_dev_t *dev)
{
    /* Send CONTROL(0x0001) — read firmware version to confirm alive */
    uint8_t ctrl[2] = { 0x01u, 0x00u };
    uiox_fw_err_t rc = uiox_fw_i2c_write_reg(dev->i2c, dev->addr,
                                               BQ27742_REG_CTRL, ctrl, 2u);
    return rc;
}

static void bq_deinit(uiox_bms_dev_t *dev) { (void)dev; }

static uiox_fw_err_t bq_read(uiox_bms_dev_t *dev, uiox_bms_data_t *out)
{
    int16_t v; uiox_fw_err_t rc;
    rc = bq_rd16(dev, BQ27742_REG_VOLT,     &v); if (rc) return rc;
    out->voltage_mv  = (uint16_t)v;
    rc = bq_rd16(dev, BQ27742_REG_AVG_CURR, &v); if (rc) return rc;
    out->current_ma  = v;
    rc = bq_rd16(dev, BQ27742_REG_SOC,      &v); if (rc) return rc;
    out->soc_pct     = (uint8_t)v;
    rc = bq_rd16(dev, BQ27742_REG_SOH,      &v); if (rc) return rc;
    out->soh_pct     = (uint8_t)v;
    rc = bq_rd16(dev, BQ27742_REG_RM,       &v); if (rc) return rc;
    out->rem_cap_mah = (uint16_t)v;
    rc = bq_rd16(dev, BQ27742_REG_FULL_CAP, &v); if (rc) return rc;
    out->full_cap_mah= (uint16_t)v;
    rc = bq_rd16(dev, BQ27742_REG_TEMP,     &v); if (rc) return rc;
    out->temp_dc = (int16_t)((int32_t)v - 2731);  /* K×10 → °C×10 */
    uint8_t flags[2];
    uiox_fw_i2c_read_reg(dev->i2c, dev->addr, BQ27742_REG_FLAGS, flags, 2u);
    uint16_t f = (uint16_t)((uint16_t)flags[1]<<8u | flags[0]);
    out->charging = !(f & BQ27742_FLAG_DSG);
    out->full     = !!(f & BQ27742_FLAG_FC);
    dev->last = *out;
    return UIOX_FW_OK;
}

static uiox_fw_err_t bq_reset(uiox_bms_dev_t *dev)
{
    uint8_t r[2] = { 0x41u, 0x00u };  /* RESET subcmd */
    return uiox_fw_i2c_write_reg(dev->i2c, dev->addr, BQ27742_REG_CTRL, r, 2u);
}

static const uiox_bms_ops_t bq27742_ops = {
    .init   = bq_init,
    .deinit = bq_deinit,
    .read   = bq_read,
    .reset  = bq_reset,
};

uiox_fw_err_t uiox_fw_bms_init(uiox_bms_dev_t *dev, const uiox_bms_ops_t *ops)
{
    if (!dev || !ops || !ops->init) return UIOX_FW_ERR_INVAL;
    dev->priv = (void *)ops;
    uiox_fw_err_t rc = ops->init(dev);
    if (rc == UIOX_FW_OK) dev->initialized = true;
    return rc;
}
void uiox_fw_bms_deinit(uiox_bms_dev_t *dev)
{ if (dev && dev->priv && OPS_BMS(dev)->deinit) OPS_BMS(dev)->deinit(dev); }

uiox_fw_err_t uiox_fw_bms_read(uiox_bms_dev_t *dev, uiox_bms_data_t *out)
{
    if (!dev || !dev->priv || !OPS_BMS(dev)->read) return UIOX_FW_ERR_INVAL;
    return OPS_BMS(dev)->read(dev, out);
}

uiox_fw_err_t uiox_fw_bms_init_bq27742(uiox_bms_dev_t *dev,
                                          uiox_i2c_dev_t *i2c)
{
    if (!dev || !i2c) return UIOX_FW_ERR_INVAL;
    uint8_t *p = (uint8_t *)dev;
    for (size_t i=0u;i<sizeof(*dev);i++) p[i]=0u;
    dev->i2c  = i2c;
    dev->addr = BQ27742_ADDR;
    dev->chip = UIOX_BMS_BQ27742;
    return uiox_fw_bms_init(dev, &bq27742_ops);
}
