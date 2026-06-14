/**
 * @file  uiox_als_hw.c
 * @brief UIOX ALS HAL implementation.
 * @date  2026-06-11
 */

 #include "uiox_als_hw.h"
 #include <string.h>
 #include <errno.h>
 
 #define OPS(hw) ((const uiox_als_hw_ops_t *)(hw)->priv)
 
 int uiox_als_hw_init(uiox_als_hw_t *hw, const uiox_als_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv        = (void *)ops;
     hw->pending_irq = 0u;
     hw->raw_als     = 0u;
     hw->raw_white   = 0u;
     hw->raw_ir      = 0u;
     hw->gain        = UIOX_ALS_GAIN_1X;
     hw->itime       = UIOX_ALS_ITIME_100MS;
     return ops->init(hw);
 }
 
 void uiox_als_hw_deinit(uiox_als_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     if (OPS(hw)->deinit) OPS(hw)->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_als_hw_power_on(uiox_als_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->power_on) return -ENOSYS;
     return OPS(hw)->power_on(hw);
 }
 
 void uiox_als_hw_power_off(uiox_als_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     if (OPS(hw)->power_off) OPS(hw)->power_off(hw);
 }
 
 int uiox_als_hw_reg_read(uiox_als_hw_t *hw, uint8_t reg, uint16_t *val)
 {
     if (!hw || !hw->priv || !val) return -EINVAL;
     if (!OPS(hw)->reg_read) return -ENOSYS;
     return OPS(hw)->reg_read(hw, reg, val);
 }
 
 int uiox_als_hw_reg_write(uiox_als_hw_t *hw, uint8_t reg, uint16_t val)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->reg_write) return -ENOSYS;
     return OPS(hw)->reg_write(hw, reg, val);
 }
 
 int uiox_als_hw_set_gain(uiox_als_hw_t *hw, uiox_als_gain_t g)
 {
     if (!hw || !hw->priv || g >= UIOX_ALS_GAIN_MAX) return -EINVAL;
     if (!OPS(hw)->set_gain) return -ENOSYS;
     int rc = OPS(hw)->set_gain(hw, g);
     if (rc == 0) hw->gain = g;
     return rc;
 }
 
 int uiox_als_hw_set_itime(uiox_als_hw_t *hw, uiox_als_itime_t t)
 {
     if (!hw || !hw->priv || t >= UIOX_ALS_ITIME_MAX) return -EINVAL;
     if (!OPS(hw)->set_itime) return -ENOSYS;
     int rc = OPS(hw)->set_itime(hw, t);
     if (rc == 0) hw->itime = t;
     return rc;
 }
 
 int uiox_als_hw_read_als(uiox_als_hw_t *hw,
                           uint16_t *als, uint16_t *white)
 {
     if (!hw || !hw->priv || !als || !white) return -EINVAL;
     if (!OPS(hw)->read_als) return -ENOSYS;
     int rc = OPS(hw)->read_als(hw, als, white);
     if (rc == 0) { hw->raw_als = *als; hw->raw_white = *white; }
     return rc;
 }
 
 int uiox_als_hw_read_ir(uiox_als_hw_t *hw, uint16_t *ir)
 {
     if (!hw || !hw->priv || !ir) return -EINVAL;
     if (!OPS(hw)->read_ir) return -ENOSYS;
     int rc = OPS(hw)->read_ir(hw, ir);
     if (rc == 0) hw->raw_ir = *ir;
     return rc;
 }
 
 int uiox_als_hw_set_threshold(uiox_als_hw_t *hw,
                                uint16_t low, uint16_t high)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->set_threshold) return -ENOSYS;
     int rc = OPS(hw)->set_threshold(hw, low, high);
     if (rc == 0) { hw->thresh_low = low; hw->thresh_high = high; }
     return rc;
 }
 
 int uiox_als_hw_int_enable(uiox_als_hw_t *hw, bool en)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->int_enable) return -ENOSYS;
     return OPS(hw)->int_enable(hw, en);
 }
 
 int uiox_als_hw_int_clear(uiox_als_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->int_clear) return -ENOSYS;
     return OPS(hw)->int_clear(hw);
 }
 
 int uiox_als_hw_trigger(uiox_als_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->trigger) return -ENOSYS;
     return OPS(hw)->trigger(hw);
 }
 