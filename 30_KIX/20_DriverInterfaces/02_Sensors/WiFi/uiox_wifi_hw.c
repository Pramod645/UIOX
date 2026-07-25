/**
 * @file    uiox_wifi_hw.c
 * @brief   UIOX WiFi HAL — generic hardware lifecycle management.
 * @date    2026-05-28
 */

 #include "uiox_wifi_hw.h"
 
 int uiox_wifi_hw_init(uiox_wifi_hw_t *hw, const uiox_wifi_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv      = (void *)ops;
     hw->tx_head   = 0;
     hw->tx_tail   = 0;
     hw->rx_head   = 0;
     hw->associated= false;
     hw->rssi_dbm  = -100;
     return ops->init(hw);
 }
 
 void uiox_wifi_hw_deinit(uiox_wifi_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_wifi_hw_ops_t *ops = (const uiox_wifi_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_wifi_hw_start(uiox_wifi_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_wifi_hw_ops_t *ops = (const uiox_wifi_hw_ops_t *)hw->priv;
     return ops->start ? ops->start(hw) : 0;
 }
 
 void uiox_wifi_hw_stop(uiox_wifi_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_wifi_hw_ops_t *ops = (const uiox_wifi_hw_ops_t *)hw->priv;
     if (ops->stop) ops->stop(hw);
 }
 
 int uiox_wifi_hw_set_channel(uiox_wifi_hw_t *hw,
                               const uiox_wifi_channel_t *ch)
 {
     if (!hw || !hw->priv || !ch) return -EINVAL;
     const uiox_wifi_hw_ops_t *ops = (const uiox_wifi_hw_ops_t *)hw->priv;
     if (!ops->set_channel) return -ENOSYS;
     int rc = ops->set_channel(hw, ch);
     if (rc == 0) memcpy(&hw->channel, ch, sizeof(*ch));
     return rc;
 }
 
 int uiox_wifi_hw_set_mode(uiox_wifi_hw_t *hw, uiox_wifi_mode_t mode)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_wifi_hw_ops_t *ops = (const uiox_wifi_hw_ops_t *)hw->priv;
     if (!ops->set_mode) return -ENOSYS;
     int rc = ops->set_mode(hw, mode);
     if (rc == 0) hw->mode = mode;
     return rc;
 }
 
 int uiox_wifi_hw_get_rssi(uiox_wifi_hw_t *hw, int8_t *rssi_dbm)
 {
     if (!hw || !hw->priv || !rssi_dbm) return -EINVAL;
     const uiox_wifi_hw_ops_t *ops = (const uiox_wifi_hw_ops_t *)hw->priv;
     if (!ops->get_rssi) return -ENOSYS;
     int rc = ops->get_rssi(hw, rssi_dbm);
     if (rc == 0) hw->rssi_dbm = *rssi_dbm;
     return rc;
 }
 