/**
 * @file    uiox_usb_class.c
 * @brief   UIOX USB class driver implementations (HID, CDC, MSC).
 * @date    2026-05-28
 */

 #include "uiox_usb_class.h"
 #include <string.h>
 #include <errno.h>
 
 /* =========================================================================
  * HID class driver
  * ====================================================================== */
 
 static int hid_bind(uiox_usb_class_drv_t *drv, uiox_usb_if_t *uif)
 {
     uiox_usb_hid_t *hid = (uiox_usb_hid_t *)drv;
     hid->uif = uif;
     /* Open interrupt IN endpoint */
     return uiox_usb_if_ep_open(uif, hid->ep_in,
                                 UIOX_USB_EP_INTR, 64u, 1u);
 }
 
 static void hid_unbind(uiox_usb_class_drv_t *drv)
 {
     uiox_usb_hid_t *hid = (uiox_usb_hid_t *)drv;
     if (hid->uif) uiox_usb_if_ep_close(hid->uif, hid->ep_in);
 }
 
 static int hid_setup(uiox_usb_class_drv_t *drv,
                       const uiox_usb_setup_t *setup,
                       uint8_t *resp, uint16_t *resp_len)
 {
     uiox_usb_hid_t *hid = (uiox_usb_hid_t *)drv;
     *resp_len = 0;
 
     if ((setup->bmRequestType & 0x60u) == UIOX_USB_TYPE_STANDARD) {
         /* GET_DESCRIPTOR for HID Report */
         if (setup->bRequest == UIOX_USB_REQ_GET_DESCRIPTOR &&
             (setup->wValue >> 8u) == UIOX_USB_DT_REPORT) {
             *resp_len = hid->report_desc_len;
             if (*resp_len > setup->wLength) *resp_len = setup->wLength;
             memcpy(resp, hid->report_desc, *resp_len);
             return 0;
         }
     }
 
     switch (setup->bRequest) {
     case UIOX_HID_REQ_GET_IDLE:
         resp[0]   = hid->idle_rate;
         *resp_len = 1u;
         break;
     case UIOX_HID_REQ_SET_IDLE:
         hid->idle_rate = (uint8_t)(setup->wValue >> 8u);
         break;
     case UIOX_HID_REQ_GET_PROTOCOL:
         resp[0]   = hid->protocol;
         *resp_len = 1u;
         break;
     case UIOX_HID_REQ_SET_PROTOCOL:
         hid->protocol = (uint8_t)(setup->wValue & 0xFFu);
         break;
     case UIOX_HID_REQ_GET_REPORT:
         if (setup->wLength <= 64u) {
             memcpy(resp, hid->report_buf, setup->wLength);
             *resp_len = (uint16_t)setup->wLength;
         }
         break;
     case UIOX_HID_REQ_SET_REPORT:
         /* Output report received — will arrive via EP OUT data phase */
         break;
     default:
         return -ENOTSUP;
     }
     return 0;
 }
 
 static void hid_ep_event(uiox_usb_class_drv_t *drv,
                           uint8_t ep_addr, uint32_t bytes, bool success)
 {
     (void)drv; (void)ep_addr; (void)bytes; (void)success;
 }
 
 static void hid_tick(uiox_usb_class_drv_t *drv, uint32_t now_ms)
 {
     (void)drv; (void)now_ms;
 }
 
 int uiox_usb_hid_init(uiox_usb_hid_t *hid,
                        uint8_t ep_in,
                        const uint8_t *report_desc,
                        uint16_t report_desc_len)
 {
     if (!hid || !report_desc) return -EINVAL;
     memset(hid, 0, sizeof(*hid));
     hid->base.name      = "HID";
     hid->base.bind      = hid_bind;
     hid->base.unbind    = hid_unbind;
     hid->base.setup     = hid_setup;
     hid->base.ep_event  = hid_ep_event;
     hid->base.tick      = hid_tick;
     hid->ep_in          = ep_in;
     hid->report_desc    = report_desc;
     hid->report_desc_len= report_desc_len;
     hid->protocol       = 1u;  /* Report protocol */
     hid->idle_rate      = 0u;  /* Indefinite */
     return 0;
 }
 
 int uiox_usb_hid_send(uiox_usb_hid_t *hid,
                        const uint8_t *data, uint8_t len)
 {
     if (!hid || !hid->uif || !data || !len) return -EINVAL;
     uiox_usb_urb_t *urb = uiox_usb_buf_alloc();
     if (!urb) return -ENOMEM;
     if (len > (uint8_t)UIOX_USB_URB_DATA_MAX) len = UIOX_USB_URB_DATA_MAX;
     memcpy(urb->buf, data, len);
     urb->buf_len = len;
     urb->ep_addr = hid->ep_in;
     urb->type    = UIOX_URB_INTR;
     return uiox_usb_if_submit(hid->uif, urb);
 }
 
 /* =========================================================================
  * CDC class driver
  * ====================================================================== */
 
 static int cdc_bind(uiox_usb_class_drv_t *drv, uiox_usb_if_t *uif)
 {
     uiox_usb_cdc_t *cdc = (uiox_usb_cdc_t *)drv;
     cdc->uif = uif;
     uiox_usb_if_ep_open(uif, cdc->ep_in,     UIOX_USB_EP_BULK, 64u, 0u);
     uiox_usb_if_ep_open(uif, cdc->ep_out,    UIOX_USB_EP_BULK, 64u, 0u);
     uiox_usb_if_ep_open(uif, cdc->ep_notify, UIOX_USB_EP_INTR, 16u, 10u);
 
     /* Prime RX endpoint */
     uiox_usb_urb_t *rx = uiox_usb_buf_alloc();
     if (rx) {
         rx->ep_addr  = cdc->ep_out;
         rx->type     = UIOX_URB_BULK;
         rx->buf_len  = sizeof(cdc->rx_buf);
         uiox_usb_if_submit(uif, rx);
     }
     return 0;
 }
 
 static void cdc_unbind(uiox_usb_class_drv_t *drv)
 {
     uiox_usb_cdc_t *cdc = (uiox_usb_cdc_t *)drv;
     if (!cdc->uif) return;
     uiox_usb_if_ep_close(cdc->uif, cdc->ep_in);
     uiox_usb_if_ep_close(cdc->uif, cdc->ep_out);
     uiox_usb_if_ep_close(cdc->uif, cdc->ep_notify);
 }
 
 static int cdc_setup(uiox_usb_class_drv_t *drv,
                       const uiox_usb_setup_t *setup,
                       uint8_t *resp, uint16_t *resp_len)
 {
     uiox_usb_cdc_t *cdc = (uiox_usb_cdc_t *)drv;
     *resp_len = 0;
     switch (setup->bRequest) {
     case UIOX_CDC_REQ_GET_LINE_CODING:
         memcpy(resp, &cdc->line_coding, sizeof(cdc->line_coding));
         *resp_len = sizeof(cdc->line_coding);
         break;
     case UIOX_CDC_REQ_SET_LINE_CODING:
         /* Data arrives in OUT data phase — handled in ep_event */
         break;
     case UIOX_CDC_REQ_SET_CTRL_LINE_STATE:
         cdc->dtr = (setup->wValue & 0x01u) != 0u;
         cdc->rts = (setup->wValue & 0x02u) != 0u;
         if (cdc->on_ctrl) cdc->on_ctrl(cdc->dtr, cdc->rts, cdc->rx_ctx);
         break;
     default: return -ENOTSUP;
     }
     return 0;
 }
 
 static void cdc_ep_event(uiox_usb_class_drv_t *drv,
                           uint8_t ep_addr, uint32_t bytes, bool success)
 {
     uiox_usb_cdc_t *cdc = (uiox_usb_cdc_t *)drv;
     if (!success) return;
     if (ep_addr == cdc->ep_out && bytes > 0) {
         /* Data received from host */
         uiox_usb_urb_t *urb = cdc->uif->queues[ep_addr & 0x0Fu].urbs[
             cdc->uif->queues[ep_addr & 0x0Fu].head % UIOX_USB_EP_QUEUE_DEPTH];
         if (urb && cdc->on_rx)
             cdc->on_rx(urb->buf, (uint16_t)bytes, cdc->rx_ctx);
     }
 }
 
 static void cdc_tick(uiox_usb_class_drv_t *drv, uint32_t now_ms)
 { (void)drv; (void)now_ms; }
 
 int uiox_usb_cdc_init(uiox_usb_cdc_t *cdc,
                        uint8_t ep_in, uint8_t ep_out, uint8_t ep_notify)
 {
     if (!cdc) return -EINVAL;
     memset(cdc, 0, sizeof(*cdc));
     cdc->base.name      = "CDC";
     cdc->base.bind      = cdc_bind;
     cdc->base.unbind    = cdc_unbind;
     cdc->base.setup     = cdc_setup;
     cdc->base.ep_event  = cdc_ep_event;
     cdc->base.tick      = cdc_tick;
     cdc->ep_in          = ep_in;
     cdc->ep_out         = ep_out;
     cdc->ep_notify      = ep_notify;
     cdc->line_coding.dwDTERate   = 115200u;
     cdc->line_coding.bCharFormat = 0u;
     cdc->line_coding.bParityType = 0u;
     cdc->line_coding.bDataBits   = 8u;
     return 0;
 }
 
 int uiox_usb_cdc_write(uiox_usb_cdc_t *cdc,
                         const uint8_t *data, uint16_t len)
 {
     if (!cdc || !cdc->uif || !data || !len) return -EINVAL;
     uiox_usb_urb_t *urb = uiox_usb_buf_alloc();
     if (!urb) return -ENOMEM;
     if (len > UIOX_USB_URB_DATA_MAX) len = UIOX_USB_URB_DATA_MAX;
     memcpy(urb->buf, data, len);
     urb->buf_len = len;
     urb->ep_addr = cdc->ep_in;
     urb->type    = UIOX_URB_BULK;
     return uiox_usb_if_submit(cdc->uif, urb);
 }
 
 uint16_t uiox_usb_cdc_read(uiox_usb_cdc_t *cdc,
                              uint8_t *buf, uint16_t max_len)
 {
     if (!cdc || !buf || !max_len) return 0u;
     uint16_t n = (cdc->rx_len < max_len) ? cdc->rx_len : max_len;
     memcpy(buf, cdc->rx_buf, n);
     cdc->rx_len = 0u;
     return n;
 }
 
 /* =========================================================================
  * MSC class driver
  * ====================================================================== */
 
 static int msc_bind(uiox_usb_class_drv_t *drv, uiox_usb_if_t *uif)
 {
     uiox_usb_msc_t *msc = (uiox_usb_msc_t *)drv;
     msc->uif = uif;
     uiox_usb_if_ep_open(uif, msc->ep_in,  UIOX_USB_EP_BULK, 512u, 0u);
     uiox_usb_if_ep_open(uif, msc->ep_out, UIOX_USB_EP_BULK, 512u, 0u);
     /* Prime CBW receive */
     uiox_usb_urb_t *rx = uiox_usb_buf_alloc();
     if (rx) {
         rx->ep_addr = msc->ep_out;
         rx->type    = UIOX_URB_BULK;
         rx->buf_len = sizeof(uiox_msc_cbw_t);
         uiox_usb_if_submit(uif, rx);
     }
     return 0;
 }
 
 static void msc_unbind(uiox_usb_class_drv_t *drv)
 {
     uiox_usb_msc_t *msc = (uiox_usb_msc_t *)drv;
     if (!msc->uif) return;
     uiox_usb_if_ep_close(msc->uif, msc->ep_in);
     uiox_usb_if_ep_close(msc->uif, msc->ep_out);
 }
 
 static int msc_setup(uiox_usb_class_drv_t *drv,
                       const uiox_usb_setup_t *setup,
                       uint8_t *resp, uint16_t *resp_len)
 {
     uiox_usb_msc_t *msc = (uiox_usb_msc_t *)drv;
     *resp_len = 0;
     switch (setup->bRequest) {
     case UIOX_MSC_REQ_GET_MAX_LUN:
         resp[0]   = msc->max_lun;
         *resp_len = 1u;
         break;
     case UIOX_MSC_REQ_RESET:
         uiox_usb_hw_ep_stall(msc->uif->hw, msc->ep_in,  false);
         uiox_usb_hw_ep_stall(msc->uif->hw, msc->ep_out, false);
         break;
     default: return -ENOTSUP;
     }
     return 0;
 }
 
 static void msc_ep_event(uiox_usb_class_drv_t *drv,
                           uint8_t ep_addr, uint32_t bytes, bool success)
 {
     uiox_usb_msc_t *msc = (uiox_usb_msc_t *)drv;
     if (!success || ep_addr != msc->ep_out) return;
     if (bytes < sizeof(uiox_msc_cbw_t)) return;
 
     /* Process CBW */
     memcpy(&msc->cbw, msc->data_buf, sizeof(msc->cbw));
     if (msc->cbw.dCBWSignature != UIOX_MSC_CBW_SIGNATURE) return;
 
     uint32_t data_len = 0;
     bool data_in = (msc->cbw.bmCBWFlags & 0x80u) != 0u;
     int rc = 0;
     if (msc->scsi_cmd)
         rc = msc->scsi_cmd(msc->cbw.bCBWLUN,
                             msc->cbw.CBWCB, msc->cbw.bCBWCBLength,
                             msc->data_buf, &data_len, data_in,
                             msc->scsi_ctx);
 
     /* Send data phase */
     if (data_in && data_len > 0) {
         uiox_usb_urb_t *tx = uiox_usb_buf_alloc();
         if (tx) {
             memcpy(tx->buf, msc->data_buf, data_len);
             tx->buf_len = data_len;
             tx->ep_addr = msc->ep_in;
             tx->type    = UIOX_URB_BULK;
             uiox_usb_if_submit(msc->uif, tx);
         }
     }
 
     /* Send CSW */
     msc->csw.dCSWSignature   = UIOX_MSC_CSW_SIGNATURE;
     msc->csw.dCSWTag         = msc->cbw.dCBWTag;
     msc->csw.dCSWDataResidue = msc->cbw.dCBWDataTransferLength - data_len;
     msc->csw.bCSWStatus      = (rc == 0) ? 0u : 1u;
 
     uiox_usb_urb_t *csw = uiox_usb_buf_alloc();
     if (csw) {
         memcpy(csw->buf, &msc->csw, sizeof(msc->csw));
         csw->buf_len = sizeof(msc->csw);
         csw->ep_addr = msc->ep_in;
         csw->type    = UIOX_URB_BULK;
         uiox_usb_if_submit(msc->uif, csw);
     }
 }
 
 static void msc_tick(uiox_usb_class_drv_t *drv, uint32_t now_ms)
 { (void)drv; (void)now_ms; }
 
 int uiox_usb_msc_init(uiox_usb_msc_t *msc,
                        uint8_t ep_in, uint8_t ep_out, uint8_t max_lun)
 {
     if (!msc) return -EINVAL;
     memset(msc, 0, sizeof(*msc));
     msc->base.name     = "MSC";
     msc->base.bind     = msc_bind;
     msc->base.unbind   = msc_unbind;
     msc->base.setup    = msc_setup;
     msc->base.ep_event = msc_ep_event;
     msc->base.tick     = msc_tick;
     msc->ep_in         = ep_in;
     msc->ep_out        = ep_out;
     msc->max_lun       = max_lun;
     return 0;
 } 
 