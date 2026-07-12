#ifndef UIOX_FW_FAN_H
#define UIOX_FW_FAN_H
#include "uiox_fw_types.h"
#ifdef __cplusplus
extern "C" {
#endif

/* PWM fan via general-purpose timer PWM output */
typedef struct {
    uintptr_t pwm_base;    /**< Timer base driving PWM output           */
    uint32_t  pwm_period;  /**< Period in timer ticks                   */
    uint32_t  irq;         /**< Tach input IRQ (0=polling)              */
    uint8_t   duty_pct;    /**< Current duty cycle 0–100 %             */
    uint32_t  rpm;         /**< Last measured RPM (from tach)           */
    uint32_t  min_rpm;     /**< Fault if below this (0=disabled)        */
    bool      initialized;
    void     *priv;
} uiox_fan_dev_t;

typedef struct {
    uiox_fw_err_t (*init)      (uiox_fan_dev_t *dev);
    void          (*deinit)    (uiox_fan_dev_t *dev);
    uiox_fw_err_t (*set_duty)  (uiox_fan_dev_t *dev, uint8_t pct);
    uiox_fw_err_t (*read_rpm)  (uiox_fan_dev_t *dev, uint32_t *rpm);
    uiox_fw_err_t (*set_auto)  (uiox_fan_dev_t *dev,
                                  uint32_t low_temp_dc, uint32_t hi_temp_dc);
} uiox_fan_ops_t;

uiox_fw_err_t uiox_fw_fan_init     (uiox_fan_dev_t *dev, const uiox_fan_ops_t *ops);
void          uiox_fw_fan_deinit   (uiox_fan_dev_t *dev);
uiox_fw_err_t uiox_fw_fan_set_duty (uiox_fan_dev_t *dev, uint8_t pct);
uiox_fw_err_t uiox_fw_fan_read_rpm (uiox_fan_dev_t *dev, uint32_t *rpm);
uiox_fw_err_t uiox_fw_fan_init_pwm (uiox_fan_dev_t *dev, uintptr_t pwm_base);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_FAN_H */
