/**
 * @file    uiox_usb_hw.h
 * @brief   UIOX USB Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to USB controller hardware. Owns:
 *   - MMIO register access to USB OTG / Host / Device controller
 *   - USB PHY programming (FS/HS/SS transceiver)
 *   - DMA engine for endpoint data transfer
 *   - IRQ handling (SOF, reset, suspend, resume, EP complete)
 *   - VBUS sensing and OTG ID pin monitoring
 *   - Clock and reset control
 *
 * Supports:
 *   - USB 1.1 Full-Speed (12 Mbit/s)
 *   - USB 2.0 High-Speed (480 Mbit/s)
 *   - USB 3.x SuperSpeed (5/10/20 Gbit/s)
 *   - OTG (dual role: host + device)
 *   - Host Controller Interface (HCI): OHCI, EHCI, xHCI
 *   - Device Controller Interface (DCI): ChipIdea, DWC2, DWC3
 *
 * @version 1.0.0
 * @date    2026-05-28
 */
//Layer 1 — Hardware Abstraction
 #ifndef UIOX_USB_HW_H
 #define UIOX_USB_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_USB_CAP_HOST           (1u << 0)   /**< Host mode             */
 #define UIOX_USB_CAP_DEVICE         (1u << 1)   /**< Device mode           */
 #define UIOX_USB_CAP_OTG            (1u << 2)   /**< OTG dual-role         */
 #define UIOX_USB_CAP_FS             (1u << 3)   /**< Full-Speed 12 Mbit/s  */
 #define UIOX_USB_CAP_HS             (1u << 4)   /**< High-Speed 480 Mbit/s */
 #define UIOX_USB_CAP_SS             (1u << 5)   /**< SuperSpeed 5 Gbit/s   */
 #define UIOX_USB_CAP_SS_PLUS        (1u << 6)   /**< SuperSpeed+ 10 Gbit/s */
 #define UIOX_USB_CAP_DMA            (1u << 7)   /**< DMA transfers         */
 #define UIOX_USB_CAP_ISO            (1u << 8)   /**< Isochronous endpoints */
 #define UIOX_USB_CAP_LPM            (1u << 9)   /**< Link Power Management */
 #define UIOX_USB_CAP_HUB            (1u << 10)  /**< Internal hub          */
 #define UIOX_USB_CAP_VBUS_CTRL      (1u << 11)  /**< VBUS power control    */
 
 /* =========================================================================
  * USB speed
  * ====================================================================== */
 
 typedef enum {
     UIOX_USB_SPEED_UNKNOWN = 0,
     UIOX_USB_SPEED_LOW,       /**< 1.5 Mbit/s                             */
     UIOX_USB_SPEED_FULL,      /**< 12 Mbit/s                              */
     UIOX_USB_SPEED_HIGH,      /**< 480 Mbit/s                             */
     UIOX_USB_SPEED_SUPER,     /**< 5 Gbit/s                               */
     UIOX_USB_SPEED_SUPER_PLUS,/**< 10/20 Gbit/s                           */
 } uiox_usb_speed_t;
 
 /* =========================================================================
  * USB role
  * ====================================================================== */
 
 typedef enum {
     UIOX_USB_ROLE_NONE = 0,
     UIOX_USB_ROLE_HOST,
     UIOX_USB_ROLE_DEVICE,
     UIOX_USB_ROLE_OTG,
 } uiox_usb_role_t;
 
 /* =========================================================================
  * Endpoint types
  * ====================================================================== */
 
 typedef enum {
     UIOX_USB_EP_CTRL  = 0,
     UIOX_USB_EP_ISOC  = 1,
     UIOX_USB_EP_BULK  = 2,
     UIOX_USB_EP_INTR  = 3,
 } uiox_usb_ep_type_t;
 
 /* =========================================================================
  * Endpoint direction
  * ====================================================================== */
 
 #define UIOX_USB_EP_DIR_OUT     0x00u
 #define UIOX_USB_EP_DIR_IN      0x80u
 
 /* =========================================================================
  * Endpoint descriptor (hardware level)
  * ====================================================================== */
 
 #define UIOX_USB_MAX_EP         16
 
 typedef struct {
     uint8_t            addr;      /**< EP address (number | direction)     */
     uiox_usb_ep_type_t type;
     uint16_t           mps;       /**< Max packet size                     */
     uint8_t            interval;  /**< Polling interval (frames / µframes) */
     bool               active;
     bool               stalled;
     uint32_t           xfer_count;/**< Total bytes transferred             */
 } uiox_usb_ep_hw_t;
 
 /* =========================================================================
  * DMA descriptor
  * ====================================================================== */
 
 #define UIOX_USB_DMA_DESC_ALIGN  64
 
 typedef struct __attribute__((packed, aligned(UIOX_USB_DMA_DESC_ALIGN))) {
     volatile uint32_t  status;    /**< OWN + done/error flags              */
     uint32_t           ctrl;      /**< Byte count + interrupt enable       */
     uint32_t           buf_lo;    /**< Buffer physical address lo          */
     uint32_t           buf_hi;    /**< Buffer physical address hi          */
     uint32_t           bytes_done;/**< Written by HW on completion         */
     uint32_t           reserved[3];
 } uiox_usb_dma_desc_t;
 
 #define UIOX_USB_DESC_OWN   (1u << 31)
 #define UIOX_USB_DESC_DONE  (1u << 1)
 #define UIOX_USB_DESC_ERR   (1u << 0)
 
 /* =========================================================================
  * USB hardware device descriptor
  * ====================================================================== */
 
 typedef struct {
     uintptr_t           base_addr;   /**< MMIO base of USB controller      */
     uint32_t            irq;         /**< IRQ line                         */
     uint32_t            caps;        /**< UIOX_USB_CAP_* bitmask          */
     uiox_usb_role_t     role;
     uiox_usb_speed_t    speed;
     uint8_t             num_ep;      /**< Number of endpoints supported    */
     uiox_usb_ep_hw_t    ep[UIOX_USB_MAX_EP];
 
     /* DMA */
     uiox_usb_dma_desc_t *tx_ring;
     uiox_usb_dma_desc_t *rx_ring;
     uint16_t             tx_ring_sz;
     uint16_t             rx_ring_sz;
 
     /* State */
     uint8_t              address;    /**< Assigned USB device address       */
     bool                 connected;
     bool                 suspended;
     bool                 vbus_present;
 
     void                *priv;
 } uiox_usb_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     int  (*init)          (uiox_usb_hw_t *hw);
     void (*deinit)        (uiox_usb_hw_t *hw);
     int  (*start)         (uiox_usb_hw_t *hw);
     void (*stop)          (uiox_usb_hw_t *hw);
     int  (*set_role)      (uiox_usb_hw_t *hw, uiox_usb_role_t role);
     int  (*set_address)   (uiox_usb_hw_t *hw, uint8_t addr);
     int  (*ep_config)     (uiox_usb_hw_t *hw, uint8_t ep_addr,
                            uiox_usb_ep_type_t type, uint16_t mps,
                            uint8_t interval);
     int  (*ep_disable)    (uiox_usb_hw_t *hw, uint8_t ep_addr);
     int  (*ep_stall)      (uiox_usb_hw_t *hw, uint8_t ep_addr, bool stall);
     int  (*ep_flush)      (uiox_usb_hw_t *hw, uint8_t ep_addr);
     int  (*tx_submit)     (uiox_usb_hw_t *hw, uint8_t ep_addr,
                            uintptr_t phys, uint32_t len);
     int  (*rx_submit)     (uiox_usb_hw_t *hw, uint8_t ep_addr,
                            uintptr_t phys, uint32_t len);
     int  (*tx_complete)   (uiox_usb_hw_t *hw, uint8_t ep_addr,
                            uint32_t *bytes_done);
     int  (*rx_complete)   (uiox_usb_hw_t *hw, uint8_t ep_addr,
                            uint32_t *bytes_done);
     bool (*vbus_sense)    (uiox_usb_hw_t *hw);
     int  (*remote_wakeup) (uiox_usb_hw_t *hw);
     void (*isr_sof)       (uiox_usb_hw_t *hw);
     void (*isr_reset)     (uiox_usb_hw_t *hw);
     void (*isr_suspend)   (uiox_usb_hw_t *hw);
     void (*isr_resume)    (uiox_usb_hw_t *hw);
     void (*isr_ep)        (uiox_usb_hw_t *hw, uint8_t ep_addr);
     void (*isr_connect)   (uiox_usb_hw_t *hw);
     void (*isr_disconnect)(uiox_usb_hw_t *hw);
 } uiox_usb_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int  uiox_usb_hw_init      (uiox_usb_hw_t *hw,
                              const uiox_usb_hw_ops_t *ops);
 void uiox_usb_hw_deinit    (uiox_usb_hw_t *hw);
 int  uiox_usb_hw_start     (uiox_usb_hw_t *hw);
 void uiox_usb_hw_stop      (uiox_usb_hw_t *hw);
 int  uiox_usb_hw_ep_config (uiox_usb_hw_t *hw, uint8_t ep_addr,
                              uiox_usb_ep_type_t type, uint16_t mps,
                              uint8_t interval);
 int  uiox_usb_hw_ep_stall  (uiox_usb_hw_t *hw, uint8_t ep_addr, bool stall);
 int  uiox_usb_hw_tx        (uiox_usb_hw_t *hw, uint8_t ep_addr,
                              uintptr_t phys, uint32_t len);
 int  uiox_usb_hw_rx        (uiox_usb_hw_t *hw, uint8_t ep_addr,
                              uintptr_t phys, uint32_t len);
 bool uiox_usb_hw_connected (uiox_usb_hw_t *hw);
 
 static inline uint32_t uiox_usb_caps(const uiox_usb_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_USB_HW_H */
 