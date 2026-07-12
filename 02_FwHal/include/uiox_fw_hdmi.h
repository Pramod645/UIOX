
/* ─────────────────────────────────────────────────────────────────────────
 * uiox_fw_hdmi.h — HDMI transmitter HAL (DesignWare HDMI)
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef UIOX_FW_HDMI_H
#define UIOX_FW_HDMI_H
#include "uiox_fw_types.h"
#include "uiox_fw_i2c.h"
#ifdef __cplusplus
extern "C" {
#endif

#define DW_HDMI_BASE_ARM64      0x09070000u
#define DW_HDMI_REG_PHY_CONF0   0x3000u
#define DW_HDMI_REG_MC_CLKDIS   0x4001u
#define HDMI_DDC_ADDR           0x50u   /**< EDID EEPROM via DDC/I2C  */

typedef struct { uint32_t pixel_clk_hz; uint16_t h,v,hfp,hbp,vfp,vbp; } uiox_hdmi_mode_t;

typedef struct {
    uintptr_t       base;
    uiox_i2c_dev_t *ddc_i2c;   /**< DDC channel for EDID            */
    uint8_t         edid[256];
    bool            connected;
    uiox_hdmi_mode_t active_mode;
    bool            initialized;
    void           *priv;
} uiox_hdmi_dev_t;

typedef struct {
    uiox_fw_err_t (*init)       (uiox_hdmi_dev_t *dev);
    void          (*deinit)     (uiox_hdmi_dev_t *dev);
    uiox_fw_err_t (*read_edid)  (uiox_hdmi_dev_t *dev, uint8_t *buf);
    uiox_fw_err_t (*set_mode)   (uiox_hdmi_dev_t *dev,
                                   const uiox_hdmi_mode_t *mode);
    bool          (*hpd_status) (uiox_hdmi_dev_t *dev);
    void          (*isr)        (uiox_hdmi_dev_t *dev);
} uiox_hdmi_ops_t;

uiox_fw_err_t uiox_fw_hdmi_init      (uiox_hdmi_dev_t *dev, const uiox_hdmi_ops_t *ops);
void          uiox_fw_hdmi_deinit    (uiox_hdmi_dev_t *dev);
uiox_fw_err_t uiox_fw_hdmi_read_edid (uiox_hdmi_dev_t *dev, uint8_t *buf);
uiox_fw_err_t uiox_fw_hdmi_set_mode  (uiox_hdmi_dev_t *dev,
                                        const uiox_hdmi_mode_t *mode);
bool          uiox_fw_hdmi_hpd       (uiox_hdmi_dev_t *dev);
uiox_fw_err_t uiox_fw_hdmi_init_dw   (uiox_hdmi_dev_t *dev,
                                        uintptr_t base, uiox_i2c_dev_t *ddc);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_HDMI_H */
