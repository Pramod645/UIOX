/**
 * @file    uiox_mon_hw.c
 * @brief   UIOX Monitor HAL — generic hardware lifecycle management.
 * @date    2026-05-27
 */

 #include "uiox_mon_hw.h"
 
 int uiox_mon_hw_init(uiox_mon_hw_t *hw, const uiox_mon_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv         = (void *)ops;
     hw->bl_level     = 255u;
     hw->dpms         = UIOX_MON_DPMS_OFF;
     hw->connected    = false;
     hw->enabled      = false;
     hw->vblank_count = 0u;
     hw->flip_pending = false;
     return ops->init(hw);
 }
 
 void uiox_mon_hw_deinit(uiox_mon_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_mon_hw_ops_t *ops = (const uiox_mon_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_mon_hw_enable(uiox_mon_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_mon_hw_ops_t *ops = (const uiox_mon_hw_ops_t *)hw->priv;
     if (!ops->enable) return -ENOSYS;
     int rc = ops->enable(hw);
     if (rc == 0) hw->enabled = true;
     return rc;
 }
 
 void uiox_mon_hw_disable(uiox_mon_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_mon_hw_ops_t *ops = (const uiox_mon_hw_ops_t *)hw->priv;
     if (ops->disable) ops->disable(hw);
     hw->enabled = false;
 }
 
 int uiox_mon_hw_set_timing(uiox_mon_hw_t *hw,
                             const uiox_mon_timing_t *t)
 {
     if (!hw || !hw->priv || !t) return -EINVAL;
     const uiox_mon_hw_ops_t *ops = (const uiox_mon_hw_ops_t *)hw->priv;
     if (!ops->set_timing) return -ENOSYS;
     int rc = ops->set_timing(hw, t);
     if (rc == 0) memcpy(&hw->timing, t, sizeof(*t));
     return rc;
 }
 
 int uiox_mon_hw_flip(uiox_mon_hw_t *hw, uintptr_t phys, uint32_t stride)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_mon_hw_ops_t *ops = (const uiox_mon_hw_ops_t *)hw->priv;
     if (!ops->flip) return -ENOSYS;
     hw->flip_pending = true;
     return ops->flip(hw, phys, stride);
 }
 
 int uiox_mon_hw_wait_vblank(uiox_mon_hw_t *hw, uint32_t timeout_ms)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_mon_hw_ops_t *ops = (const uiox_mon_hw_ops_t *)hw->priv;
     if (!ops->wait_vblank) return -ENOSYS;
     int rc = ops->wait_vblank(hw, timeout_ms);
     if (rc == 0) hw->flip_pending = false;
     return rc;
 }
 
 int uiox_mon_hw_read_edid(uiox_mon_hw_t *hw, uint8_t *buf)
 {
     if (!hw || !hw->priv || !buf) return -EINVAL;
     const uiox_mon_hw_ops_t *ops = (const uiox_mon_hw_ops_t *)hw->priv;
     if (!ops->read_edid) return -ENOSYS;
     return ops->read_edid(hw, buf);
 }
 
 int uiox_mon_hw_set_dpms(uiox_mon_hw_t *hw, uiox_mon_dpms_t dpms)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_mon_hw_ops_t *ops = (const uiox_mon_hw_ops_t *)hw->priv;
     if (!ops->set_dpms) return -ENOSYS;
     int rc = ops->set_dpms(hw, dpms);
     if (rc == 0) hw->dpms = dpms;
     return rc;
 }
 
 int uiox_mon_hw_set_backlight(uiox_mon_hw_t *hw, uint8_t level)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_mon_hw_ops_t *ops = (const uiox_mon_hw_ops_t *)hw->priv;
     if (!ops->set_backlight) return -ENOSYS;
     int rc = ops->set_backlight(hw, level);
     if (rc == 0) hw->bl_level = level;
     return rc;
 }
 
 bool uiox_mon_hw_connected(uiox_mon_hw_t *hw)
 {
     if (!hw || !hw->priv) return false;
     const uiox_mon_hw_ops_t *ops = (const uiox_mon_hw_ops_t *)hw->priv;
     if (ops->hotplug_state)
         hw->connected = ops->hotplug_state(hw);
     return hw->connected;
 }
 