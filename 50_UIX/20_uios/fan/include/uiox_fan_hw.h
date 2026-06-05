/**
 * @file    uiox_fan_hw.h
 * @brief   UIOX Fan Controller Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to fan controller hardware. Supports:
 *   - EMC2301 / EMC2302 / EMC2305 (I2C fan controller, 1–5 fans)
 *   - NCT6793D / NCT6776 (SuperI/O with fan PWM and TACH)
 *   - MAX6620 / MAX6639 (4-channel fan controller, I2C)
 *   - IT8987E Embedded Controller (LPC/eSPI, fan + thermal)
 *   - Direct GPIO PWM (SoC PWM output → MOSFET → fan)
 *   - IPMI BMC fan interface
 *
 * Owns:
 *   - I2C/SPI register read/write to fan IC
 *   - PWM output programming (duty cycle 0–100 %)
 *   - Tachometer input capture (fan RPM measurement)
 *   - Fan fault / stall IRQ handling
 *   - Thermal sensor I2C reads (NTC, PCT2075, LM75)
 *   - GPIO fan-enable / fan-present pins
 *
 * @version 1.0.0
 * @date    2026-06-05
 */

 #ifndef UIOX_FAN_HW_H
 #define UIOX_FAN_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_FAN_CAP_PWM            (1u << 0)  /**< PWM speed control      */
 #define UIOX_FAN_CAP_TACH           (1u << 1)  /**< Tachometer input       */
 #define UIOX_FAN_CAP_RPM_TARGET     (1u << 2)  /**< HW RPM closed-loop     */
 #define UIOX_FAN_CAP_MULTI_FAN      (1u << 3)  /**< Multiple fan channels  */
 #define UIOX_FAN_CAP_TEMP_SENSE     (1u << 4)  /**< Integrated temp sensor */
 #define UIOX_FAN_CAP_STALL_DET      (1u << 5)  /**< Stall detection IRQ    */
 #define UIOX_FAN_CAP_SPIN_UP        (1u << 6)  /**< HW spin-up sequence    */
 #define UIOX_FAN_CAP_RAMP_CTRL      (1u << 7)  /**< PWM ramp-rate control  */
 #define UIOX_FAN_CAP_FAULT_IRQ      (1u << 8)  /**< Fault interrupt        */
 #define UIOX_FAN_CAP_DRIVE_FAIL     (1u << 9)  /**< Drive fail detection   */
 #define UIOX_FAN_CAP_4WIRE          (1u << 10) /**< 4-wire fan (tach+PWM)  */
 #define UIOX_FAN_CAP_3WIRE          (1u << 11) /**< 3-wire fan (tach only) */
 #define UIOX_FAN_CAP_2WIRE          (1u << 12) /**< 2-wire (voltage ctrl)  */
 
 /* =========================================================================
  * Fan bus interface
  * ====================================================================== */
 
 typedef enum {
     UIOX_FAN_BUS_I2C  = 0,
     UIOX_FAN_BUS_SPI,
     UIOX_FAN_BUS_LPC,    /**< SuperI/O via LPC                            */
     UIOX_FAN_BUS_GPIO,   /**< Direct SoC GPIO + PWM                       */
     UIOX_FAN_BUS_IPMI,   /**< IPMI BMC interface                          */
 } uiox_fan_bus_t;
 
 /* =========================================================================
  * Fan fault flags
  * ====================================================================== */
 
 #define UIOX_FAN_FAULT_STALL        (1u << 0)  /**< Fan stalled / stopped */
 #define UIOX_FAN_FAULT_DRIVE_FAIL   (1u << 1)  /**< Drive failure         */
 #define UIOX_FAN_FAULT_SPIN_UP_FAIL (1u << 2)  /**< Spin-up timeout       */
 #define UIOX_FAN_FAULT_OTP          (1u << 3)  /**< Over-temperature      */
 #define UIOX_FAN_FAULT_WATCHDOG     (1u << 4)  /**< Watchdog expired      */
 #define UIOX_FAN_FAULT_TACH_ERR     (1u << 5)  /**< Tach signal error     */
 
 /* =========================================================================
  * Hardware limits
  * ====================================================================== */
 
 #define UIOX_FAN_MAX_CHANNELS       5
 #define UIOX_FAN_MAX_TEMP_SENSORS   4
 #define UIOX_FAN_MODEL_LEN          32
 #define UIOX_FAN_PWM_MAX            255u    /**< PWM resolution (8-bit)    */
 #define UIOX_FAN_PWM_FREQ_DEFAULT   25000u  /**< 25 kHz default PWM freq  */
 
 /* =========================================================================
  * Per-channel hardware state
  * ====================================================================== */
 
 typedef struct {
     uint8_t   pwm_duty;       /**< Current PWM duty (0..255)               */
     uint16_t  rpm_measured;   /**< Last measured RPM                        */
     uint16_t  rpm_target;     /**< Target RPM (HW closed-loop mode)         */
     uint32_t  fault_flags;    /**< Active faults for this channel           */
     bool      enabled;
     bool      spinning;
 } uiox_fan_chan_t;
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 typedef struct {
     uintptr_t        i2c_base;     /**< I2C controller MMIO base            */
     uint8_t          i2c_addr;     /**< 7-bit I2C address                   */
     uint32_t         irq;          /**< Fan fault IRQ line                  */
     uint32_t         caps;
     uiox_fan_bus_t   bus;
     char             model[UIOX_FAN_MODEL_LEN];
     uint8_t          num_fans;     /**< Physical fan channels               */
     uint8_t          num_temps;    /**< Temperature sensor channels         */
     uint32_t         pwm_freq_hz;  /**< PWM carrier frequency               */
     uint32_t         tach_edges;   /**< Tach pulses per revolution (2 or 4) */
 
     /* GPIO */
     uint32_t         en_pin;       /**< Fan enable GPIO                     */
     uint32_t         fault_pin;    /**< Fault alert GPIO                    */
 
     /* Temperature sensors (raw ADC counts or °C × 10) */
     int16_t          temp_dc[UIOX_FAN_MAX_TEMP_SENSORS];
 
     /* Per-channel state */
     uiox_fan_chan_t  chan[UIOX_FAN_MAX_CHANNELS];
 
     /* Global fault */
     volatile uint32_t global_fault;
     bool              initialised;
 
     void             *priv;
 } uiox_fan_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     int  (*init)           (uiox_fan_hw_t *hw);
     void (*deinit)         (uiox_fan_hw_t *hw);
 
     /* Register access */
     int  (*reg_read)       (uiox_fan_hw_t *hw, uint8_t reg, uint8_t *val);
     int  (*reg_write)      (uiox_fan_hw_t *hw, uint8_t reg, uint8_t val);
     int  (*reg_read16)     (uiox_fan_hw_t *hw, uint8_t reg, uint16_t *val);
     int  (*reg_write16)    (uiox_fan_hw_t *hw, uint8_t reg, uint16_t val);
 
     /** Set PWM duty cycle for a channel (0..255). */
     int  (*set_pwm)        (uiox_fan_hw_t *hw, uint8_t ch, uint8_t duty);
 
     /** Get current PWM duty cycle. */
     int  (*get_pwm)        (uiox_fan_hw_t *hw, uint8_t ch, uint8_t *duty);
 
     /** Set RPM target (HW closed-loop mode, 0=open-loop). */
     int  (*set_rpm_target) (uiox_fan_hw_t *hw, uint8_t ch, uint16_t rpm);
 
     /** Read tachometer (returns measured RPM). */
     int  (*read_rpm)       (uiox_fan_hw_t *hw, uint8_t ch, uint16_t *rpm);
 
     /** Read temperature sensor channel (returns °C × 10). */
     int  (*read_temp)      (uiox_fan_hw_t *hw, uint8_t ch, int16_t *temp_dc);
 
     /** Read fault status register. */
     int  (*fault_status)   (uiox_fan_hw_t *hw, uint32_t *flags);
 
     /** Clear fault flags. */
     int  (*fault_clear)    (uiox_fan_hw_t *hw, uint32_t flags);
 
     /** Enable or disable a fan channel. */
     int  (*chan_enable)    (uiox_fan_hw_t *hw, uint8_t ch, bool en);
 
     /** GPIO control (fan enable, etc.). */
     void (*gpio_write)     (uiox_fan_hw_t *hw, uint32_t pin, bool val);
     bool (*gpio_read)      (uiox_fan_hw_t *hw, uint32_t pin);
 
     /** Watchdog kick. */
     int  (*wdt_kick)       (uiox_fan_hw_t *hw);
 
     /** ISR top-half. */
     void (*isr)            (uiox_fan_hw_t *hw);
 } uiox_fan_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int      uiox_fan_hw_init        (uiox_fan_hw_t *hw,
                                    const uiox_fan_hw_ops_t *ops);
 void     uiox_fan_hw_deinit      (uiox_fan_hw_t *hw);
 int      uiox_fan_hw_set_pwm     (uiox_fan_hw_t *hw, uint8_t ch,
                                    uint8_t duty);
 int      uiox_fan_hw_read_rpm    (uiox_fan_hw_t *hw, uint8_t ch,
                                    uint16_t *rpm);
 int      uiox_fan_hw_read_temp   (uiox_fan_hw_t *hw, uint8_t ch,
                                    int16_t *temp_dc);
 int      uiox_fan_hw_fault_status(uiox_fan_hw_t *hw, uint32_t *flags);
 int      uiox_fan_hw_fault_clear (uiox_fan_hw_t *hw, uint32_t flags);
 int      uiox_fan_hw_chan_enable (uiox_fan_hw_t *hw, uint8_t ch, bool en);
 
 static inline uint32_t uiox_fan_caps(const uiox_fan_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FAN_HW_H */
 