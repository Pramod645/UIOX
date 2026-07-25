/**
 * @file  uiox_chg_hw.h
 * @brief UIOX Charger Hardware Abstraction Layer (HAL).
 *
 * Supports:
 *   - TI BQ25895  (I²C, USB-C PD + barrel jack, 5 A)
 *   - ONSEMI FUSB302  (USB-C PD PHY, I²C)
 *   - Maxim MAX77958  (USB-C PD + fast-charge, I²C)
 *   - Generic barrel-jack charger IC (CC/CV only)
 *
 * Owns:
 *   - I²C register read/write (7-bit address bus)
 *   - GPIO: INT#, CE# (charge enable), OTG, ACOK, BATFET
 *   - ADC: VBUS, VBAT, IBAT, VSYS, TDIE, TNTC readings
 *   - USB-C CC line detection and PD PHY messaging
 *   - Barrel-jack presence detection (ACOK pin / ADC threshold)
 *   - IRQ: charge status, fault, PD message, VBUS change
 *
 * @version 1.0.0
 * @date    2026-06-11
 */

 #ifndef UIOX_CHG_HW_H
 #define UIOX_CHG_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Charger IC variant
  * ====================================================================== */
 
 typedef enum {
     UIOX_CHG_IC_BQ25895   = 0,  /**< TI BQ25895 (PD + barrel, 5 A)      */
     UIOX_CHG_IC_FUSB302,         /**< ONSEMI FUSB302 (PD PHY only)        */
     UIOX_CHG_IC_MAX77958,        /**< Maxim MAX77958 (PD + fast-charge)   */
     UIOX_CHG_IC_GENERIC_BARREL,  /**< Generic barrel-jack CV/CC IC        */
 } uiox_chg_ic_t;
 
 /* =========================================================================
  * Input source type
  * ====================================================================== */
 
 typedef enum {
     UIOX_CHG_SRC_NONE    = 0,  /**< No input connected                   */
     UIOX_CHG_SRC_USBC_PD,      /**< USB-C Power Delivery                 */
     UIOX_CHG_SRC_USBC_STD,     /**< USB-C 5 V standard (no PD)          */
     UIOX_CHG_SRC_BARREL,       /**< Barrel jack DC input                 */
     UIOX_CHG_SRC_USB_SDP,      /**< USB Standard Downstream Port 500 mA  */
     UIOX_CHG_SRC_USB_CDP,      /**< USB Charging Downstream Port 1.5 A   */
     UIOX_CHG_SRC_USB_DCP,      /**< USB Dedicated Charging Port (BC1.2)  */
 } uiox_chg_src_t;
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_CHG_CAP_USBC_PD        (1u << 0)  /**< USB-C PD 3.x          */
 #define UIOX_CHG_CAP_USBC_PPS       (1u << 1)  /**< PD Programmable Power */
 #define UIOX_CHG_CAP_BARREL         (1u << 2)  /**< Barrel-jack input      */
 #define UIOX_CHG_CAP_BC12           (1u << 3)  /**< BC 1.2 detection       */
 #define UIOX_CHG_CAP_OTG            (1u << 4)  /**< OTG 5 V boost output  */
 #define UIOX_CHG_CAP_FAST_CHARGE    (1u << 5)  /**< Fast-charge (QC/AFC)  */
 #define UIOX_CHG_CAP_ADC_VBUS       (1u << 6)  /**< VBUS ADC              */
 #define UIOX_CHG_CAP_ADC_VBAT       (1u << 7)  /**< VBAT ADC              */
 #define UIOX_CHG_CAP_ADC_IBAT       (1u << 8)  /**< IBAT ADC              */
 #define UIOX_CHG_CAP_ADC_TDIE       (1u << 9)  /**< Die-temp ADC          */
 #define UIOX_CHG_CAP_ADC_NTC        (1u << 10) /**< NTC thermistor ADC    */
 #define UIOX_CHG_CAP_BATFET         (1u << 11) /**< BATFET control        */
 #define UIOX_CHG_CAP_WATCHDOG       (1u << 12) /**< Host watchdog timer   */
 #define UIOX_CHG_CAP_VINDPM         (1u << 13) /**< VINDPM threshold      */
 #define UIOX_CHG_CAP_IINDPM         (1u << 14) /**< IINDPM (input curr)   */
 
 /* =========================================================================
  * BQ25895 Register map (representative; used by IF layer)
  * ====================================================================== */
 
 /* I²C address */
 #define BQ25895_I2C_ADDR            0x6Au
 
 /* Registers */
 #define BQ25895_REG00               0x00u  /**< Input source control      */
 #define BQ25895_REG01               0x01u  /**< Power-on config           */
 #define BQ25895_REG02               0x02u  /**< Charge current control    */
 #define BQ25895_REG03               0x03u  /**< Discharge / OTG control   */
 #define BQ25895_REG04               0x04u  /**< Charge voltage limit      */
 #define BQ25895_REG05               0x05u  /**< Charge term/timer control */
 #define BQ25895_REG06               0x06u  /**< IR compensation           */
 #define BQ25895_REG07               0x07u  /**< Misc operation control    */
 #define BQ25895_REG08               0x08u  /**< System status reg 1       */
 #define BQ25895_REG09               0x09u  /**< Fault register            */
 #define BQ25895_REG0A               0x0Au  /**< Boost / voltage limit     */
 #define BQ25895_REG0B               0x0Bu  /**< Status / part number      */
 #define BQ25895_REG0C               0x0Cu  /**< Fault register 2          */
 #define BQ25895_REG0D               0x0Du  /**< VINDPM threshold          */
 #define BQ25895_REG0E               0x0Eu  /**< ADC: VBUS                 */
 #define BQ25895_REG0F               0x0Fu  /**< ADC: VBAT                 */
 #define BQ25895_REG10               0x10u  /**< ADC: VSYS                 */
 #define BQ25895_REG11               0x11u  /**< ADC: TDIE                 */
 #define BQ25895_REG12               0x12u  /**< ADC: VBUS IBUS            */
 #define BQ25895_REG13               0x13u  /**< ADC: ICHGR                */
 #define BQ25895_REG14               0x14u  /**< Device ID                 */
 
 /* REG08 (System Status 1) bit fields */
 #define BQ_VBUS_STAT_SHIFT          5u
 #define BQ_VBUS_STAT_MASK           (0x07u << BQ_VBUS_STAT_SHIFT)
 #define BQ_CHRG_STAT_SHIFT          3u
 #define BQ_CHRG_STAT_MASK           (0x03u << BQ_CHRG_STAT_SHIFT)
 #define BQ_PG_STAT                  (1u << 2u)  /**< Power good            */
 #define BQ_VSYS_STAT                (1u << 0u)  /**< VSYS regulation       */
 
 /* REG09 (Fault) bit fields */
 #define BQ_FAULT_WATCHDOG           (1u << 7u)
 #define BQ_FAULT_BOOST              (1u << 6u)
 #define BQ_FAULT_CHRG_MASK          (0x03u << 4u)
 #define BQ_FAULT_BAT_OVP            (1u << 3u)
 #define BQ_FAULT_NTC_MASK           (0x07u << 0u)
 
 /* Charge status codes (REG08 bits [4:3]) */
 #define BQ_CHRG_STAT_NONE           0u
 #define BQ_CHRG_STAT_PRECHARGE      1u
 #define BQ_CHRG_STAT_FAST           2u
 #define BQ_CHRG_STAT_DONE           3u
 
 /* =========================================================================
  * GPIO pin IDs (board-specific; platform assigns these)
  * ====================================================================== */
 
 #define UIOX_CHG_GPIO_INT_N         0u  /**< INT# (active-low IRQ)        */
 #define UIOX_CHG_GPIO_CE_N          1u  /**< CE#  (charge enable, act-low)*/
 #define UIOX_CHG_GPIO_OTG           2u  /**< OTG boost enable             */
 #define UIOX_CHG_GPIO_ACOK          3u  /**< AC-OK / PGOOD input detect   */
 #define UIOX_CHG_GPIO_BATFET        4u  /**< BATFET disable               */
 
 /* =========================================================================
  * ADC channel IDs
  * ====================================================================== */
 
 typedef enum {
     UIOX_CHG_ADC_VBUS  = 0,
     UIOX_CHG_ADC_VBAT,
     UIOX_CHG_ADC_IBAT,
     UIOX_CHG_ADC_VSYS,
     UIOX_CHG_ADC_TDIE,
     UIOX_CHG_ADC_NTC,
     UIOX_CHG_ADC_MAX,
 } uiox_chg_adc_ch_t;
 
 /* =========================================================================
  * Charge state (reported by hardware status register)
  * ====================================================================== */
 
 typedef enum {
     UIOX_CHG_CHRG_IDLE      = 0,
     UIOX_CHG_CHRG_PRECHARGE,
     UIOX_CHG_CHRG_FAST,
     UIOX_CHG_CHRG_TAPER,
     UIOX_CHG_CHRG_DONE,
     UIOX_CHG_CHRG_FAULT,
 } uiox_chg_chrg_t;
 
 /* =========================================================================
  * Fault bitmap
  * ====================================================================== */
 
 #define UIOX_CHG_FAULT_NONE         0u
 #define UIOX_CHG_FAULT_OVP          (1u << 0)  /**< VBUS over-voltage      */
 #define UIOX_CHG_FAULT_OCP          (1u << 1)  /**< Over-current           */
 #define UIOX_CHG_FAULT_OTP          (1u << 2)  /**< Over-temperature       */
 #define UIOX_CHG_FAULT_BAT_OVP      (1u << 3)  /**< Battery over-voltage   */
 #define UIOX_CHG_FAULT_WATCHDOG     (1u << 4)  /**< Watchdog timeout       */
 #define UIOX_CHG_FAULT_BOOST        (1u << 5)  /**< OTG boost fault        */
 #define UIOX_CHG_FAULT_NTC_COLD     (1u << 6)  /**< NTC cold (< 0 °C)     */
 #define UIOX_CHG_FAULT_NTC_HOT      (1u << 7)  /**< NTC hot (> 60 °C)     */
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 #define UIOX_CHG_MODEL_LEN          48u
 
 typedef struct {
     /* I²C */
     uint8_t         i2c_addr;       /**< 7-bit I²C address               */
     uint32_t        i2c_bus;        /**< Host I²C bus index               */
     /* IRQ */
     uint32_t        irq;            /**< Platform IRQ number (INT# pin)  */
     /* Capabilities */
     uint32_t        caps;
     uiox_chg_ic_t   ic_type;
     char            model[UIOX_CHG_MODEL_LEN];
     /* Configuration limits */
     uint32_t        vbus_max_mv;    /**< Max VBUS input voltage (mV)     */
     uint32_t        vbat_max_mv;    /**< Max battery charge voltage (mV) */
     uint32_t        ibat_max_ma;    /**< Max charge current (mA)         */
     uint32_t        iin_max_ma;     /**< Max input current limit (mA)    */
     /* Runtime state */
     uiox_chg_src_t  src;
     uiox_chg_chrg_t chrg_state;
     uint32_t        fault_flags;
     /* ADC cache (last read values) */
     int32_t         adc_mv[UIOX_CHG_ADC_MAX];  /**< mV or mA or m°C     */
     /* Pending IRQ bitmask */
     volatile uint32_t pending_irq;
     /* Private (ops vtable) */
     void           *priv;
 } uiox_chg_hw_t;
 
 /* Pending IRQ bits */
 #define UIOX_CHG_IRQ_STATUS         (1u << 0)  /**< Charge status change  */
 #define UIOX_CHG_IRQ_FAULT          (1u << 1)  /**< Fault asserted        */
 #define UIOX_CHG_IRQ_VBUS           (1u << 2)  /**< VBUS plug/unplug      */
 #define UIOX_CHG_IRQ_PD_MSG         (1u << 3)  /**< PD message received   */
 #define UIOX_CHG_IRQ_ACOK           (1u << 4)  /**< ACOK / barrel plug    */
 
 /* =========================================================================
  * Hardware operations vtable  (18-op table)
  * ====================================================================== */
 
 typedef struct {
     /* Lifecycle */
     int  (*init)          (uiox_chg_hw_t *hw);
     void (*deinit)        (uiox_chg_hw_t *hw);
 
     /* Raw I²C register access */
     int  (*reg_read)      (uiox_chg_hw_t *hw, uint8_t reg, uint8_t *val);
     int  (*reg_write)     (uiox_chg_hw_t *hw, uint8_t reg, uint8_t  val);
     int  (*reg_rmw)       (uiox_chg_hw_t *hw, uint8_t reg,
                            uint8_t mask, uint8_t bits);
 
     /* ADC */
     int  (*adc_read)      (uiox_chg_hw_t *hw, uiox_chg_adc_ch_t ch,
                            int32_t *val_mv);
 
     /* Charge configuration */
     int  (*set_ichg)      (uiox_chg_hw_t *hw, uint32_t ma);
     int  (*set_vchg)      (uiox_chg_hw_t *hw, uint32_t mv);
     int  (*set_iin_lim)   (uiox_chg_hw_t *hw, uint32_t ma);
     int  (*set_vindpm)    (uiox_chg_hw_t *hw, uint32_t mv);
 
     /* Charge enable / disable */
     int  (*charge_enable) (uiox_chg_hw_t *hw, bool en);
 
     /* OTG boost */
     int  (*otg_enable)    (uiox_chg_hw_t *hw, bool en);
 
     /* Status poll */
     int  (*get_status)    (uiox_chg_hw_t *hw,
                            uiox_chg_chrg_t *chrg,
                            uiox_chg_src_t  *src,
                            uint32_t        *faults);
 
     /* Watchdog */
     int  (*wdog_reset)    (uiox_chg_hw_t *hw);
 
     /* GPIO */
     void (*gpio_write)    (uiox_chg_hw_t *hw, uint32_t pin, bool val);
     bool (*gpio_read)     (uiox_chg_hw_t *hw, uint32_t pin);
 
     /* USB-C PD PHY (optional; NULL for non-PD ICs) */
     int  (*pd_tx_msg)     (uiox_chg_hw_t *hw,
                            const uint8_t *buf, uint8_t len);
     int  (*pd_rx_msg)     (uiox_chg_hw_t *hw,
                            uint8_t *buf, uint8_t max_len);
 
     /* ISR */
     void (*isr)           (uiox_chg_hw_t *hw);
 } uiox_chg_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int  uiox_chg_hw_init        (uiox_chg_hw_t *hw,
                                const uiox_chg_hw_ops_t *ops);
 void uiox_chg_hw_deinit      (uiox_chg_hw_t *hw);
 int  uiox_chg_hw_reg_read    (uiox_chg_hw_t *hw,
                                uint8_t reg, uint8_t *val);
 int  uiox_chg_hw_reg_write   (uiox_chg_hw_t *hw,
                                uint8_t reg, uint8_t  val);
 int  uiox_chg_hw_reg_rmw     (uiox_chg_hw_t *hw,
                                uint8_t reg, uint8_t mask, uint8_t bits);
 int  uiox_chg_hw_adc_read    (uiox_chg_hw_t *hw,
                                uiox_chg_adc_ch_t ch, int32_t *val_mv);
 int  uiox_chg_hw_set_ichg    (uiox_chg_hw_t *hw, uint32_t ma);
 int  uiox_chg_hw_set_vchg    (uiox_chg_hw_t *hw, uint32_t mv);
 int  uiox_chg_hw_set_iin_lim (uiox_chg_hw_t *hw, uint32_t ma);
 int  uiox_chg_hw_set_vindpm  (uiox_chg_hw_t *hw, uint32_t mv);
 int  uiox_chg_hw_charge_en   (uiox_chg_hw_t *hw, bool en);
 int  uiox_chg_hw_otg_en      (uiox_chg_hw_t *hw, bool en);
 int  uiox_chg_hw_get_status  (uiox_chg_hw_t *hw,
                                uiox_chg_chrg_t *chrg,
                                uiox_chg_src_t  *src,
                                uint32_t        *faults);
 int  uiox_chg_hw_wdog_reset  (uiox_chg_hw_t *hw);
 int  uiox_chg_hw_pd_tx       (uiox_chg_hw_t *hw,
                                const uint8_t *buf, uint8_t len);
 int  uiox_chg_hw_pd_rx       (uiox_chg_hw_t *hw,
                                uint8_t *buf, uint8_t max_len);
 
 static inline uint32_t uiox_chg_caps(const uiox_chg_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_CHG_HW_H */
 