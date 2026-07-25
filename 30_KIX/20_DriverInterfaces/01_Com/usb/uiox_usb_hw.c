/**
 * @file    uiox_usb_hw.c
 * @brief   UIOX USB HAL — generic hardware lifecycle management.
 * @date    2026-05-28
 */

 #include "uiox_usb_hw.h"
 
 int uiox_usb_hw_init(uiox_usb_hw_t *hw, const uiox_usb_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv      = (void *)ops;
     hw->address   = 0u;
     hw->connected = false;
     hw->suspended = false;
     memset(hw->ep, 0, sizeof(hw->ep));
     return ops->init(hw);
 }
 
 void uiox_usb_hw_deinit(uiox_usb_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_usb_hw_ops_t *ops = (const uiox_usb_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_usb_hw_start(uiox_usb_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_usb_hw_ops_t *ops = (const uiox_usb_hw_ops_t *)hw->priv;
     return ops->start ? ops->start(hw) : 0;
 }
 
 void uiox_usb_hw_stop(uiox_usb_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_usb_hw_ops_t *ops = (const uiox_usb_hw_ops_t *)hw->priv;
     if (ops->stop) ops->stop(hw);
 }
 
 int uiox_usb_hw_ep_config(uiox_usb_hw_t *hw, uint8_t ep_addr,
                            uiox_usb_ep_type_t type, uint16_t mps,
                            uint8_t interval)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_usb_hw_ops_t *ops = (const uiox_usb_hw_ops_t *)hw->priv;
     if (!ops->ep_config) return -ENOSYS;
     int rc = ops->ep_config(hw, ep_addr, type, mps, interval);
     if (rc == 0) {
         uint8_t idx = ep_addr & 0x0Fu;
         if (idx < UIOX_USB_MAX_EP) {
             hw->ep[idx].addr     = ep_addr;
             hw->ep[idx].type     = type;
             hw->ep[idx].mps      = mps;
             hw->ep[idx].interval = interval;
             hw->ep[idx].active   = true;
         }
     }
     return rc;
 }
 
 int uiox_usb_hw_ep_stall(uiox_usb_hw_t *hw, uint8_t ep_addr, bool stall)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_usb_hw_ops_t *ops = (const uiox_usb_hw_ops_t *)hw->priv;
     if (!ops->ep_stall) return -ENOSYS;
     return ops->ep_stall(hw, ep_addr, stall);
 }
 
 int uiox_usb_hw_tx(uiox_usb_hw_t *hw, uint8_t ep_addr,
                     uintptr_t phys, uint32_t len)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_usb_hw_ops_t *ops = (const uiox_usb_hw_ops_t *)hw->priv;
     if (!ops->tx_submit) return -ENOSYS;
     return ops->tx_submit(hw, ep_addr, phys, len);
 }
 
 int uiox_usb_hw_rx(uiox_usb_hw_t *hw, uint8_t ep_addr,
                     uintptr_t phys, uint32_t len)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_usb_hw_ops_t *ops = (const uiox_usb_hw_ops_t *)hw->priv;
     if (!ops->rx_submit) return -ENOSYS;
     return ops->rx_submit(hw, ep_addr, phys, len);
 }
 
 bool uiox_usb_hw_connected(uiox_usb_hw_t *hw)
 {
     if (!hw || !hw->priv) return false;
     const uiox_usb_hw_ops_t *ops = (const uiox_usb_hw_ops_t *)hw->priv;
     if (ops->vbus_sense) hw->vbus_present = ops->vbus_sense(hw);
     return hw->connected;
 }
 