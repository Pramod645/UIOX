/* ─────────────────────────────────────────────────────────────────────────
 * uiox_fw_speaker.h — I2S audio DAC + speaker amplifier HAL
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef UIOX_FW_SPEAKER_H
#define UIOX_FW_SPEAKER_H
#include "uiox_fw_types.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { UIOX_SPK_I2S=0, UIOX_SPK_PWM=1 } uiox_spk_type_t;

typedef struct {
    uintptr_t      base;
    uiox_spk_type_t type;
    uint32_t       sample_rate_hz;
    uint8_t        channels;
    uint8_t        bits_per_sample;
    uint8_t        volume_pct;
    bool           muted;
    bool           initialized;
    void          *priv;
} uiox_spk_dev_t;

typedef struct {
    uiox_fw_err_t (*init)      (uiox_spk_dev_t *dev,
                                  uint32_t rate, uint8_t ch, uint8_t bits);
    void          (*deinit)    (uiox_spk_dev_t *dev);
    uiox_fw_err_t (*play)      (uiox_spk_dev_t *dev,
                                  const int16_t *buf, uint32_t frames);
    void          (*stop)      (uiox_spk_dev_t *dev);
    uiox_fw_err_t (*set_volume)(uiox_spk_dev_t *dev, uint8_t pct);
    void          (*mute)      (uiox_spk_dev_t *dev, bool en);
} uiox_spk_ops_t;

uiox_fw_err_t uiox_fw_spk_init       (uiox_spk_dev_t *dev,
                                        const uiox_spk_ops_t *ops,
                                        uint32_t rate, uint8_t ch, uint8_t bits);
void          uiox_fw_spk_deinit     (uiox_spk_dev_t *dev);
uiox_fw_err_t uiox_fw_spk_play       (uiox_spk_dev_t *dev,
                                        const int16_t *buf, uint32_t frames);
void          uiox_fw_spk_stop       (uiox_spk_dev_t *dev);
uiox_fw_err_t uiox_fw_spk_set_volume (uiox_spk_dev_t *dev, uint8_t pct);
void          uiox_fw_spk_mute       (uiox_spk_dev_t *dev, bool en);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_SPEAKER_H */
