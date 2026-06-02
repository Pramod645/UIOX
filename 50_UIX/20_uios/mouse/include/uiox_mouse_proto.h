/**
 * @file    uiox_mouse_proto.h
 * @brief   UIOX Mouse protocol layer (HID, PS/2, I2C trackpad).
 *
 * Decodes protocol-specific report formats:
 *   - USB HID Boot Protocol: 3-byte report
 *   - USB HID Report Protocol: descriptor-driven
 *   - PS/2: 3/4-byte packets with sync bit validation
 *   - IntelliMouse: 4-byte with scroll wheel
 *   - I2C Synaptics RMI4: register-based coordinate
 *   - Serial Microsoft 2-button: 3-byte
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_MOUSE_PROTO_H
 #define UIOX_MOUSE_PROTO_H
 
 #include "uiox_mouse_if.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Protocol types
  * ====================================================================== */
 
 typedef enum {
     UIOX_MOUSE_PROTO_HID_BOOT = 0,  /**< USB HID Boot (3 bytes)          */
     UIOX_MOUSE_PROTO_HID_REPORT,     /**< USB HID Report Protocol         */
     UIOX_MOUSE_PROTO_PS2_STD,        /**< PS/2 standard (3 bytes)         */
     UIOX_MOUSE_PROTO_PS2_INTELLI,    /**< PS/2 IntelliMouse (4 bytes)     */
     UIOX_MOUSE_PROTO_SYNAPTICS,      /**< I2C Synaptics RMI4              */
     UIOX_MOUSE_PROTO_ELAN,           /**< I2C ELAN trackpad               */
     UIOX_MOUSE_PROTO_SERIAL_MS,      /**< Serial Microsoft 2-button       */
 } uiox_mouse_proto_t;
 
 /* =========================================================================
  * Protocol context
  * ====================================================================== */
 
 typedef struct {
     uiox_mouse_proto_t   proto;
     uiox_mouse_if_t     *mif;
 
     /* PS/2 partial packet reassembly */
     uint8_t              ps2_buf[4];
     uint8_t              ps2_len;
     uint8_t              ps2_expected;
 
     /* Serial reassembly */
     uint8_t              serial_buf[5];
     uint8_t              serial_len;
 
     /* HID Report descriptor parsing (simplified) */
     uint8_t              report_id;
     bool                 has_report_id;
     uint8_t              btn_byte_off;
     uint8_t              x_byte_off;
     uint8_t              y_byte_off;
     uint8_t              wheel_byte_off;
 
     bool                 primed;
 } uiox_mouse_proto_ctx_t;
 
 /* =========================================================================
  * Protocol API
  * ====================================================================== */
 
 int  uiox_mouse_proto_init  (uiox_mouse_proto_ctx_t *ctx,
                               uiox_mouse_if_t        *mif,
                               uiox_mouse_proto_t      proto);
 
 /**
  * @brief  Process raw bytes from hardware and decode into raw_t.
  *
  * Handles multi-byte packet reassembly for PS/2 and serial protocols.
  * For HID protocols, decodes directly from report buffer.
  *
  * @return 1 = complete report decoded, 0 = partial, <0 = error.
  */
 int  uiox_mouse_proto_decode(uiox_mouse_proto_ctx_t *ctx,
                               const uint8_t *data, uint8_t len,
                               uiox_mouse_raw_t *raw_out);
 
 /** Validate PS/2 sync bit in first byte. */
 bool uiox_mouse_proto_ps2_valid(uint8_t first_byte);
 
 /** Send PS/2 command byte to mouse. */
 int  uiox_mouse_proto_ps2_cmd (uiox_mouse_proto_ctx_t *ctx,
                                 uint8_t cmd, uint8_t *ack_out);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MOUSE_PROTO_H */
 
