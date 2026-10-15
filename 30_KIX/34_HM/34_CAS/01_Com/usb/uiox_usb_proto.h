/**
 * @file    uiox_usb_proto.h
 * @brief   UIOX USB protocol layer — descriptors, standard requests.
 *
 * Implements:
 *   - USB standard descriptors (Device, Config, Interface, Endpoint, String)
 *   - Standard device requests (GET/SET Descriptor, SET Address, etc.)
 *   - Device state machine (Powered→Default→Address→Configured)
 *   - String descriptor table management
 *   - Configuration descriptor building helper
 *
 * @date    2026-05-28
 */
//Layer 2b — USB Protocol
 #ifndef UIOX_USB_PROTO_H
 #define UIOX_USB_PROTO_H
 
 #include "uiox_usb_if.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Descriptor types
  * ====================================================================== */
 
 #define UIOX_USB_DT_DEVICE          0x01u
 #define UIOX_USB_DT_CONFIG          0x02u
 #define UIOX_USB_DT_STRING          0x03u
 #define UIOX_USB_DT_INTERFACE       0x04u
 #define UIOX_USB_DT_ENDPOINT        0x05u
 #define UIOX_USB_DT_DEVICE_QUAL     0x06u
 #define UIOX_USB_DT_OTHER_SPEED     0x07u
 #define UIOX_USB_DT_INTERFACE_POWER 0x08u
 #define UIOX_USB_DT_HID             0x21u
 #define UIOX_USB_DT_REPORT          0x22u
 #define UIOX_USB_DT_BOS             0x0Fu
 
 /* =========================================================================
  * USB class codes
  * ====================================================================== */
 
 #define UIOX_USB_CLASS_AUDIO        0x01u
 #define UIOX_USB_CLASS_CDC          0x02u
 #define UIOX_USB_CLASS_HID          0x03u
 #define UIOX_USB_CLASS_PHYSICAL     0x05u
 #define UIOX_USB_CLASS_IMAGE        0x06u
 #define UIOX_USB_CLASS_PRINTER      0x07u
 #define UIOX_USB_CLASS_MSC          0x08u
 #define UIOX_USB_CLASS_HUB          0x09u
 #define UIOX_USB_CLASS_CDC_DATA     0x0Au
 #define UIOX_USB_CLASS_VIDEO        0x0Eu
 #define UIOX_USB_CLASS_VENDOR       0xFFu
 
 /* =========================================================================
  * Standard descriptors (packed)
  * ====================================================================== */
 
 typedef struct __attribute__((packed)) {
     uint8_t  bLength;
     uint8_t  bDescriptorType;
     uint16_t bcdUSB;
     uint8_t  bDeviceClass;
     uint8_t  bDeviceSubClass;
     uint8_t  bDeviceProtocol;
     uint8_t  bMaxPacketSize0;
     uint16_t idVendor;
     uint16_t idProduct;
     uint16_t bcdDevice;
     uint8_t  iManufacturer;
     uint8_t  iProduct;
     uint8_t  iSerialNumber;
     uint8_t  bNumConfigurations;
 } uiox_usb_dev_desc_t;
 
 typedef struct __attribute__((packed)) {
     uint8_t  bLength;
     uint8_t  bDescriptorType;
     uint16_t wTotalLength;
     uint8_t  bNumInterfaces;
     uint8_t  bConfigurationValue;
     uint8_t  iConfiguration;
     uint8_t  bmAttributes;
     uint8_t  bMaxPower;           /**< In units of 2 mA                   */
 } uiox_usb_cfg_desc_t;
 
 typedef struct __attribute__((packed)) {
     uint8_t  bLength;
     uint8_t  bDescriptorType;
     uint8_t  bInterfaceNumber;
     uint8_t  bAlternateSetting;
     uint8_t  bNumEndpoints;
     uint8_t  bInterfaceClass;
     uint8_t  bInterfaceSubClass;
     uint8_t  bInterfaceProtocol;
     uint8_t  iInterface;
 } uiox_usb_if_desc_t;
 
 typedef struct __attribute__((packed)) {
     uint8_t  bLength;
     uint8_t  bDescriptorType;
     uint8_t  bEndpointAddress;
     uint8_t  bmAttributes;
     uint16_t wMaxPacketSize;
     uint8_t  bInterval;
 } uiox_usb_ep_desc_t;
 
 typedef struct __attribute__((packed)) {
     uint8_t  bLength;
     uint8_t  bDescriptorType;
     uint16_t wLANGID;             /**< Language ID (e.g. 0x0409 = en-US) */
 } uiox_usb_str_desc_lang_t;
 
 /* =========================================================================
  * Device state machine
  * ====================================================================== */
 
 typedef enum {
     UIOX_USB_DEV_POWERED = 0,
     UIOX_USB_DEV_DEFAULT,
     UIOX_USB_DEV_ADDRESS,
     UIOX_USB_DEV_CONFIGURED,
     UIOX_USB_DEV_SUSPENDED,
 } uiox_usb_dev_state_t;
 
 /* =========================================================================
  * String descriptor table
  * ====================================================================== */
 
 #define UIOX_USB_MAX_STRINGS    8
 #define UIOX_USB_STR_MAX_LEN    64
 
 typedef struct {
     const char *str;
     uint8_t     idx;
 } uiox_usb_string_entry_t;
 
 /* =========================================================================
  * Protocol context
  * ====================================================================== */
 
 typedef struct {
     uiox_usb_if_t            *uif;
     uiox_usb_dev_state_t      state;
     const uiox_usb_dev_desc_t *dev_desc;
     const uint8_t             *cfg_desc_buf;  /**< Full config descriptor   */
     uint16_t                   cfg_desc_len;
     uiox_usb_string_entry_t    strings[UIOX_USB_MAX_STRINGS];
     uint8_t                    num_strings;
     uint8_t                    active_config;
     uint16_t                   remote_wakeup : 1;
     uint16_t                   self_powered  : 1;
 } uiox_usb_proto_t;
 
 /* =========================================================================
  * Protocol API
  * ====================================================================== */
 
 int  uiox_usb_proto_init       (uiox_usb_proto_t          *proto,
                                  uiox_usb_if_t             *uif,
                                  const uiox_usb_dev_desc_t *dev_desc,
                                  const uint8_t             *cfg_buf,
                                  uint16_t                   cfg_len);
 
 /** Add a string descriptor entry. */
 int  uiox_usb_proto_add_string (uiox_usb_proto_t *proto,
                                  uint8_t idx, const char *str);
 
 /**
  * @brief  Handle incoming SETUP packet on EP0.
  *         Processes standard requests; delegates class/vendor to callback.
  */
 int  uiox_usb_proto_setup      (uiox_usb_proto_t *proto,
                                  const uiox_usb_setup_t *setup,
                                  uint8_t *resp_buf, uint16_t *resp_len);
 
 /** Handle EP0 DATA IN complete (status phase). */
 void uiox_usb_proto_ep0_in_done(uiox_usb_proto_t *proto);
 
 /** Handle bus reset — return to default state. */
 void uiox_usb_proto_reset      (uiox_usb_proto_t *proto);
 
 /** Handle suspend. */
 void uiox_usb_proto_suspend    (uiox_usb_proto_t *proto);
 
 /** Handle resume. */
 void uiox_usb_proto_resume     (uiox_usb_proto_t *proto);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_USB_PROTO_H */
 