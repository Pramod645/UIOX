/**
 * @file    uiox_usb_subsys.h
 * @brief   UIOX USB subsystem — enumeration, class binding, power, events.
 *
 * Top subsystem layer. Manages:
 *   - USB device enumeration pipeline (reset → address → configure)
 *   - Class driver registration and binding
 *   - USB power management (suspend, resume, remote wakeup)
 *   - SOF frame counter and timing
 *   - USB event callbacks (connect, disconnect, suspend, configure)
 *   - Per-frame statistics
 *
 * @date    2026-05-28
 */
//Layer 4 — USB Subsystem
 #ifndef UIOX_USB_SUBSYS_H
 #define UIOX_USB_SUBSYS_H
 
 #include "uiox_usb_proto.h"
 #include "uiox_usb_class.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * USB events
  * ====================================================================== */
 
 typedef enum {
     UIOX_USB_EVT_CONNECT = 0,
     UIOX_USB_EVT_DISCONNECT,
     UIOX_USB_EVT_RESET,
     UIOX_USB_EVT_SUSPEND,
     UIOX_USB_EVT_RESUME,
     UIOX_USB_EVT_CONFIGURED,
     UIOX_USB_EVT_EP_COMPLETE,
     UIOX_USB_EVT_ERROR,
 } uiox_usb_evt_t;
 
 typedef void (*uiox_usb_evt_cb_t)(uiox_usb_evt_t evt,
                                    uint8_t        ep_addr,
                                    void          *ctx);
 
 /* =========================================================================
  * Subsystem state
  * ====================================================================== */
 
 typedef enum {
     UIOX_USB_SUBSYS_STOPPED = 0,
     UIOX_USB_SUBSYS_POWERED,
     UIOX_USB_SUBSYS_ENUMERATED,
     UIOX_USB_SUBSYS_RUNNING,
     UIOX_USB_SUBSYS_SUSPENDED,
 } uiox_usb_subsys_state_t;
 
 /* =========================================================================
  * Subsystem statistics
  * ====================================================================== */
 
 typedef struct {
     uint64_t  sof_count;
     uint64_t  reset_count;
     uint64_t  suspend_count;
     uint64_t  resume_count;
     uint64_t  setup_count;
     uint64_t  ep_complete_count;
     uint64_t  error_count;
 } uiox_usb_subsys_stats_t;
 
 /* =========================================================================
  * Subsystem descriptor
  * ====================================================================== */
 
 #define UIOX_USB_MAX_CLASS_DRV  4
 #define UIOX_USB_EP0_MPS        64
 
 typedef struct {
     uiox_usb_if_t             uif;
     uiox_usb_proto_t          proto;
     uiox_usb_subsys_state_t   state;
     uiox_usb_subsys_stats_t   stats;
 
     /* Class drivers */
     uiox_usb_class_drv_t     *class_drv[UIOX_USB_MAX_CLASS_DRV];
     uint8_t                   class_count;
 
     /* Events */
     uiox_usb_evt_cb_t         evt_cb;
     void                     *evt_ctx;
 
     /* EP0 response buffer */
     uint8_t                   ep0_resp[512];
 
     /* SOF frame number */
     uint32_t                  frame_num;
 
     /* Suspend timeout */
     uint32_t                  suspend_ms;
     uint32_t                  suspend_timeout_ms;
 } uiox_usb_subsys_t;
 
 /* =========================================================================
  * Subsystem API
  * ====================================================================== */
 
 int  uiox_usb_subsys_init       (uiox_usb_subsys_t          *sys,
                                   uiox_usb_hw_t              *hw,
                                   const uiox_usb_dev_desc_t  *dev_desc,
                                   const uint8_t              *cfg_buf,
                                   uint16_t                    cfg_len);
 
 int  uiox_usb_subsys_start      (uiox_usb_subsys_t *sys);
 void uiox_usb_subsys_stop       (uiox_usb_subsys_t *sys);
 
 /** Register a class driver (bind called on SET_CONFIGURATION). */
 int  uiox_usb_subsys_register   (uiox_usb_subsys_t    *sys,
                                   uiox_usb_class_drv_t *drv);
 
 /** Set event callback. */
 void uiox_usb_subsys_set_evt_cb (uiox_usb_subsys_t *sys,
                                   uiox_usb_evt_cb_t  cb,
                                   void              *ctx);
 
 /** Process all pending hardware events — call from main loop or task. */
 void uiox_usb_subsys_process    (uiox_usb_subsys_t *sys);
 
 /** Periodic tick — drives suspend timeout, class driver ticks. */
 void uiox_usb_subsys_tick       (uiox_usb_subsys_t *sys, uint32_t now_ms);
 
 /** Handle incoming SETUP packet (called from IRQ bottom-half). */
 void uiox_usb_subsys_setup_rx   (uiox_usb_subsys_t      *sys,
                                   const uiox_usb_setup_t *setup);
 
 /** Handle EP transfer completion (called from IRQ bottom-half). */
 void uiox_usb_subsys_ep_complete(uiox_usb_subsys_t *sys,
                                   uint8_t ep_addr, uint32_t bytes,
                                   bool success);
 
 /** Handle bus reset (called from IRQ). */
 void uiox_usb_subsys_reset      (uiox_usb_subsys_t *sys);
 
 /** Handle suspend (called from IRQ). */
 void uiox_usb_subsys_suspend    (uiox_usb_subsys_t *sys);
 
 /** Handle resume (called from IRQ). */
 void uiox_usb_subsys_resume     (uiox_usb_subsys_t *sys);
 
 /** Handle SOF (called from IRQ). */
 void uiox_usb_subsys_sof        (uiox_usb_subsys_t *sys);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_USB_SUBSYS_H */
 