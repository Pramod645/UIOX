/**
 * @file    uiox_net_hw.h
 * @brief   UIOX Network Hardware Abstraction Layer (HAL)
 *
 * Provides the lowest-level interface between physical network hardware
 * (NIC / MAC / PHY) and the driver layer above it. All hardware
 * register access, DMA descriptor ring management, and PHY link
 * negotiation live here.
 *
 * Supported hardware targets:
 *   - ARM64 GMAC (Gigabit MAC)
 *   - ARM32 EMAC (10/100 MAC)
 *   - Generic MMIO-mapped Ethernet controller
 *
 * @version 1.0.0
 * @date    2026-05-25
 */
//Layer 1 — Hardware Abstraction
 #ifndef UIOX_NET_HW_H
 #define UIOX_NET_HW_H
 
 #include <stdint.h>
 #include <stddef.h>
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hardware constants
  * ====================================================================== */
 
 #define UIOX_HW_MAC_ADDR_LEN        6       /**< Bytes in a MAC address      */
 #define UIOX_HW_MTU_ETHERNET        1500    /**< Standard Ethernet MTU       */
 #define UIOX_HW_MTU_JUMBO           9000    /**< Jumbo frame MTU             */
 #define UIOX_HW_TX_DESC_COUNT       256     /**< TX DMA descriptor ring size */
 #define UIOX_HW_RX_DESC_COUNT       256     /**< RX DMA descriptor ring size */
 #define UIOX_HW_DESC_ALIGN          64      /**< DMA descriptor alignment    */
 
 /** PHY auto-negotiation timeout (milliseconds). */
 #define UIOX_HW_PHY_AUTONEG_TIMEOUT_MS  5000
 
 /* =========================================================================
  * Hardware capability flags  (uiox_hw_dev_t.caps)
  * ====================================================================== */
 
 #define UIOX_HW_CAP_CHECKSUM_TX     (1u << 0)  /**< HW TX checksum offload  */
 #define UIOX_HW_CAP_CHECKSUM_RX     (1u << 1)  /**< HW RX checksum offload  */
 #define UIOX_HW_CAP_TSO             (1u << 2)  /**< TCP segmentation offload */
 #define UIOX_HW_CAP_LRO             (1u << 3)  /**< Large receive offload    */
 #define UIOX_HW_CAP_VLAN            (1u << 4)  /**< VLAN tag insertion/strip */
 #define UIOX_HW_CAP_JUMBO           (1u << 5)  /**< Jumbo frame support      */
 #define UIOX_HW_CAP_MULTIQUEUE      (1u << 6)  /**< Multiple TX/RX queues    */
 #define UIOX_HW_CAP_WOL             (1u << 7)  /**< Wake-on-LAN              */
 
 /* =========================================================================
  * Link speed / duplex
  * ====================================================================== */
 
 typedef enum {
     UIOX_HW_SPEED_UNKNOWN = 0,
     UIOX_HW_SPEED_10M,
     UIOX_HW_SPEED_100M,
     UIOX_HW_SPEED_1G,
     UIOX_HW_SPEED_2_5G,
     UIOX_HW_SPEED_10G,
 } uiox_hw_speed_t;
 
 typedef enum {
     UIOX_HW_DUPLEX_HALF = 0,
     UIOX_HW_DUPLEX_FULL,
 } uiox_hw_duplex_t;
 
 /* =========================================================================
  * DMA descriptor (one per packet fragment)
  * Must be placed in DMA-accessible memory, aligned to UIOX_HW_DESC_ALIGN.
  * ====================================================================== */
 
 typedef struct __attribute__((packed, aligned(UIOX_HW_DESC_ALIGN))) {
     volatile uint32_t   status;     /**< Ownership / status flags (HW writes)  */
     uint32_t            ctrl;       /**< Length, interrupt, first/last flags    */
     uint32_t            buf_lo;     /**< Low 32 bits of buffer physical address */
     uint32_t            buf_hi;     /**< High 32 bits (for 64-bit DMA)         */
     uint32_t            next_lo;    /**< Next descriptor physical address (lo)  */
     uint32_t            next_hi;
     uint32_t            reserved[2];
 } uiox_hw_dma_desc_t;
 
 /* Descriptor status bits */
 #define UIOX_HW_DESC_OWN        (1u << 31)  /**< 1 = owned by HW, 0 = by SW  */
 #define UIOX_HW_DESC_EOR        (1u << 30)  /**< End of ring                  */
 #define UIOX_HW_DESC_FS         (1u << 29)  /**< First segment of packet      */
 #define UIOX_HW_DESC_LS         (1u << 28)  /**< Last segment of packet       */
 #define UIOX_HW_DESC_ERROR      (1u << 0)   /**< Error summary                */
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 typedef struct {
     uintptr_t           base_addr;  /**< MMIO base address of MAC controller  */
     uint32_t            irq;        /**< IRQ number                           */
     uint8_t             mac_addr[UIOX_HW_MAC_ADDR_LEN];
     uint32_t            caps;       /**< UIOX_HW_CAP_* bitmask               */
     uiox_hw_speed_t     speed;
     uiox_hw_duplex_t    duplex;
     bool                link_up;
 
     /* DMA rings */
     uiox_hw_dma_desc_t *tx_ring;    /**< TX descriptor ring (DMA memory)     */
     uiox_hw_dma_desc_t *rx_ring;    /**< RX descriptor ring (DMA memory)     */
     uint16_t            tx_head;    /**< Next TX descriptor to fill          */
     uint16_t            tx_tail;    /**< Next TX descriptor to reclaim       */
     uint16_t            rx_head;    /**< Next RX descriptor to fill          */
 
     void               *priv;       /**< Driver private data                 */
 } uiox_hw_dev_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * Implemented by each concrete NIC driver (e.g. uiox_drv_gmac.c)
  * ====================================================================== */
 
 typedef struct {
     /** One-time hardware initialisation (clocks, resets, DMA rings). */
     int  (*init)       (uiox_hw_dev_t *dev);
 
     /** Release all hardware resources. */
     void (*deinit)     (uiox_hw_dev_t *dev);
 
     /** Start the MAC/DMA engine and enable IRQs. */
     int  (*start)      (uiox_hw_dev_t *dev);
 
     /** Stop the MAC/DMA engine. */
     void (*stop)       (uiox_hw_dev_t *dev);
 
     /** Trigger PHY auto-negotiation; block until complete or timeout. */
     int  (*phy_autoneg)(uiox_hw_dev_t *dev);
 
     /**
      * Submit a TX buffer to the DMA ring.
      * @param buf   Physical address of packet data.
      * @param len   Length in bytes (≤ MTU).
      * @return      0 on success, -ENOSPC if ring full.
      */
     int  (*tx_submit)  (uiox_hw_dev_t *dev, uintptr_t buf, uint16_t len);
 
     /** Reclaim completed TX descriptors; free associated buffers. */
     void (*tx_reclaim) (uiox_hw_dev_t *dev);
 
     /**
      * Poll for a received packet.
      * @param buf   Destination buffer.
      * @param maxlen Maximum bytes to copy.
      * @return      Bytes received, 0 if no packet ready, <0 on error.
      */
     int  (*rx_poll)    (uiox_hw_dev_t *dev, void *buf, uint16_t maxlen);
 
     /** Top-half interrupt service routine (called from ISR context). */
     void (*isr)        (uiox_hw_dev_t *dev);
 
     /** Read a PHY register via MDIO. */
     uint16_t (*mdio_read) (uiox_hw_dev_t *dev, uint8_t phy, uint8_t reg);
 
     /** Write a PHY register via MDIO. */
     void     (*mdio_write)(uiox_hw_dev_t *dev, uint8_t phy,
                            uint8_t reg, uint16_t val);
 } uiox_hw_ops_t;
 
 /* =========================================================================
  * Public HAL API
  * ====================================================================== */
 
 /**
  * @brief  Initialise hardware device using provided ops vtable.
  * @return 0 on success, negative errno on failure.
  */
 int  uiox_hw_init   (uiox_hw_dev_t *dev, const uiox_hw_ops_t *ops);
 
 /** @brief  Bring the link up (calls phy_autoneg + start). */
 int  uiox_hw_up     (uiox_hw_dev_t *dev);
 
 /** @brief  Bring the link down gracefully. */
 void uiox_hw_down   (uiox_hw_dev_t *dev);
 
 /** @brief  Query current link state. */
 bool uiox_hw_link_ok(const uiox_hw_dev_t *dev);
 
 /** @brief  Return hardware-reported link speed. */
 uiox_hw_speed_t uiox_hw_speed(const uiox_hw_dev_t *dev);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* UIOX_NET_HW_H */
 