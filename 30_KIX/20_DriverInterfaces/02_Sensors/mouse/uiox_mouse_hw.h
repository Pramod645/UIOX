/**
 * @file    uiox_mouse_hw.h
 * @brief   UIOX Mouse Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to mouse hardware. Owns:
 *   - USB HID interrupt endpoint polling / IRQ
 *   - PS/2 clock/data GPIO bit-bang or dedicated controller
 *   - I2C trackpad register access (Synaptics, ELAN, Apple)
 *   - UART mouse (serial mouse protocol)
 *   - GPIO button inputs for standalone buttons
 *   - IRQ handling: data-ready, connect, disconnect
 *
 * Supports:
 *   - USB HID Boot Protocol mouse (3-byte report)
 *   - USB HID Report Protocol mouse (variable report)
 *   - PS/2 standard (3-button, IntelliMouse scroll)
 *   - I2C trackpad (Synaptics RMI4, ELAN, Apple Magic)
 *   - Serial mouse (Microsoft 2-button, Logitech)
 *
 * @version 1.0.0
 * @date    2026-06-01
 */

 #ifndef UIOX_MOUSE_HW_H
 #define UIOX_MOUSE_HW_H
 
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_MOUSE_CAP_USB          (1u << 0)  /**< USB HID interface      */
 #define UIOX_MOUSE_CAP_PS2          (1u << 1)  /**< PS/2 interface         */
 #define UIOX_MOUSE_CAP_I2C          (1u << 2)  /**< I2C trackpad           */
 #define UIOX_MOUSE_CAP_UART         (1u << 3)  /**< Serial mouse           */
 #define UIOX_MOUSE_CAP_SCROLL_WHEEL (1u << 4)  /**< Scroll wheel           */
 #define UIOX_MOUSE_CAP_HSCROLL      (1u << 5)  /**< Horizontal scroll      */
 #define UIOX_MOUSE_CAP_5_BUTTONS    (1u << 6)  /**< 4th + 5th button       */
 #define UIOX_MOUSE_CAP_HIGHRES      (1u << 7)  /**< High-res movement      */
 #define UIOX_MOUSE_CAP_GESTURE      (1u << 8)  /**< HW gesture engine      */
 #define UIOX_MOUSE_CAP_MULTITOUCH   (1u << 9)  /**< Multi-touch trackpad   */
 #define UIOX_MOUSE_CAP_PRESSURE     (1u << 10) /**< Pressure sensing       */
 #define UIOX_MOUSE_CAP_WAKEUP       (1u << 11) /**< Wake-from-sleep        */
 
 /* =========================================================================
  * Mouse interface types
  * ====================================================================== */
 
 typedef enum {
     UIOX_MOUSE_IF_USB_HID = 0,
     UIOX_MOUSE_IF_PS2,
     UIOX_MOUSE_IF_I2C_TRACKPAD,
     UIOX_MOUSE_IF_UART,
     UIOX_MOUSE_IF_GPIO,          /**< Standalone GPIO buttons + encoder   */
 } uiox_mouse_if_type_t;
 
 /* =========================================================================
  * Raw hardware report (filled by HAL from device data)
  * ====================================================================== */
 
 #define UIOX_MOUSE_MAX_BUTTONS  8
 #define UIOX_MOUSE_RAW_BUF_LEN  16  /**< Max raw report bytes             */
 
 typedef struct {
     int16_t   dx;               /**< X movement (signed, device units)    */
     int16_t   dy;               /**< Y movement (signed, device units)    */
     int8_t    dz;               /**< Scroll wheel delta (signed)          */
     int8_t    dw;               /**< Horizontal scroll delta (signed)     */
     uint8_t   buttons;          /**< Button bitmask (bit0=left,1=right…)  */
     uint8_t   raw[UIOX_MOUSE_RAW_BUF_LEN]; /**< Raw bytes from device    */
     uint8_t   raw_len;
     uint64_t  ts_ns;            /**< Timestamp at IRQ                     */
     bool      connected;        /**< Device presence flag                 */
 } uiox_mouse_raw_t;
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 typedef struct {
     uintptr_t             base_addr;  /**< MMIO base (USB/PS2 ctrl)       */
     uint32_t              irq;        /**< Data-ready IRQ line            */
     uint32_t              caps;       /**< UIOX_MOUSE_CAP_* bitmask      */
     uiox_mouse_if_type_t  if_type;
     uint8_t               i2c_addr;  /**< I2C trackpad 7-bit address      */
     uint32_t              poll_rate_hz; /**< Report rate (125/250/500/1000)*/
     uint8_t               resolution_dpi; /**< Sensor resolution class    */
     volatile bool         irq_pending;
     bool                  connected;
     void                 *priv;
 } uiox_mouse_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     int  (*init)         (uiox_mouse_hw_t *hw);
     void (*deinit)       (uiox_mouse_hw_t *hw);
     int  (*enable)       (uiox_mouse_hw_t *hw);
     void (*disable)      (uiox_mouse_hw_t *hw);
 
     /**
      * Read latest report from device.
      * @return 1 if data available, 0 if not, <0 on error.
      */
     int  (*read_report)  (uiox_mouse_hw_t *hw, uiox_mouse_raw_t *raw);
 
     /** Set poll rate (Hz). */
     int  (*set_rate)     (uiox_mouse_hw_t *hw, uint32_t rate_hz);
 
     /** Set DPI / sensitivity on sensor (if supported). */
     int  (*set_dpi)      (uiox_mouse_hw_t *hw, uint16_t dpi);
 
     /** I2C register read (trackpad). */
     int  (*i2c_read)     (uiox_mouse_hw_t *hw,
                           uint8_t reg, uint8_t *buf, uint16_t len);
 
     /** I2C register write (trackpad). */
     int  (*i2c_write)    (uiox_mouse_hw_t *hw,
                           uint8_t reg, const uint8_t *buf, uint16_t len);
 
     /** Query connection status. */
     bool (*connected)    (uiox_mouse_hw_t *hw);
 
     /** Top-half ISR. */
     void (*isr)          (uiox_mouse_hw_t *hw);
 } uiox_mouse_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int  uiox_mouse_hw_init       (uiox_mouse_hw_t *hw,
                                 const uiox_mouse_hw_ops_t *ops);
 void uiox_mouse_hw_deinit     (uiox_mouse_hw_t *hw);
 int  uiox_mouse_hw_enable     (uiox_mouse_hw_t *hw);
 void uiox_mouse_hw_disable    (uiox_mouse_hw_t *hw);
 int  uiox_mouse_hw_read_report(uiox_mouse_hw_t *hw, uiox_mouse_raw_t *raw);
 bool uiox_mouse_hw_connected  (uiox_mouse_hw_t *hw);
 
 static inline uint32_t uiox_mouse_caps(const uiox_mouse_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MOUSE_HW_H */
 