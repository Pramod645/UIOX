/**
 * @file    uiox_usb_device.h
 * @brief   UIOX USB top-level application-facing device API.
 * @date    2026-05-28
 */
//Layer 5 — Device API
 #ifndef UIOX_USB_DEVICE_H
 #define UIOX_USB_DEVICE_H
 
 #include "uiox_usb_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_usb_hw_t             *hw;
     const uiox_usb_hw_ops_t   *hw_ops;
     const uiox_usb_dev_desc_t *dev_desc;
     const uint8_t             *cfg_buf;
     uint16_t                   cfg_len;
     uiox_usb_evt_cb_t          evt_cb;
     void                      *evt_ctx;
     /* String descriptors */
     struct { uint8_t idx; const char *str; } strings[UIOX_USB_MAX_STRINGS];
     uint8_t num_strings;
 } uiox_usb_open_params_t;
 
 typedef struct {
     uiox_usb_subsys_t  subsys;
     uiox_usb_hw_t     *hw;
     bool               open;
 } uiox_usb_device_t;
 
 /* =========================================================================
  * Application API
  * ====================================================================== */
 
 int  uiox_usb_open        (uiox_usb_device_t           *dev,
                             const uiox_usb_open_params_t *p);
 int  uiox_usb_start       (uiox_usb_device_t *dev);
 void uiox_usb_stop        (uiox_usb_device_t *dev);
 void uiox_usb_close       (uiox_usb_device_t *dev);
 void uiox_usb_tick        (uiox_usb_device_t *dev, uint32_t now_ms);
 void uiox_usb_process     (uiox_usb_device_t *dev);
 
 int  uiox_usb_register_class(uiox_usb_device_t    *dev,
                                uiox_usb_class_drv_t *drv);
 
 /** Simulate incoming SETUP (test/demo use). */
 void uiox_usb_inject_setup(uiox_usb_device_t      *dev,
                             const uiox_usb_setup_t *setup);
 
 /** Simulate EP completion (test/demo use). */
 void uiox_usb_inject_ep_complete(uiox_usb_device_t *dev,
                                   uint8_t ep_addr, uint32_t bytes,
                                   bool success);
 
 /** Simulate bus reset. */
 void uiox_usb_inject_reset  (uiox_usb_device_t *dev);
 void uiox_usb_inject_suspend(uiox_usb_device_t *dev);
 void uiox_usb_inject_resume (uiox_usb_device_t *dev);
 
 bool uiox_usb_connected   (const uiox_usb_device_t *dev);
 bool uiox_usb_configured  (const uiox_usb_device_t *dev);
 
 void uiox_usb_print_stats (const uiox_usb_device_t *dev);
 
 const char *uiox_usb_state_name(uiox_usb_subsys_state_t state);
 const char *uiox_usb_evt_name  (uiox_usb_evt_t evt);
 const char *uiox_usb_speed_name(uiox_usb_speed_t speed);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_USB_DEVICE_H */
 