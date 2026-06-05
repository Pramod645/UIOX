/**
 * @file    uiox_pmic_hw.c
 * @brief   UIOX PMIC HAL implementation.
 * @date    2026-06-04
 */

 #include "uiox_pmic_hw.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_pmic_hw_init(uiox_pmic_hw_t *hw, const uiox_pmic_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv        = (void *)ops;
     hw->powered     = false;
     hw->fault       = false;
     hw->fault_flags = 0u;
     hw->die_temp_c  = 25;
     return ops->init(hw);
 }
 
 void uiox_pmic_hw_deinit(uiox_pmic_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_pmic_hw_ops_t *ops = (const uiox_pmic_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_pmic_hw_enable(uiox_pmic_hw_t *hw, bool on)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_pmic_hw_ops_t *ops = (const uiox_pmic_hw_ops_t *)hw->priv;
     if (!ops->enable) return -ENOSYS;
     int rc = ops->enable(hw, on);
     if (rc == 0) hw->powered = on;
     return rc;
 }
 
 int uiox_pmic_hw_reg_read(uiox_pmic_hw_t *hw, uint16_t reg, uint8_t *val)
 {
     if (!hw || !hw->priv || !val) return -EINVAL;
     const uiox_pmic_hw_ops_t *ops = (const uiox_pmic_hw_ops_t *)hw->priv;
     if (!ops->reg_read) return -ENOSYS;
     return ops->reg_read(hw, reg, val);
 }
 
 int uiox_pmic_hw_reg_write(uiox_pmic_hw_t *hw, uint16_t reg, uint8_t val)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_pmic_hw_ops_t *ops = (const uiox_pmic_hw_ops_t *)hw->priv;
     if (!ops->reg_write) return -ENOSYS;
     return ops->reg_write(hw, reg, val);
 }
 
 int uiox_pmic_hw_reg_update(uiox_pmic_hw_t *hw,
                              uint16_t reg, uint8_t mask, uint8_t val)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_pmic_hw_ops_t *ops = (const uiox_pmic_hw_ops_t *)hw->priv;
     if (!ops->reg_update) return -ENOSYS;
     return ops->reg_update(hw, reg, mask, val);
 }
 
 int uiox_pmic_hw_adc_read(uiox_pmic_hw_t *hw,
                            uiox_pmic_adc_ch_t ch, uint32_t *result)
 {
     if (!hw || !hw->priv || !result) return -EINVAL;
     const uiox_pmic_hw_ops_t *ops = (const uiox_pmic_hw_ops_t *)hw->priv;
     if (!ops->adc_read) return -ENOSYS;
     return ops->adc_read(hw, ch, result);
 }
 
 int uiox_pmic_hw_wdt_kick(uiox_pmic_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_pmic_hw_ops_t *ops = (const uiox_pmic_hw_ops_t *)hw->priv;
     if (!ops->wdt_kick) return -ENOSYS;
     return ops->wdt_kick(hw);
 }
 
 int uiox_pmic_hw_irq_status(uiox_pmic_hw_t *hw, uint32_t *flags)
 {
     if (!hw || !hw->priv || !flags) return -EINVAL;
     const uiox_pmic_hw_ops_t *ops = (const uiox_pmic_hw_ops_t *)hw->priv;
     if (!ops->irq_status) return -ENOSYS;
     int rc = ops->irq_status(hw, flags);
     if (rc == 0) hw->fault_flags = *flags;
     return rc;
 }
 
 int uiox_pmic_hw_irq_clear(uiox_pmic_hw_t *hw, uint32_t flags)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_pmic_hw_ops_t *ops = (const uiox_pmic_hw_ops_t *)hw->priv;
     if (!ops->irq_clear) return -ENOSYS;
     return ops->irq_clear(hw, flags);
 }
 