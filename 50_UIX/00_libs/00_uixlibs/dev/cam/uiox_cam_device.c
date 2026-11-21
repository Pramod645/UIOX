/*
 * 50_UIX/00_libs/00_uixlibs/dev/cam/uiox_cam_device.c
 *
 * CAMERA on the STREAM shape.  Same three rules as the other five:
 *
 *   1. There is no uiox_dev_stream_t — the shape takes uiox_dev_handle_t *.
 *   2. open/close delegate to uiox_dev_stream_open_name/_close, which own
 *      the sys_open/sys_close — so no syscall header here.
 *   3. No ioctl arm.  Camera's rows (417-420) are reached by the kernel's
 *      own dispatch.
 *
 * ── no size arithmetic anywhere ─────────────────────────────────────
 * Every argument is a scalar, so unlike mic (samples -> bytes) and spk
 * (pairs -> bytes) this layer passes values straight through — no
 * conversion, and no bound before a multiply.
 *
 * ── open() sends the mode ───────────────────────────────────────────
 * width/height/fps/pixfmt/stride are the app's choice, and rows 417-420
 * carry none of them, so open() writes them to the node as one
 * uiox_cam_open_params_t before the first stream.  Whether the kernel
 * accepts a mode change after probe is a kernel-side question: the
 * pipeline config call exists, but no row routes to it yet.
 */
#include "uiox_cam_device.h"
#include "uix_string.h"          /* uix_memset */

/* ── lifecycle ─────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_cam_open(uiox_cam_device_t *dev,
                             const uiox_cam_open_params_t *p,
                             int unit, int flags)
{
    if (!dev) return UIOX_DEV_EBADNAME;

    uix_memset(dev, 0, sizeof(*dev));

    /* "cam0" — devparse.c declares { "cam", UIOX_DEVCLASS_CAMERA }.
     * open_name splits the trailing decimal so ("cam", 0) and "cam0"
     * reach the same node. */
    if (uiox_dev_stream_open_name(&dev->h, "cam0", flags) != UIOX_DEV_OK)
        return UIOX_DEV_ENODEV;

    /* the mode, before the first stream.  A NULL params means "keep
     * whatever the kernel configured at probe" — legal, because the
     * probe has a mode of its own. */
    if (p) {
        uiox_cam_open_params_t mode = *p;

        if (mode.width == 0u || mode.height == 0u || mode.stride == 0u) {
            uiox_dev_stream_close(&dev->h);
            return UIOX_DEV_EIO;
        }

        if (uiox_dev_stream_write(&dev->h, &mode, sizeof(mode)) < 0) {
            uiox_dev_stream_close(&dev->h);
            return UIOX_DEV_EIO;
        }
    }

    (void)unit;
    dev->last_state = UIOX_CAM_STATE_STOPPED;
    dev->open       = UIX_TRUE;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_cam_close(uiox_cam_device_t *dev)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;
    dev->open = UIX_FALSE;
    return uiox_dev_stream_close(&dev->h);
}

/* ── the control verbs ─────────────────────────────────────────────── */

uiox_dev_err_t uiox_cam_set_exposure(uiox_cam_device_t *dev, uix_uint32_t us)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;

    if (uiox_dev_stream_write(&dev->h, &us, sizeof(us)) < 0)
        return UIOX_DEV_EIO;

    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_cam_set_gain(uiox_cam_device_t *dev, uix_uint16_t x100)
{
    if (!dev || !dev->open)       return UIOX_DEV_ENODEV;
    if (x100 > UIOX_CAM_GAIN_MAX) return UIOX_DEV_EIO;

    if (uiox_dev_stream_write(&dev->h, &x100, sizeof(x100)) < 0)
        return UIOX_DEV_EIO;

    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_cam_stream(uiox_cam_device_t *dev, uix_bool_t on)
{
    uix_uint8_t flag = on ? 1u : 0u;

    if (!dev || !dev->open) return UIOX_DEV_ENODEV;

    /* the kernel's row 420 branches: pipeline_start returns int,
     * pipeline_stop returns void — both reached by this one flag */
    if (uiox_dev_stream_write(&dev->h, &flag, sizeof(flag)) < 0)
        return UIOX_DEV_EIO;

    dev->last_state = on ? UIOX_CAM_STATE_STREAMING : UIOX_CAM_STATE_STOPPED;
    return UIOX_DEV_OK;
}

uiox_cam_state_t uiox_cam_state(const uiox_cam_device_t *dev)
{
    if (!dev || !dev->open) return UIOX_CAM_STATE_STOPPED;
    return dev->last_state;
}

/* ── name helpers ──────────────────────────────────────────────────── */

const char *uiox_cam_state_name(uiox_cam_state_t s)
{
    switch (s) {
    case UIOX_CAM_STATE_STOPPED:   return "STOPPED";
    case UIOX_CAM_STATE_STREAMING: return "STREAMING";
    default:                        return "UNKNOWN";
    }
}

/* the demo's pixfmt_name, kept — the app prints the format it asked for */
const char *uiox_cam_pixfmt_name(uiox_cam_pixfmt_t f)
{
    switch (f) {
    case UIOX_CAM_PIX_FMT_RAW8:   return "RAW8";
    case UIOX_CAM_PIX_FMT_RAW10:  return "RAW10";
    case UIOX_CAM_PIX_FMT_RAW12:  return "RAW12";
    case UIOX_CAM_PIX_FMT_YUV420: return "YUV420";
    case UIOX_CAM_PIX_FMT_YUV422: return "YUV422";
    case UIOX_CAM_PIX_FMT_RGB888: return "RGB888";
    default:                       return "UNKNOWN";
    }
}
