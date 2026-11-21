/*
 * 50_UIX/00_libs/00_uixlibs/dev/mic/uiox_mic_device.c
 *
 * MIC on the STREAM shape.  Same three rules as dev/WiFi and dev/bt:
 *
 *   1. There is no uiox_dev_stream_t — the shape takes uiox_dev_handle_t *,
 *      so this layer holds one and passes &dev->h.
 *   2. open/close delegate to uiox_dev_stream_open_name/_close, which own
 *      the sys_open/sys_close — so this file includes no syscall header.
 *   3. No ioctl arm.  MIC's rows (409-412) are reached by the kernel's
 *      own dispatch, not by anything here.
 *
 * ── read() counts SAMPLES at this end and BYTES at the shape ────────
 * uiox_dev_stream_read takes a byte count; uiox_mic_subsys_read returns
 * a sample count.  So the request is n_samples * sizeof(uix_int16_t) and
 * the reply is divided back by the same factor.
 */
#include "uiox_mic_device.h"
#include "uix_string.h"          /* uix_memset */

/* ── lifecycle ─────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_mic_open(uiox_mic_device_t *dev, int unit, int flags)
{
    if (!dev) return UIOX_DEV_EBADNAME;

    uix_memset(dev, 0, sizeof(*dev));

    /* "mic0" is devparse.c's spelling — { "mic", UIOX_DEVCLASS_MIC }.
     * open_name splits the trailing decimal so ("mic", 0) and "mic0"
     * reach the same node. */
    if (uiox_dev_stream_open_name(&dev->h, "mic0", flags) != UIOX_DEV_OK)
        return UIOX_DEV_ENODEV;

    (void)unit;
    dev->last_state = UIOX_MIC_STATE_STOPPED;
    dev->open       = UIX_TRUE;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_mic_close(uiox_mic_device_t *dev)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;
    dev->open = UIX_FALSE;
    return uiox_dev_stream_close(&dev->h);
}

/* ── read: the one verb that copies OUT ────────────────────────────── */

uix_ssize_t uiox_mic_read(uiox_mic_device_t *dev,
                          uix_int16_t *buf, uix_uint32_t n_samples)
{
    uix_ssize_t got;

    if (!dev || !dev->open || !buf || !n_samples)
        return (uix_ssize_t)-1;

    /* bound before the multiply, so the size arithmetic cannot overflow */
    if (n_samples > UIOX_MIC_MAX_READ_SAMPLES) return (uix_ssize_t)-1;

    got = uiox_dev_stream_read(&dev->h, buf,
                               (uix_size_t)n_samples * sizeof(uix_int16_t));
    if (got <= 0) return got;

    /* the shape returns bytes; this layer's callers count samples */
    return got / (uix_ssize_t)sizeof(uix_int16_t);
}

/* ── the scalar verbs ──────────────────────────────────────────────── */

uiox_dev_err_t uiox_mic_set_gain(uiox_mic_device_t *dev, uix_uint8_t gain_db)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;

    if (uiox_dev_stream_write(&dev->h, &gain_db, sizeof(gain_db)) < 0)
        return UIOX_DEV_EIO;

    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_mic_set_mute(uiox_mic_device_t *dev, uix_bool_t mute)
{
    uix_uint8_t on = mute ? 1u : 0u;

    if (!dev || !dev->open) return UIOX_DEV_ENODEV;

    if (uiox_dev_stream_write(&dev->h, &on, sizeof(on)) < 0)
        return UIOX_DEV_EIO;

    dev->last_state = mute ? UIOX_MIC_STATE_MUTED : UIOX_MIC_STATE_RUNNING;
    return UIOX_DEV_OK;
}

uiox_mic_state_t uiox_mic_state(const uiox_mic_device_t *dev)
{
    if (!dev || !dev->open) return UIOX_MIC_STATE_STOPPED;
    return dev->last_state;
}

/* ── name helpers, unchanged from the kernel-side file ─────────────── */

const char *uiox_mic_state_name(uiox_mic_state_t s)
{
    switch (s) {
    case UIOX_MIC_STATE_STOPPED: return "STOPPED";
    case UIOX_MIC_STATE_RUNNING: return "RUNNING";
    case UIOX_MIC_STATE_MUTED:   return "MUTED";
    case UIOX_MIC_STATE_ERROR:   return "ERROR";
    default:                      return "UNKNOWN";
    }
}

const char *uiox_mic_evt_name(uiox_mic_evt_t evt)
{
    switch (evt) {
    case UIOX_MIC_EVT_STARTED:     return "STARTED";
    case UIOX_MIC_EVT_STOPPED:     return "STOPPED";
    case UIOX_MIC_EVT_VOICE_START: return "VOICE_START";
    case UIOX_MIC_EVT_VOICE_END:   return "VOICE_END";
    case UIOX_MIC_EVT_OVERRUN:     return "OVERRUN";
    case UIOX_MIC_EVT_GAIN_CHANGE: return "GAIN_CHANGE";
    default:                        return "UNKNOWN";
    }
}
