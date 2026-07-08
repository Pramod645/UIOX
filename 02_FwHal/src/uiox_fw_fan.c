/* ── uiox_fw_fan.c ──────────────────────────────────────────── */
#include "../include/uiox_fw_fan.h"
#define OPS_FAN(d) ((const uiox_fan_ops_t *)(d)->priv)

static uiox_fw_err_t fan_pwm_init(uiox_fan_dev_t *dev)
{ (void)dev; return UIOX_FW_OK; }
static void fan_pwm_deinit(uiox_fan_dev_t *dev) { (void)dev; }
static uiox_fw_err_t fan_pwm_set_duty(uiox_fan_dev_t *dev, uint8_t pct)
{
    if (pct > 100u) pct = 100u;
    /* Write SP804 match register to set PWM duty */
    uint32_t match = (uint32_t)dev->pwm_period * pct / 100u;
    *((volatile uint32_t*)(dev->pwm_base + 0x010u)) = match;
    dev->duty_pct = pct;
    return UIOX_FW_OK;
}
static uiox_fw_err_t fan_pwm_read_rpm(uiox_fan_dev_t *dev, uint32_t *rpm)
{ dev->rpm = 3000u; *rpm = dev->rpm; return UIOX_FW_OK; }

static const uiox_fan_ops_t fan_pwm_ops = {
    .init=fan_pwm_init, .deinit=fan_pwm_deinit,
    .set_duty=fan_pwm_set_duty, .read_rpm=fan_pwm_read_rpm,
    .set_auto=NULL
};
uiox_fw_err_t uiox_fw_fan_init(uiox_fan_dev_t *dev, const uiox_fan_ops_t *ops)
{ if(!dev||!ops||!ops->init) return UIOX_FW_ERR_INVAL;
  dev->priv=(void*)ops; return ops->init(dev); }
void uiox_fw_fan_deinit(uiox_fan_dev_t *dev)
{ if(dev&&dev->priv&&OPS_FAN(dev)->deinit) OPS_FAN(dev)->deinit(dev); }
uiox_fw_err_t uiox_fw_fan_set_duty(uiox_fan_dev_t *dev, uint8_t pct)
{ if(!dev||!dev->priv||!OPS_FAN(dev)->set_duty) return UIOX_FW_ERR_INVAL;
  return OPS_FAN(dev)->set_duty(dev,pct); }
uiox_fw_err_t uiox_fw_fan_read_rpm(uiox_fan_dev_t *dev, uint32_t *rpm)
{ if(!dev||!dev->priv||!OPS_FAN(dev)->read_rpm) return UIOX_FW_ERR_INVAL;
  return OPS_FAN(dev)->read_rpm(dev,rpm); }
uiox_fw_err_t uiox_fw_fan_init_pwm(uiox_fan_dev_t *dev, uintptr_t pwm_base)
{
    if(!dev) return UIOX_FW_ERR_INVAL;
    uint8_t *p=(uint8_t*)dev; for(size_t i=0u;i<sizeof(*dev);i++) p[i]=0u;
    dev->pwm_base=pwm_base; dev->pwm_period=1000u; dev->min_rpm=500u;
    return uiox_fw_fan_init(dev,&fan_pwm_ops);
}
