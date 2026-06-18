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
#ifdef __cplusplus
}
#endif
#endif /* UIOX_BT_SUBSYS_H */
