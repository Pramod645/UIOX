/* ── uiox_fw_pmic.c ─────────────────────────────────────────── */
#include "../include/uiox_fw_pmic.h"
#define OPS_PMIC(d) ((const uiox_pmic_ops_t*)(d)->priv)

static uiox_fw_err_t da9062_init(uiox_pmic_dev_t *d)
{
    uint8_t v = 0u;
    uiox_fw_i2c_read_reg(d->i2c, d->addr, DA9062_REG_STATUS_A, &v, 1u);
    d->initialized = true;
    return UIOX_FW_OK;
}
static void da9062_deinit(uiox_pmic_dev_t *d) { (void)d; }

static uiox_fw_err_t da9062_set_voltage(uiox_pmic_dev_t *d,
                                          uint8_t rail, uint32_t mv)
{
    if (rail >= d->num_rails) return UIOX_FW_ERR_INVAL;
    /* Write BUCK/LDO voltage register */
    uint8_t reg_base = (rail < 4u) ? DA9062_REG_BUCK1 : DA9062_REG_LDO1;
    uint8_t code = (mv > 600u) ? (uint8_t)((mv - 600u) / 10u) : 0u;
    uiox_fw_i2c_write_reg(d->i2c, d->addr,
                           reg_base + rail, &code, 1u);
    d->rails[rail].voltage_mv = mv;
    return UIOX_FW_OK;
}

static uiox_fw_err_t da9062_get_voltage(uiox_pmic_dev_t *d,
                                          uint8_t rail, uint32_t *mv)
{
    if (rail >= d->num_rails) return UIOX_FW_ERR_INVAL;
    *mv = d->rails[rail].voltage_mv;
    return UIOX_FW_OK;
}

static uiox_fw_err_t da9062_enable_rail(uiox_pmic_dev_t *d, uint8_t rail)
{
    if (rail >= d->num_rails) return UIOX_FW_ERR_INVAL;
    d->rails[rail].enabled = true;
    return UIOX_FW_OK;
}

static uiox_fw_err_t da9062_disable_rail(uiox_pmic_dev_t *d, uint8_t rail)
{
    if (rail >= d->num_rails) return UIOX_FW_ERR_INVAL;
    d->rails[rail].enabled = false;
    return UIOX_FW_OK;
}

static uiox_fw_err_t da9062_read_adc(uiox_pmic_dev_t *d,
                                        uint8_t ch, uint32_t *mv)
{
    (void)d; (void)ch;
    *mv = 3300u;   /* stub: 3.3 V */
    return UIOX_FW_OK;
}

static const uiox_pmic_ops_t da9062_pmic_ops = {
    da9062_init,     da9062_deinit,
    da9062_set_voltage, da9062_get_voltage,
    da9062_enable_rail, da9062_disable_rail,
    da9062_read_adc
};

uiox_fw_err_t uiox_fw_pmic_init(uiox_pmic_dev_t *d,
                                   const uiox_pmic_ops_t *o)
{ if(!d||!o||!o->init)return UIOX_FW_ERR_INVAL;
  d->priv=(void*)o; return o->init(d); }
void uiox_fw_pmic_deinit(uiox_pmic_dev_t *d)
{ if(d&&d->priv&&OPS_PMIC(d)->deinit)OPS_PMIC(d)->deinit(d); }
uiox_fw_err_t uiox_fw_pmic_set_voltage(uiox_pmic_dev_t *d,
                                          uint8_t rail, uint32_t mv)
{ if(!d||!d->priv||!OPS_PMIC(d)->set_voltage)return UIOX_FW_ERR_INVAL;
  return OPS_PMIC(d)->set_voltage(d,rail,mv); }
uiox_fw_err_t uiox_fw_pmic_get_voltage(uiox_pmic_dev_t *d,
                                          uint8_t rail, uint32_t *mv)
{ if(!d||!d->priv||!OPS_PMIC(d)->get_voltage)return UIOX_FW_ERR_INVAL;
  return OPS_PMIC(d)->get_voltage(d,rail,mv); }
uiox_fw_err_t uiox_fw_pmic_enable_rail(uiox_pmic_dev_t *d, uint8_t rail)
{ if(!d||!d->priv||!OPS_PMIC(d)->enable_rail)return UIOX_FW_ERR_INVAL;
  return OPS_PMIC(d)->enable_rail(d,rail); }
uiox_fw_err_t uiox_fw_pmic_init_da9062(uiox_pmic_dev_t *d,
                                          uiox_i2c_dev_t *i2c)
{
    if(!d||!i2c)return UIOX_FW_ERR_INVAL;
    uint8_t *p=(uint8_t*)d;
    for(size_t i=0u;i<sizeof(*d);i++)p[i]=0u;
    d->i2c=i2c; d->addr=DA9062_ADDR; d->chip=UIOX_PMIC_DA9062;
    /* Register 8 rails */
    d->num_rails=8u;
    const char *names[]={"BUCK1","BUCK2","BUCK3","BUCK4",
                          "LDO1","LDO2","LDO3","LDO4"};
    const uint32_t volts[]={1000u,1200u,1800u,3300u,
                             1800u,2800u,3300u,2500u};
    for(uint8_t i=0u;i<8u;i++){
        d->rails[i].idx=i;
        d->rails[i].voltage_mv=volts[i];
        d->rails[i].enabled=true;
        for(int j=0;names[i][j]&&j<7;j++)
            d->rails[i].name[j]=names[i][j];
    }
    return uiox_fw_pmic_init(d, &da9062_pmic_ops);
}
