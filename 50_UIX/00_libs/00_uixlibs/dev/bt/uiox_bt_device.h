/*
 * 50_UIX/00_libs/00_uixlibs/dev/bt/uiox_bt_device.h
 *
 * UIOX Bluetooth top-level application-facing device API — EL0.
 *
 *   21_apps/app_bt.c → dev/bt/uiox_bt_device.c
 *     → dev/uiox_dev_stream.c → 40_SCIX → 30_KIX
 *
 * No 30_KIX header.
 *
 * ── CHANGED from the kernel-side original ───────────────────────────
 *   · "uiox_bt_subsys.h" removed — it is EL1.  The enums and types it
 *     carried are MIRRORED here, source named beside each.
 *   · uiox_bt_open_params_t lost hw/hw_ops — an EL0 caller has no HAL.
 *   · uiox_bt_device_t holds a uiox_dev_handle_t, not a
 *     uiox_bt_subsys_t.
 *
 * ── WHICH KERNEL ROWS EXIST ─────────────────────────────────────────
 *   405  bt_scan_start(timeout)      this layer's scan()
 *   406  bt_scan_stop()              scan_stop()
 *   407  bt_disconnect(handle)       disconnect()
 *   408  bt_state()                  state()
 *
 * Everything else here is declared but has NO kernel row, and each is
 * named rather than silently omitted — see the API block below.
 */
#ifndef UIOX_BT_DEVICE_H
#define UIOX_BT_DEVICE_H

#include "../uiox_dev_stream.h"
#include "../uiox_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UIOX_BT_DEVPATH      "bt"
#define UIOX_BT_ADDR_LEN      6
#define UIOX_BT_NAME_MAX     248

typedef uix_uint8_t uiox_bt_addr_t[UIOX_BT_ADDR_LEN];

/* uiox_bt_subsys.h */
typedef enum {
    UIOX_BT_SUBSYS_OFF = 0,
    UIOX_BT_SUBSYS_IDLE,
    UIOX_BT_SUBSYS_SCANNING,
    UIOX_BT_SUBSYS_ADVERTISING,
    UIOX_BT_SUBSYS_CONNECTING,
    UIOX_BT_SUBSYS_CONNECTED,
    UIOX_BT_SUBSYS_ERROR,
} uiox_bt_subsys_state_t;

/* uiox_bt_subsys.h */
typedef enum {
    UIOX_BT_EV_POWERED_ON = 0, UIOX_BT_EV_POWERED_OFF,
    UIOX_BT_EV_SCAN_STARTED,   UIOX_BT_EV_SCAN_STOPPED,
    UIOX_BT_EV_DEVICE_FOUND,   UIOX_BT_EV_CONNECTED,
    UIOX_BT_EV_DISCONNECTED,   UIOX_BT_EV_PAIR_REQ,
    UIOX_BT_EV_PAIRED,         UIOX_BT_EV_PAIR_FAILED,
    UIOX_BT_EV_GATT_DATA,      UIOX_BT_EV_ACL_DATA,
    UIOX_BT_EV_ERROR,
} uiox_bt_ev_t;

typedef enum {
    UIOX_BT_VER_BT40 = 0, UIOX_BT_VER_BT41, UIOX_BT_VER_BT42,
    UIOX_BT_VER_BT50,     UIOX_BT_VER_BT51, UIOX_BT_VER_BT52,
    UIOX_BT_VER_BT53,     UIOX_BT_VER_BT54,
} uiox_bt_ver_t;

typedef void (*uiox_bt_evt_cb_t)(uiox_bt_ev_t ev, void *dev, void *ctx);

/* ── the device handle ───────────────────────────────────────────────
 * The shape's own handle.  No HAL pointer: the kernel owns the adapter
 * and /dev/bt0 is how it is reached. */
typedef struct {
    uiox_dev_handle_t        h;
    uiox_bt_subsys_state_t   last_state;
    uix_bool_t               open;
} uiox_bt_device_t;

/* ── the API ─────────────────────────────────────────────────────────
 * Four verbs have kernel rows (405-408) and are the ones an app can
 * actually use today:
 */
uiox_dev_err_t uiox_bt_open      (uiox_bt_device_t *dev, int unit, int flags);
uiox_dev_err_t uiox_bt_close     (uiox_bt_device_t *dev);

uiox_dev_err_t uiox_bt_scan      (uiox_bt_device_t *dev, uix_uint32_t dur_s);
uiox_dev_err_t uiox_bt_scan_stop (uiox_bt_device_t *dev);
uiox_dev_err_t uiox_bt_disconnect(uiox_bt_device_t *dev, uix_uint16_t handle);

uiox_bt_subsys_state_t uiox_bt_state(const uiox_bt_device_t *dev);

/* ── declared, but with NO kernel row yet ────────────────────────────
 *   connect(addr, is_ble)     uiox_bt_addr_t is a STRUCT — six register
 *                             slots cannot carry it without unpacking.
 *   le_scan_start(iv, win, act)  three small values, but no row exists.
 *   adv_start(data, len)      a caller buffer; needs a gate.
 *   gatt_write / acl_send     byte buffers both ways; each needs a gate
 *                             and a length bound.
 *   hci_cmd(op, params, …)    SEVEN arguments.  The trap carries six.
 *   find_device(addr)         returns a pointer into the manager's table.
 *   set_name / tick / start / stop / print_*  lifecycle and diagnostics.
 *
 * None is declared here, so a compile error is the honest signal rather
 * than a link error at the end.
 */

const char *uiox_bt_state_name(uiox_bt_subsys_state_t s);
const char *uiox_bt_ev_name   (uiox_bt_ev_t ev);
const char *uiox_bt_ver_name  (uiox_bt_ver_t v);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_BT_DEVICE_H */
