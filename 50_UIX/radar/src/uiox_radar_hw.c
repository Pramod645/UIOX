/**
 * @file    uiox_radar_hw.c
 * @brief   UIOX Radar HAL — generic hardware lifecycle management.
 * @date    2026-05-26
 */

 #include "uiox_radar_hw.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_radar_hw_init(uiox_radar_hw_t *hw, const uiox_radar_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
 
     /* Store ops in priv slot */
     hw->priv    = (void *)ops;
     hw->rx_head = 0;
     hw->rx_tail = 0;
 
     return ops->init(hw);
 }
 
 int uiox_radar_hw_start(uiox_radar_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_radar_hw_ops_t *ops = (const uiox_radar_hw_ops_t *)hw->priv;
     return ops->start ? ops->start(hw) : 0;
 }
 
 void uiox_radar_hw_stop(uiox_radar_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_radar_hw_ops_t *ops = (const uiox_radar_hw_ops_t *)hw->priv;
     if (ops->stop) ops->stop(hw);
 }
 
 void uiox_radar_hw_deinit(uiox_radar_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_radar_hw_ops_t *ops = (const uiox_radar_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv = NULL;
 }
 