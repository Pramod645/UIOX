/**
 * @file    uiox_tpwd_hw.c
 * @brief   UIOX Touch-Password HAL implementation.
 * @date    2026-06-01
 */

 #include "uiox_tpwd_hw.h"
 
 int uiox_tpwd_hw_init(uiox_tpwd_hw_t *hw, const uiox_tpwd_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->ops        = (void *)ops;
     hw->powered     = false;
     hw->irq_pending = false;
     return ops->init(hw);
 }
 
 void uiox_tpwd_hw_deinit(uiox_tpwd_hw_t *hw)
 {
     if (!hw || !hw->ops) return;
     const uiox_tpwd_hw_ops_t *ops = (const uiox_tpwd_hw_ops_t *)hw->ops;
     if (ops->deinit) ops->deinit(hw);
     hw->ops = NULL;
 }
 
 int uiox_tpwd_hw_power(uiox_tpwd_hw_t *hw, bool on)
 {
     if (!hw || !hw->ops) return -EINVAL;
     const uiox_tpwd_hw_ops_t *ops = (const uiox_tpwd_hw_ops_t *)hw->ops;
     if (!ops->power) return -ENOSYS;
     int rc = ops->power(hw, on);
     if (rc == 0) hw->powered = on;
     return rc;
 }
 
 int uiox_tpwd_hw_reset(uiox_tpwd_hw_t *hw)
 {
     if (!hw || !hw->ops) return -EINVAL;
     const uiox_tpwd_hw_ops_t *ops = (const uiox_tpwd_hw_ops_t *)hw->ops;
     if (!ops->reset) return -ENOSYS;
     return ops->reset(hw);
 }
 
 int uiox_tpwd_hw_read_touch(uiox_tpwd_hw_t *hw, uiox_tpwd_raw_evt_t *evt)
 {
     if (!hw || !hw->ops || !evt) return -EINVAL;
     const uiox_tpwd_hw_ops_t *ops = (const uiox_tpwd_hw_ops_t *)hw->ops;
     if (!ops->read_touch) return -ENOSYS;
     hw->irq_pending = false;
     return ops->read_touch(hw, evt);
 }
 
 int uiox_tpwd_hw_set_backlight(uiox_tpwd_hw_t *hw, uint8_t level)
 {
     if (!hw || !hw->ops) return -EINVAL;
     const uiox_tpwd_hw_ops_t *ops = (const uiox_tpwd_hw_ops_t *)hw->ops;
     if (!ops->set_backlight) return -ENOSYS;
     return ops->set_backlight(hw, level);
 }
 