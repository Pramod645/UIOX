/*
 * 50_UIX/00_libs/00_uixlibs/dev/monitor/uiox_mon_device.c
 *
 * Monitor on the STREAM shape — control only.
 *
 * ── three writes at open, then one each ─────────────────────────────
 * The mode, the DPMS timeout and the frame rate are policy, so all
 * three go at open.  dpms and fps also have rows of their own (426,
 * 427) because both change while running.
 */
#include "uiox_mon_device.h"
#include "uix_string.h"          /* uix_memset */

/* ── lifecycle ─────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_mon_open(uiox_mon_device_t *dev,
                             const uiox_mon_open_params_t *p,
                             int unit, int flags)
{
    uix_uint64_t packed;

    if (!dev) return UIOX_DEV_EBADNAME;

    uix_memset(dev, 0, sizeof(*dev));

    /* "mon0" — open_name splits the trailing decimal, so ("mon", 0)
     * and "mon0" reach the same node. */
    if (uiox_dev_stream_open_name(&dev->h, "mon0", flags) != UIOX_DEV_OK)
        return UIOX_DEV_ENODEV;

    if (p) {
        if (!p->pref_w || !p->pref_h || !p->pref_hz) {
            uiox_dev_stream_close(&dev->h);
            return UIOX_DEV_EIO;
        }

        /* the mode, with HDMI's packing — w | h<<16 | hz<<32 */
        packed = (uix_uint64_t)p->pref_w
               | ((uix_uint64_t)p->pref_h  << 16)
               | ((uix_uint64_t)p->pref_hz << 32);

        if (uiox_dev_stream_write(&dev->h, &packed, sizeof(packed)) < 0) {
            uiox_dev_stream_close(&dev->h);
            return UIOX_DEV_EIO;
        }

        /* 0 is a stated policy for both — uiox_mon_subsys.h:64 and :67
         * — so each is passed through rather than refused */
        if (uiox_dev_stream_write(&dev->h, &p->dpms_timeout_ms,
                                  sizeof(p->dpms_timeout_ms)) < 0) {
            uiox_dev_stream_close(&dev->h);
            return UIOX_DEV_EIO;
        }

        if (uiox_dev_stream_write(&dev->h, &p->target_fps,
                                  sizeof(p->target_fps)) < 0) {
            uiox_dev_stream_close(&dev->h);
            return UIOX_DEV_EIO;
        }
    }

    (void)unit;
    dev->last_state = UIOX_MON_SUBSYS_STOPPED;
    dev->open       = UIX_TRUE;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_mon_close(uiox_mon_device_t *dev)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;
    dev->open = UIX_FALSE;
    return uiox_dev_stream_close(&dev->h);
}

/* ── the verbs ─────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_mon_set_dpms(uiox_mon_device_t *dev, uix_uint32_t timeout_ms)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;

    if (uiox_dev_stream_write(&dev->h, &timeout_ms, sizeof(timeout_ms)) < 0)
        return UIOX_DEV_EIO;

    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_mon_set_fps(uiox_mon_device_t *dev, uix_uint32_t fps)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;

    if (uiox_dev_stream_write(&dev->h, &fps, sizeof(fps)) < 0)
        return UIOX_DEV_EIO;

    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_mon_activity(uiox_mon_device_t *dev, uix_uint32_t now_ms)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;

    if (uiox_dev_stream_write(&dev->h, &now_ms, sizeof(now_ms)) < 0)
        return UIOX_DEV_EIO;

    return UIOX_DEV_OK;
}

uiox_mon_subsys_state_t uiox_mon_state(const uiox_mon_device_t *dev)
{
    if (!dev || !dev->open) return UIOX_MON_SUBSYS_STOPPED;
    return dev->last_state;
}

/* ── name helpers, unchanged from the kernel-side file ─────────────── */

const char *uiox_mon_state_name(uiox_mon_subsys_state_t s)
{
    switch (s) {
    case UIOX_MON_SUBSYS_STOPPED:     return "STOPPED";
    case UIOX_MON_SUBSYS_RUNNING:     return "RUNNING";
    case UIOX_MON_SUBSYS_DPMS_SAVING: return "DPMS_SAVING";
    default:                           return "UNKNOWN";
    }
}

const char *uiox_mon_dpms_name(uiox_mon_dpms_t dpms)
{
    switch (dpms) {
    case UIOX_MON_DPMS_ON:      return "ON";
    case UIOX_MON_DPMS_STANDBY: return "STANDBY";
    case UIOX_MON_DPMS_SUSPEND: return "SUSPEND";
    case UIOX_MON_DPMS_OFF:     return "OFF";
    default:                     return "UNKNOWN";
    }
}
