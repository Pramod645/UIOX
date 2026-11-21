/*
 * 50_UIX/00_libs/00_uixlibs/dev/spk/uiox_spk_device.c
 *
 * SPK on the STREAM shape.  Same three rules as dev/WiFi, dev/bt and
 * dev/mic:
 *
 *   1. There is no uiox_dev_stream_t — the shape takes uiox_dev_handle_t *.
 *   2. open/close delegate to uiox_dev_stream_open_name/_close, which own
 *      the sys_open/sys_close — so no syscall header here.
 *   3. No ioctl arm.  SPK's rows (413-416) are reached by the kernel's
 *      own dispatch.
 *
 * ── write() counts PAIRS here and BYTES at the shape ────────────────
 * uiox_spk_subsys_write takes n_stereo pairs; uiox_dev_stream_write
 * takes a byte count.  So the request is n_stereo * 4 — two uix_int16_t
 * per pair — and the reply is divided back by the same factor.
 */
#include "uiox_spk_device.h"
#include "uix_string.h"          /* uix_memset */

/* ── lifecycle ─────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_spk_open(uiox_spk_device_t *dev, int unit, int flags)
{
    if (!dev) return UIOX_DEV_EBADNAME;

    uix_memset(dev, 0, sizeof(*dev));

    /* "spk0" — devparse.c declares { "spk", UIOX_DEVCLASS_SPEAKER }.
     * open_name splits the trailing decimal so ("spk", 0) and "spk0"
     * reach the same node. */
    if (uiox_dev_stream_open_name(&dev->h, "spk0", flags) != UIOX_DEV_OK)
        return UIOX_DEV_ENODEV;

    (void)unit;
    dev->last_state = UIOX_SPK_STATE_STOPPED;
    dev->open       = UIX_TRUE;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_spk_close(uiox_spk_device_t *dev)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;
    dev->open = UIX_FALSE;
    return uiox_dev_stream_close(&dev->h);
}

/* ── write: the verb that sends EL0 memory INTO the kernel ─────────── */

uix_ssize_t uiox_spk_write(uiox_spk_device_t *dev,
                           const uix_int16_t *samples,
                           uix_uint32_t n_stereo)
{
    uix_ssize_t sent;
    uix_size_t  bytes;

    if (!dev || !dev->open || !samples || !n_stereo)
        return (uix_ssize_t)-1;

    /* bound before the multiply, so the size arithmetic cannot overflow */
    if (n_stereo > UIOX_SPK_MAX_WRITE_PAIRS) return (uix_ssize_t)-1;

    bytes = (uix_size_t)n_stereo * 4u;   /* 2 x uix_int16_t per pair */

    sent = uiox_dev_stream_write(&dev->h, samples, bytes);
    if (sent <= 0) return sent;

    /* the shape returns bytes; this layer's callers count pairs */
    return sent / 4;
}

/* ── the scalar verbs ──────────────────────────────────────────────── */

uiox_dev_err_t uiox_spk_set_volume(uiox_spk_device_t *dev, uix_uint8_t vol_pct)
{
    if (!dev || !dev->open)  return UIOX_DEV_ENODEV;
    if (vol_pct > 100u)      return UIOX_DEV_EIO;   /* 0..100 % */

    if (uiox_dev_stream_write(&dev->h, &vol_pct, sizeof(vol_pct)) < 0)
        return UIOX_DEV_EIO;

    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_spk_set_mute(uiox_spk_device_t *dev, uix_bool_t mute)
{
    uix_uint8_t on = mute ? 1u : 0u;

    if (!dev || !dev->open) return UIOX_DEV_ENODEV;

    if (uiox_dev_stream_write(&dev->h, &on, sizeof(on)) < 0)
        return UIOX_DEV_EIO;

    return UIOX_DEV_OK;
}

uiox_spk_state_t uiox_spk_state(const uiox_spk_device_t *dev)
{
    if (!dev || !dev->open) return UIOX_SPK_STATE_STOPPED;
    return dev->last_state;
}

/* ── name helpers, unchanged from the kernel-side file ─────────────── */

const char *uiox_spk_state_name(uiox_spk_state_t s)
{
    switch (s) {
    case UIOX_SPK_STATE_STOPPED:  return "STOPPED";
    case UIOX_SPK_STATE_PLAYING:  return "PLAYING";
    case UIOX_SPK_STATE_PAUSED:   return "PAUSED";
    case UIOX_SPK_STATE_STOPPING: return "STOPPING";
    default:                       return "UNKNOWN";
    }
}

const char *uiox_spk_evt_name(uiox_spk_evt_t evt)
{
    switch (evt) {
    case UIOX_SPK_EVT_START:         return "START";
    case UIOX_SPK_EVT_STOP:          return "STOP";
    case UIOX_SPK_EVT_UNDERRUN:      return "UNDERRUN";
    case UIOX_SPK_EVT_STREAM_END:    return "STREAM_END";
    case UIOX_SPK_EVT_VOLUME_CHANGE: return "VOLUME_CHANGE";
    case UIOX_SPK_EVT_FAULT:         return "FAULT";
    default:                          return "UNKNOWN";
    }
}
