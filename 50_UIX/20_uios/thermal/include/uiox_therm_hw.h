/**
 * @file    uiox_therm_hw.h
 * @brief   UIOX Thermal Sensor Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to thermal sensor hardware. Supports:
 *   - PCT2075 / LM75 / LM75A  (I2C digital, 9-bit, 0.5°C LSB)
 *   - TMP117 / TMP116          (I2C, 16-bit, 0.0078°C LSB)
 *   - TMP112 / TMP102          (I2C, 12-bit, 0.0625°C LSB)
 *   - MAX31875                 (I2C, 16-bit, configurable range)
 *   - NTC thermistor via on-chip ADC (Steinhart-Hart linearisation)
 *   - Internal SoC thermal diode (CPU/GPU junction via MMIO)
 *   - SMSC EMC1413 / ADT7461 (remote diode + local)
 *
 * Owns:
 *   - I2C register read/write to digital temp sensors
 *   - ADC raw count read + NTC lookup/calculation
 *   - Alert/THERM GPIO threshold programming
 *   - Over-temperature alert IRQ handling
 *   - Sensor power-down / one-shot modes
 *
 * @version 1.0.0
 * @date    2026-06-05
 */

 #ifndef UIOX_THERM_HW_H
 #define UIOX_THERM_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_THERM_CAP_I2C          (1u << 0)  /**< I2C digital sensor    */
 #define UIOX_THERM_CAP_SPI          (1u << 1)  /**< SPI digital sensor    */
 #define UIOX_THERM_CAP_ADC_NTC      (1u << 2)  /**< NTC via ADC           */
 #define UIOX_THERM_CAP_INTERNAL     (1u << 3)  /**< SoC internal diode    */
 #define UIOX_THERM_CAP_REMOTE       (1u << 4)  /**< Remote diode meas.    */
 #define UIOX_THERM_CAP_ALERT        (1u << 5)  /**< HW alert threshold    */
 #define UIOX_THERM_CAP_THERM        (1u << 6)  /**< THERM (hw throttle)   */
 #define UIOX_THERM_CAP_ONESHOT      (1u << 7)  /**< One-shot conv mode    */
 #define UIOX_THERM_CAP_SHUTDOWN     (1u << 8)  /**< Power-down mode       */
 #define UIOX_THERM_CAP_RESOLUTION   (1u << 9)  /**< Configurable res.     */
 #define UIOX_THERM_CAP_MULTI_CH     (1u << 10) /**< Multiple channels     */
 #define UIOX_THERM_CAP_16BIT        (1u << 11) /**< 16-bit resolution     */
 #define UIOX_THERM_CAP_FAULT_QUEUE  (1u << 12) /**< Fault queue filter    */
 
 /* =========================================================================
  * Sensor type
  * ====================================================================== */
 
 typedef enum {
     UIOX_THERM_TYPE_PCT2075  = 0,
     UIOX_THERM_TYPE_LM75,
     UIOX_THERM_TYPE_TMP117,
     UIOX_THERM_TYPE_TMP112,
     UIOX_THERM_TYPE_MAX31875,
     UIOX_THERM_TYPE_ADT7461,   /**< Remote + local diode                  */
     UIOX_THERM_TYPE_NTC,       /**< NTC thermistor via ADC                */
     UIOX_THERM_TYPE_INTERNAL,  /**< SoC MMIO thermal register             */
     UIOX_THERM_TYPE_CUSTOM,
 } uiox_therm_type_t;
 
 /* =========================================================================
  * Bus interface
  * ====================================================================== */
 
 typedef enum {
     UIOX_THERM_BUS_I2C  = 0,
     UIOX_THERM_BUS_SPI,
     UIOX_THERM_BUS_ADC,    /**< On-chip ADC (for NTC)                     */
     UIOX_THERM_BUS_MMIO,   /**< Direct MMIO (SoC internal)                */
     UIOX_THERM_BUS_SMBUS,
 } uiox_therm_bus_t;
 
 /* =========================================================================
  * Alert mode
  * ====================================================================== */
 
 typedef enum {
     UIOX_THERM_ALERT_COMPARATOR = 0,  /**< Deasserts when below T_hyst   */
     UIOX_THERM_ALERT_INTERRUPT,        /**< Latched, cleared by read      */
 } uiox_therm_alert_mode_t;
 
 /* =========================================================================
  * Resolution
  * ====================================================================== */
 
 typedef enum {
     UIOX_THERM_RES_9BIT  = 0,   /**< 0.5°C   — LM75/PCT2075            */
     UIOX_THERM_RES_10BIT,        /**< 0.25°C                            */
     UIOX_THERM_RES_11BIT,        /**< 0.125°C                           */
     UIOX_THERM_RES_12BIT,        /**< 0.0625°C — TMP112                 */
     UIOX_THERM_RES_16BIT,        /**< 0.0078°C — TMP117                 */
 } uiox_therm_res_t;
 
 /* =========================================================================
  * NTC thermistor parameters (Steinhart-Hart)
  * ====================================================================== */
 
 typedef struct {
     float    r_nominal;    /**< Resistance at T_nominal (Ohm)             */
     float    t_nominal;    /**< Nominal temperature (°C, typically 25)    */
     float    beta;         /**< Beta coefficient (K)                      */
     float    r_series;     /**< Series resistor in voltage divider (Ohm)  */
     uint32_t adc_vref_mv;  /**< ADC reference voltage (mV)                */
     uint16_t adc_bits;     /**< ADC resolution (bits)                     */
 } uiox_therm_ntc_cfg_t;
 
 /* =========================================================================
  * Per-channel hardware measurement
  * ====================================================================== */
 
 #define UIOX_THERM_MAX_CHANNELS    8
 #define UIOX_THERM_MODEL_LEN       32
 
 typedef struct {
     int16_t  temp_dc;      /**< Last measurement (°C × 10)               */
     bool     alert_active; /**< Over-temp alert asserted                  */
     bool     valid;
 } uiox_therm_meas_t;
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 typedef struct {
     uintptr_t              i2c_base;
     uint8_t                i2c_addr;
     uint32_t               irq;
     uint32_t               caps;
     uiox_therm_type_t      type;
     uiox_therm_bus_t       bus;
     uiox_therm_res_t       resolution;
     uiox_therm_alert_mode_t alert_mode;
     char                   model[UIOX_THERM_MODEL_LEN];
     uint8_t                num_channels;
 
     /* Alert thresholds (°C × 10) */
     int16_t                t_high_dc;  /**< Upper alert threshold         */
     int16_t                t_hyst_dc;  /**< Hysteresis temperature        */
     int16_t                t_crit_dc;  /**< THERM / critical temp         */
 
     /* GPIO */
     uint32_t               alert_pin;
     uint32_t               therm_pin;
 
     /* NTC config */
     uiox_therm_ntc_cfg_t   ntc;
 
     /* MMIO base for SoC internal sensors */
     uintptr_t              mmio_temp_reg;
 
     /* Measurements */
     uiox_therm_meas_t      meas[UIOX_THERM_MAX_CHANNELS];
     volatile bool          alert_pending;
     bool                   initialised;
 
     void                  *priv;
 } uiox_therm_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     int  (*init)           (uiox_therm_hw_t *hw);
     void (*deinit)         (uiox_therm_hw_t *hw);
 
     /* Register access */
     int  (*reg_read)       (uiox_therm_hw_t *hw, uint8_t reg, uint8_t *val);
     int  (*reg_write)      (uiox_therm_hw_t *hw, uint8_t reg, uint8_t val);
     int  (*reg_read16)     (uiox_therm_hw_t *hw, uint8_t reg, uint16_t *val);
     int  (*reg_write16)    (uiox_therm_hw_t *hw, uint8_t reg, uint16_t val);
 
     /** Read temperature from a channel (returns °C × 10). */
     int  (*read_temp)      (uiox_therm_hw_t *hw, uint8_t ch,
                             int16_t *temp_dc);
 
     /** Set alert threshold (°C × 10). */
     int  (*set_t_high)     (uiox_therm_hw_t *hw, int16_t temp_dc);
 
     /** Set hysteresis temperature (°C × 10). */
     int  (*set_t_hyst)     (uiox_therm_hw_t *hw, int16_t temp_dc);
 
     /** Set critical/THERM temperature (°C × 10). */
     int  (*set_t_crit)     (uiox_therm_hw_t *hw, int16_t temp_dc);
 
     /** Set resolution. */
     int  (*set_resolution) (uiox_therm_hw_t *hw, uiox_therm_res_t res);
 
     /** Set operating mode (normal / shutdown / one-shot). */
     int  (*set_mode)       (uiox_therm_hw_t *hw, bool shutdown);
 
     /** Trigger one-shot conversion (shutdown mode). */
     int  (*oneshot)        (uiox_therm_hw_t *hw);
 
     /** Read alert status and clear latched interrupt. */
     int  (*alert_status)   (uiox_therm_hw_t *hw, bool *alert_out);
     int  (*alert_clear)    (uiox_therm_hw_t *hw);
 
     /** Read ADC raw count (for NTC). */
     int  (*adc_read)       (uiox_therm_hw_t *hw, uint8_t ch, uint16_t *raw);
 
     /** GPIO. */
     bool (*gpio_read)      (uiox_therm_hw_t *hw, uint32_t pin);
 
     /** ISR top-half. */
     void (*isr)            (uiox_therm_hw_t *hw);
 } uiox_therm_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int      uiox_therm_hw_init       (uiox_therm_hw_t *hw,
                                     const uiox_therm_hw_ops_t *ops);
 void     uiox_therm_hw_deinit     (uiox_therm_hw_t *hw);
 int      uiox_therm_hw_read_temp  (uiox_therm_hw_t *hw, uint8_t ch,
                                     int16_t *temp_dc);
 int      uiox_therm_hw_set_t_high (uiox_therm_hw_t *hw, int16_t temp_dc);
 int      uiox_therm_hw_set_t_hyst (uiox_therm_hw_t *hw, int16_t temp_dc);
 int      uiox_therm_hw_set_t_crit (uiox_therm_hw_t *hw, int16_t temp_dc);
 int      uiox_therm_hw_alert_clear(uiox_therm_hw_t *hw);
 
 static inline uint32_t uiox_therm_caps(const uiox_therm_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_THERM_HW_H */
 