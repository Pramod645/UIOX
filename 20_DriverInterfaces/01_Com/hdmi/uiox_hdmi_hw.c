/**
 * @file    uiox_hdmi_hw.c
 * @brief   UIOX HDMI HAL — generic hardware lifecycle management.
 * @date    2026-05-28
 */

 #include "uiox_hdmi_hw.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_hdmi_hw_init(uiox_hdmi_hw_t *hw, const uiox_hdmi_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv         = (void *)ops;
     hw->connected    = false;
     hw->enabled      = false;
     hw->hdcp_state   = UIOX_HDCP_DISABLED;
     hw->vblank_count = 0u;
     hw->flip_pending = false;
     hw->bl_level     = 255u;
     return ops->init(hw);
 }
 
 void uiox_hdmi_hw_deinit(uiox_hdmi_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_hdmi_hw_ops_t *ops = (const uiox_hdmi_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_hdmi_hw_enable(uiox_hdmi_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_hdmi_hw_ops_t *ops = (const uiox_hdmi_hw_ops_t *)hw->priv;
     if (!ops->enable) return -ENOSYS;
     int rc = ops->enable(hw);
     if (rc == 0) hw->enabled = true;
     return rc;
 }
 
 void uiox_hdmi_hw_disable(uiox_hdmi_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_hdmi_hw_ops_t *ops = (const uiox_hdmi_hw_ops_t *)hw->priv;
     if (ops->disable) ops->disable(hw);
     hw->enabled = false;
 }
 
 int uiox_hdmi_hw_set_timing(uiox_hdmi_hw_t *hw,
                              const uiox_hdmi_timing_t *t)
 {
     if (!hw || !hw->priv || !t) return -EINVAL;
     const uiox_hdmi_hw_ops_t *ops = (const uiox_hdmi_hw_ops_t *)hw->priv;
     if (!ops->set_timing) return -ENOSYS;
     int rc = ops->set_timing(hw, t);
     if (rc == 0) memcpy(&hw->timing, t, sizeof(*t));
     return rc;
 }
 
 int uiox_hdmi_hw_flip(uiox_hdmi_hw_t *hw, uintptr_t phys, uint32_t stride)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_hdmi_hw_ops_t *ops = (const uiox_hdmi_hw_ops_t *)hw->priv;
     if (!ops->flip) return -ENOSYS;
     hw->flip_pending = true;
     return ops->flip(hw, phys, stride);
 }
 
 int uiox_hdmi_hw_wait_vblank(uiox_hdmi_hw_t *hw, uint32_t timeout_ms)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_hdmi_hw_ops_t *ops = (const uiox_hdmi_hw_ops_t *)hw->priv;
     if (!ops->wait_vblank) return -ENOSYS;
     int rc = ops->wait_vblank(hw, timeout_ms);
     if (rc == 0) hw->flip_pending = false;
     return rc;
 }
 
 int uiox_hdmi_hw_ddc_read(uiox_hdmi_hw_t *hw, uint8_t dev_addr,
                            uint8_t reg, uint8_t *buf, uint16_t len)
 {
     if (!hw || !hw->priv || !buf) return -EINVAL;
     const uiox_hdmi_hw_ops_t *ops = (const uiox_hdmi_hw_ops_t *)hw->priv;
     if (!ops->ddc_read) return -ENOSYS;
     return ops->ddc_read(hw, dev_addr, reg, buf, len);
 }
 
 bool uiox_hdmi_hw_connected(uiox_hdmi_hw_t *hw)
 {
     if (!hw || !hw->priv) return false;
     const uiox_hdmi_hw_ops_t *ops = (const uiox_hdmi_hw_ops_t *)hw->priv;
     if (ops->hpd_state) hw->connected = ops->hpd_state(hw);
     return hw->connected;
 }
 