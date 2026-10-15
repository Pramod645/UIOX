/**
 * @file    uiox_usb_subsys.c
 * @brief   UIOX USB subsystem implementation.
 * @date    2026-05-28
 */

 #include "uiox_usb_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 static void fire(uiox_usb_subsys_t *sys, uiox_usb_evt_t evt, uint8_t ep)
 {
     if (sys->evt_cb) sys->evt_cb(evt, ep, sys->evt_ctx);
 }
 
 int uiox_usb_subsys_init(uiox_usb_subsys_t         *sys,
                           uiox_usb_hw_t             *hw,
                           const uiox_usb_dev_desc_t *dev_desc,
                           const uint8_t             *cfg_buf,
                           uint16_t                   cfg_len)
 {
     if (!sys || !hw || !dev_desc || !cfg_buf) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
 
     int rc = uiox_usb_if_config(&sys->uif, hw);
     if (rc < 0) return rc;
 
     rc = uiox_usb_proto_init(&sys->proto, &sys->uif,
                                dev_desc, cfg_buf, cfg_len);
     if (rc < 0) return rc;
 
     /* Open EP0 */
     rc = uiox_usb_if_ep_open(&sys->uif, 0x00u,
                                UIOX_USB_EP_CTRL, UIOX_USB_EP0_MPS, 0u);
     if (rc < 0) return rc;
     rc = uiox_usb_if_ep_open(&sys->uif, 0x80u,
                                UIOX_USB_EP_CTRL, UIOX_USB_EP0_MPS, 0u);
     if (rc < 0) return rc;
 
     sys->state               = UIOX_USB_SUBSYS_STOPPED;
     sys->suspend_timeout_ms  = 3000u;
     return 0;
 }
 
 int uiox_usb_subsys_start(uiox_usb_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     uiox_usb_buf_init();
     int rc = uiox_usb_hw_start(sys->uif.hw);
     if (rc < 0) return rc;
     sys->state = UIOX_USB_SUBSYS_POWERED;
     return 0;
 }
 
 void uiox_usb_subsys_stop(uiox_usb_subsys_t *sys)
 {
     if (!sys) return;
     /* Unbind all class drivers */
     for (uint8_t i = 0; i < sys->class_count; i++)
         if (sys->class_drv[i] && sys->class_drv[i]->unbind)
             sys->class_drv[i]->unbind(sys->class_drv[i]);
     uiox_usb_hw_stop(sys->uif.hw);
     sys->state = UIOX_USB_SUBSYS_STOPPED;
 }
 
 int uiox_usb_subsys_register(uiox_usb_subsys_t    *sys,
                                uiox_usb_class_drv_t *drv)
 {
     if (!sys || !drv) return -EINVAL;
     if (sys->class_count >= UIOX_USB_MAX_CLASS_DRV) return -ENOSPC;
     sys->class_drv[sys->class_count++] = drv;
     return 0;
 }
 
 void uiox_usb_subsys_set_evt_cb(uiox_usb_subsys_t *sys,
                                  uiox_usb_evt_cb_t  cb,
                                  void              *ctx)
 {
     if (!sys) return;
     sys->evt_cb  = cb;
     sys->evt_ctx = ctx;
 }
 
 void uiox_usb_subsys_setup_rx(uiox_usb_subsys_t      *sys,
                                 const uiox_usb_setup_t *setup)
 {
     if (!sys || !setup) return;
     sys->stats.setup_count++;
 
     uint16_t resp_len = 0;
     int rc = uiox_usb_proto_setup(&sys->proto, setup,
                                    sys->ep0_resp, &resp_len);
 
     if (rc == -ENOTSUP) {
         /* Delegate to class drivers */
         uint8_t req_type = setup->bmRequestType & 0x60u;
         if (req_type == UIOX_USB_TYPE_CLASS ||
             req_type == UIOX_USB_TYPE_VENDOR) {
             for (uint8_t i = 0; i < sys->class_count; i++) {
                 if (sys->class_drv[i] && sys->class_drv[i]->setup) {
                     rc = sys->class_drv[i]->setup(sys->class_drv[i],
                                                    setup,
                                                    sys->ep0_resp,
                                                    &resp_len);
                     if (rc == 0) break;
                 }
             }
         }
     }
 
     /* Check if SET_CONFIGURATION — bind class drivers */
     if (setup->bRequest == UIOX_USB_REQ_SET_CONFIGURATION &&
         setup->wValue != 0u) {
         for (uint8_t i = 0; i < sys->class_count; i++) {
             if (sys->class_drv[i] && sys->class_drv[i]->bind)
                 sys->class_drv[i]->bind(sys->class_drv[i], &sys->uif);
         }
         sys->state = UIOX_USB_SUBSYS_RUNNING;
         fire(sys, UIOX_USB_EVT_CONFIGURED, 0u);
     }
 
     /* Send EP0 IN response */
     if (resp_len > 0) {
         uiox_usb_urb_t *urb = uiox_usb_buf_alloc();
         if (urb) {
             memcpy(urb->buf, sys->ep0_resp, resp_len);
             urb->buf_len = resp_len;
             urb->ep_addr = 0x80u;  /* EP0 IN */
             urb->type    = UIOX_URB_CTRL;
             uiox_usb_if_submit(&sys->uif, urb);
         }
     } else if (rc == 0) {
         /* ZLP status */
         uiox_usb_urb_t *zlp = uiox_usb_buf_alloc();
         if (zlp) {
             zlp->buf_len = 0;
             zlp->ep_addr = 0x80u;
             zlp->type    = UIOX_URB_CTRL;
             uiox_usb_if_submit(&sys->uif, zlp);
         }
     }
 }
 
 void uiox_usb_subsys_ep_complete(uiox_usb_subsys_t *sys,
                                   uint8_t ep_addr, uint32_t bytes,
                                   bool success)
 {
     if (!sys) return;
     sys->stats.ep_complete_count++;
 
     uiox_usb_if_complete(&sys->uif, ep_addr, success, bytes);
 
     /* Notify class drivers */
     for (uint8_t i = 0; i < sys->class_count; i++) {
         if (sys->class_drv[i] && sys->class_drv[i]->ep_event)
             sys->class_drv[i]->ep_event(sys->class_drv[i],
                                          ep_addr, bytes, success);
     }
     fire(sys, UIOX_USB_EVT_EP_COMPLETE, ep_addr);
 }
 
 void uiox_usb_subsys_reset(uiox_usb_subsys_t *sys)
 {
     if (!sys) return;
     sys->stats.reset_count++;
     uiox_usb_proto_reset(&sys->proto);
     sys->state = UIOX_USB_SUBSYS_ENUMERATED;
     fire(sys, UIOX_USB_EVT_RESET, 0u);
 }
 
 void uiox_usb_subsys_suspend(uiox_usb_subsys_t *sys)
 {
     if (!sys) return;
     sys->stats.suspend_count++;
     uiox_usb_proto_suspend(&sys->proto);
     sys->state = UIOX_USB_SUBSYS_SUSPENDED;
     fire(sys, UIOX_USB_EVT_SUSPEND, 0u);
 }
 
 void uiox_usb_subsys_resume(uiox_usb_subsys_t *sys)
 {
     if (!sys) return;
     sys->stats.resume_count++;
     uiox_usb_proto_resume(&sys->proto);
     sys->state = UIOX_USB_SUBSYS_RUNNING;
     fire(sys, UIOX_USB_EVT_RESUME, 0u);
 }
 
 void uiox_usb_subsys_sof(uiox_usb_subsys_t *sys)
 {
     if (!sys) return;
     sys->stats.sof_count++;
     sys->frame_num++;
 }
 
 void uiox_usb_subsys_process(uiox_usb_subsys_t *sys)
 {
     if (!sys || sys->state == UIOX_USB_SUBSYS_STOPPED) return;
     /* Poll for connect/disconnect */
     bool connected = uiox_usb_hw_connected(sys->uif.hw);
     if (connected && sys->state == UIOX_USB_SUBSYS_POWERED) {
         sys->state = UIOX_USB_SUBSYS_ENUMERATED;
         fire(sys, UIOX_USB_EVT_CONNECT, 0u);
     } else if (!connected && sys->state != UIOX_USB_SUBSYS_POWERED) {
         /* Unbind class drivers */
         for (uint8_t i = 0; i < sys->class_count; i++)
             if (sys->class_drv[i] && sys->class_drv[i]->unbind)
                 sys->class_drv[i]->unbind(sys->class_drv[i]);
         sys->state = UIOX_USB_SUBSYS_POWERED;
         fire(sys, UIOX_USB_EVT_DISCONNECT, 0u);
     }
 }
 
 void uiox_usb_subsys_tick(uiox_usb_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state == UIOX_USB_SUBSYS_STOPPED) return;
     uiox_usb_subsys_process(sys);
     /* Tick class drivers */
     for (uint8_t i = 0; i < sys->class_count; i++)
         if (sys->class_drv[i] && sys->class_drv[i]->tick)
             sys->class_drv[i]->tick(sys->class_drv[i], now_ms);
 }
 