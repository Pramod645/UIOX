/**
 * @file    uiox_fan_hw.c
 * @brief   UIOX Fan Controller HAL implementation.
 * @date    2026-06-05
 */

 #include "uiox_fan_hw.h"
 
 int uiox_fan_hw_init(uiox_fan_hw_t *hw, const uiox_fan_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv         = (void *)ops;
     hw->global_fault = 0u;
     hw->initialised  = false;
     memset(hw->chan,    0, sizeof(hw->chan));
     memset(hw->temp_dc, 0, sizeof(hw->temp_dc));
     int rc = ops->init(hw);
     if (rc == 0) hw->initialised = true;
     return rc;
 }
 
 void uiox_fan_hw_deinit(uiox_fan_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_fan_hw_ops_t *ops = (const uiox_fan_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv        = NULL;
     hw->initialised = false;
 }
 
 int uiox_fan_hw_set_pwm(uiox_fan_hw_t *hw, uint8_t ch, uint8_t duty)
 {
     if (!hw || !hw->priv || ch >= hw->num_fans) return -EINVAL;
     const uiox_fan_hw_ops_t *ops = (const uiox_fan_hw_ops_t *)hw->priv;
     if (!ops->set_pwm) return -ENOSYS;
     int rc = ops->set_pwm(hw, ch, duty);
     if (rc == 0) hw->chan[ch].pwm_duty = duty;
     return rc;
 }
 
 int uiox_fan_hw_read_rpm(uiox_fan_hw_t *hw, uint8_t ch, uint16_t *rpm)
 {
     if (!hw || !hw->priv || !rpm || ch >= hw->num_fans) return -EINVAL;
     const uiox_fan_hw_ops_t *ops = (const uiox_fan_hw_ops_t *)hw->priv;
     if (!ops->read_rpm) return -ENOSYS;
     int rc = ops->read_rpm(hw, ch, rpm);
     if (rc == 0) {
         hw->chan[ch].rpm_measured = *rpm;
         hw->chan[ch].spinning     = (*rpm > 0u);
     }
     return rc;
 }
 
 int uiox_fan_hw_read_temp(uiox_fan_hw_t *hw, uint8_t ch, int16_t *temp_dc)
 {
     if (!hw || !hw->priv || !temp_dc || ch >= hw->num_temps) return -EINVAL;
     const uiox_fan_hw_ops_t *ops = (const uiox_fan_hw_ops_t *)hw->priv;
     if (!ops->read_temp) return -ENOSYS;
     int rc = ops->read_temp(hw, ch, temp_dc);
     if (rc == 0) hw->temp_dc[ch] = *temp_dc;
     return rc;
 }
 
 int uiox_fan_hw_fault_status(uiox_fan_hw_t *hw, uint32_t *flags)
 {
     if (!hw || !hw->priv || !flags) return -EINVAL;
     const uiox_fan_hw_ops_t *ops = (const uiox_fan_hw_ops_t *)hw->priv;
     if (!ops->fault_status) return -ENOSYS;
     int rc = ops->fault_status(hw, flags);
     if (rc == 0) hw->global_fault = *flags;
     return rc;
 }
 
 int uiox_fan_hw_fault_clear(uiox_fan_hw_t *hw, uint32_t flags)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_fan_hw_ops_t *ops = (const uiox_fan_hw_ops_t *)hw->priv;
     if (!ops->fault_clear) return -ENOSYS;
     return ops->fault_clear(hw, flags);
 }
 
 int uiox_fan_hw_chan_enable(uiox_fan_hw_t *hw, uint8_t ch, bool en)
 {
     if (!hw || !hw->priv || ch >= hw->num_fans) return -EINVAL;
     const uiox_fan_hw_ops_t *ops = (const uiox_fan_hw_ops_t *)hw->priv;
     if (!ops->chan_enable) return -ENOSYS;
     int rc = ops->chan_enable(hw, ch, en);
     if (rc == 0) hw->chan[ch].enabled = en;
     return rc;
 }
 