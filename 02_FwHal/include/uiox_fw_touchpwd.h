/* ─────────────────────────────────────────────────────────────────────────
 * uiox_fw_touchpwd.h — Fingerprint / touch password sensor (I2C)
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef UIOX_FW_TOUCHPWD_H
#define UIOX_FW_TOUCHPWD_H
#include "uiox_fw_types.h"
#include "uiox_fw_i2c.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { UIOX_FP_FPC1020=0, UIOX_FP_GOODIX=1,
               UIOX_FP_CYPRESS=2 } uiox_fp_chip_t;

#define UIOX_FP_TEMPLATE_SIZE  512u
#define UIOX_FP_MAX_TEMPLATES  10u

typedef enum { UIOX_FP_EVT_NONE=0, UIOX_FP_EVT_TOUCH=1,
               UIOX_FP_EVT_LIFT=2,  UIOX_FP_EVT_MATCH=3,
               UIOX_FP_EVT_NOMATCH=4 } uiox_fp_evt_t;

typedef void (*uiox_fp_cb_t)(uiox_fp_evt_t ev, uint8_t template_id, void *p);

typedef struct {
    uiox_i2c_dev_t *i2c;
    uint8_t         addr;
    uiox_fp_chip_t  chip;
    uint32_t        gpio_int;
    uint32_t        gpio_reset;
    uint8_t         num_templates;
    uiox_fp_cb_t    ev_cb;
    void           *ev_priv;
    bool            initialized;
    void           *priv;
} uiox_fp_dev_t;

typedef struct {
    uiox_fw_err_t (*init)          (uiox_fp_dev_t *dev);
    void          (*deinit)        (uiox_fp_dev_t *dev);
    uiox_fw_err_t (*enroll_start)  (uiox_fp_dev_t *dev, uint8_t slot);
    uiox_fw_err_t (*enroll_capture)(uiox_fp_dev_t *dev);
    uiox_fw_err_t (*enroll_commit) (uiox_fp_dev_t *dev, uint8_t slot);
    uiox_fw_err_t (*verify)        (uiox_fp_dev_t *dev, uint8_t *match_id);
    uiox_fw_err_t (*delete_template)(uiox_fp_dev_t *dev, uint8_t slot);
    void          (*isr)           (uiox_fp_dev_t *dev);
} uiox_fp_ops_t;

uiox_fw_err_t uiox_fw_fp_init    (uiox_fp_dev_t *dev, const uiox_fp_ops_t *ops);
void          uiox_fw_fp_deinit  (uiox_fp_dev_t *dev);
uiox_fw_err_t uiox_fw_fp_verify  (uiox_fp_dev_t *dev, uint8_t *match_id);
uiox_fw_err_t uiox_fw_fp_enroll  (uiox_fp_dev_t *dev, uint8_t slot,
                                    uint8_t num_captures);
void          uiox_fw_fp_set_cb  (uiox_fp_dev_t *dev, uiox_fp_cb_t cb, void *p);
uiox_fw_err_t uiox_fw_fp_init_fpc1020(uiox_fp_dev_t *dev, uiox_i2c_dev_t *i2c,
                                        uint32_t gpio_int, uint32_t gpio_reset);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_TOUCHPWD_H */
