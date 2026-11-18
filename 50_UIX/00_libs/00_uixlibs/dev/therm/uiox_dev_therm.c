/*
 * 50_UIX/00_libs/00_uixlibs/dev/therm/uiox_dev_therm.c
 *
 * THERM on the REG shape.  Reaches the kernel only through
 * uiox_dev_reg_get / _set — no 30_KIX header, no kernel types.
 */
#include "uiox_dev_therm.h"

/* ── lifecycle ────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_dev_therm_open(uiox_dev_therm_t *t, int unit, int flags)
{
    uiox_dev_err_t rc;
    if (!t) return UIOX_DEV_EBADNAME;
    (void)unit;                   /* reg_open is unit-0 only */
    rc = uiox_dev_reg_open(&t->h, UIOX_DEV_THERM_DEVPATH, flags);
    if (rc != UIOX_DEV_OK) return rc;
    t->num_channels = 0u;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_therm_close(uiox_dev_therm_t *t)
{
    if (!t) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_close(&t->h);
}

/* ── the snapshot ─────────────────────────────────────────────────────
 * The staging buffer is sized to the driver's own cap so the copy never
 * overruns.  A calloc-free, freestanding layer cannot allocate, so the
 * buffer is a local of a fixed size — which is why the two MAX_CH
 * constants have to agree. */

struct therm_telem_kernel {
    unsigned int  ts_ms;
    unsigned char num_channels;
    short         temp_dc[UIOX_DEV_THERM_MAX_CH];
    unsigned char alert[UIOX_DEV_THERM_MAX_CH];
};

uiox_dev_err_t uiox_dev_therm_snapshot(uiox_dev_therm_t *t,
                                       uiox_dev_therm_telem_t *out)
{
    struct therm_telem_kernel k;
    unsigned int i, n;

    if (!t || !out) return UIOX_DEV_EBADNAME;

    for (i = 0u; i < sizeof k; i++) ((unsigned char *)&k)[i] = 0u;
    if (uiox_dev_reg_get(&t->h, UIOX_DEV_THERM_IOC_GET_TELEM, &k) != 0)
        return UIOX_DEV_EIO;

    n = (unsigned int)k.num_channels;
    if (n > UIOX_DEV_THERM_MAX_CH) n = UIOX_DEV_THERM_MAX_CH;

    out->ts_ms        = k.ts_ms;
    out->num_channels = n;
    for (i = 0u; i < UIOX_DEV_THERM_MAX_CH; i++) {
        out->temp_dc[i] = (i < n) ? k.temp_dc[i] : 0;
        out->alert[i]   = (i < n) ? (k.alert[i] ? 1 : 0) : 0;
    }
    t->num_channels = n;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_therm_read_ch(uiox_dev_therm_t *t,
                                      unsigned int ch, short *temp_dc)
{
    uiox_dev_therm_telem_t s;
    if (!t || !temp_dc) return UIOX_DEV_EBADNAME;
    if (uiox_dev_therm_snapshot(t, &s) != UIOX_DEV_OK) return UIOX_DEV_EIO;
    if (ch >= s.num_channels) return UIOX_DEV_EBADNAME;
    *temp_dc = s.temp_dc[ch];
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_therm_alert_active(uiox_dev_therm_t *t,
                                           unsigned int ch, int *active)
{
    uiox_dev_therm_telem_t s;
    if (!t || !active) return UIOX_DEV_EBADNAME;
    if (uiox_dev_therm_snapshot(t, &s) != UIOX_DEV_OK) return UIOX_DEV_EIO;
    if (ch >= s.num_channels) return UIOX_DEV_EBADNAME;
    *active = s.alert[ch];
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_therm_set_alert(uiox_dev_therm_t *t,
                                        short t_high_dc, short t_hyst_dc)
{
    uiox_dev_therm_alert_t a;
    if (!t) return UIOX_DEV_EBADNAME;
    if (t_hyst_dc >= t_high_dc) return UIOX_DEV_EBADNAME;
    a.t_high_dc = t_high_dc;
    a.t_hyst_dc = t_hyst_dc;
    return uiox_dev_reg_set(&t->h, UIOX_DEV_THERM_IOC_SET_ALERT, &a)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_therm_get_stats(uiox_dev_therm_t *t, void *out)
{
    if (!t || !out) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_get(&t->h, UIOX_DEV_THERM_IOC_GET_STATS, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}
