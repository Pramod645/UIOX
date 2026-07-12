/**
 * @file  uiox_fw_als.h
 * @brief UIOX FwHal — Ambient Light Sensor (VEML7700 / OPT3001 / BH1750).
 *        Communicates over I2C via uiox_fw_i2c_read_reg().
 * @version 1.0.0
 */

 #ifndef UIOX_FW_ALS_H
 #define UIOX_FW_ALS_H
 
 #include "uiox_fw_types.h"
 #include "uiox_fw_i2c.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* ── Chip variant ────────────────────────────────────────────── */
 typedef enum {
     UIOX_ALS_VEML7700 = 0,
     UIOX_ALS_OPT3001  = 1,
     UIOX_ALS_BH1750   = 2,
     UIOX_ALS_TSL2591  = 3,
 } uiox_als_chip_t;
 
 /* ── VEML7700 registers ──────────────────────────────────────── */
 #define VEML7700_ADDR           0x10u
 #define VEML7700_REG_ALS_CONF   0x00u
 #define VEML7700_REG_ALS        0x04u
 #define VEML7700_REG_WHITE      0x05u
 #define VEML7700_REG_ALS_INT    0x06u
 #define VEML7700_CONF_SD        (1u << 0)  /**< Shutdown              */
 #define VEML7700_CONF_INT_EN    (1u << 1)  /**< Interrupt enable      */
 #define VEML7700_CONF_GAIN_1X   (0u << 11)
 #define VEML7700_CONF_GAIN_2X   (1u << 11)
 #define VEML7700_CONF_IT_100MS  (0u << 6)
 #define VEML7700_CONF_IT_200MS  (1u << 6)
 
 /* ── OPT3001 registers ───────────────────────────────────────── */
 #define OPT3001_ADDR            0x44u
 #define OPT3001_REG_RESULT      0x00u
 #define OPT3001_REG_CONFIG      0x01u
 #define OPT3001_CFG_CONT        (0x02u << 9u)  /**< Continuous conv    */
 #define OPT3001_CFG_RANGE_AUTO  (0x0Cu << 12u)
 
 /* ── Gain + integration time (IC-agnostic) ───────────────────── */
 typedef enum {
     UIOX_ALS_GAIN_1X = 0,
     UIOX_ALS_GAIN_2X = 1,
     UIOX_ALS_GAIN_8X = 2,
 } uiox_als_gain_t;
 
 typedef enum {
     UIOX_ALS_ITIME_25MS  = 0,
     UIOX_ALS_ITIME_100MS = 1,
     UIOX_ALS_ITIME_200MS = 2,
     UIOX_ALS_ITIME_800MS = 3,
 } uiox_als_itime_t;
 
 /* ── Device context ──────────────────────────────────────────── */
 typedef struct {
     uiox_i2c_dev_t  *i2c;
     uint8_t          addr;
     uiox_als_chip_t  chip;
     uiox_als_gain_t  gain;
     uiox_als_itime_t itime;
     uint32_t         lux_milli;     /**< Last reading ×1000            */
     bool             initialized;
 } uiox_als_dev_t;
 
 /* ── Ops vtable ──────────────────────────────────────────────── */
 typedef struct {
     uiox_fw_err_t (*init)       (uiox_als_dev_t *dev);
     void          (*deinit)     (uiox_als_dev_t *dev);
     uiox_fw_err_t (*configure)  (uiox_als_dev_t *dev,
                                   uiox_als_gain_t gain,
                                   uiox_als_itime_t itime);
     uiox_fw_err_t (*read_lux)   (uiox_als_dev_t *dev,
                                   uint32_t *lux_milli);
     uiox_fw_err_t (*read_raw)   (uiox_als_dev_t *dev,
                                   uint16_t *als, uint16_t *white);
     uiox_fw_err_t (*set_thresh) (uiox_als_dev_t *dev,
                                   uint16_t low, uint16_t high);
     void          (*isr)        (uiox_als_dev_t *dev);
 } uiox_als_ops_t;
 
 /* ── API ─────────────────────────────────────────────────────── */
 uiox_fw_err_t uiox_fw_als_init      (uiox_als_dev_t *dev,
                                        const uiox_als_ops_t *ops);
 void          uiox_fw_als_deinit    (uiox_als_dev_t *dev);
 uiox_fw_err_t uiox_fw_als_configure (uiox_als_dev_t *dev,
                                        uiox_als_gain_t gain,
                                        uiox_als_itime_t itime);
 uiox_fw_err_t uiox_fw_als_read_lux  (uiox_als_dev_t *dev,
                                        uint32_t *lux_milli);
 uiox_fw_err_t uiox_fw_als_read_raw  (uiox_als_dev_t *dev,
                                        uint16_t *als, uint16_t *white);
 
 /* Auto-gain: adjusts gain/itime if reading saturated or too dark */
 uiox_fw_err_t uiox_fw_als_auto_gain (uiox_als_dev_t *dev);
 
 /* Platform helper — create VEML7700 device on given I2C bus */
 uiox_fw_err_t uiox_fw_als_init_veml7700(uiox_als_dev_t *dev,
                                           uiox_i2c_dev_t *i2c);
 uiox_fw_err_t uiox_fw_als_init_opt3001 (uiox_als_dev_t *dev,
                                           uiox_i2c_dev_t *i2c);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_ALS_H */
 