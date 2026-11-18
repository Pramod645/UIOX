/*
 * 50_UIX/00_libs/00_uixlibs/dev/fan/uiox_dev_fan.h
 *
 * FAN — the application's view of the fan controller, at EL0.
 *
 * ── FAMILY 4 (REG) ──────────────────────────────────────────────────
 * Four commands:
 *   GET_TELEM  0x01  uiox_fan_telem_t
 *   GET_RPM    0x02  uint16_t
 *   SET_PWM    0x03  uiox_fan_pwm_t
 *   GET_STATS  0x04  uiox_fan_if_stats_t
 *
 * GET_TELEM carries everything — fault flags, every fan's RPM and PWM,
 * every sensor's temperature — so get_pct(), get_temp() and stalled()
 * are derived from it rather than looking for commands that don't exist.
 *
 * add_fan() and add_zone() stay kernel-side: they take uiox_fan_ch_t and
 * uiox_fan_zone_t with a nested trips array, and no command carries those.
 *
 * v1.0.0  @date 2026-11-18
 */
#ifndef UIOX_DEV_FAN_H
#define UIOX_DEV_FAN_H

#include "../uiox_dev_reg.h"

/* Mirrored from 30_KIX/30_DSS/20_DD/03_NonSensors/fan/uiox_fan_if.h:133-136. */
#define UIOX_DEV_FAN_IOC_GET_TELEM 0x01u
#define UIOX_DEV_FAN_IOC_GET_RPM   0x02u
#define UIOX_DEV_FAN_IOC_SET_PWM   0x03u
#define UIOX_DEV_FAN_IOC_GET_STATS 0x04u

#define UIOX_DEV_FAN_DEVPATH  "fan"

/* Mirrored from 30_KIX/30_DSS/10_DI/03_NonSensors/fan/uiox_fan_hw.h:79-82.
 * GET_TELEM fills both arrays whole, so neither may be smaller. */
#define UIOX_DEV_FAN_MAX_CHANNELS     5u
#define UIOX_DEV_FAN_MAX_TEMP_SENSORS 4u
#define UIOX_DEV_FAN_PWM_MAX          255u

/* The kernel's uiox_fan_telem_t (uiox_fan_if.h:98-104), mirrored.
 * rpm[] is uint16_t, pwm[] uint8_t, temp_dc[] int16_t SIGNED — a
 * sub-zero sensor reads negative, so the copy widens through int. */
typedef struct {
    unsigned int   ts_ms;
    unsigned int   fault_flags;
    unsigned short rpm[UIOX_DEV_FAN_MAX_CHANNELS];
    unsigned char  pwm[UIOX_DEV_FAN_MAX_CHANNELS];
    int            temp_dc[UIOX_DEV_FAN_MAX_TEMP_SENSORS];
} uiox_dev_fan_telem_t;

/* The kernel's uiox_fan_pwm_t (uiox_fan_if.h:138), mirrored:
 * {uint8_t fan_id, duty; uint32_t now_ms} — THREE fields, and note the
 * timestamp is part of the command rather than added by the driver. */
typedef struct {
    unsigned char  fan_id;
    unsigned char  duty;
    unsigned int   now_ms;
} uiox_dev_fan_pwm_t;

typedef struct {
    uiox_dev_handle_t h;
    unsigned int      num_fans;
} uiox_dev_fan_t;

uiox_dev_err_t uiox_dev_fan_open (uiox_dev_fan_t *f, int unit, int flags);
uiox_dev_err_t uiox_dev_fan_close(uiox_dev_fan_t *f);

uiox_dev_err_t uiox_dev_fan_get_telemetry(uiox_dev_fan_t *f,
                                          uiox_dev_fan_telem_t *out);

/* GET_RPM — the one value with its own command.  Reads fan 0. */
uiox_dev_err_t uiox_dev_fan_get_rpm(uiox_dev_fan_t *f, unsigned int *rpm);

/* SET_PWM — the single write.  duty is 0..255. */
uiox_dev_err_t uiox_dev_fan_set_pwm(uiox_dev_fan_t *f, unsigned int fan_id,
                                    unsigned int duty, unsigned int now_ms);

/* Derived from the telemetry — no command per value. */
uiox_dev_err_t uiox_dev_fan_get_pct (uiox_dev_fan_t *f, unsigned int fan_id,
                                     unsigned int *pct);
uiox_dev_err_t uiox_dev_fan_get_temp(uiox_dev_fan_t *f, unsigned int sensor_id,
                                     int *temp_dc);

uiox_dev_err_t uiox_dev_fan_get_stats(uiox_dev_fan_t *f, void *out);

#endif
