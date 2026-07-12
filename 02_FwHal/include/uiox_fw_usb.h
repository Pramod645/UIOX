/* ─────────────────────────────────────────────────────────────────────────
 * uiox_fw_usb.h — USB host controller (xHCI / EHCI)
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef UIOX_FW_USB_H
#define UIOX_FW_USB_H
#include "uiox_fw_types.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { UIOX_USB_XHCI=0, UIOX_USB_EHCI=1, UIOX_USB_OHCI=2 } uiox_usb_hc_t;
typedef enum { UIOX_USB_SPEED_LS=0, UIOX_USB_SPEED_FS=1,
               UIOX_USB_SPEED_HS=2, UIOX_USB_SPEED_SS=3,
               UIOX_USB_SPEED_SS_PLUS=4 } uiox_usb_speed_t;

/* xHCI capability registers */
#define XHCI_CAPLENGTH      0x00u
#define XHCI_HCSPARAMS1     0x04u
#define XHCI_HCCPARAMS1     0x10u
/* xHCI operational registers (base + caplength) */
#define XHCI_USBCMD         0x00u
#define XHCI_USBSTS         0x04u
#define XHCI_DNCTRL         0x14u
#define XHCI_USBCMD_RUN     (1u << 0)
#define XHCI_USBCMD_HCRST   (1u << 1)
#define XHCI_USBSTS_HCH     (1u << 0)

typedef struct {
    uint8_t          addr;
    uiox_usb_speed_t speed;
    uint16_t         vid, pid;
    uint8_t          class_code;
    bool             connected;
} uiox_usb_device_t;

#define UIOX_USB_MAX_DEVICES  16u

typedef struct {
    uintptr_t         mmio_base;
    uint32_t          irq;
    uiox_usb_hc_t     hc_type;
    uiox_usb_device_t devices[UIOX_USB_MAX_DEVICES];
    uint8_t           num_devices;
    uint8_t           num_ports;
    bool              initialized;
    void             *priv;
} uiox_usb_hc_dev_t;

typedef struct {
    uiox_fw_err_t (*init)        (uiox_usb_hc_dev_t *dev);
    void          (*deinit)      (uiox_usb_hc_dev_t *dev);
    uiox_fw_err_t (*port_reset)  (uiox_usb_hc_dev_t *dev, uint8_t port);
    uiox_fw_err_t (*enumerate)   (uiox_usb_hc_dev_t *dev);
    uiox_fw_err_t (*ctrl_xfer)   (uiox_usb_hc_dev_t *dev, uint8_t addr,
                                    uint8_t bmReqType, uint8_t bRequest,
                                    uint16_t wValue, uint16_t wIndex,
                                    void *data, uint16_t wLength);
    uiox_fw_err_t (*bulk_xfer)   (uiox_usb_hc_dev_t *dev, uint8_t addr,
                                    uint8_t ep, void *buf, uint32_t len);
    void          (*isr)         (uiox_usb_hc_dev_t *dev);
} uiox_usb_ops_t;

uiox_fw_err_t uiox_fw_usb_init       (uiox_usb_hc_dev_t *dev,
                                         const uiox_usb_ops_t *ops);
void          uiox_fw_usb_deinit     (uiox_usb_hc_dev_t *dev);
uiox_fw_err_t uiox_fw_usb_enumerate  (uiox_usb_hc_dev_t *dev);
uiox_fw_err_t uiox_fw_usb_ctrl_xfer  (uiox_usb_hc_dev_t *dev, uint8_t addr,
                                         uint8_t bmReqType, uint8_t bRequest,
                                         uint16_t wValue, uint16_t wIndex,
                                         void *data, uint16_t wLength);
uiox_fw_err_t uiox_fw_usb_init_xhci  (uiox_usb_hc_dev_t *dev,
                                         uintptr_t base, uint32_t irq);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_USB_H */
