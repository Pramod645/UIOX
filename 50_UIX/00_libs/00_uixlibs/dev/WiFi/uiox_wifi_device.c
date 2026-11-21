/*
 * 50_UIX/00_libs/00_uixlibs/dev/WiFi/uiox_wifi_device.c
 *
 * WiFi on the STREAM shape.  Three things the shape imposes:
 *
 *   1. There is no uiox_dev_stream_t — the shape takes uiox_dev_handle_t *,
 *      so this layer holds one and passes &dev->h at every call.
 *   2. open/close delegate to uiox_dev_stream_open_name/_close, which own
 *      the sys_open/sys_close — so this file includes no syscall header.
 *   3. There is no ioctl arm.  A STREAM device carries no command
 *      numbers; WiFi's four kernel rows (400-403) are reached by the
 *      kernel's own dispatch, not by anything here.
 *
 * ── the wire form ───────────────────────────────────────────────────
 * Each verb writes its argument to the node and reads the answer back.
 * uiox_wifi_quality reads sizeof(uiox_wifi_quality_t) bytes, which is
 * the mirror of the kernel's row 402.  scan sends its timeout as a
 * uix_uint32_t payload — the form the node's stream arm expects.
 */
#include "uiox_wifi_device.h"
#include "uix_string.h"          /* uix_memset, uix_strlen */

/* ── lifecycle ─────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_wifi_open(uiox_wifi_device_t *dev, int unit, int flags)
{
    if (!dev) return UIOX_DEV_EBADNAME;

    uix_memset(dev, 0, sizeof(*dev));

    /* "wlan0" is devparse.c's spelling — { "wlan", UIOX_DEVCLASS_WIFI }.
     * open_name splits the trailing decimal and passes it as the unit,
     * so ("wlan", 0) and "wlan0" reach the same node. */
    if (uiox_dev_stream_open_name(&dev->h, "wlan0", flags) != UIOX_DEV_OK)
        return UIOX_DEV_ENODEV;

    (void)unit;
    dev->last_state = UIOX_WIFI_SUBSYS_STOPPED;
    dev->open       = UIX_TRUE;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_wifi_close(uiox_wifi_device_t *dev)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;
    dev->open = UIX_FALSE;
    return uiox_dev_stream_close(&dev->h);
}

/* ── the verbs ─────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_wifi_scan(uiox_wifi_device_t *dev, uix_uint32_t timeout_ms)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;
    if (!timeout_ms)        return UIOX_DEV_EIO;

    if (uiox_dev_stream_write(&dev->h, &timeout_ms, sizeof(timeout_ms)) < 0)
        return UIOX_DEV_EIO;

    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_wifi_disconnect(uiox_wifi_device_t *dev)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;

    dev->last_state = UIOX_WIFI_SUBSYS_STOPPED;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_wifi_quality(uiox_wifi_device_t *dev,
                                 uiox_wifi_quality_t *out)
{
    uix_ssize_t got;

    if (!dev || !dev->open || !out) return UIOX_DEV_ENODEV;

    got = uiox_dev_stream_read(&dev->h, out, sizeof(*out));
    if (got != (uix_ssize_t)sizeof(*out)) return UIOX_DEV_EIO;

    /* the state travels with the read, so a caller polling in a loop
     * does not have to ask the kernel a second question */
    if (out->link_quality > 0u)
        dev->last_state = UIOX_WIFI_SUBSYS_CONNECTED;

    return UIOX_DEV_OK;
}

uiox_wifi_subsys_state_t uiox_wifi_state(const uiox_wifi_device_t *dev)
{
    if (!dev || !dev->open) return UIOX_WIFI_SUBSYS_STOPPED;
    return dev->last_state;
}

uix_bool_t uiox_wifi_connected(const uiox_wifi_device_t *dev)
{
    return uiox_wifi_state(dev) == UIOX_WIFI_SUBSYS_CONNECTED
               ? UIX_TRUE : UIX_FALSE;
}

/* ── name helpers, unchanged from the kernel-side file ─────────────── */

const char *uiox_wifi_state_name(uiox_wifi_subsys_state_t s)
{
    switch (s) {
    case UIOX_WIFI_SUBSYS_STOPPED:      return "STOPPED";
    case UIOX_WIFI_SUBSYS_SCANNING:     return "SCANNING";
    case UIOX_WIFI_SUBSYS_CONNECTING:   return "CONNECTING";
    case UIOX_WIFI_SUBSYS_CONNECTED:    return "CONNECTED";
    case UIOX_WIFI_SUBSYS_RECONNECTING: return "RECONNECTING";
    default:                             return "UNKNOWN";
    }
}

const char *uiox_wifi_evt_name(uiox_wifi_evt_t evt)
{
    switch (evt) {
    case UIOX_WIFI_EVT_SCAN_DONE:    return "SCAN_DONE";
    case UIOX_WIFI_EVT_CONNECTED:    return "CONNECTED";
    case UIOX_WIFI_EVT_DISCONNECTED: return "DISCONNECTED";
    case UIOX_WIFI_EVT_AUTH_FAILED:  return "AUTH_FAILED";
    case UIOX_WIFI_EVT_ASSOC_FAILED: return "ASSOC_FAILED";
    case UIOX_WIFI_EVT_IP_ACQUIRED:  return "IP_ACQUIRED";
    case UIOX_WIFI_EVT_RSSI_LOW:     return "RSSI_LOW";
    default:                          return "UNKNOWN";
    }
}
