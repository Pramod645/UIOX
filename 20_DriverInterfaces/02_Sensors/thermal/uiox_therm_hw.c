/**
 * @file    uiox_therm_hw.c
 * @brief   UIOX Thermal Sensor HAL implementation.
 * @date    2026-06-05
 */

 #include "uiox_therm_hw.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_therm_hw_init(uiox_therm_hw_t *hw,
                         const uiox_therm_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv           = (void *)ops;
     hw->alert_pending  = false;
     hw->initialised    = false;
     for (uint8_t i = 0; i < UIOX_THERM_MAX_CHANNELS; i++) {
         hw->meas[i].temp_dc     = 0;
         hw->meas[i].alert_active= false;
         hw->meas[i].valid       = false;
     }
     int rc = ops->init(hw);
     if (rc == 0) hw->initialised = true;
     return rc;
 }
 
 void uiox_therm_hw_deinit(uiox_therm_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_therm_hw_ops_t *ops = (const uiox_therm_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv        = NULL;
     hw->initialised = false;
 }
 
 int uiox_therm_hw_read_temp(uiox_therm_hw_t *hw,
                              uint8_t ch, int16_t *temp_dc)
 {
     if (!hw || !hw->priv || !temp_dc || ch >= hw->num_channels)
         return -EINVAL;
     const uiox_therm_hw_ops_t *ops = (const uiox_therm_hw_ops_t *)hw->priv;
     if (!ops->read_temp) return -ENOSYS;
     int rc = ops->read_temp(hw, ch, temp_dc);
     if (rc == 0) {
         hw->meas[ch].temp_dc = *temp_dc;
         hw->meas[ch].valid   = true;
     }
     return rc;
 }
 
 int uiox_therm_hw_set_t_high(uiox_therm_hw_t *hw, int16_t temp_dc)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_therm_hw_ops_t *ops = (const uiox_therm_hw_ops_t *)hw->priv;
     if (!ops->set_t_high) return -ENOSYS;
     int rc = ops->set_t_high(hw, temp_dc);
     if (rc == 0) hw->t_high_dc = temp_dc;
     return rc;
 }
 
 int uiox_therm_hw_set_t_hyst(uiox_therm_hw_t *hw, int16_t temp_dc)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_therm_hw_ops_t *ops = (const uiox_therm_hw_ops_t *)hw->priv;
     if (!ops->set_t_hyst) return -ENOSYS;
     int rc = ops->set_t_hyst(hw, temp_dc);
     if (rc == 0) hw->t_hyst_dc = temp_dc;
     return rc;
 }
 
 int uiox_therm_hw_set_t_crit(uiox_therm_hw_t *hw, int16_t temp_dc)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_therm_hw_ops_t *ops = (const uiox_therm_hw_ops_t *)hw->priv;
     if (!ops->set_t_crit) return -ENOSYS;
     int rc = ops->set_t_crit(hw, temp_dc);
     if (rc == 0) hw->t_crit_dc = temp_dc;
     return rc;
 }
 
 int uiox_therm_hw_alert_clear(uiox_therm_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_therm_hw_ops_t *ops = (const uiox_therm_hw_ops_t *)hw->priv;
     if (!ops->alert_clear) return -ENOSYS;
     hw->alert_pending = false;
     return ops->alert_clear(hw);
 }
 