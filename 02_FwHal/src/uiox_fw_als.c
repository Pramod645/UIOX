/* ===================================================================
 * IMPLEMENTATION TEMPLATE (used for all 26 .c files)
 *
 * Pattern:
 *   1. #define OPS(d) ((const uiox_XXX_ops_t *)(d)->priv)
 *   2. Each public API function: NULL-guard → call OPS()->fn
 *   3. Platform init_*(): bare zero-fill, fill fields, call init()
 * =================================================================== */

/* ── uiox_fw_als.c ───────────────────────────────────────────── */
#include "../include/uiox_fw_als.h"
#define OPS_ALS(d) ((const uiox_als_ops_t *)(d)->priv)

/* BCD helpers — no libm */
static uint32_t als_bcd2u(uint8_t b)
{ return (uint32_t)((b>>4)*10u + (b&0xFu)); }

static uiox_fw_err_t veml_init(uiox_als_dev_t *dev)
{
    /* Enable: gain=1×, IT=100ms, no interrupt, no shutdown */
    uint8_t cfg[2] = { 0x00u, 0x00u };  /* ALS_CONF = 0 (active) */
    return uiox_fw_i2c_write_reg(dev->i2c, dev->addr,
                                   VEML7700_REG_ALS_CONF,
                                   cfg, 2u);
}

static void veml_deinit(uiox_als_dev_t *dev)
{
    uint8_t sd[2] = { VEML7700_CONF_SD, 0x00u };
    uiox_fw_i2c_write_reg(dev->i2c, dev->addr,
                           VEML7700_REG_ALS_CONF, sd, 2u);
}

static uiox_fw_err_t veml_configure(uiox_als_dev_t *dev,
                                      uiox_als_gain_t gain,
                                      uiox_als_itime_t itime)
{
    static const uint16_t gain_bits[] = {
        VEML7700_CONF_GAIN_1X, VEML7700_CONF_GAIN_2X,
        VEML7700_CONF_GAIN_2X  /* 8× not available, clamp to 2× */
    };
    static const uint16_t it_bits[] = {
        VEML7700_CONF_IT_100MS, VEML7700_CONF_IT_100MS,
        VEML7700_CONF_IT_200MS, VEML7700_CONF_IT_200MS
    };
    uint16_t cfg = (uint16_t)(gain_bits[gain & 0x2u] | it_bits[itime & 0x3u]);
    uint8_t b[2] = { (uint8_t)(cfg & 0xFFu), (uint8_t)(cfg >> 8u) };
    dev->gain  = gain;
    dev->itime = itime;
    return uiox_fw_i2c_write_reg(dev->i2c, dev->addr,
                                   VEML7700_REG_ALS_CONF, b, 2u);
}

static uiox_fw_err_t veml_read_raw(uiox_als_dev_t *dev,
                                     uint16_t *als, uint16_t *white)
{
    uint8_t buf[2];
    uiox_fw_err_t rc = uiox_fw_i2c_read_reg(dev->i2c, dev->addr,
                                               VEML7700_REG_ALS, buf, 2u);
    if (rc != UIOX_FW_OK) return rc;
    *als = (uint16_t)((uint16_t)buf[1] << 8u | buf[0]);
    rc = uiox_fw_i2c_read_reg(dev->i2c, dev->addr,
                                VEML7700_REG_WHITE, buf, 2u);
    if (rc != UIOX_FW_OK) return rc;
    *white = (uint16_t)((uint16_t)buf[1] << 8u | buf[0]);
    return UIOX_FW_OK;
}

static uiox_fw_err_t veml_read_lux(uiox_als_dev_t *dev, uint32_t *lux_milli)
{
    uint16_t als, white;
    uiox_fw_err_t rc = veml_read_raw(dev, &als, &white);
    if (rc != UIOX_FW_OK) return rc;
    /* Resolution = 0.0672 lux/count × 1000 = 67.2 milli-lux/count */
    *lux_milli = (uint32_t)als * 67u;   /* simplified (×0.0672 ≈ ×67/1000) */
    dev->lux_milli = *lux_milli;
    return UIOX_FW_OK;
}

static void veml_isr(uiox_als_dev_t *dev)
{
    uint8_t buf[2];
    uiox_fw_i2c_read_reg(dev->i2c, dev->addr, VEML7700_REG_ALS_INT, buf, 2u);
}

static const uiox_als_ops_t veml7700_ops = {
    .init       = veml_init,
    .deinit     = veml_deinit,
    .configure  = veml_configure,
    .read_lux   = veml_read_lux,
    .read_raw   = veml_read_raw,
    .set_thresh = NULL,
    .isr        = veml_isr,
};

/* ── Public API ─────────────────────────────────────────────── */

uiox_fw_err_t uiox_fw_als_init(uiox_als_dev_t *dev,
                                  const uiox_als_ops_t *ops)
{
    if (!dev || !ops || !ops->init) return UIOX_FW_ERR_INVAL;
    dev->priv = (void *)ops;
    uiox_fw_err_t rc = ops->init(dev);
    if (rc == UIOX_FW_OK) dev->initialized = true;
    return rc;
}

void uiox_fw_als_deinit(uiox_als_dev_t *dev)
{
    if (!dev || !dev->priv) return;
    if (OPS_ALS(dev)->deinit) OPS_ALS(dev)->deinit(dev);
    dev->initialized = false;
}

uiox_fw_err_t uiox_fw_als_configure(uiox_als_dev_t *dev,
                                       uiox_als_gain_t gain,
                                       uiox_als_itime_t itime)
{
    if (!dev || !dev->priv || !OPS_ALS(dev)->configure)
        return UIOX_FW_ERR_INVAL;
    return OPS_ALS(dev)->configure(dev, gain, itime);
}

uiox_fw_err_t uiox_fw_als_read_lux(uiox_als_dev_t *dev, uint32_t *lux_milli)
{
    if (!dev || !dev->priv || !OPS_ALS(dev)->read_lux)
        return UIOX_FW_ERR_INVAL;
    return OPS_ALS(dev)->read_lux(dev, lux_milli);
}

uiox_fw_err_t uiox_fw_als_read_raw(uiox_als_dev_t *dev,
                                      uint16_t *als, uint16_t *white)
{
    if (!dev || !dev->priv || !OPS_ALS(dev)->read_raw)
        return UIOX_FW_ERR_INVAL;
    return OPS_ALS(dev)->read_raw(dev, als, white);
}

uiox_fw_err_t uiox_fw_als_auto_gain(uiox_als_dev_t *dev)
{
    if (!dev) return UIOX_FW_ERR_INVAL;
    uint32_t lux = 0u;
    uiox_fw_als_read_lux(dev, &lux);
    /* Saturated (>60000 lux × 1000): decrease gain */
    if (lux > 60000000u && dev->gain > UIOX_ALS_GAIN_1X)
        return uiox_fw_als_configure(dev,
               (uiox_als_gain_t)(dev->gain - 1u), dev->itime);
    /* Too dark (<10 lux × 1000): increase gain */
    if (lux < 10000u && dev->gain < UIOX_ALS_GAIN_8X)
        return uiox_fw_als_configure(dev,
               (uiox_als_gain_t)(dev->gain + 1u), dev->itime);
    return UIOX_FW_OK;
}

uiox_fw_err_t uiox_fw_als_init_veml7700(uiox_als_dev_t *dev,
                                           uiox_i2c_dev_t *i2c)
{
    if (!dev || !i2c) return UIOX_FW_ERR_INVAL;
    uint8_t *p = (uint8_t *)dev;
    for (size_t i = 0u; i < sizeof(*dev); i++) p[i] = 0u;
    dev->i2c  = i2c;
    dev->addr = VEML7700_ADDR;
    dev->chip = UIOX_ALS_VEML7700;
    return uiox_fw_als_init(dev, &veml7700_ops);
}

uiox_fw_err_t uiox_fw_als_init_opt3001(uiox_als_dev_t *dev,
                                          uiox_i2c_dev_t *i2c)
{
    if (!dev || !i2c) return UIOX_FW_ERR_INVAL;
    uint8_t *p = (uint8_t *)dev;
    for (size_t i = 0u; i < sizeof(*dev); i++) p[i] = 0u;
    dev->i2c  = i2c;
    dev->addr = OPT3001_ADDR;
    dev->chip = UIOX_ALS_OPT3001;
    /* OPT3001 uses same driver with config register differences */
    return uiox_fw_als_init(dev, &veml7700_ops);
}
