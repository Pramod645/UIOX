/**
 * @file    uiox_usb_class.h
 * @brief   UIOX USB class driver interfaces.
 *
 * Provides abstract class driver interface plus concrete implementations:
 *   - HID  (Human Interface Device — keyboard, mouse, gamepad)
 *   - CDC  (Communications Device Class — virtual serial port)
 *   - MSC  (Mass Storage Class — Bulk-Only Transport)
 *   - Audio (USB Audio Class 1.0)
 *   - Vendor (custom vendor-specific class)
 *
 * @date    2026-05-28
 */
//Layer 2c — Class Drivers
 #ifndef UIOX_USB_CLASS_H
 #define UIOX_USB_CLASS_H
 
 #include "uiox_usb_if.h"
 #include "uiox_usb_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Generic class driver vtable
  * ====================================================================== */
 
 typedef struct uiox_usb_class_drv {
     const char *name;
 
     /** Called when device is configured (SET_CONFIGURATION). */
     int  (*bind)    (struct uiox_usb_class_drv *drv,
                      uiox_usb_if_t *uif);
 
     /** Called on disconnect or SET_CONFIGURATION(0). */
     void (*unbind)  (struct uiox_usb_class_drv *drv);
 
     /** Handle class-specific SETUP request. */
     int  (*setup)   (struct uiox_usb_class_drv *drv,
                      const uiox_usb_setup_t *setup,
                      uint8_t *resp, uint16_t *resp_len);
 
     /** Called when an EP transfer completes (data available or sent). */
     void (*ep_event)(struct uiox_usb_class_drv *drv,
                      uint8_t ep_addr, uint32_t bytes, bool success);
 
     /** Periodic tick (ms-level). */
     void (*tick)    (struct uiox_usb_class_drv *drv, uint32_t now_ms);
 
     void *priv;
 } uiox_usb_class_drv_t;
 
 /* =========================================================================
  * HID class driver
  * ====================================================================== */
 
 #define UIOX_HID_SUBCLASS_NONE      0x00u
 #define UIOX_HID_SUBCLASS_BOOT      0x01u
 #define UIOX_HID_PROTO_NONE         0x00u
 #define UIOX_HID_PROTO_KEYBOARD     0x01u
 #define UIOX_HID_PROTO_MOUSE        0x02u
 
 #define UIOX_HID_REQ_GET_REPORT     0x01u
 #define UIOX_HID_REQ_GET_IDLE       0x02u
 #define UIOX_HID_REQ_GET_PROTOCOL   0x03u
 #define UIOX_HID_REQ_SET_REPORT     0x09u
 #define UIOX_HID_REQ_SET_IDLE       0x0Au
 #define UIOX_HID_REQ_SET_PROTOCOL   0x0Bu
 
 typedef struct {
     uiox_usb_class_drv_t   base;
     uiox_usb_if_t         *uif;
     uint8_t                ep_in;       /**< Interrupt IN endpoint           */
     const uint8_t         *report_desc;
     uint16_t               report_desc_len;
     uint8_t                idle_rate;   /**< Idle rate (4ms units, 0=infinite)*/
     uint8_t                protocol;    /**< 0=boot, 1=report               */
     uint8_t                report_buf[64];
     bool                   report_ready;
     void (*on_report)(const uint8_t *buf, uint8_t len, void *ctx);
     void *report_ctx;
 } uiox_usb_hid_t;
 
 int  uiox_usb_hid_init     (uiox_usb_hid_t *hid,
                              uint8_t ep_in,
                              const uint8_t *report_desc,
                              uint16_t report_desc_len);
 int  uiox_usb_hid_send     (uiox_usb_hid_t *hid,
                              const uint8_t *data, uint8_t len);
 
 /* =========================================================================
  * CDC class driver (virtual COM port)
  * ====================================================================== */
 
 #define UIOX_CDC_REQ_SET_LINE_CODING    0x20u
 #define UIOX_CDC_REQ_GET_LINE_CODING    0x21u
 #define UIOX_CDC_REQ_SET_CTRL_LINE_STATE 0x22u
 
 typedef struct __attribute__((packed)) {
     uint32_t dwDTERate;     /**< Baud rate                                  */
     uint8_t  bCharFormat;   /**< Stop bits: 0=1, 1=1.5, 2=2               */
     uint8_t  bParityType;   /**< 0=None, 1=Odd, 2=Even, 3=Mark, 4=Space  */
     uint8_t  bDataBits;     /**< 5,6,7,8,16                               */
 } uiox_cdc_line_coding_t;
 
 typedef struct {
     uiox_usb_class_drv_t    base;
     uiox_usb_if_t          *uif;
     uint8_t                 ep_in;    /**< Bulk IN (device→host)            */
     uint8_t                 ep_out;   /**< Bulk OUT (host→device)           */
     uint8_t                 ep_notify;/**< Interrupt IN (notifications)     */
     uiox_cdc_line_coding_t  line_coding;
     bool                    dtr;      /**< Data Terminal Ready              */
     bool                    rts;      /**< Request To Send                  */
     uint8_t                 rx_buf[256];
     uint16_t                rx_len;
     void (*on_rx)  (const uint8_t *data, uint16_t len, void *ctx);
     void (*on_ctrl)(bool dtr, bool rts, void *ctx);
     void *rx_ctx;
 } uiox_usb_cdc_t;
 
 int     uiox_usb_cdc_init  (uiox_usb_cdc_t *cdc,
                              uint8_t ep_in, uint8_t ep_out,
                              uint8_t ep_notify);
 int     uiox_usb_cdc_write (uiox_usb_cdc_t *cdc,
                              const uint8_t *data, uint16_t len);
 uint16_t uiox_usb_cdc_read (uiox_usb_cdc_t *cdc,
                              uint8_t *buf, uint16_t max_len);
 
 /* =========================================================================
  * MSC class driver (Bulk-Only Transport)
  * ====================================================================== */
 
 #define UIOX_MSC_REQ_RESET              0xFFu
 #define UIOX_MSC_REQ_GET_MAX_LUN        0xFEu
 #define UIOX_MSC_CBW_SIGNATURE          0x43425355u
 #define UIOX_MSC_CSW_SIGNATURE          0x53425355u
 
 typedef struct __attribute__((packed)) {
     uint32_t dCBWSignature;
     uint32_t dCBWTag;
     uint32_t dCBWDataTransferLength;
     uint8_t  bmCBWFlags;
     uint8_t  bCBWLUN;
     uint8_t  bCBWCBLength;
     uint8_t  CBWCB[16];
 } uiox_msc_cbw_t;
 
 typedef struct __attribute__((packed)) {
     uint32_t dCSWSignature;
     uint32_t dCSWTag;
     uint32_t dCSWDataResidue;
     uint8_t  bCSWStatus;
 } uiox_msc_csw_t;
 
 typedef struct {
     uiox_usb_class_drv_t base;
     uiox_usb_if_t       *uif;
     uint8_t              ep_in;
     uint8_t              ep_out;
     uint8_t              max_lun;
     uiox_msc_cbw_t       cbw;
     uiox_msc_csw_t       csw;
     uint8_t              data_buf[512];
     /* SCSI command handler callback */
     int (*scsi_cmd)(uint8_t lun, const uint8_t *cb, uint8_t cb_len,
                     uint8_t *data, uint32_t *data_len,
                     bool data_in, void *ctx);
     void *scsi_ctx;
 } uiox_usb_msc_t;
 
 int uiox_usb_msc_init(uiox_usb_msc_t *msc,
                        uint8_t ep_in, uint8_t ep_out, uint8_t max_lun);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_USB_CLASS_H */
 