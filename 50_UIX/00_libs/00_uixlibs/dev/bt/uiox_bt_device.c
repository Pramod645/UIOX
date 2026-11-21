/*
 * 50_UIX/00_libs/00_uixlibs/dev/bt/uiox_bt_device.c
 *
 * BT on the STREAM shape.  Same three rules as dev/WiFi:
 *
 *   1. There is no uiox_dev_stream_t — the shape takes uiox_dev_handle_t *,
 *      so this layer holds one and passes &dev->h.
 *   2. open/close delegate to uiox_dev_stream_open_name/_close, which own
 *      the sys_open/sys_close — so this file includes no syscall header.
 *   3. No ioctl arm.  BT's rows (405-408) are reached by the kernel's own
 *      dispatch, not by anything here.
 */
#include "uiox_bt_device.h"
#include "uix_string.h"          /* uix_memset */

/* ── lifecycle ─────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_bt_open(uiox_bt_device_t *dev, int unit, int flags)
{
    if (!dev) return UIOX_DEV_EBADNAME;

    uix_memset(dev, 0, sizeof(*dev));

    /* "bt0" is devparse.c's spelling — { "bt", UIOX_DEVCLASS_BT }.
     * open_name splits the trailing decimal so ("bt", 0) and "bt0"
     * reach the same node. */
    if (uiox_dev_stream_open_name(&dev->h, "bt0", flags) != UIOX_DEV_OK)
        return UIOX_DEV_ENODEV;

    (void)unit;
    dev->last_state = UIOX_BT_SUBSYS_OFF;
    dev->open       = UIX_TRUE;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_bt_close(uiox_bt_device_t *dev)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;
    dev->open = UIX_FALSE;
    return uiox_dev_stream_close(&dev->h);
}

/* ── the four verbs with kernel rows ───────────────────────────────── */

uiox_dev_err_t uiox_bt_scan(uiox_bt_device_t *dev, uix_uint32_t dur_s)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;
    if (!dur_s)             return UIOX_DEV_EIO;

    if (uiox_dev_stream_write(&dev->h, &dur_s, sizeof(dur_s)) < 0)
        return UIOX_DEV_EIO;

    dev->last_state = UIOX_BT_SUBSYS_SCANNING;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_bt_scan_stop(uiox_bt_device_t *dev)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;

    dev->last_state = UIOX_BT_SUBSYS_IDLE;
    return UIOX_DEV_OK;             /* row 406 takes no argument */
}

uiox_dev_err_t uiox_bt_disconnect(uiox_bt_device_t *dev, uix_uint16_t handle)
{
    if (!dev || !dev->open) return UIOX_DEV_ENODEV;

    if (uiox_dev_stream_write(&dev->h, &handle, sizeof(handle)) < 0)
        return UIOX_DEV_EIO;

    dev->last_state = UIOX_BT_SUBSYS_IDLE;
    return UIOX_DEV_OK;
}

uiox_bt_subsys_state_t uiox_bt_state(const uiox_bt_device_t *dev)
{
    if (!dev || !dev->open) return UIOX_BT_SUBSYS_OFF;
    return dev->last_state;
}

/* ── name helpers, unchanged from the kernel-side file ─────────────── */

const char *uiox_bt_state_name(uiox_bt_subsys_state_t s)
{
    switch (s) {
    case UIOX_BT_SUBSYS_OFF:         return "OFF";
    case UIOX_BT_SUBSYS_IDLE:        return "IDLE";
    case UIOX_BT_SUBSYS_SCANNING:    return "SCANNING";
    case UIOX_BT_SUBSYS_ADVERTISING: return "ADVERTISING";
    case UIOX_BT_SUBSYS_CONNECTING:  return "CONNECTING";
    case UIOX_BT_SUBSYS_CONNECTED:   return "CONNECTED";
    case UIOX_BT_SUBSYS_ERROR:       return "ERROR";
    default:                          return "UNKNOWN";
    }
}

const char *uiox_bt_ev_name(uiox_bt_ev_t ev)
{
    switch (ev) {
    case UIOX_BT_EV_POWERED_ON:   return "POWERED_ON";
    case UIOX_BT_EV_POWERED_OFF:  return "POWERED_OFF";
    case UIOX_BT_EV_SCAN_STARTED: return "SCAN_STARTED";
    case UIOX_BT_EV_SCAN_STOPPED: return "SCAN_STOPPED";
    case UIOX_BT_EV_DEVICE_FOUND: return "DEVICE_FOUND";
    case UIOX_BT_EV_CONNECTED:    return "CONNECTED";
    case UIOX_BT_EV_DISCONNECTED: return "DISCONNECTED";
    case UIOX_BT_EV_PAIR_REQ:     return "PAIR_REQ";
    case UIOX_BT_EV_PAIRED:       return "PAIRED";
    case UIOX_BT_EV_PAIR_FAILED:  return "PAIR_FAILED";
    case UIOX_BT_EV_GATT_DATA:    return "GATT_DATA";
    case UIOX_BT_EV_ACL_DATA:     return "ACL_DATA";
    case UIOX_BT_EV_ERROR:        return "ERROR";
    default:                       return "UNKNOWN";
    }
}

const char *uiox_bt_ver_name(uiox_bt_ver_t v)
{
    switch (v) {
    case UIOX_BT_VER_BT40: return "BT 4.0";
    case UIOX_BT_VER_BT41: return "BT 4.1";
    case UIOX_BT_VER_BT42: return "BT 4.2";
    case UIOX_BT_VER_BT50: return "BT 5.0";
    case UIOX_BT_VER_BT51: return "BT 5.1";
    case UIOX_BT_VER_BT52: return "BT 5.2";
    case UIOX_BT_VER_BT53: return "BT 5.3";
    case UIOX_BT_VER_BT54: return "BT 5.4";
    default:                return "Unknown";
    }
}
