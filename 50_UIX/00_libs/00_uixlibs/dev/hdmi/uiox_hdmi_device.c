/*
 * 50_UIX/00_libs/00_uixlibs/dev/hdmi/uiox_hdmi_device.c
 *
 * HDMI on the STREAM shape — control only.
 *
 * ── the mode goes at open, the DPMS timeout after ───────────────────
 * Rows 421-424 carry state, dpms, activity and mode.  The mode arrives
 * once, at open, and the DPMS timeout also has a row of its own because
 * it changes while running.
 */
#include "uiox_hdmi_device.h"
#include "uix_string.h"          /* uix_memset */

/* ── lifecycle ─────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_hdmi_open(uiox_hdmi_device_t *dev,
                              const uiox_hdmi_open_params_t *p,
                              int unit, int flags)
{
    uix_uint64_t packed;

    if (!dev) return UIOX_DEV_EBADNAME;

    uix_memset(dev, 0, sizeof(*dev));

    /* "hdmi0" — devparse.c declares the class; open_name splits the
     * trailing decimal so ("hdmi", 0) and "hdmi0" reach one node. */
    if (uiox_dev_stream_open_name(&dev->h, "hdmi0", flags) != UIOX_DEV_OK)
        return UIOX_DEV_ENODEV;

    /* the mode first: row 424's packing (w | h<<16 | hz<<32) */
    if (p) {
        if (!p->pref_w || !p->pref_h || !p->pref_hz) {
            uiox_dev_stream_close(&dev->h);
            return UIOX_DEV_EIO;
        }
        packed = (uix_uint64_t)p->pref_w
               | ((uix_uint64_t)p->pref_h  << 16)
               | ((uix_uint64_t)p->pref_hz << 32);

        if (uiox_dev_stream_write(&dev->h, &packed, sizeof(packed)) < 0) {
            uiox_dev_stream_close(&dev->h);
            return UIOX_DEV_EIO;
        }

        /* then the DPMS policy, if the caller stated one */
        if (p->dpms_timeout_ms) {
            if (uiox_dev_stream_write(&dev->h, &p->dpms_timeout_ms,
                                      sizeof(p->dpms_timeout_ms)) < 0) {
                uiox_dev_stream_close(&dev->h);
                return UIOX_DEV_EIO;
            }
        }
    }

    (void)unit;
    dev->last_state = UIOX_HDMI_SUBSYS_STOPPED;
    dev->open       = UIX_TRUE;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_hdmi_close(uiox_hdmi_device_t *dev)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;
    dev->open = UIX_FALSE;
    return uiox_dev_stream_close(&dev->h);
}

/* ── the verbs ─────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_hdmi_activity(uiox_hdmi_device_t *dev, uix_uint32_t now_ms)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;

    if (uiox_dev_stream_write(&dev->h, &now_ms, sizeof(now_ms)) < 0)
        return UIOX_DEV_EIO;

    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_hdmi_set_dpms(uiox_hdmi_device_t *dev, uix_uint32_t timeout_ms)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;

    if (uiox_dev_stream_write(&dev->h, &timeout_ms, sizeof(timeout_ms)) < 0)
        return UIOX_DEV_EIO;

    return UIOX_DEV_OK;
}

uiox_hdmi_subsys_state_t uiox_hdmi_state(const uiox_hdmi_device_t *dev)
{
    if (!dev || !dev->open) return UIOX_HDMI_SUBSYS_STOPPED;
    return dev->last_state;
}

/* ── name helpers, unchanged from the kernel-side file ─────────────── */

const char *uiox_hdmi_state_name(uiox_hdmi_subsys_state_t s)
{
    switch (s) {
    case UIOX_HDMI_SUBSYS_STOPPED:   return "STOPPED";
    case UIOX_HDMI_SUBSYS_PROBING:   return "PROBING";
    case UIOX_HDMI_SUBSYS_RUNNING:   return "RUNNING";
    case UIOX_HDMI_SUBSYS_SUSPENDED: return "SUSPENDED";
    default:                          return "UNKNOWN";
    }
}

const char *uiox_hdmi_evt_name(uiox_hdmi_evt_t evt)
{
    switch (evt) {
    case UIOX_HDMI_EVT_CONNECT:        return "CONNECT";
    case UIOX_HDMI_EVT_DISCONNECT:     return "DISCONNECT";
    case UIOX_HDMI_EVT_HDCP_AUTH:      return "HDCP_AUTH";
    case UIOX_HDMI_EVT_HDCP_FAIL:      return "HDCP_FAIL";
    case UIOX_HDMI_EVT_VBLANK:         return "VBLANK";
    case UIOX_HDMI_EVT_AUDIO_UNDERRUN: return "AUDIO_UNDERRUN";
    case UIOX_HDMI_EVT_ERROR:          return "ERROR";
    default:                            return "UNKNOWN";
    }
}
