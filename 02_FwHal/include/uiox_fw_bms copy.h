/**
 * @file  uiox_fw_bms.h
 * @brief UIOX FwHal — Battery Management System (BQ27742 / MAX17055).
 *        Reports SOC, voltage, current, temperature, health.
 */

 #ifndef UIOX_FW_BMS_H
 #define UIOX_FW_BMS_H
 
 #include "uiox_fw_types.h"
 #include "uiox_fw_i2c.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_BMS_BQ27742  = 0,
     UIOX_BMS_MAX17055 = 1,
     UIOX_BMS_BQ27621  = 2,
 } uiox_bms_chip_t;
 
 /* ── BQ27742 standard registers ─────────────────────────────── */
 #define BQ27742_ADDR            0x55u
 #define BQ27742_REG_CTRL        0x00u
 #define BQ27742_REG_TEMP        0x06u  /**< Temperature ×10 K         */
 #define BQ27742_REG_VOLT        0x08u  /**< Voltage mV                */
 #define BQ27742_REG_FLAGS       0x0Au
 #define BQ27742_REG_NOM_CAP     0x0Cu  /**< Nominal capacity mAh      */
 #define BQ27742_REG_FULL_CAP    0x0Eu
 #define BQ27742_REG_RM          0x10u  /**< Remaining capacity mAh    */
 #define BQ27742_REG_AVG_CURR    0x14u  /**< Avg current mA (signed)   */
 #define BQ27742_REG_SOC         0x1Cu  /**< State of charge %         */
 #define BQ27742_REG_ISOC        0x1Eu  /**< Internal SOC              */
 #define BQ27742_REG_SOH         0x28u  /**< State of health %         */
 #define BQ27742_FLAG_DSG        (1u << 0)  /**< Discharging            */
 #define BQ27742_FLAG_FC         (1u << 9)  /**< Fully charged          */
 
 /* ── Battery data snapshot ───────────────────────────────────── */
 typedef struct {
     uint16_t voltage_mv;     /**< Cell voltage in mV                  */
     int16_t  current_ma;     /**< Positive = charging, neg = discharging*/
     uint8_t  soc_pct;        /**< State of charge 0–100 %             */
     uint8_t  soh_pct;        /**< State of health 0–100 %             */
     uint16_t rem_cap_mah;    /**< Remaining capacity mAh              */
     uint16_t full_cap_mah;
     int16_t  temp_dc;        /**< Temperature in decidegrees Celsius  */
     bool     charging;
     bool     full;
 } uiox_bms_data_t;
 
 typedef struct {
     uiox_i2c_dev_t *i2c;
     uint8_t         addr;
     uiox_bms_chip_t chip;
     uiox_bms_data_t last;
     bool            initialized;
 } uiox_bms_dev_t;
 
 typedef struct {
     uiox_fw_err_t (*init)   (uiox_bms_dev_t *dev);
     void          (*deinit) (uiox_bms_dev_t *dev);
     uiox_fw_err_t (*read)   (uiox_bms_dev_t *dev, uiox_bms_data_t *out);
     uiox_fw_err_t (*reset)  (uiox_bms_dev_t *dev);
 } uiox_bms_ops_t;
 
 uiox_fw_err_t uiox_fw_bms_init     (uiox_bms_dev_t *dev,
                                       const uiox_bms_ops_t *ops);
 void          uiox_fw_bms_deinit   (uiox_bms_dev_t *dev);
 uiox_fw_err_t uiox_fw_bms_read     (uiox_bms_dev_t *dev,
                                       uiox_bms_data_t *out);
 uiox_fw_err_t uiox_fw_bms_init_bq27742(uiox_bms_dev_t *dev,
                                          uiox_i2c_dev_t *i2c);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_BMS_H */
 