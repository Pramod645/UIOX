/**
 * @file    uiox_bms_hw.h
 * @brief   UIOX BMS Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to BMS hardware. Supports:
 *   - TI BQ76940 / BQ76920 (3–15 cell AFE)
 *   - TI BQ40Z50 / BQ40Z80 (standalone gauge + protection)
 *   - Maxim DS2760 / DS2782 (single-cell fuel gauge)
 *   - Renesas ISL94202 / ISL9238 (multi-cell BMS IC)
 *   - NXP MC33772 / MC33774 (automotive Li-ion AFE)
 *
 * Owns:
 *   - I2C/SPI register read/write to AFE
 *   - Cell voltage measurement trigger
 *   - Pack current measurement (shunt/Hall sensor)
 *   - Temperature measurement (NTC/internal)
 *   - Protection FET control (charge/discharge FET GPIO)
 *   - Alert/fault IRQ handling
 *   - Cell balancing FET control
 *
 * @version 1.0.0
 * @date    2026-06-04
 */

 #ifndef UIOX_BMS_HW_H
 #define UIOX_BMS_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_BMS_CAP_CELL_VOLTAGE   (1u << 0)  /**< Per-cell voltage meas */
 #define UIOX_BMS_CAP_PACK_VOLTAGE   (1u << 1)  /**< Pack voltage          */
 #define UIOX_BMS_CAP_CURRENT        (1u << 2)  /**< Pack current (shunt)  */
 #define UIOX_BMS_CAP_TEMPERATURE    (1u << 3)  /**< Temperature sensor    */
 #define UIOX_BMS_CAP_COULOMB_COUNT  (1u << 4)  /**< HW coulomb counter    */
 #define UIOX_BMS_CAP_OVP            (1u << 5)  /**< Over-voltage protect  */
 #define UIOX_BMS_CAP_UVP            (1u << 6)  /**< Under-voltage protect */
 #define UIOX_BMS_CAP_OCP_CHG        (1u << 7)  /**< OC protect (charge)   */
 #define UIOX_BMS_CAP_OCP_DSG        (1u << 8)  /**< OC protect (discharge)*/
 #define UIOX_BMS_CAP_SCP            (1u << 9)  /**< Short-circuit protect */
 #define UIOX_BMS_CAP_OTP            (1u << 10) /**< Over-temp protect     */
 #define UIOX_BMS_CAP_BALANCING      (1u << 11) /**< Cell balancing FETs   */
 #define UIOX_BMS_CAP_FET_CHG        (1u << 12) /**< Charge FET control    */
 #define UIOX_BMS_CAP_FET_DSG        (1u << 13) /**< Discharge FET control */
 #define UIOX_BMS_CAP_WAKE_COMP      (1u << 14) /**< Wake comparator       */
 #define UIOX_BMS_CAP_SHIP_MODE      (1u << 15) /**< Ultra-low power ship  */
 
 /* =========================================================================
  * AFE bus interface
  * ====================================================================== */
 
 typedef enum {
     UIOX_BMS_BUS_I2C = 0,
     UIOX_BMS_BUS_SPI,
     UIOX_BMS_BUS_HDQ,   /**< Single-wire HDQ (TI BQ series)              */
     UIOX_BMS_BUS_SMBUS, /**< SMBus (compatible with I2C)                 */
 } uiox_bms_bus_t;
 
 /* =========================================================================
  * BMS fault flags (mapped from AFE status registers)
  * ====================================================================== */
 
 #define UIOX_BMS_FAULT_OVP          (1u << 0)
 #define UIOX_BMS_FAULT_UVP          (1u << 1)
 #define UIOX_BMS_FAULT_OCP_CHG      (1u << 2)
 #define UIOX_BMS_FAULT_OCP_DSG      (1u << 3)
 #define UIOX_BMS_FAULT_SCP          (1u << 4)
 #define UIOX_BMS_FAULT_OTP          (1u << 5)
 #define UIOX_BMS_FAULT_UTP          (1u << 6)  /**< Under-temperature     */
 #define UIOX_BMS_FAULT_AFE_COMM     (1u << 7)  /**< AFE communication err */
 #define UIOX_BMS_FAULT_CELL_OPEN    (1u << 8)  /**< Open cell wire detect */
 #define UIOX_BMS_FAULT_CHARGE_FET   (1u << 9)
 #define UIOX_BMS_FAULT_DSG_FET      (1u << 10)
 
 /* =========================================================================
  * BMS hardware device descriptor
  * ====================================================================== */
 
 #define UIOX_BMS_MAX_CELLS          16
 #define UIOX_BMS_MAX_TEMPS          4
 #define UIOX_BMS_MODEL_LEN          32
 
 typedef struct {
     uintptr_t          i2c_base;      /**< I2C controller MMIO base        */
     uint8_t            i2c_addr;      /**< 7-bit I2C / SMBus address       */
     uint32_t           irq;           /**< AFE alert IRQ line              */
     uint32_t           caps;
     uiox_bms_bus_t     bus;
     char               model[UIOX_BMS_MODEL_LEN];
     uint8_t            num_cells;     /**< Cells in series                 */
     uint8_t            num_temps;     /**< Temperature sensors             */
 
     /* GPIO */
     uint32_t           chg_fet_pin;   /**< Charge FET enable GPIO          */
     uint32_t           dsg_fet_pin;   /**< Discharge FET enable GPIO       */
     uint32_t           alert_pin;     /**< Alert input GPIO                */
     uint32_t           pres_pin;      /**< Pack present detect GPIO        */
 
     /* Shunt / current sense */
     uint32_t           shunt_uohm;    /**< Shunt resistor (µΩ)            */
 
     /* Measurement results (filled by HAL) */
     uint32_t           cell_mv[UIOX_BMS_MAX_CELLS]; /**< Cell voltages (mV)*/
     uint32_t           pack_mv;       /**< Total pack voltage (mV)         */
     int32_t            current_ma;    /**< Pack current (mA, +ve=charge)  */
     int16_t            temp_dc[UIOX_BMS_MAX_TEMPS];  /**< Temps (°C × 10) */
     int32_t            coulombs_mah;  /**< Coulomb counter (mAh)           */
     volatile uint32_t  fault_flags;
 
     /* State */
     bool               chg_fet_on;
     bool               dsg_fet_on;
     bool               present;
     bool               initialised;
 
     void              *priv;
 } uiox_bms_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     int  (*init)          (uiox_bms_hw_t *hw);
     void (*deinit)        (uiox_bms_hw_t *hw);
 
     /* Register access */
     int  (*reg_read)      (uiox_bms_hw_t *hw,
                            uint8_t reg, uint8_t *val);
     int  (*reg_write)     (uiox_bms_hw_t *hw,
                            uint8_t reg, uint8_t val);
     int  (*reg_read16)    (uiox_bms_hw_t *hw,
                            uint8_t reg, uint16_t *val);
     int  (*reg_write16)   (uiox_bms_hw_t *hw,
                            uint8_t reg, uint16_t val);
     int  (*bulk_read)     (uiox_bms_hw_t *hw,
                            uint8_t reg, uint8_t *buf, uint8_t len);
 
     /* Measurements */
     int  (*measure_cells) (uiox_bms_hw_t *hw);
     int  (*measure_current)(uiox_bms_hw_t *hw);
     int  (*measure_temp)  (uiox_bms_hw_t *hw);
     int  (*read_coulombs) (uiox_bms_hw_t *hw, int32_t *mah_out);
 
     /* Protection FETs */
     int  (*set_chg_fet)   (uiox_bms_hw_t *hw, bool on);
     int  (*set_dsg_fet)   (uiox_bms_hw_t *hw, bool on);
 
     /* Cell balancing */
     int  (*set_balance)   (uiox_bms_hw_t *hw, uint16_t cell_mask);
     int  (*get_balance)   (uiox_bms_hw_t *hw, uint16_t *cell_mask);
 
     /* Fault / IRQ */
     int  (*fault_status)  (uiox_bms_hw_t *hw, uint32_t *flags_out);
     int  (*fault_clear)   (uiox_bms_hw_t *hw, uint32_t flags);
 
     /* Pack presence */
     bool (*pack_present)  (uiox_bms_hw_t *hw);
 
     /* Ship mode */
     int  (*ship_mode)     (uiox_bms_hw_t *hw);
 
     /* ISR */
     void (*isr)           (uiox_bms_hw_t *hw);
 } uiox_bms_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int      uiox_bms_hw_init          (uiox_bms_hw_t *hw,
                                      const uiox_bms_hw_ops_t *ops);
 void     uiox_bms_hw_deinit        (uiox_bms_hw_t *hw);
 int      uiox_bms_hw_measure_cells (uiox_bms_hw_t *hw);
 int      uiox_bms_hw_measure_current(uiox_bms_hw_t *hw);
 int      uiox_bms_hw_measure_temp  (uiox_bms_hw_t *hw);
 int      uiox_bms_hw_set_chg_fet   (uiox_bms_hw_t *hw, bool on);
 int      uiox_bms_hw_set_dsg_fet   (uiox_bms_hw_t *hw, bool on);
 int      uiox_bms_hw_set_balance   (uiox_bms_hw_t *hw, uint16_t mask);
 int      uiox_bms_hw_fault_status  (uiox_bms_hw_t *hw, uint32_t *flags);
 int      uiox_bms_hw_fault_clear   (uiox_bms_hw_t *hw, uint32_t flags);
 bool     uiox_bms_hw_pack_present  (uiox_bms_hw_t *hw);
 
 static inline uint32_t uiox_bms_caps(const uiox_bms_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BMS_HW_H */
 