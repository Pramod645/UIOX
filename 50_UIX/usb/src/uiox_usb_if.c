/**
 * @file    uiox_usb_if.c
 * @brief   UIOX USB interface driver implementation.
 * @date    2026-05-28
 */

 #include "uiox_usb_if.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_usb_if_config(uiox_usb_if_t *uif, uiox_usb_hw_t *hw)
 {
     if (!uif || !hw) return -EINVAL;
     memset(uif, 0, sizeof(*uif));
     uif->hw      = hw;
     uif->ep0_state = UIOX_EP0_IDLE;
     uif->primed  = true;
     return 0;
 }
 
 int uiox_usb_if_ep_open(uiox_usb_if_t *uif, uint8_t ep_addr,
                          uiox_usb_ep_type_t type, uint16_t mps,
                          uint8_t interval)
 {
     if (!uif || !uif->hw) return -EINVAL;
     int rc = uiox_usb_hw_ep_config(uif->hw, ep_addr, type, mps, interval);
     if (rc < 0) return rc;
     uint8_t idx = ep_addr & 0x0Fu;
     memset(&uif->queues[idx], 0, sizeof(uif->queues[idx]));
     return 0;
 }
 
 void uiox_usb_if_ep_close(uiox_usb_if_t *uif, uint8_t ep_addr)
 {
     if (!uif || !uif->hw) return;
     uint8_t idx = ep_addr & 0x0Fu;
     const uiox_usb_hw_ops_t *ops =
         (const uiox_usb_hw_ops_t *)uif->hw->priv;
 
     /* Drain pending URBs */
     uiox_usb_ep_queue_t *q = &uif->queues[idx];
     while (q->count) {
         uiox_usb_urb_t *u = q->urbs[q->head % UIOX_USB_EP_QUEUE_DEPTH];
         q->head++; q->count--;
         u->status = UIOX_URB_ERROR;
         if (u->complete) u->complete(u, u->ctx);
         uiox_usb_buf_free(u);
     }
 
     if (ops && ops->ep_disable)
         ops->ep_disable(uif->hw, ep_addr);
 }
 
 int uiox_usb_if_submit(uiox_usb_if_t *uif, uiox_usb_urb_t *urb)
 {
     if (!uif || !urb) return -EINVAL;
     uint8_t idx = urb->ep_addr & 0x0Fu;
     uiox_usb_ep_queue_t *q = &uif->queues[idx];
 
     if (q->count >= UIOX_USB_EP_QUEUE_DEPTH) return -ENOSPC;
 
     urb->status = UIOX_URB_PENDING;
     q->urbs[q->tail % UIOX_USB_EP_QUEUE_DEPTH] = urb;
     q->tail++;
     q->count++;
 
     /* Submit first URB immediately if queue was empty */
     if (q->count == 1) {
         bool is_in = (urb->ep_addr & UIOX_USB_EP_DIR_IN) != 0u;
         int rc;
         if (is_in) {
             rc = uiox_usb_hw_tx(uif->hw, urb->ep_addr,
                                  urb->paddr, urb->buf_len);
             if (rc == 0) uif->stats.tx_urbs++;
         } else {
             rc = uiox_usb_hw_rx(uif->hw, urb->ep_addr,
                                  urb->paddr, urb->buf_len);
             if (rc == 0) uif->stats.rx_urbs++;
         }
         if (rc < 0) {
             urb->status = UIOX_URB_ERROR;
             q->head++; q->count--;
             uif->stats.errors++;
             return rc;
         }
     }
     return 0;
 }
 
 void uiox_usb_if_complete(uiox_usb_if_t *uif, uint8_t ep_addr,
                            bool success, uint32_t bytes)
 {
     if (!uif) return;
     uint8_t idx = ep_addr & 0x0Fu;
     uiox_usb_ep_queue_t *q = &uif->queues[idx];
     if (!q->count) return;
 
     uiox_usb_urb_t *done = q->urbs[q->head % UIOX_USB_EP_QUEUE_DEPTH];
     q->head++; q->count--;
 
     done->actual_len = bytes;
     done->status     = success ? UIOX_URB_COMPLETE : UIOX_URB_ERROR;
 
     bool is_in = (ep_addr & UIOX_USB_EP_DIR_IN) != 0u;
     if (is_in)  uif->stats.tx_bytes += bytes;
     else        uif->stats.rx_bytes += bytes;
     if (!success) uif->stats.errors++;
 
     /* Fire completion callback */
     if (done->complete) done->complete(done, done->ctx);
 
     /* Submit next queued URB */
     if (q->count) {
         uiox_usb_urb_t *next = q->urbs[q->head % UIOX_USB_EP_QUEUE_DEPTH];
         if (is_in)
             uiox_usb_hw_tx(uif->hw, ep_addr, next->paddr, next->buf_len);
         else
             uiox_usb_hw_rx(uif->hw, ep_addr, next->paddr, next->buf_len);
     }
 }
 
 int uiox_usb_if_setup_rx(uiox_usb_if_t *uif,
                           const uiox_usb_setup_t *setup)
 {
     if (!uif || !setup) return -EINVAL;
     uif->ep0_state = UIOX_EP0_SETUP;
     /* Allocate EP0 URB */
     if (uif->ep0_urb) {
         uiox_usb_buf_free(uif->ep0_urb);
         uif->ep0_urb = NULL;
     }
     uiox_usb_urb_t *urb = uiox_usb_buf_alloc();
     if (!urb) return -ENOMEM;
     memcpy(&urb->setup, setup, sizeof(*setup));
     urb->type    = UIOX_URB_CTRL;
     urb->ep_addr = 0x00u;
     uif->ep0_urb = urb;
     /* Data phase direction */
     if (setup->wLength > 0) {
         uif->ep0_state = (setup->bmRequestType & UIOX_USB_DIR_DEV_TO_HOST) ?
                           UIOX_EP0_DATA_IN : UIOX_EP0_DATA_OUT;
     } else {
         uif->ep0_state = UIOX_EP0_STATUS;
     }
     return 0;
 }
 
 void uiox_usb_if_stats_get(const uiox_usb_if_t *uif,
                             uiox_usb_if_stats_t *out)
 {
     if (!uif || !out) return;
     memcpy(out, &uif->stats, sizeof(*out));
 }
 
 void uiox_usb_if_stats_reset(uiox_usb_if_t *uif)
 {
     if (!uif) return;
     memset(&uif->stats, 0, sizeof(uif->stats));
 }
 