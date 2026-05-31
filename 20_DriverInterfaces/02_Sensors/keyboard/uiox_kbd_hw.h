/**
 * @file    uiox_kbd_hw.h
 * @brief   UIOX Keyboard Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to keyboard hardware. Owns:
 *   - GPIO row/column drive and sense for matrix keyboards
 *   - I2C/SPI register access for digital keyboard controllers
 *   - Hardware timer for scan period and debounce
 *   - IRQ handling (key-change interrupt, scan complete)
 *   - LED GPIO control (Caps Lock, Num Lock, Scroll Lock)
 *   - Clock and reset control
 *
 * Supports:
 *   - Row/column matrix keyboards (up to 16×16 = 256 keys)
 *   - Direct GPIO keyboards (up to 32 individual keys)
 *   - I2C keyboard controllers (TCA8418, PCF8574, MCP23017)
 *   - USB HID (via controller bridge)
 *   - PS/2 keyboards (clock/data GPIO)
 *
 * @version 1.0.0
 * @date    2026-05-27
 */
//Layer 1 — Hardware Abstraction
 #ifndef UIOX_KBD_HW_H
 #define UIOX_KBD_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_KBD_CAP_MATRIX        (1u << 0)  /**< Row/col matrix scan      */
 #define UIOX_KBD_CAP_GPIO_DIRECT   (1u << 1)  /**< Direct GPIO key inputs   */
 #define UIOX_KBD_CAP_I2C           (1u << 2)  /**< I2C keyboard controller  */
 #define UIOX_KBD_CAP_SPI           (1u << 3)  /**< SPI keyboard controller  */
 #define UIOX_KBD_CAP_PS2           (1u << 4)  /**< PS/2 interface           */
 #define UIOX_KBD_CAP_USB_HID       (1u << 5)  /**< USB HID bridge           */
 #define UIOX_KBD_CAP_LED           (1u << 6)  /**< LED output control       */
 #define UIOX_KBD_CAP_IRQ           (1u << 7)  /**< Key-change IRQ           */
 #define UIOX_KBD_CAP_BACKLIGHT     (1u << 8)  /**< Keyboard backlight       */
 #define UIOX_KBD_CAP_WAKEUP        (1u << 9)  /**< Wake-up from sleep       */
 
 /* =========================================================================
  * Interface types
  * ====================================================================== */
 
 typedef enum {
     UIOX_KBD_IF_MATRIX = 0,   /**< Row × column matrix                    */
     UIOX_KBD_IF_GPIO,         /**< Direct GPIO per key                    */
     UIOX_KBD_IF_I2C,          /**< I2C keyboard controller                */
     UIOX_KBD_IF_SPI,          /**< SPI keyboard controller                */
     UIOX_KBD_IF_PS2,          /**< PS/2 serial interface                  */
     UIOX_KBD_IF_USB_HID,      /**< USB HID bridge                         */
 } uiox_kbd_if_type_t;
 
 /* =========================================================================
  * LED identifiers
  * ====================================================================== */
 
 #define UIOX_KBD_LED_CAPSLOCK      (1u << 0)
 #define UIOX_KBD_LED_NUMLOCK       (1u << 1)
 #define UIOX_KBD_LED_SCROLLLOCK    (1u << 2)
 #define UIOX_KBD_LED_COMPOSE       (1u << 3)
 #define UIOX_KBD_LED_KANA          (1u << 4)
 #define UIOX_KBD_LED_BACKLIGHT     (1u << 5)
 
 /* =========================================================================
  * Matrix scan configuration
  * ====================================================================== */
 
 #define UIOX_KBD_MAX_ROWS           16
 #define UIOX_KBD_MAX_COLS           16
 #define UIOX_KBD_MAX_DIRECT_KEYS    32
 
 typedef struct {
     uint32_t  row_pins[UIOX_KBD_MAX_ROWS]; /**< GPIO pin IDs for rows      */
     uint32_t  col_pins[UIOX_KBD_MAX_COLS]; /**< GPIO pin IDs for columns   */
     uint8_t   num_rows;
     uint8_t   num_cols;
     bool      active_low;     /**< true = key press pulls line low          */
     uint32_t  scan_period_us; /**< Scan period (µs)                        */
     uint32_t  settle_us;      /**< GPIO settle time after row drive (µs)   */
 } uiox_kbd_matrix_cfg_t;
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 typedef struct {
     uintptr_t              base_addr;   /**< MMIO base (timer/GPIO ctrl)   */
     uint32_t               irq;         /**< Key-change IRQ line           */
     uint32_t               caps;        /**< UIOX_KBD_CAP_* bitmask       */
     uiox_kbd_if_type_t     if_type;
     uint8_t                i2c_addr;    /**< I2C device address (7-bit)    */
     uint32_t               clk_hz;      /**< Timer/controller clock        */
 
     /* Matrix config */
     uiox_kbd_matrix_cfg_t  matrix;
 
     /* Direct GPIO config */
     uint32_t  direct_pins[UIOX_KBD_MAX_DIRECT_KEYS];
     uint8_t   num_direct;
     bool      direct_active_low;
 
     /* LED GPIO pins */
     uint32_t  led_pins[6];   /**< One per UIOX_KBD_LED_* bit              */
     uint8_t   led_state;     /**< Current LED bitmask                      */
 
     /* PS/2 */
     uint32_t  ps2_clk_pin;
     uint32_t  ps2_data_pin;
 
     /* Backlight PWM */
     uint8_t   backlight_level; /**< 0..255                                 */
 
     void     *priv;            /**< Driver-private data                    */
 } uiox_kbd_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     /** One-time init: GPIO direction, timer config, pull-up/down. */
     int  (*init)           (uiox_kbd_hw_t *hw);
     void (*deinit)         (uiox_kbd_hw_t *hw);
 
     /**
      * Drive one matrix row active and read all column sense lines.
      * @param row     Row index to drive.
      * @param cols_out Bitmask of column states (1 = key pressed).
      */
     int  (*scan_row)       (uiox_kbd_hw_t *hw, uint8_t row,
                             uint16_t *cols_out);
 
     /**
      * Read all direct GPIO keys.
      * @param keys_out Bitmask of key states (1 = pressed).
      */
     int  (*read_direct)    (uiox_kbd_hw_t *hw, uint32_t *keys_out);
 
     /** I2C register read. */
     int  (*i2c_read)       (uiox_kbd_hw_t *hw,
                             uint8_t reg, uint8_t *val);
     /** I2C register write. */
     int  (*i2c_write)      (uiox_kbd_hw_t *hw,
                             uint8_t reg, uint8_t val);
 
     /** SPI transfer. */
     int  (*spi_transfer)   (uiox_kbd_hw_t *hw,
                             const uint8_t *tx, uint8_t *rx, uint16_t len);
 
     /** PS/2 send command byte. */
     int  (*ps2_send)       (uiox_kbd_hw_t *hw, uint8_t cmd);
     /** PS/2 receive byte (blocking up to timeout_ms). */
     int  (*ps2_recv)       (uiox_kbd_hw_t *hw, uint8_t *val,
                             uint32_t timeout_ms);
 
     /** Set LED state (bitmask of UIOX_KBD_LED_*). */
     int  (*set_leds)       (uiox_kbd_hw_t *hw, uint8_t led_mask);
 
     /** Set backlight brightness (0=off, 255=max). */
     int  (*set_backlight)  (uiox_kbd_hw_t *hw, uint8_t level);
 
     /** Microsecond-accurate delay. */
     void (*delay_us)       (uiox_kbd_hw_t *hw, uint32_t us);
 
     /** Top-half ISR (called from interrupt context). */
     void (*isr)            (uiox_kbd_hw_t *hw);
 
 } uiox_kbd_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int  uiox_kbd_hw_init        (uiox_kbd_hw_t *hw,
                                const uiox_kbd_hw_ops_t *ops);
 void uiox_kbd_hw_deinit      (uiox_kbd_hw_t *hw);
 int  uiox_kbd_hw_scan_row    (uiox_kbd_hw_t *hw, uint8_t row,
                                uint16_t *cols_out);
 int  uiox_kbd_hw_read_direct (uiox_kbd_hw_t *hw, uint32_t *keys_out);
 int  uiox_kbd_hw_set_leds    (uiox_kbd_hw_t *hw, uint8_t led_mask);
 int  uiox_kbd_hw_set_backlight(uiox_kbd_hw_t *hw, uint8_t level);
 
 static inline uint32_t uiox_kbd_caps(const uiox_kbd_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KBD_HW_H */
 