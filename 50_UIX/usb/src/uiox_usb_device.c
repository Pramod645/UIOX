/**
 * @file    uiox_usb_device.c
 * @brief   UIOX USB device API implementation.
 * @date    2026-05-28
 */

 #include "uiox_usb_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_usb_open(uiox_usb_device_t           *dev,
                    const uiox_usb_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
 
     int rc = uiox_usb_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
 
     rc = uiox_usb_subsys_init(&dev->subsys, p->hw,
                                p->dev_desc, p->cfg_buf, p->cfg_len);
     if (rc < 0) return rc;
 
     /* Add string descriptors */
     for (uint8_t i = 0; i < p->num_strings; i++)
         uiox_usb_proto_add_string(&dev->subsys.proto,
                                    p->strings[i].idx,
                                    p->strings[i].str);
 
     if (p->evt_cb)
         uiox_usb_subsys_set_evt_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
 
     dev->open = true;
     return 0;
 }
 
 int uiox_usb_start(uiox_usb_device_t *dev)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_usb_subsys_start(&dev->subsys);
 }
 
 void uiox_usb_stop(uiox_usb_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_usb_subsys_stop(&dev->subsys);
 }
 
 void uiox_usb_close(uiox_usb_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_usb_stop(dev);
     uiox_usb_hw_deinit(dev->hw);
     dev->open = false;
 }
 
 void uiox_usb_tick(uiox_usb_device_t *dev, uint32_t now_ms)
 {
     if (!dev || !dev->open) return;
     uiox_usb_subsys_tick(&dev->subsys, now_ms);
 }
 
 void uiox_usb_process(uiox_usb_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_usb_subsys_process(&dev->subsys);
 }
 
 int uiox_usb_register_class(uiox_usb_device_t    *dev,
                               uiox_usb_class_drv_t *drv)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_usb_subsys_register(&dev->subsys, drv);
 }
 
 void uiox_usb_inject_setup(uiox_usb_device_t      *dev,
                             const uiox_usb_setup_t *setup)
 {
     if (!dev || !setup) return;
     uiox_usb_subsys_setup_rx(&dev->subsys, setup);
 }
 
 void uiox_usb_inject_ep_complete(uiox_usb_device_t *dev,
                                   uint8_t ep_addr, uint32_t bytes,
                                   bool success)
 {
     if (!dev) return;
     uiox_usb_subsys_ep_complete(&dev->subsys, ep_addr, bytes, success);
 }
 
 void uiox_usb_inject_reset(uiox_usb_device_t *dev)
 { if (dev) uiox_usb_subsys_reset(&dev->subsys); }
 
 void uiox_usb_inject_suspend(uiox_usb_device_t *dev)
 { if (dev) uiox_usb_subsys_suspend(&dev->subsys); }
 
 void uiox_usb_inject_resume(uiox_usb_device_t *dev)
 { if (dev) uiox_usb_subsys_resume(&dev->subsys); }
 
 bool uiox_usb_connected(const uiox_usb_device_t *dev)
 {
     if (!dev || !dev->open) return false;
     return dev->subsys.state >= UIOX_USB_SUBSYS_ENUMERATED;
 }
 
 bool uiox_usb_configured(const uiox_usb_device_t *dev)
 {
     if (!dev || !dev->open) return false;
     return dev->subsys.state == UIOX_USB_SUBSYS_RUNNING;
 }
 
 void uiox_usb_print_stats(const uiox_usb_device_t *dev)
 {
     if (!dev) return;
     const uiox_usb_subsys_stats_t *s = &dev->subsys.stats;
     printf("  State          : %s\n",
            uiox_usb_state_name(dev->subsys.state));
     printf("  SOF count      : %llu\n", (unsigned long long)s->sof_count);
     printf("  Reset count    : %llu\n", (unsigned long long)s->reset_count);
     printf("  Suspend count  : %llu\n", (unsigned long long)s->suspend_count);
     printf("  Resume count   : %llu\n", (unsigned long long)s->resume_count);
     printf("  SETUP count    : %llu\n", (unsigned long long)s->setup_count);
     printf("  EP completions : %llu\n",
            (unsigned long long)s->ep_complete_count);
     printf("  Errors         : %llu\n", (unsigned long long)s->error_count);
     uiox_usb_if_stats_t is;
     uiox_usb_if_stats_get(&dev->subsys.uif, &is);
     printf("  TX bytes       : %llu\n", (unsigned long long)is.tx_bytes);
     printf("  RX bytes       : %llu\n", (unsigned long long)is.rx_bytes);
     printf("  TX URBs        : %llu\n", (unsigned long long)is.tx_urbs);
     printf("  RX URBs        : %llu\n", (unsigned long long)is.rx_urbs);
     printf("  URB buf free   : %u / %u\n",
            uiox_usb_buf_free_count(), UIOX_USB_URB_POOL_SIZE);
 }
 
 const char *uiox_usb_state_name(uiox_usb_subsys_state_t s)
 {
     switch (s) {
     case UIOX_USB_SUBSYS_STOPPED:    return "STOPPED";
     case UIOX_USB_SUBSYS_POWERED:    return "POWERED";
     case UIOX_USB_SUBSYS_ENUMERATED: return "ENUMERATED";
     case UIOX_USB_SUBSYS_RUNNING:    return "RUNNING";
     case UIOX_USB_SUBSYS_SUSPENDED:  return "SUSPENDED";
     default:                          return "UNKNOWN";
     }
 }
 
 const char *uiox_usb_evt_name(uiox_usb_evt_t e)
 {
     switch (e) {
     case UIOX_USB_EVT_CONNECT:     return "CONNECT";
     case UIOX_USB_EVT_DISCONNECT:  return "DISCONNECT";
     case UIOX_USB_EVT_RESET:       return "RESET";
     case UIOX_USB_EVT_SUSPEND:     return "SUSPEND";
     case UIOX_USB_EVT_RESUME:      return "RESUME";
     case UIOX_USB_EVT_CONFIGURED:  return "CONFIGURED";
     case UIOX_USB_EVT_EP_COMPLETE: return "EP_COMPLETE";
     case UIOX_USB_EVT_ERROR:       return "ERROR";
     default:                        return "UNKNOWN";
     }
 }
 
 const char *uiox_usb_speed_name(uiox_usb_speed_t sp)
 {
     switch (sp) {
     case UIOX_USB_SPEED_LOW:        return "Low-Speed  (1.5 Mbit/s)";
     case UIOX_USB_SPEED_FULL:       return "Full-Speed (12 Mbit/s)";
     case UIOX_USB_SPEED_HIGH:       return "High-Speed (480 Mbit/s)";
     case UIOX_USB_SPEED_SUPER:      return "SuperSpeed (5 Gbit/s)";
     case UIOX_USB_SPEED_SUPER_PLUS: return "SuperSpeed+ (10 Gbit/s)";
     default:                         return "Unknown";
     }
 }
 