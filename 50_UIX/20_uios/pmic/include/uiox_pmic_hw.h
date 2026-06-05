/**
 * @file    uiox_pmic_hw.h
 * @brief   UIOX PMIC Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to PMIC hardware. Supports:
 *   - TI TPS65988 / TPS65219 (laptop power management)
 *   - Qualcomm PM8998 / PMI8998
 *   - Maxim MAX77686 / MAX77620
 *   - Dialog DA9063 / DA9121
 *   - NXP PF8100 / PF8200
 *   - Renesas RAA215300
 *
 * Owns:
 *   - I2C/SPI register read/write
 *   - Interrupt GPIO (PMIC → SoC IRQ line)
 *   - Enable GPIO (SoC → PMIC power enable)
 *   - Reset / PMIC_RESET_N GPIO
 *   - PMIC watchdog kick
 *   - Hardware fault detect (OCP, OVP, UVP, OTP)
 *
 * @version 1.0.0
 * @date    2026-06-04
 */

 #ifndef UIOX_PMIC_HW_H
 #define UIOX_PMIC_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * PMIC hardware capability flags
  * ====================================================================== */
 
 #define UIOX_PMIC_CAP_BUCK          (1u << 0)  /**< Buck converters        */
 #define UIOX_PMIC_CAP_LDO           (1u << 1)  /**< LDO regulators        */
 #define UIOX_PMIC_CAP_LOAD_SWITCH   (1u << 2)  /**< Load switches         */
 #define UIOX_PMIC_CAP_BOOST         (1u << 3)  /**< Boost converter       */
 #define UIOX_PMIC_CAP_CHG           (1u << 4)  /**< Battery charger       */
 #define UIOX_PMIC_CAP_FUEL_GAUGE    (1u << 5)  /**< Coulomb counter       */
 #define UIOX_PMIC_CAP_OTP           (1u << 6)  /**< Over-temp protection  */
 #define UIOX_PMIC_CAP_OCP           (1u << 7)  /**< Over-current protect  */
 #define UIOX_PMIC_CAP_OVP           (1u << 8)  /**< Over-voltage protect  */
 #define UIOX_PMIC_CAP_UVP           (1u << 9)  /**< Under-voltage protect */
 #define UIOX_PMIC_CAP_WATCHDOG      (1u << 10) /**< HW watchdog timer     */
 #define UIOX_PMIC_CAP_RTC           (1u << 11) /**< Real-time clock       */
 #define UIOX_PMIC_CAP_ADC           (1u << 12) /**< On-chip ADC           */
 #define UIOX_PMIC_CAP_GPIO          (1u << 13) /**< PMIC GPIO outputs     */
 #define UIOX_PMIC_CAP_SEQUENCER     (1u << 14) /**< HW power sequencer   */
 #define UIOX_PMIC_CAP_DVFS          (1u << 15) /**< Dynamic volt scaling  */
 
 /* =========================================================================
  * Bus interface type
  * ====================================================================== */
 
 typedef enum {
     UIOX_PMIC_BUS_I2C = 0,
     UIOX_PMIC_BUS_SPI,
     UIOX_PMIC_BUS_SPMI,   /**< Qualcomm SPMI bus                          */
 } uiox_pmic_bus_t;
 
 /* =========================================================================
  * PMIC fault flags (IRQ status register bits)
  * ====================================================================== */
 
 #define UIOX_PMIC_FAULT_OTP         (1u << 0)
 #define UIOX_PMIC_FAULT_OCP         (1u << 1)
 #define UIOX_PMIC_FAULT_OVP         (1u << 2)
 #define UIOX_PMIC_FAULT_UVP         (1u << 3)
 #define UIOX_PMIC_FAULT_WDT         (1u << 4)  /**< Watchdog timeout      */
 #define UIOX_PMIC_FAULT_PGOOD       (1u << 5)  /**< Power-good lost       */
 #define UIOX_PMIC_FAULT_RESET       (1u << 6)  /**< PMIC reset event      */
 
 /* =========================================================================
  * ADC channel IDs
  * ====================================================================== */
 
 typedef enum {
     UIOX_PMIC_ADC_VBAT   = 0,  /**< Battery voltage                      */
     UIOX_PMIC_ADC_VSYS,         /**< System rail voltage                  */
     UIOX_PMIC_ADC_TEMP_DIE,     /**< PMIC die temperature                 */
     UIOX_PMIC_ADC_TEMP_NTC,     /**< External NTC thermistor              */
     UIOX_PMIC_ADC_IBAT,         /**< Battery current (shunt)              */
     UIOX_PMIC_ADC_VBUS,         /**< USB VBUS voltage                     */
     UIOX_PMIC_ADC_MAX,
 } uiox_pmic_adc_ch_t;
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 #define UIOX_PMIC_MODEL_LEN     32
 #define UIOX_PMIC_MAX_RAILS     16
 
 typedef struct {
     uintptr_t          i2c_base;      /**< I2C controller MMIO base        */
     uint8_t            i2c_addr;      /**< 7-bit I2C address               */
     uint32_t           irq;           /**< PMIC IRQ line                   */
     uint32_t           caps;
     uiox_pmic_bus_t    bus;
     char               model[UIOX_PMIC_MODEL_LEN]; /**< e.g. "DA9063"     */
     uint8_t            num_bucks;
     uint8_t            num_ldos;
     uint8_t            num_switches;
 
     /* GPIO */
     uint32_t           en_pin;        /**< PMIC enable GPIO                */
     uint32_t           rst_pin;       /**< PMIC reset GPIO                 */
     uint32_t           int_pin;       /**< Interrupt input GPIO            */
     uint32_t           pgood_pin;     /**< Power-good output GPIO          */
 
     /* State */
     volatile uint32_t  fault_flags;   /**< Active fault bitmask            */
     bool               powered;
     bool               fault;
     int8_t             die_temp_c;    /**< Last read die temperature       */
 
     void              *priv;
 } uiox_pmic_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     int  (*init)          (uiox_pmic_hw_t *hw);
     void (*deinit)        (uiox_pmic_hw_t *hw);
     int  (*enable)        (uiox_pmic_hw_t *hw, bool on);
     int  (*reset)         (uiox_pmic_hw_t *hw);
 
     /* Register access */
     int  (*reg_read)      (uiox_pmic_hw_t *hw,
                            uint16_t reg, uint8_t *val);
     int  (*reg_write)     (uiox_pmic_hw_t *hw,
                            uint16_t reg, uint8_t val);
     int  (*reg_update)    (uiox_pmic_hw_t *hw,
                            uint16_t reg, uint8_t mask, uint8_t val);
     int  (*bulk_read)     (uiox_pmic_hw_t *hw,
                            uint16_t reg, uint8_t *buf, uint16_t len);
     int  (*bulk_write)    (uiox_pmic_hw_t *hw,
                            uint16_t reg, const uint8_t *buf, uint16_t len);
 
     /* ADC */
     int  (*adc_read)      (uiox_pmic_hw_t *hw,
                            uiox_pmic_adc_ch_t ch, uint32_t *mv_or_mc);
 
     /* Watchdog */
     int  (*wdt_kick)      (uiox_pmic_hw_t *hw);
     int  (*wdt_enable)    (uiox_pmic_hw_t *hw, bool en, uint32_t timeout_ms);
 
     /* IRQ / fault */
     int  (*irq_status)    (uiox_pmic_hw_t *hw, uint32_t *flags_out);
     int  (*irq_clear)     (uiox_pmic_hw_t *hw, uint32_t flags);
     int  (*irq_mask)      (uiox_pmic_hw_t *hw, uint32_t mask);
     int  (*irq_unmask)    (uiox_pmic_hw_t *hw, uint32_t mask);
 
     /* GPIO */
     bool (*gpio_read)     (uiox_pmic_hw_t *hw, uint32_t pin);
     void (*gpio_write)    (uiox_pmic_hw_t *hw, uint32_t pin, bool val);
 
     /* ISR */
     void (*isr)           (uiox_pmic_hw_t *hw);
 } uiox_pmic_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int      uiox_pmic_hw_init      (uiox_pmic_hw_t *hw,
                                   const uiox_pmic_hw_ops_t *ops);
 void     uiox_pmic_hw_deinit    (uiox_pmic_hw_t *hw);
 int      uiox_pmic_hw_enable    (uiox_pmic_hw_t *hw, bool on);
 int      uiox_pmic_hw_reg_read  (uiox_pmic_hw_t *hw,
                                   uint16_t reg, uint8_t *val);
 int      uiox_pmic_hw_reg_write (uiox_pmic_hw_t *hw,
                                   uint16_t reg, uint8_t val);
 int      uiox_pmic_hw_reg_update(uiox_pmic_hw_t *hw,
                                   uint16_t reg, uint8_t mask, uint8_t val);
 int      uiox_pmic_hw_adc_read  (uiox_pmic_hw_t *hw,
                                   uiox_pmic_adc_ch_t ch, uint32_t *result);
 int      uiox_pmic_hw_wdt_kick  (uiox_pmic_hw_t *hw);
 int      uiox_pmic_hw_irq_status(uiox_pmic_hw_t *hw, uint32_t *flags);
 int      uiox_pmic_hw_irq_clear (uiox_pmic_hw_t *hw, uint32_t flags);
 
 static inline uint32_t uiox_pmic_caps(const uiox_pmic_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_PMIC_HW_H */
  