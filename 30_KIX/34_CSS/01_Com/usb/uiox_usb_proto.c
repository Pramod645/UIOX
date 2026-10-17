/**
 * @file    uiox_usb_proto.c
 * @brief   UIOX USB protocol layer implementation.
 * @date    2026-05-28
 */

 #include "uiox_usb_proto.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_usb_proto_init(uiox_usb_proto_t          *proto,
                          uiox_usb_if_t             *uif,
                          const uiox_usb_dev_desc_t *dev_desc,
                          const uint8_t             *cfg_buf,
                          uint16_t                   cfg_len)
 {
     if (!proto || !uif || !dev_desc || !cfg_buf) return -EINVAL;
     memset(proto, 0, sizeof(*proto));
     proto->uif          = uif;
     proto->dev_desc     = dev_desc;
     proto->cfg_desc_buf = cfg_buf;
     proto->cfg_desc_len = cfg_len;
     proto->state        = UIOX_USB_DEV_POWERED;
     return 0;
 }
 
 int uiox_usb_proto_add_string(uiox_usb_proto_t *proto,
                                uint8_t idx, const char *str)
 {
     if (!proto || !str) return -EINVAL;
     if (proto->num_strings >= UIOX_USB_MAX_STRINGS) return -ENOSPC;
     proto->strings[proto->num_strings].idx = idx;
     proto->strings[proto->num_strings].str = str;
     proto->num_strings++;
     return 0;
 }
 
 /* -------------------------------------------------------------------------
  * Build a USB string descriptor (UTF-16LE encoded)
  * ---------------------------------------------------------------------- */
 
 static uint16_t build_string_desc(const char *str,
                                    uint8_t *buf, uint16_t max_len)
 {
     uint8_t slen = (uint8_t)strlen(str);
     uint16_t desc_len = (uint16_t)(2u + slen * 2u);
     if (desc_len > max_len) desc_len = max_len;
     buf[0] = (uint8_t)desc_len;
     buf[1] = UIOX_USB_DT_STRING;
     for (uint8_t i = 0; i < slen && (2u + i*2u + 1u) < desc_len; i++) {
         buf[2 + i*2]     = (uint8_t)str[i];
         buf[2 + i*2 + 1] = 0x00u;
     }
     return desc_len;
 }
 
 /* =========================================================================
  * Standard request handler
  * ====================================================================== */
 
 int uiox_usb_proto_setup(uiox_usb_proto_t  *proto,
                           const uiox_usb_setup_t *setup,
                           uint8_t *resp_buf, uint16_t *resp_len)
 {
     if (!proto || !setup || !resp_buf || !resp_len) return -EINVAL;
     *resp_len = 0;
 
     uint8_t req_type = setup->bmRequestType & 0x60u;
 
     /* Only handle standard requests here */
     if (req_type != UIOX_USB_TYPE_STANDARD) return -ENOTSUP;
 
     switch (setup->bRequest) {
 
     case UIOX_USB_REQ_GET_DESCRIPTOR: {
         uint8_t  dt  = (uint8_t)(setup->wValue >> 8u);
         uint8_t  idx = (uint8_t)(setup->wValue & 0xFFu);
         uint16_t len = setup->wLength;
 
         switch (dt) {
         case UIOX_USB_DT_DEVICE:
             *resp_len = (uint16_t)sizeof(*proto->dev_desc);
             if (*resp_len > len) *resp_len = len;
             memcpy(resp_buf, proto->dev_desc, *resp_len);
             break;
 
         case UIOX_USB_DT_CONFIG:
             *resp_len = proto->cfg_desc_len;
             if (*resp_len > len) *resp_len = len;
             memcpy(resp_buf, proto->cfg_desc_buf, *resp_len);
             break;
 
         case UIOX_USB_DT_STRING:
             if (idx == 0u) {
                 /* Language ID descriptor */
                 resp_buf[0] = 0x04u;
                 resp_buf[1] = UIOX_USB_DT_STRING;
                 resp_buf[2] = 0x09u; /* en-US lo */
                 resp_buf[3] = 0x04u; /* en-US hi */
                 *resp_len   = 4u;
             } else {
                 for (uint8_t i = 0; i < proto->num_strings; i++) {
                     if (proto->strings[i].idx == idx) {
                         *resp_len = build_string_desc(
                             proto->strings[i].str,
                             resp_buf, len);
                         break;
                     }
                 }
             }
             break;
 
         default:
             uiox_usb_hw_ep_stall(proto->uif->hw, 0x00u, true);
             return -ENOTSUP;
         }
         break;
     }
 
     case UIOX_USB_REQ_SET_ADDRESS:
     proto->uif->hw->address = (uint8_t)(setup->wValue & 0x7Fu);
     proto->state = UIOX_USB_DEV_ADDRESS;
     {
         const uiox_usb_hw_ops_t *ops =
             (const uiox_usb_hw_ops_t *)proto->uif->hw->priv;
         if (ops && ops->set_address)
             ops->set_address(proto->uif->hw, proto->uif->hw->address);
     }
     *resp_len = 0;
     break;

 case UIOX_USB_REQ_GET_CONFIGURATION:
     resp_buf[0] = proto->active_config;
     *resp_len   = 1u;
     break;

 case UIOX_USB_REQ_SET_CONFIGURATION:
     proto->active_config = (uint8_t)(setup->wValue & 0xFFu);
     proto->state = (proto->active_config) ?
                     UIOX_USB_DEV_CONFIGURED :
                     UIOX_USB_DEV_ADDRESS;
     *resp_len = 0;
     break;

 case UIOX_USB_REQ_GET_STATUS: {
     uint16_t status = 0u;
     if (proto->self_powered)   status |= (1u << 0);
     if (proto->remote_wakeup)  status |= (1u << 1);
     resp_buf[0] = (uint8_t)(status & 0xFFu);
     resp_buf[1] = (uint8_t)(status >> 8u);
     *resp_len   = 2u;
     break;
 }

 case UIOX_USB_REQ_SET_FEATURE:
     if (setup->wValue == 1u) /* DEVICE_REMOTE_WAKEUP */
         proto->remote_wakeup = 1u;
     *resp_len = 0;
     break;

 case UIOX_USB_REQ_CLEAR_FEATURE:
     if (setup->wValue == 1u)
         proto->remote_wakeup = 0u;
     else if (setup->wValue == 0u && /* ENDPOINT_HALT */
              (setup->bmRequestType & 0x1Fu) == UIOX_USB_RECIP_ENDPOINT)
         uiox_usb_hw_ep_stall(proto->uif->hw,
                               (uint8_t)(setup->wIndex & 0xFFu), false);
     *resp_len = 0;
     break;

 default:
     uiox_usb_hw_ep_stall(proto->uif->hw, 0x00u, true);
     return -ENOTSUP;
 }

 return 0;
}

void uiox_usb_proto_ep0_in_done(uiox_usb_proto_t *proto)
{
 if (!proto) return;
 proto->uif->ep0_state = UIOX_EP0_STATUS;
}

void uiox_usb_proto_reset(uiox_usb_proto_t *proto)
{
 if (!proto) return;
 proto->state        = UIOX_USB_DEV_DEFAULT;
 proto->active_config= 0u;
 proto->uif->hw->address = 0u;
 proto->uif->ep0_state   = UIOX_EP0_IDLE;
 if (proto->uif->ep0_urb) {
     uiox_usb_buf_free(proto->uif->ep0_urb);
     proto->uif->ep0_urb = NULL;
 }
}

void uiox_usb_proto_suspend(uiox_usb_proto_t *proto)
{
 if (!proto) return;
 proto->state = UIOX_USB_DEV_SUSPENDED;
}

void uiox_usb_proto_resume(uiox_usb_proto_t *proto)
{
 if (!proto) return;
 proto->state = proto->active_config ?
                UIOX_USB_DEV_CONFIGURED :
                UIOX_USB_DEV_ADDRESS;
}
 