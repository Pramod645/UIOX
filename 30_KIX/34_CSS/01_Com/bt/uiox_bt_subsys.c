/* uiox_bt_subsys.c */
#include "uiox_bt_subsys.h"
#include <string.h>
#include <errno.h>

static void fire(uiox_bt_subsys_t *sys, uiox_bt_ev_t ev,
                 uiox_bt_remote_dev_t *dev)
{ if (sys->evt_cb) sys->evt_cb(ev, dev, sys->evt_ctx); }

int uiox_bt_subsys_init(uiox_bt_subsys_t *sys, uiox_bt_hw_t *hw)
{
    if (!sys || !hw) return -EINVAL;
    memset(sys, 0, sizeof(*sys));
    int rc = uiox_bt_if_config(&sys->bif, hw);
    if (rc < 0) return rc;
    rc = uiox_bt_mgr_init(&sys->mgr, &sys->bif);
    if (rc < 0) return rc;
    rc = uiox_bt_proto_init(&sys->proto, &sys->mgr);
    if (rc < 0) return rc;
    sys->poll_interval_ms = 50u;
    sys->state            = UIOX_BT_SUBSYS_OFF;
    return 0;
}

int uiox_bt_subsys_start(uiox_bt_subsys_t *sys)
{
    if (!sys) return -EINVAL;
    int rc = uiox_bt_if_start(&sys->bif);
    if (rc < 0) return rc;
    rc = uiox_bt_mgr_init_ctrl(&sys->mgr);
    if (rc < 0) { sys->state = UIOX_BT_SUBSYS_ERROR; return rc; }
    sys->state = UIOX_BT_SUBSYS_IDLE;
    fire(sys, UIOX_BT_EV_POWERED_ON, NULL);
    return 0;
}

void uiox_bt_subsys_stop(uiox_bt_subsys_t *sys)
{
    if (!sys) return;
    uiox_bt_if_stop(&sys->bif);
    sys->state = UIOX_BT_SUBSYS_OFF;
    fire(sys, UIOX_BT_EV_POWERED_OFF, NULL);
}

void uiox_bt_subsys_tick(uiox_bt_subsys_t *sys, uint32_t now_ms)
{
    if (!sys || sys->state == UIOX_BT_SUBSYS_OFF) return;
    sys->tick_count++;
    sys->uptime_ms += 10u;

    /* Poll for HCI events */
    if ((now_ms - sys->last_poll_ms) >= sys->poll_interval_ms) {
        sys->last_poll_ms = now_ms;
        uiox_bt_if_rx_poll(&sys->bif);
    }

    /* Dispatch buffered events */
    uiox_bt_pkt_t pkt;
    while (!uiox_bt_evt_empty(&sys->bif.evt_ring)) {
        if (!uiox_bt_evt_pop(&sys->bif.evt_ring, &pkt)) break;
        uiox_bt_mgr_process_evt(&sys->mgr, &pkt);

        /* Fire application callbacks for known events */
        if (pkt.pkt_type == HCI_EVENT_PKT) {
            uint8_t ev_code = pkt.data[1];
            if (ev_code == HCI_EV_INQUIRY_RESULT)
                fire(sys, UIOX_BT_EV_DEVICE_FOUND, NULL);
            else if (ev_code == HCI_EV_CONN_COMPLETE && pkt.data[3]==0)
                fire(sys, UIOX_BT_EV_CONNECTED, NULL);
            else if (ev_code == HCI_EV_DISCONN_COMPLETE)
                fire(sys, UIOX_BT_EV_DISCONNECTED, NULL);
            else if (ev_code == HCI_EV_LE_META) {
                uint8_t sub = pkt.data[3];
                if (sub == HCI_LE_EV_ADV_REPORT)
                    fire(sys, UIOX_BT_EV_DEVICE_FOUND, NULL);
                else if (sub == HCI_LE_EV_CONN_COMPLETE && pkt.data[4]==0)
                    fire(sys, UIOX_BT_EV_CONNECTED, NULL);
            }
        } else if (pkt.pkt_type == HCI_ACL_PKT) {
            fire(sys, UIOX_BT_EV_ACL_DATA, NULL);
        }
    }
}

void uiox_bt_subsys_set_cb(uiox_bt_subsys_t *sys,
                             uiox_bt_evt_cb_t cb, void *ctx)
{ if (!sys) return; sys->evt_cb = cb; sys->evt_ctx = ctx; }
