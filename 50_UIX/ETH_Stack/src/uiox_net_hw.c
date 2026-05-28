/**
 * @file    uiox_net_hw.c
 * @brief   UIOX Network HAL — generic hardware lifecycle management.
 *
 * Concrete DMA register sequences live in the per-controller drivers
 * (e.g. uiox_drv_gmac.c). This file wires up the vtable, validates
 * arguments, and manages the device state machine.
 *
 * @date    2026-05-25
 */

 #include "uiox_net_hw.h"
 #include <errno.h>
 #include <string.h>
 #include <assert.h>
 
 /* Retained ops vtable for each device */
 static const uiox_hw_ops_t *s_ops[4];   /* support up to 4 NICs */
 static uint8_t              s_dev_count;
 
 /* -------------------------------------------------------------------------
  * Internal helpers
  * ---------------------------------------------------------------------- */
 
 static inline const uiox_hw_ops_t *hw_ops(const uiox_hw_dev_t *dev)
 {
     /* ops pointer stored in priv by uiox_hw_init */
     return (const uiox_hw_ops_t *)((uintptr_t *)dev->priv)[0];
 }
 
 /* -------------------------------------------------------------------------
  * Public API
  * ---------------------------------------------------------------------- */
 
 int uiox_hw_init(uiox_hw_dev_t *dev, const uiox_hw_ops_t *ops)
 {
     if (!dev || !ops || !ops->init)
         return -EINVAL;
 
     /* Store ops vtable in priv slot 0 */
     dev->priv = (void *)ops;
 
     memset(dev->mac_addr, 0, UIOX_HW_MAC_ADDR_LEN);
     dev->link_up = false;
     dev->speed   = UIOX_HW_SPEED_UNKNOWN;
     dev->duplex  = UIOX_HW_DUPLEX_HALF;
     dev->tx_head = 0;
     dev->tx_tail = 0;
     dev->rx_head = 0;
 
     return ops->init(dev);
 }
 
 int uiox_hw_up(uiox_hw_dev_t *dev)
 {
     if (!dev || !dev->priv)
         return -EINVAL;
 
     const uiox_hw_ops_t *ops = (const uiox_hw_ops_t *)dev->priv;
     int rc;
 
     /* Run PHY negotiation first */
     if (ops->phy_autoneg) {
         rc = ops->phy_autoneg(dev);
         if (rc < 0)
             return rc;
     }
 
     /* Start MAC/DMA engine */
     if (ops->start) {
         rc = ops->start(dev);
         if (rc < 0)
             return rc;
     }
 
     dev->link_up = true;
     return 0;
 }
 
 void uiox_hw_down(uiox_hw_dev_t *dev)
 {
     if (!dev || !dev->priv)
         return;
 
     const uiox_hw_ops_t *ops = (const uiox_hw_ops_t *)dev->priv;
     if (ops->stop)
         ops->stop(dev);
 
     dev->link_up = false;
     dev->speed   = UIOX_HW_SPEED_UNKNOWN;
 }
 
 bool uiox_hw_link_ok(const uiox_hw_dev_t *dev)
 {
     return dev ? dev->link_up : false;
 }
 
 uiox_hw_speed_t uiox_hw_speed(const uiox_hw_dev_t *dev)
 {
     return dev ? dev->speed : UIOX_HW_SPEED_UNKNOWN;
 }
 