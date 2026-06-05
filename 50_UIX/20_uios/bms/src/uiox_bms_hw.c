/**
 * @file    uiox_bms_hw.c
 * @brief   UIOX BMS HAL implementation.
 * @date    2026-06-04
 */

 #include "uiox_bms_hw.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_bms_hw_init(uiox_bms_hw_t *hw, const uiox_bms_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv        = (void *)ops;
     hw->fault_flags = 0u;
     hw->chg_fet_on  = false;
     hw->dsg_fet_on  = false;
     hw->present     = false;
     hw->initialised = false;
     memset(hw->cell_mv,  0, sizeof(hw->cell_mv));
     memset(hw->temp_dc,  0, sizeof(hw->temp_dc));
     hw->pack_mv     = 0;
     hw->current_ma  = 0;
     hw->coulombs_mah= 0;
     int rc = ops->init(hw);
     if (rc == 0) hw->initialised = true;
     return rc;
 }
 
 void uiox_bms_hw_deinit(uiox_bms_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_bms_hw_ops_t *ops = (const uiox_bms_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv        = NULL;
     hw->initialised = false;
 }
 
 int uiox_bms_hw_measure_cells(uiox_bms_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_bms_hw_ops_t *ops = (const uiox_bms_hw_ops_t *)hw->priv;
     if (!ops->measure_cells) return -ENOSYS;
     return ops->measure_cells(hw);
 }
 
 int uiox_bms_hw_measure_current(uiox_bms_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_bms_hw_ops_t *ops = (const uiox_bms_hw_ops_t *)hw->priv;
     if (!ops->measure_current) return -ENOSYS;
     return ops->measure_current(hw);
 }
 
 int uiox_bms_hw_measure_temp(uiox_bms_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_bms_hw_ops_t *ops = (const uiox_bms_hw_ops_t *)hw->priv;
     if (!ops->measure_temp) return -ENOSYS;
     return ops->measure_temp(hw);
 }
 
 int uiox_bms_hw_set_chg_fet(uiox_bms_hw_t *hw, bool on)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_bms_hw_ops_t *ops = (const uiox_bms_hw_ops_t *)hw->priv;
     if (!ops->set_chg_fet) return -ENOSYS;
     int rc = ops->set_chg_fet(hw, on);
     if (rc == 0) hw->chg_fet_on = on;
     return rc;
 }
 
 int uiox_bms_hw_set_dsg_fet(uiox_bms_hw_t *hw, bool on)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_bms_hw_ops_t *ops = (const uiox_bms_hw_ops_t *)hw->priv;
     if (!ops->set_dsg_fet) return -ENOSYS;
     int rc = ops->set_dsg_fet(hw, on);
     if (rc == 0) hw->dsg_fet_on = on;
     return rc;
 }
 
 int uiox_bms_hw_set_balance(uiox_bms_hw_t *hw, uint16_t mask)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_bms_hw_ops_t *ops = (const uiox_bms_hw_ops_t *)hw->priv;
     if (!ops->set_balance) return -ENOSYS;
     return ops->set_balance(hw, mask);
 }
 
 int uiox_bms_hw_fault_status(uiox_bms_hw_t *hw, uint32_t *flags)
 {
     if (!hw || !hw->priv || !flags) return -EINVAL;
     const uiox_bms_hw_ops_t *ops = (const uiox_bms_hw_ops_t *)hw->priv;
     if (!ops->fault_status) return -ENOSYS;
     int rc = ops->fault_status(hw, flags);
     if (rc == 0) hw->fault_flags = *flags;
     return rc;
 }
 
 int uiox_bms_hw_fault_clear(uiox_bms_hw_t *hw, uint32_t flags)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_bms_hw_ops_t *ops = (const uiox_bms_hw_ops_t *)hw->priv;
     if (!ops->fault_clear) return -ENOSYS;
     return ops->fault_clear(hw, flags);
 }
 
 bool uiox_bms_hw_pack_present(uiox_bms_hw_t *hw)
 {
     if (!hw || !hw->priv) return false;
     const uiox_bms_hw_ops_t *ops = (const uiox_bms_hw_ops_t *)hw->priv;
     if (ops->pack_present) hw->present = ops->pack_present(hw);
     return hw->present;
 }
 