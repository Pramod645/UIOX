/**
 * @file  uiox_fw_charger.h
 * @brief UIOX FwHal — USB-C PD / barrel-jack charger (BQ25895 / FUSB302).
 *        Reuses uiox_fw_i2c for register access.
 */

 #ifndef UIOX_FW_CHARGER_H
 #define UIOX_FW_CHARGER_H
 
 #include "uiox_fw_types.h"
 #include "uiox_fw_i2c.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_CHG_IC_BQ25895   = 0,
     UIOX_CHG_IC_FUSB302   = 1,
     UIOX_CHG_IC_MAX77958  = 2,
     UIOX_CHG_IC_BARREL    = 3,
 } uiox_chg_ic_t;
 
 typedef enum {
     UIOX_CHG_SRC_NONE     = 0,
     UIOX_CHG_SRC_USBC_PD  = 1,
     UIOX_CHG_SRC_USBC_STD = 2,
     UIOX_CHG_SRC_BARREL   = 3,
     UIOX_CHG_SRC_USB_DCP  = 4,
 } uiox_chg_src_t;
 
 typedef enum {
     UIOX_CHG_STAT_IDLE      = 0,
     UIOX_CHG_STAT_PRECHARGE = 1,
     UIOX_CHG_STAT_FAST      = 2,
     UIOX_CHG_STAT_DONE      = 3,
     UIOX_CHG_STAT_FAULT     = 4,
 } uiox_chg_stat_t;
 
 /* ── BQ25895 key registers ───────────────────────────────────── */
 #define BQ25895_ADDR            0x6Au
 #define BQ25895_REG00           0x00u  /**< Input source control      */
 #define BQ25895_REG02           0x02u  /**< Charge current control    */
 #define BQ25895_REG04           0x04u  /**< Charge voltage            */
 #define BQ25895_REG08           0x08u  /**< System status             */
 #define BQ25895_REG09           0x09u  /**< Fault                     */
 #define BQ25895_REG0E           0x0Eu  /**< ADC VBUS                  */
 #define BQ25895_REG0F           0x0Fu  /**< ADC VBAT                  */
 #define BQ25895_REG13           0x13u  /**< ADC ICHGR                 */
 #define BQ25895_VBUS_STAT_SHIFT 5u
 #define BQ25895_CHRG_STAT_SHIFT 3u
 
 typedef struct {
     uiox_i2c_dev_t  *i2c;
     uint8_t          addr;
     uiox_chg_ic_t    chip;
     uiox_chg_src_t   src;
     uiox_chg_stat_t  stat;
     uint32_t         vbus_mv;
     uint32_t         vbat_mv;
     uint32_t         ichg_ma;
     uint32_t         iin_limit_ma;
     bool             initialized;
     void            *priv;
 } uiox_chg_dev_t;
 
 typedef struct {
     uiox_fw_err_t (*init)         (uiox_chg_dev_t *dev);
     void          (*deinit)       (uiox_chg_dev_t *dev);
     uiox_fw_err_t (*get_status)   (uiox_chg_dev_t *dev,
                                     uiox_chg_stat_t *stat,
                                     uiox_chg_src_t  *src);
     uiox_fw_err_t (*set_ichg)     (uiox_chg_dev_t *dev, uint32_t ma);
     uiox_fw_err_t (*set_vchg)     (uiox_chg_dev_t *dev, uint32_t mv);
     uiox_fw_err_t (*set_iin_lim)  (uiox_chg_dev_t *dev, uint32_t ma);
     uiox_fw_err_t (*read_adc)     (uiox_chg_dev_t *dev,
                                     uint32_t *vbus_mv,
                                     uint32_t *vbat_mv,
                                     uint32_t *ichg_ma);
     uiox_fw_err_t (*enable_otg)   (uiox_chg_dev_t *dev, bool en);
     void          (*isr)          (uiox_chg_dev_t *dev);
 } uiox_chg_ops_t;
 
 uiox_fw_err_t uiox_fw_chg_init        (uiox_chg_dev_t *dev,
                                          const uiox_chg_ops_t *ops);
 void          uiox_fw_chg_deinit      (uiox_chg_dev_t *dev);
 uiox_fw_err_t uiox_fw_chg_get_status  (uiox_chg_dev_t *dev,
                                          uiox_chg_stat_t *stat,
                                          uiox_chg_src_t  *src);
 uiox_fw_err_t uiox_fw_chg_set_ichg    (uiox_chg_dev_t *dev, uint32_t ma);
 uiox_fw_err_t uiox_fw_chg_set_vchg    (uiox_chg_dev_t *dev, uint32_t mv);
 uiox_fw_err_t uiox_fw_chg_read_adc    (uiox_chg_dev_t *dev,
                                          uint32_t *vbus_mv,
                                          uint32_t *vbat_mv,
                                          uint32_t *ichg_ma);
 uiox_fw_err_t uiox_fw_chg_init_bq25895(uiox_chg_dev_t *dev,
                                          uiox_i2c_dev_t *i2c);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_CHARGER_H */
 