/**
 * @file    uiox_usb_buf.h
 * @brief   UIOX USB URB/transfer buffer pool.
 *
 * USB Request Block (URB) pool for zero-copy DMA transfers.
 * Each URB carries one transfer request: control setup + data,
 * bulk data, interrupt data, or isochronous packet.
 *
 * @date    2026-05-28
 */
//Layer 1.5 — Buffer Manager
 #ifndef UIOX_USB_BUF_H
 #define UIOX_USB_BUF_H
 
 #include "uiox_usb_hw.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Pool configuration
  * ====================================================================== */
 
 #define UIOX_USB_URB_POOL_SIZE      32
 #define UIOX_USB_URB_DATA_MAX       4096    /**< Max data per URB          */
 #define UIOX_USB_URB_ALIGN          64
 
 /* =========================================================================
  * USB setup packet (for control transfers)
  * ====================================================================== */
 
 typedef struct __attribute__((packed)) {
     uint8_t  bmRequestType;
     uint8_t  bRequest;
     uint16_t wValue;
     uint16_t wIndex;
     uint16_t wLength;
 } uiox_usb_setup_t;
 
 /* Setup packet request types */
 #define UIOX_USB_REQ_GET_STATUS         0x00u
 #define UIOX_USB_REQ_CLEAR_FEATURE      0x01u
 #define UIOX_USB_REQ_SET_FEATURE        0x03u
 #define UIOX_USB_REQ_SET_ADDRESS        0x05u
 #define UIOX_USB_REQ_GET_DESCRIPTOR     0x06u
 #define UIOX_USB_REQ_SET_DESCRIPTOR     0x07u
 #define UIOX_USB_REQ_GET_CONFIGURATION  0x08u
 #define UIOX_USB_REQ_SET_CONFIGURATION  0x09u
 #define UIOX_USB_REQ_GET_INTERFACE      0x0Au
 #define UIOX_USB_REQ_SET_INTERFACE      0x0Bu
 #define UIOX_USB_REQ_SYNCH_FRAME        0x0Cu
 
 /* bmRequestType direction */
 #define UIOX_USB_DIR_HOST_TO_DEV        0x00u
 #define UIOX_USB_DIR_DEV_TO_HOST        0x80u
 
 /* bmRequestType type */
 #define UIOX_USB_TYPE_STANDARD          0x00u
 #define UIOX_USB_TYPE_CLASS             0x20u
 #define UIOX_USB_TYPE_VENDOR            0x40u
 
 /* bmRequestType recipient */
 #define UIOX_USB_RECIP_DEVICE           0x00u
 #define UIOX_USB_RECIP_INTERFACE        0x01u
 #define UIOX_USB_RECIP_ENDPOINT         0x02u
 
 /* =========================================================================
  * URB status
  * ====================================================================== */
 
 typedef enum {
     UIOX_URB_IDLE = 0,
     UIOX_URB_PENDING,
     UIOX_URB_COMPLETE,
     UIOX_URB_ERROR,
     UIOX_URB_STALL,
     UIOX_URB_TIMEOUT,
 } uiox_urb_status_t;
 
 /* =========================================================================
  * URB transfer type
  * ====================================================================== */
 
 typedef enum {
     UIOX_URB_CTRL = 0,
     UIOX_URB_BULK,
     UIOX_URB_INTR,
     UIOX_URB_ISOC,
 } uiox_urb_type_t;
 
 /* =========================================================================
  * USB Request Block
  * ====================================================================== */
 
 typedef struct uiox_usb_urb {
     /* DMA-accessible data buffer */
     uint8_t    *buf;            /**< Virtual address of data buffer        */
     uintptr_t   paddr;          /**< Physical address (for DMA)            */
     uint32_t    buf_len;        /**< Buffer capacity                       */
     uint32_t    actual_len;     /**< Bytes actually transferred            */
 
     /* Control setup (type == UIOX_URB_CTRL) */
     uiox_usb_setup_t setup;
 
     /* Transfer parameters */
     uiox_urb_type_t   type;
     uint8_t           ep_addr;  /**< Endpoint address (num | dir)          */
     uiox_urb_status_t status;
     uint8_t           in_use;   /**< Reference count                       */
     uint32_t          timeout_ms;
 
     /* Completion callback */
     void (*complete)(struct uiox_usb_urb *urb, void *ctx);
     void  *ctx;
 
     uint64_t   ts_ns;           /**< Submit timestamp                      */
     struct uiox_usb_urb *next;  /**< Free-list linkage                     */
 } uiox_usb_urb_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void           uiox_usb_buf_init  (void);
 uiox_usb_urb_t *uiox_usb_buf_alloc(void);
 void           uiox_usb_buf_ref   (uiox_usb_urb_t *urb);
 void           uiox_usb_buf_free  (uiox_usb_urb_t *urb);
 uint16_t       uiox_usb_buf_free_count(void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_USB_BUF_H */
 