/**
 * @file    uiox_usb_if.h
 * @brief   UIOX USB interface driver (endpoint management, URB queue).
 *
 * Sits between HAL and protocol layer. Manages:
 *   - Endpoint activation / deactivation
 *   - URB submission queue per endpoint
 *   - Transfer completion dispatch
 *   - Endpoint statistics
 *   - Control endpoint 0 state machine
 *
 * @date    2026-05-28
 */
//Layer 2 — Interface Driver
 #ifndef UIOX_USB_IF_H
 #define UIOX_USB_IF_H
 
 #include "uiox_usb_hw.h"
 #include "uiox_usb_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Per-endpoint URB queue
  * ====================================================================== */
 
 #define UIOX_USB_EP_QUEUE_DEPTH     8
 
 typedef struct {
     uiox_usb_urb_t *urbs[UIOX_USB_EP_QUEUE_DEPTH];
     uint8_t         head;
     uint8_t         tail;
     uint8_t         count;
 } uiox_usb_ep_queue_t;
 
 /* =========================================================================
  * Interface statistics
  * ====================================================================== */
 
 typedef struct {
     uint64_t  tx_bytes;
     uint64_t  rx_bytes;
     uint64_t  tx_urbs;
     uint64_t  rx_urbs;
     uint64_t  errors;
     uint64_t  stalls;
     uint64_t  timeouts;
 } uiox_usb_if_stats_t;
 
 /* =========================================================================
  * Control EP0 state machine
  * ====================================================================== */
 
 typedef enum {
     UIOX_EP0_IDLE = 0,
     UIOX_EP0_SETUP,
     UIOX_EP0_DATA_IN,
     UIOX_EP0_DATA_OUT,
     UIOX_EP0_STATUS,
 } uiox_ep0_state_t;
 
 /* =========================================================================
  * Interface descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_usb_hw_t        *hw;
     uiox_usb_ep_queue_t   queues[UIOX_USB_MAX_EP];
     uiox_usb_if_stats_t   stats;
     uiox_ep0_state_t      ep0_state;
     uiox_usb_urb_t       *ep0_urb;     /**< Active control transfer        */
     bool                  primed;
 } uiox_usb_if_t;
 
 /* =========================================================================
  * Interface API
  * ====================================================================== */
 
 int  uiox_usb_if_config    (uiox_usb_if_t *uif, uiox_usb_hw_t *hw);
 
 /** Configure and activate an endpoint. */
 int  uiox_usb_if_ep_open   (uiox_usb_if_t *uif, uint8_t ep_addr,
                              uiox_usb_ep_type_t type, uint16_t mps,
                              uint8_t interval);
 
 /** Deactivate an endpoint and drain its queue. */
 void uiox_usb_if_ep_close  (uiox_usb_if_t *uif, uint8_t ep_addr);
 
 /** Submit a URB for transfer. */
 int  uiox_usb_if_submit    (uiox_usb_if_t *uif, uiox_usb_urb_t *urb);
 
 /** Process completion of one endpoint (call from IRQ bottom-half). */
 void uiox_usb_if_complete  (uiox_usb_if_t *uif, uint8_t ep_addr,
                              bool success, uint32_t bytes);
 
 /** Process an incoming SETUP packet on EP0. */
 int  uiox_usb_if_setup_rx  (uiox_usb_if_t *uif,
                              const uiox_usb_setup_t *setup);
 
 void uiox_usb_if_stats_get (const uiox_usb_if_t *uif,
                              uiox_usb_if_stats_t *out);
 void uiox_usb_if_stats_reset(uiox_usb_if_t *uif);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_USB_IF_H */
 