/*
 * 50_UIX/00_libs/00_uixlibs/dev/fan/uiox_dev_fan.c
 */
#include "uiox_dev_fan.h"

/* The kernel telem struct is byte-for-byte the layout in the header,
 * so no staging copy is needed — the buffer is passed straight through.
 * The one trap is temp_dc[]: int16_t SIGNED, widened through int on
 * read so a negative sensor reading stays negative. */

uiox_dev_err_t uiox_dev_fan_open(uiox_dev_fan_t *f, int unit, int flags)
{
    if (!f) return UIOX_DEV_EBADNAME;
    (void)unit;
    f->num_fans = 0u;
    return uiox_dev_reg_open(&f->h, UIOX_DEV_FAN_DEVPATH, flags);
}

uiox_dev_err_t uiox_dev_fan_close(uiox_dev_fan_t *f)
{
    if (!f) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_close(&f->h);
}

uiox_dev_err_t uiox_dev_fan_get_telemetry(uiox_dev_fan_t *f,
                                          uiox_dev_fan_telem_t *out)
{
    unsigned int i, n = 0u;
    if (!f || !out) return UIOX_DEV_EBADNAME;

    if (uiox_dev_reg_get(&f->h, UIOX_DEV_FAN_IOC_GET_TELEM, out) != 0)
        return UIOX_DEV_EIO;

    /* FANS are "channels with a non-zero rpm or pwm".  The struct has no
     * count field, so the number is inferred — the same choice BMS made
     * for its cell array, and worth revisiting if the driver ever adds
     * one. */
    for (i = 0u; i < UIOX_DEV_FAN_MAX_CHANNELS; i++)
        if (out->rpm[i] != 0u || out->pwm[i] != 0u) n = i + 1u;
    f->num_fans = n;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_fan_get_rpm(uiox_dev_fan_t *f, unsigned int *rpm)
{
    unsigned short v = 0u;
    if (!f || !rpm) return UIOX_DEV_EBADNAME;
    if (uiox_dev_reg_get(&f->h, UIOX_DEV_FAN_IOC_GET_RPM, &v) != 0)
        return UIOX_DEV_EIO;
    *rpm = (unsigned int)v;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_fan_set_pwm(uiox_dev_fan_t *f, unsigned int fan_id,
                                    unsigned int duty, unsigned int now_ms)
{
    uiox_dev_fan_pwm_t p;
    if (!f) return UIOX_DEV_EBADNAME;
    /* Both are single bytes in the kernel struct — a wider argument
     * would be silently truncated, so it is refused here. */
    if (fan_id >= UIOX_DEV_FAN_MAX_CHANNELS) return UIOX_DEV_EBADNAME;
    if (duty > UIOX_DEV_FAN_PWM_MAX)         return UIOX_DEV_EBADNAME;
    p.fan_id = (unsigned char)fan_id;
    p.duty   = (unsigned char)duty;
    p.now_ms = now_ms;
    return uiox_dev_reg_set(&f->h, UIOX_DEV_FAN_IOC_SET_PWM, &p)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_fan_get_pct(uiox_dev_fan_t *f, unsigned int fan_id,
                                    unsigned int *pct)
{
    uiox_dev_fan_telem_t t;
    if (!f || !pct) return UIOX_DEV_EBADNAME;
    if (fan_id >= UIOX_DEV_FAN_MAX_CHANNELS) return UIOX_DEV_EBADNAME;
    if (uiox_dev_fan_get_telemetry(f, &t) != UIOX_DEV_OK) return UIOX_DEV_EIO;
    /* duty is 0..255; percent is the driver's own expression, kept
     * identical so a userspace reading matches an in-kernel one. */
    *pct = (unsigned int)((unsigned int)t.pwm[fan_id] * 100u)
           / UIOX_DEV_FAN_PWM_MAX;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_fan_get_temp(uiox_dev_fan_t *f, unsigned int sensor_id,
                                     int *temp_dc)
{
    uiox_dev_fan_telem_t t;
    if (!f || !temp_dc) return UIOX_DEV_EBADNAME;
    if (sensor_id >= UIOX_DEV_FAN_MAX_TEMP_SENSORS) return UIOX_DEV_EBADNAME;
    if (uiox_dev_fan_get_telemetry(f, &t) != UIOX_DEV_OK) return UIOX_DEV_EIO;
    *temp_dc = t.temp_dc[sensor_id];      /* already widened to int */
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_fan_get_stats(uiox_dev_fan_t *f, void *out)
{
    if (!f || !out) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_get(&f->h, UIOX_DEV_FAN_IOC_GET_STATS, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}
