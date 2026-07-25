/**
 * @file    uiox_mouse_hw.c
 * @brief   UIOX Mouse HAL — generic hardware lifecycle management.
 * @date    2026-06-01
 */

 #include "uiox_mouse_hw.h"
 
 int uiox_mouse_hw_init(uiox_mouse_hw_t *hw, const uiox_mouse_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv        = (void *)ops;
     hw->irq_pending = false;
     hw->connected   = false;
     return ops->init(hw);
 }
 
 void uiox_mouse_hw_deinit(uiox_mouse_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_mouse_hw_ops_t *ops = (const uiox_mouse_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_mouse_hw_enable(uiox_mouse_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_mouse_hw_ops_t *ops = (const uiox_mouse_hw_ops_t *)hw->priv;
     if (!ops->enable) return -ENOSYS;
     return ops->enable(hw);
 }
 
 void uiox_mouse_hw_disable(uiox_mouse_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_mouse_hw_ops_t *ops = (const uiox_mouse_hw_ops_t *)hw->priv;
     if (ops->disable) ops->disable(hw);
 }
 
 int uiox_mouse_hw_read_report(uiox_mouse_hw_t *hw, uiox_mouse_raw_t *raw)
 {
     if (!hw || !hw->priv || !raw) return -EINVAL;
     const uiox_mouse_hw_ops_t *ops = (const uiox_mouse_hw_ops_t *)hw->priv;
     if (!ops->read_report) return -ENOSYS;
     hw->irq_pending = false;
     return ops->read_report(hw, raw);
 }
 
 bool uiox_mouse_hw_connected(uiox_mouse_hw_t *hw)
 {
     if (!hw || !hw->priv) return false;
     const uiox_mouse_hw_ops_t *ops = (const uiox_mouse_hw_ops_t *)hw->priv;
     if (ops->connected) hw->connected = ops->connected(hw);
     return hw->connected;
 }
 