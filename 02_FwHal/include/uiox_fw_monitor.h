/* ─────────────────────────────────────────────────────────────────────────
 * uiox_fw_monitor.h — Display monitor EDID/DDC HAL
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef UIOX_FW_MONITOR_H
#define UIOX_FW_MONITOR_H
#include "uiox_fw_types.h"
#include "uiox_fw_i2c.h"
#ifdef __cplusplus
extern "C" {
#endif

#define EDID_SIZE       128u
#define EDID_ADDR       0x50u
#define EDID_EXT_ADDR   0x51u
#define EDID_MAGIC_0    0x00u
#define EDID_MAGIC_7    0xFFu

typedef struct {
    uint8_t  raw[EDID_SIZE*2];  /**< EDID block 0 + optional ext     */
    char     manufacturer[4];
    uint16_t product_code;
    uint32_t serial;
    uint8_t  max_h_cm;
    uint8_t  max_v_cm;
    uint16_t native_w;
    uint16_t native_h;
    bool     valid;
} uiox_edid_t;

typedef struct {
    uiox_i2c_dev_t *ddc;
    uiox_edid_t     edid;
    bool            connected;
    bool            initialized;
} uiox_monitor_dev_t;

uiox_fw_err_t uiox_fw_monitor_init      (uiox_monitor_dev_t *dev, uiox_i2c_dev_t *ddc);
void          uiox_fw_monitor_deinit    (uiox_monitor_dev_t *dev);
uiox_fw_err_t uiox_fw_monitor_read_edid (uiox_monitor_dev_t *dev);
void          uiox_fw_monitor_parse_edid(uiox_monitor_dev_t *dev);
bool          uiox_fw_monitor_connected (const uiox_monitor_dev_t *dev);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_MONITOR_H */
