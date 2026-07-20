/**
 * @file    uiox_tb4_hw.c
 * @brief   UIOX Thunderbolt 4 HAL implementation.
 * @date    2026-06-08
 */

 #include "uiox_tb4_hw.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_tb4_hw_init(uiox_tb4_hw_t *hw, const uiox_tb4_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv        = (void *)ops;
     hw->powered     = false;
     hw->icm_ready   = false;
     hw->pending_irq = 0u;
     hw->tx_prod = hw->tx_cons = 0;
     hw->rx_prod = hw->rx_cons = 0;
     return ops->init(hw);
 }
 
 void uiox_tb4_hw_deinit(uiox_tb4_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_tb4_hw_ops_t *ops = (const uiox_tb4_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_tb4_hw_power_on(uiox_tb4_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_tb4_hw_ops_t *ops = (const uiox_tb4_hw_ops_t *)hw->priv;
     if (!ops->power_on) return -ENOSYS;
     int rc = ops->power_on(hw);
     if (rc == 0) hw->powered = true;
     return rc;
 }
 
 void uiox_tb4_hw_power_off(uiox_tb4_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_tb4_hw_ops_t *ops = (const uiox_tb4_hw_ops_t *)hw->priv;
     if (ops->power_off) ops->power_off(hw);
     hw->powered = false;
 }
 
 uint32_t uiox_tb4_hw_nhi_read(uiox_tb4_hw_t *hw, uint32_t offset)
 {
     if (!hw || !hw->priv) return 0u;
     const uiox_tb4_hw_ops_t *ops = (const uiox_tb4_hw_ops_t *)hw->priv;
     return ops->nhi_read ? ops->nhi_read(hw, offset) : 0u;
 }
 
 void uiox_tb4_hw_nhi_write(uiox_tb4_hw_t *hw, uint32_t offset, uint32_t val)
 {
     if (!hw || !hw->priv) return;
     const uiox_tb4_hw_ops_t *ops = (const uiox_tb4_hw_ops_t *)hw->priv;
     if (ops->nhi_write) ops->nhi_write(hw, offset, val);
 }
 
 int uiox_tb4_hw_icm_send(uiox_tb4_hw_t *hw,
                           const uint32_t *msg, uint8_t dwords)
 {
     if (!hw || !hw->priv || !msg) return -EINVAL;
     const uiox_tb4_hw_ops_t *ops = (const uiox_tb4_hw_ops_t *)hw->priv;
     if (!ops->icm_send) return -ENOSYS;
     return ops->icm_send(hw, msg, dwords);
 }
 
 int uiox_tb4_hw_icm_recv(uiox_tb4_hw_t *hw,
                           uint32_t *msg, uint8_t max_dwords)
 {
     if (!hw || !hw->priv || !msg) return -EINVAL;
     const uiox_tb4_hw_ops_t *ops = (const uiox_tb4_hw_ops_t *)hw->priv;
     if (!ops->icm_recv) return -ENOSYS;
     return ops->icm_recv(hw, msg, max_dwords);
 }
 
 int uiox_tb4_hw_tx_submit(uiox_tb4_hw_t *hw,
                            uintptr_t phys, uint32_t len, bool eof)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_tb4_hw_ops_t *ops = (const uiox_tb4_hw_ops_t *)hw->priv;
     if (!ops->tx_submit) return -ENOSYS;
     return ops->tx_submit(hw, phys, len, eof);
 }
 
 int uiox_tb4_hw_rx_poll(uiox_tb4_hw_t *hw,
                          uintptr_t *phys_out, uint32_t *len_out)
 {
     if (!hw || !hw->priv || !phys_out || !len_out) return -EINVAL;
     const uiox_tb4_hw_ops_t *ops = (const uiox_tb4_hw_ops_t *)hw->priv;
     if (!ops->rx_poll) return -ENOSYS;
     return ops->rx_poll(hw, phys_out, len_out);
 }
 