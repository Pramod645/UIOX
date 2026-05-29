/**
 * @file    uiox_can_hw.c
 * @brief   UIOX CAN HAL — generic hardware lifecycle management.
 * @date    2026-05-26
 */

 #include "uiox_can_hw.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_can_hw_init(uiox_can_hw_t *hw, const uiox_can_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv      = (void *)ops;
     hw->tx_head   = 0;
     hw->tx_tail   = 0;
     hw->rx_head   = 0;
     hw->err_state = UIOX_CAN_ERR_ACTIVE;
     memset(&hw->err_cnt, 0, sizeof(hw->err_cnt));
     return ops->init(hw);
 }
 
 int uiox_can_hw_start(uiox_can_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_can_hw_ops_t *ops = (const uiox_can_hw_ops_t *)hw->priv;
     return ops->start ? ops->start(hw) : 0;
 }
 
 void uiox_can_hw_stop(uiox_can_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_can_hw_ops_t *ops = (const uiox_can_hw_ops_t *)hw->priv;
     if (ops->stop) ops->stop(hw);
 }
 
 void uiox_can_hw_deinit(uiox_can_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_can_hw_ops_t *ops = (const uiox_can_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_can_hw_set_mode(uiox_can_hw_t *hw, uiox_can_mode_t mode)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_can_hw_ops_t *ops = (const uiox_can_hw_ops_t *)hw->priv;
     if (!ops->set_mode) return -ENOSYS;
     hw->mode = mode;
     return ops->set_mode(hw, mode);
 }
 
 int uiox_can_hw_set_filter(uiox_can_hw_t *hw, uint8_t idx,
                             uint32_t id, uint32_t mask, bool ext)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_can_hw_ops_t *ops = (const uiox_can_hw_ops_t *)hw->priv;
     if (!ops->set_filter) return -ENOSYS;
     return ops->set_filter(hw, idx, id, mask, ext);
 }
 
 int uiox_can_hw_get_err_cnt(uiox_can_hw_t *hw, uiox_can_err_cnt_t *out)
 {
     if (!hw || !hw->priv || !out) return -EINVAL;
     const uiox_can_hw_ops_t *ops = (const uiox_can_hw_ops_t *)hw->priv;
     if (!ops->get_err_cnt) return -ENOSYS;
     return ops->get_err_cnt(hw, out);
 }
 
 int uiox_can_hw_recover(uiox_can_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_can_hw_ops_t *ops = (const uiox_can_hw_ops_t *)hw->priv;
     if (!ops->recover) return -ENOSYS;
     return ops->recover(hw);
 }
 