/**
 * @file    uiox_tpwd_hw.h
 * @brief   UIOX Touch-Password Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to capacitive touch controller hardware.
 * Owns:
 *   - I2C register access to touch IC (FT6336, GT911, CST816)
 *   - Interrupt GPIO for touch-ready notification
 *   - Reset / power-down GPIO control
 *   - Touch panel dimensions and orientation
 *   - Noise filtering and sensitivity calibration registers
 *
 * Supports:
 *   - FocalTech FT6336 / FT5406 (2-point capacitive)
 *   - Goodix GT911 / GT9271   (5-point capacitive)
 *   - HYNITRON CST816S        (1-point capacitive)
 *   - Resistive 4-wire (ADC-based)
 *
 * @version 1.0.0
 * @date    2026-06-01
 */

 #ifndef UIOX_TPWD_HW_H
 #define UIOX_TPWD_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_TPWD_CAP_SINGLE        (1u << 0)  /**< Single-point touch    */
 #define UIOX_TPWD_CAP_MULTI         (1u << 1)  /**< Multi-point touch     */
 #define UIOX_TPWD_CAP_GESTURE_HW    (1u << 2)  /**< HW gesture engine     */
 #define UIOX_TPWD_CAP_PRESSURE      (1u << 3)  /**< Pressure sensing      */
 #define UIOX_TPWD_CAP_WAKEUP        (1u << 4)  /**< Touch-to-wake         */
 #define UIOX_TPWD_CAP_ENCRYPT_HW    (1u << 5)  /**< On-chip AES engine    */
 #define UIOX_TPWD_CAP_FINGERPRINT   (1u << 6)  /**< Capacitive fingerprint*/
 #define UIOX_TPWD_CAP_BACKLIGHT     (1u << 7)  /**< Key backlight LEDs    */
 
 /* =========================================================================
  * Touch controller chip identifiers
  * ====================================================================== */
 
 typedef enum {
     UIOX_TPWD_CHIP_FT6336  = 0,
     UIOX_TPWD_CHIP_GT911,
     UIOX_TPWD_CHIP_CST816,
     UIOX_TPWD_CHIP_RESISTIVE,
     UIOX_TPWD_CHIP_CUSTOM,
 } uiox_tpwd_chip_t;
 
 /* =========================================================================
  * Touch point (raw hardware coordinates)
  * ====================================================================== */
 
 #define UIOX_TPWD_MAX_TOUCH_POINTS  5
 
 typedef struct {
     uint16_t x;          /**< Raw X coordinate (0..panel_width-1)         */
     uint16_t y;          /**< Raw Y coordinate (0..panel_height-1)        */
     uint8_t  pressure;   /**< Touch pressure (0=none, 255=max)            */
     uint8_t  area;       /**< Contact area estimate                        */
     uint8_t  id;         /**< Touch point ID (for multi-touch tracking)   */
     bool     active;
 } uiox_tpwd_touch_pt_t;
 
 /* =========================================================================
  * Raw touch event (one interrupt cycle)
  * ====================================================================== */
 
 typedef struct {
     uiox_tpwd_touch_pt_t pts[UIOX_TPWD_MAX_TOUCH_POINTS];
     uint8_t  num_points;
     uint64_t ts_ns;          /**< Timestamp at interrupt                  */
     uint8_t  gesture_id;     /**< HW gesture code (chip-specific)         */
 } uiox_tpwd_raw_evt_t;
 
 /* =========================================================================
  * Panel geometry / orientation
  * ====================================================================== */
 
 typedef enum {
     UIOX_TPWD_ROT_0   = 0,
     UIOX_TPWD_ROT_90,
     UIOX_TPWD_ROT_180,
     UIOX_TPWD_ROT_270,
 } uiox_tpwd_rotation_t;
 
 typedef struct {
     uint16_t             width;     /**< Panel width  (logical pixels)    */
     uint16_t             height;    /**< Panel height (logical pixels)    */
     uiox_tpwd_rotation_t rotation;
     bool                 flip_x;
     bool                 flip_y;
 } uiox_tpwd_panel_t;
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 typedef struct {
     uintptr_t           i2c_base;   /**< I2C controller MMIO base         */
     uint8_t             i2c_addr;   /**< 7-bit I2C address of touch IC    */
     uint32_t            irq;        /**< Touch-ready GPIO IRQ line        */
     uint32_t            rst_pin;    /**< Reset GPIO pin ID                */
     uint32_t            int_pin;    /**< Interrupt GPIO pin ID            */
     uint32_t            bl_pin;     /**< Backlight GPIO pin ID (0=none)   */
     uint32_t            caps;       /**< UIOX_TPWD_CAP_* bitmask         */
     uiox_tpwd_chip_t    chip;
     uiox_tpwd_panel_t   panel;
     uint8_t             sensitivity;/**< 0=default, 1..10 (higher=more)  */
     bool                powered;
     volatile bool       irq_pending;
 
     void               *priv;
 } uiox_tpwd_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     /** One-time init: I2C bus, GPIO direction, reset sequence. */
     int  (*init)          (uiox_tpwd_hw_t *hw);
     void (*deinit)        (uiox_tpwd_hw_t *hw);
 
     /** Power on / power off the touch controller. */
     int  (*power)         (uiox_tpwd_hw_t *hw, bool on);
 
     /** Reset touch controller (toggle RST pin). */
     int  (*reset)         (uiox_tpwd_hw_t *hw);
 
     /** Read one or more registers via I2C. */
     int  (*i2c_read)      (uiox_tpwd_hw_t *hw,
                            uint8_t reg, uint8_t *buf, uint16_t len);
 
     /** Write one or more registers via I2C. */
     int  (*i2c_write)     (uiox_tpwd_hw_t *hw,
                            uint8_t reg, const uint8_t *buf, uint16_t len);
 
     /**
      * Read current touch state from controller.
      * Fills evt with all active touch points and gesture code.
      * @return Number of active touch points, or <0 on error.
      */
     int  (*read_touch)    (uiox_tpwd_hw_t *hw, uiox_tpwd_raw_evt_t *evt);
 
     /** Set touch sensitivity (0=default, 1..10). */
     int  (*set_sensitivity)(uiox_tpwd_hw_t *hw, uint8_t level);
 
     /** Control backlight LEDs (0=off, 255=full). */
     int  (*set_backlight)  (uiox_tpwd_hw_t *hw, uint8_t level);
 
     /** Microsecond delay. */
     void (*delay_us)       (uiox_tpwd_hw_t *hw, uint32_t us);
 
     /** Top-half ISR (called from interrupt context). */
     void (*isr)            (uiox_tpwd_hw_t *hw);
 } uiox_tpwd_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int  uiox_tpwd_hw_init        (uiox_tpwd_hw_t *hw,
                                 const uiox_tpwd_hw_ops_t *ops);
 void uiox_tpwd_hw_deinit      (uiox_tpwd_hw_t *hw);
 int  uiox_tpwd_hw_power       (uiox_tpwd_hw_t *hw, bool on);
 int  uiox_tpwd_hw_reset       (uiox_tpwd_hw_t *hw);
 int  uiox_tpwd_hw_read_touch  (uiox_tpwd_hw_t *hw,
                                 uiox_tpwd_raw_evt_t *evt);
 int  uiox_tpwd_hw_set_backlight(uiox_tpwd_hw_t *hw, uint8_t level);
 
 static inline uint32_t uiox_tpwd_caps(const uiox_tpwd_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_TPWD_HW_H */
 