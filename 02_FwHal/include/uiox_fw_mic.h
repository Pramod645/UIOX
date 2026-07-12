/* ─────────────────────────────────────────────────────────────────────────
 * uiox_fw_mic.h — MEMS microphone (PDM/I2S) HAL
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef UIOX_FW_MIC_H
#define UIOX_FW_MIC_H
#include "uiox_fw_types.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { UIOX_MIC_PDM=0, UIOX_MIC_I2S=1 } uiox_mic_type_t;

typedef struct {
    uintptr_t       base;
    uiox_mic_type_t type;
    uint32_t        sample_rate_hz;
    uint8_t         channels;
    uint8_t         bits_per_sample;
    bool            initialized;
    void           *priv;
} uiox_mic_dev_t;

typedef struct {
    uiox_fw_err_t (*init)     (uiox_mic_dev_t *dev,
                                 uint32_t rate, uint8_t ch, uint8_t bits);
    void          (*deinit)   (uiox_mic_dev_t *dev);
    uiox_fw_err_t (*start)    (uiox_mic_dev_t *dev);
    void          (*stop)     (uiox_mic_dev_t *dev);
    uiox_fw_err_t (*read)     (uiox_mic_dev_t *dev,
                                 int16_t *buf, uint32_t frames);
} uiox_mic_ops_t;

uiox_fw_err_t uiox_fw_mic_init    (uiox_mic_dev_t *dev, const uiox_mic_ops_t *ops,
                                     uint32_t rate, uint8_t ch, uint8_t bits);
void          uiox_fw_mic_deinit  (uiox_mic_dev_t *dev);
uiox_fw_err_t uiox_fw_mic_start   (uiox_mic_dev_t *dev);
void          uiox_fw_mic_stop    (uiox_mic_dev_t *dev);
uiox_fw_err_t uiox_fw_mic_read    (uiox_mic_dev_t *dev, int16_t *buf, uint32_t frames);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_MIC_H */
