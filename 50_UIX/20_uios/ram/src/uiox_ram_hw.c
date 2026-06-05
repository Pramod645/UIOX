/**
 * @file    uiox_ram_hw.c
 * @brief   UIOX RAM HAL implementation.
 * @date    2026-06-03
 */

 #include "uiox_ram_hw.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_ram_hw_init(uiox_ram_hw_t *hw, const uiox_ram_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv         = (void *)ops;
     hw->pwr_state    = UIOX_RAM_PWR_ACTIVE;
     hw->initialised  = false;
     hw->ecc_ce_count = 0u;
     hw->ecc_ue_count = 0u;
     int rc = ops->init(hw);
     if (rc == 0) hw->initialised = true;
     return rc;
 }
 
 void uiox_ram_hw_deinit(uiox_ram_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_ram_hw_ops_t *ops = (const uiox_ram_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv        = NULL;
     hw->initialised = false;
 }
 
 int uiox_ram_hw_train(uiox_ram_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_ram_hw_ops_t *ops = (const uiox_ram_hw_ops_t *)hw->priv;
     if (!ops->train) return -ENOSYS;
     return ops->train(hw);
 }
 
 int uiox_ram_hw_set_power(uiox_ram_hw_t *hw, uiox_ram_pwr_t state)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_ram_hw_ops_t *ops = (const uiox_ram_hw_ops_t *)hw->priv;
     if (!ops->set_power) return -ENOSYS;
     int rc = ops->set_power(hw, state);
     if (rc == 0) hw->pwr_state = state;
     return rc;
 }
 
 int uiox_ram_hw_ecc_enable(uiox_ram_hw_t *hw, bool enable)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_ram_hw_ops_t *ops = (const uiox_ram_hw_ops_t *)hw->priv;
     if (!ops->ecc_enable) return -ENOSYS;
     return ops->ecc_enable(hw, enable);
 }
 
 int uiox_ram_hw_ecc_scrub(uiox_ram_hw_t *hw,
                            uint64_t phys_start, uint64_t size)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_ram_hw_ops_t *ops = (const uiox_ram_hw_ops_t *)hw->priv;
     if (!ops->ecc_scrub) return -ENOSYS;
     return ops->ecc_scrub(hw, phys_start, size);
 }
 
 int uiox_ram_hw_ecc_status(uiox_ram_hw_t *hw,
                             uint32_t *ce, uint32_t *ue, uint64_t *addr)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_ram_hw_ops_t *ops = (const uiox_ram_hw_ops_t *)hw->priv;
     if (!ops->ecc_status) return -ENOSYS;
     return ops->ecc_status(hw, ce, ue, addr);
 }
 
 int uiox_ram_hw_zq_cal(uiox_ram_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_ram_hw_ops_t *ops = (const uiox_ram_hw_ops_t *)hw->priv;
     if (!ops->zq_calibrate) return -ENOSYS;
     return ops->zq_calibrate(hw);
 }
 