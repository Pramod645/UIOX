/* ─────────────────────────────────────────────────────────────────────────
 * uiox_fw_pmic.h — PMIC (DA9062 / RK808) I2C HAL
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef UIOX_FW_PMIC_H
#define UIOX_FW_PMIC_H
#include "uiox_fw_types.h"
#include "uiox_fw_i2c.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { UIOX_PMIC_DA9062=0, UIOX_PMIC_RK808=1, UIOX_PMIC_ACT8865=2 } uiox_pmic_chip_t;

#define DA9062_ADDR         0x58u
#define DA9062_REG_STATUS_A 0x05u
#define DA9062_REG_BUCK1    0x9Cu  /**< BUCK1 voltage set             */
#define DA9062_REG_LDO1     0xA2u
#define UIOX_PMIC_MAX_RAILS 12u

typedef struct {
    uint8_t  idx;
    char     name[8];
    uint32_t voltage_mv;
    uint32_t min_mv, max_mv, step_mv;
    bool     enabled;
} uiox_pmic_rail_t;

typedef struct {
    uiox_i2c_dev_t   *i2c;
    uint8_t           addr;
    uiox_pmic_chip_t  chip;
    uiox_pmic_rail_t  rails[UIOX_PMIC_MAX_RAILS];
    uint8_t           num_rails;
    bool              initialized;
    void             *priv;
} uiox_pmic_dev_t;

typedef struct {
    uiox_fw_err_t (*init)         (uiox_pmic_dev_t *dev);
    void          (*deinit)       (uiox_pmic_dev_t *dev);
    uiox_fw_err_t (*set_voltage)  (uiox_pmic_dev_t *dev,
                                    uint8_t rail, uint32_t mv);
    uiox_fw_err_t (*get_voltage)  (uiox_pmic_dev_t *dev,
                                    uint8_t rail, uint32_t *mv);
    uiox_fw_err_t (*enable_rail)  (uiox_pmic_dev_t *dev, uint8_t rail);
    uiox_fw_err_t (*disable_rail) (uiox_pmic_dev_t *dev, uint8_t rail);
    uiox_fw_err_t (*read_adc)     (uiox_pmic_dev_t *dev,
                                    uint8_t ch, uint32_t *mv);
} uiox_pmic_ops_t;

uiox_fw_err_t uiox_fw_pmic_init        (uiox_pmic_dev_t *dev, const uiox_pmic_ops_t *ops);
void          uiox_fw_pmic_deinit      (uiox_pmic_dev_t *dev);
uiox_fw_err_t uiox_fw_pmic_set_voltage (uiox_pmic_dev_t *dev, uint8_t rail, uint32_t mv);
uiox_fw_err_t uiox_fw_pmic_get_voltage (uiox_pmic_dev_t *dev, uint8_t rail, uint32_t *mv);
uiox_fw_err_t uiox_fw_pmic_enable_rail (uiox_pmic_dev_t *dev, uint8_t rail);
uiox_fw_err_t uiox_fw_pmic_init_da9062 (uiox_pmic_dev_t *dev, uiox_i2c_dev_t *i2c);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_PMIC_H */
