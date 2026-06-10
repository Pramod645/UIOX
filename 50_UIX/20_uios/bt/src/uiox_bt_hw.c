/**
 * @file    uiox_bt_hw.c
 * @brief   UIOX Bluetooth HAL implementation.
 * @date    2026-06-09
 */

 #include "uiox_bt_hw.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_bt_hw_init(uiox_bt_hw_t *hw, const uiox_bt_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv               = (void *)ops;
     hw->powered            = false;
     hw->initialised        = false;
     hw->rx_len             = 0;
     hw->rx_pending         = false;
     hw->host_wake_pending  = false;
     return ops->init(hw);
 }
 
 void uiox_bt_hw_deinit(uiox_bt_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_bt_hw_ops_t *ops = (const uiox_bt_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_bt_hw_power(uiox_bt_hw_t *hw, bool on)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_bt_hw_ops_t *ops = (const uiox_bt_hw_ops_t *)hw->priv;
     if (!ops->power) return -ENOSYS;
     int rc = ops->power(hw, on);
     if (rc == 0) hw->powered = on;
     return rc;
 }
 
 int uiox_bt_hw_hci_write(uiox_bt_hw_t *hw,
                           const uint8_t *buf, uint16_t len)
 {
     if (!hw || !hw->priv || !buf || !len) return -EINVAL;
     const uiox_bt_hw_ops_t *ops = (const uiox_bt_hw_ops_t *)hw->priv;
     if (!ops->hci_write) return -ENOSYS;
     return ops->hci_write(hw, buf, len);
 }
 
 int uiox_bt_hw_hci_read(uiox_bt_hw_t *hw,
                          uint8_t *buf, uint16_t max_len)
 {
     if (!hw || !hw->priv || !buf) return -EINVAL;
     const uiox_bt_hw_ops_t *ops = (const uiox_bt_hw_ops_t *)hw->priv;
     if (!ops->hci_read) return -ENOSYS;
     return ops->hci_read(hw, buf, max_len);
 }
 
 int uiox_bt_hw_fw_download(uiox_bt_hw_t *hw,
                             const uint8_t *fw, uint32_t size)
 {
     if (!hw || !hw->priv || !fw || !size) return -EINVAL;
     const uiox_bt_hw_ops_t *ops = (const uiox_bt_hw_ops_t *)hw->priv;
     if (!ops->fw_download) return -ENOSYS;
     return ops->fw_download(hw, fw, size);
 }
 