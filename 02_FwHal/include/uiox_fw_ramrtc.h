/* ─────────────────────────────────────────────────────────────────────────
 * uiox_fw_ramrtc.h — Battery-backed SRAM + RTC (DS1307 / MC146818)
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef UIOX_FW_RAMRTC_H
#define UIOX_FW_RAMRTC_H
#include "uiox_fw_types.h"
#include "uiox_fw_i2c.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { UIOX_RAMRTC_DS1307=0, UIOX_RAMRTC_MC146818=1,
               UIOX_RAMRTC_DS3231=2  } uiox_ramrtc_chip_t;

#define DS1307_ADDR         0x68u
#define DS1307_REG_SEC      0x00u
#define DS1307_REG_MIN      0x01u
#define DS1307_REG_HOUR     0x02u
#define DS1307_REG_DOW      0x03u
#define DS1307_REG_DATE     0x04u
#define DS1307_REG_MONTH    0x05u
#define DS1307_REG_YEAR     0x06u
#define DS1307_REG_CTRL     0x07u
#define DS1307_REG_NVRAM    0x08u  /**< 56 bytes NVRAM               */

typedef struct { uint8_t sec,min,hr,dow,date,month; uint16_t year; } uiox_rtc_time_t;

typedef struct {
    uiox_i2c_dev_t    *i2c;
    uintptr_t          mmio_base;  /**< For MC146818 MMIO             */
    uint8_t            addr;
    uiox_ramrtc_chip_t chip;
    bool               battery_ok;
    bool               initialized;
    void              *priv;
} uiox_ramrtc_dev_t;

typedef struct {
    uiox_fw_err_t (*init)       (uiox_ramrtc_dev_t *dev);
    void          (*deinit)     (uiox_ramrtc_dev_t *dev);
    uiox_fw_err_t (*read_time)  (uiox_ramrtc_dev_t *dev, uiox_rtc_time_t *t);
    uiox_fw_err_t (*write_time) (uiox_ramrtc_dev_t *dev,
                                   const uiox_rtc_time_t *t);
    uiox_fw_err_t (*read_nvram) (uiox_ramrtc_dev_t *dev,
                                   uint8_t off, uint8_t *buf, uint8_t len);
    uiox_fw_err_t (*write_nvram)(uiox_ramrtc_dev_t *dev,
                                   uint8_t off, const uint8_t *buf, uint8_t len);
    bool          (*bat_ok)     (uiox_ramrtc_dev_t *dev);
} uiox_ramrtc_ops_t;

uiox_fw_err_t uiox_fw_ramrtc_init        (uiox_ramrtc_dev_t *dev,
                                            const uiox_ramrtc_ops_t *ops);
void          uiox_fw_ramrtc_deinit      (uiox_ramrtc_dev_t *dev);
uiox_fw_err_t uiox_fw_ramrtc_read_time   (uiox_ramrtc_dev_t *dev, uiox_rtc_time_t *t);
uiox_fw_err_t uiox_fw_ramrtc_write_time  (uiox_ramrtc_dev_t *dev,
                                            const uiox_rtc_time_t *t);
uiox_fw_err_t uiox_fw_ramrtc_read_nvram  (uiox_ramrtc_dev_t *dev,
                                            uint8_t off, uint8_t *buf, uint8_t len);
bool          uiox_fw_ramrtc_bat_ok      (uiox_ramrtc_dev_t *dev);
uiox_fw_err_t uiox_fw_ramrtc_init_ds1307 (uiox_ramrtc_dev_t *dev, uiox_i2c_dev_t *i2c);
uiox_fw_err_t uiox_fw_ramrtc_init_mc146818(uiox_ramrtc_dev_t *dev, uintptr_t base);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_RAMRTC_H */
