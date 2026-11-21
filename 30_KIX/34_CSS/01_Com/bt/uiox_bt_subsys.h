/* uiox_bt_subsys.h */
#ifndef UIOX_BT_SUBSYS_H
#define UIOX_BT_SUBSYS_H
#include "uiox_bt_proto.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    UIOX_BT_EV_POWERED_ON = 0, UIOX_BT_EV_POWERED_OFF,
    UIOX_BT_EV_SCAN_STARTED,   UIOX_BT_EV_SCAN_STOPPED,
    UIOX_BT_EV_DEVICE_FOUND,   UIOX_BT_EV_CONNECTED,
    UIOX_BT_EV_DISCONNECTED,   UIOX_BT_EV_PAIR_REQ,
    UIOX_BT_EV_PAIRED,         UIOX_BT_EV_PAIR_FAILED,
    UIOX_BT_EV_GATT_DATA,      UIOX_BT_EV_ACL_DATA,
    UIOX_BT_EV_ERROR,
} uiox_bt_ev_t;
typedef void (*uiox_bt_evt_cb_t)(uiox_bt_ev_t ev,
                                  uiox_bt_remote_dev_t *dev, void *ctx);
typedef enum {
    UIOX_BT_SUBSYS_OFF = 0,
    UIOX_BT_SUBSYS_IDLE,
    UIOX_BT_SUBSYS_SCANNING,
    UIOX_BT_SUBSYS_ADVERTISING,
    UIOX_BT_SUBSYS_CONNECTING,
    UIOX_BT_SUBSYS_CONNECTED,
    UIOX_BT_SUBSYS_ERROR,
} uiox_bt_subsys_state_t;
typedef struct {
    uiox_bt_if_t           bif;
    uiox_bt_mgr_t           mgr;
    uiox_bt_proto_t         proto;
    uiox_bt_subsys_state_t  state;
    uiox_bt_evt_cb_t        evt_cb;
    void                   *evt_ctx;
    uint32_t                tick_count;
    uint64_t                uptime_ms;
    uint32_t                poll_interval_ms;
    uint32_t                last_poll_ms;
} uiox_bt_subsys_t;
int  uiox_bt_subsys_init   (uiox_bt_subsys_t *sys, uiox_bt_hw_t *hw);
int  uiox_bt_subsys_start  (uiox_bt_subsys_t *sys);
void uiox_bt_subsys_stop   (uiox_bt_subsys_t *sys);
void uiox_bt_subsys_tick   (uiox_bt_subsys_t *sys, uint32_t now_ms);
void uiox_bt_subsys_set_cb (uiox_bt_subsys_t *sys,
                             uiox_bt_evt_cb_t cb, void *ctx);
/* =========================================================================
 * Subsystem API — the OPERATIONS, beside the lifecycle above
 *
 * ── why these exist ─────────────────────────────────────────────────
 * uiox_bt_device.h (one layer above, in userspace) has had scan_start,
 * scan_stop, connect and disconnect from the start, and the subsystem
 * below it had only init/start/stop/tick/set_cb.  A lifecycle alone
 * cannot be routed: init(sys, hw) is bring-up, tick(sys, now_ms) is the
 * kernel's timer, and set_cb registers a FUNCTION POINTER — a callback
 * into EL0, which is worse than a credential.
 *
 * These four delegate to sys->mgr, the manager the subsystem already
 * owns, and keep sys->state honest as they go.  No new machinery: the
 * calls the device layer makes are the calls the subsystem now exposes.
 * ========================================================================= */

/** Start a classic (BR/EDR) inquiry.  duration is in units of 1.28 s. */
int  uiox_bt_subsys_scan_start  (uiox_bt_subsys_t *sys, uint8_t dur_s);

/** Stop an inquiry already running. */
int  uiox_bt_subsys_scan_stop   (uiox_bt_subsys_t *sys);

/** Disconnect an established connection by handle. */
int  uiox_bt_subsys_disconnect  (uiox_bt_subsys_t *sys, uint16_t handle);

/* The one instance the CSS entry points reach.  Same role as
 * uiox_kix_css_wifi_current() on the WiFi side, and the same caveat:
 * the subsystem is built but nothing has called uiox_bt_subsys_init()
 * yet, so this returns a zeroed struct until something does. */
uiox_bt_subsys_t *uiox_kix_css_bt_current(void);


#ifdef __cplusplus
}
#endif
#endif /* UIOX_BT_SUBSYS_H */
