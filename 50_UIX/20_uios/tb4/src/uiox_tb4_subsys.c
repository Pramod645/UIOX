/* uiox_tb4_subsys.c */
#include "uiox_tb4_subsys.h"
#include <string.h>
#include <errno.h>

static void fire(uiox_tb4_subsys_t *sys, uiox_tb4_ev_t ev,
                 uiox_tb4_router_t *r)
{ if (sys->evt_cb) sys->evt_cb(ev, r, sys->evt_ctx); }

int uiox_tb4_subsys_init(uiox_tb4_subsys_t *sys,
                          uiox_tb4_hw_t *hw,
                          uiox_tb4_sec_t security)
{
    if (!sys || !hw) return -EINVAL;
    memset(sys, 0, sizeof(*sys));
    int rc = uiox_tb4_if_config(&sys->tif, hw);
    if (rc < 0) return rc;
    rc = uiox_tb4_topo_init(&sys->topo, &sys->tif);
    if (rc < 0) return rc;
    rc = uiox_tb4_proto_init(&sys->proto, &sys->topo);
    if (rc < 0) return rc;
    sys->security     = security;
    sys->auto_approve = (security == UIOX_TB4_SEC_NONE);
    sys->state        = UIOX_TB4_STATE_OFF;
    return 0;
}

int uiox_tb4_subsys_start(uiox_tb4_subsys_t *sys)
{
    if (!sys) return -EINVAL;
    sys->state = UIOX_TB4_STATE_INIT;
    int rc = uiox_tb4_if_start(&sys->tif);
    if (rc < 0) { sys->state = UIOX_TB4_STATE_ERROR; return rc; }
    rc = uiox_tb4_proto_driver_ready(&sys->proto);
    if (rc < 0) { sys->state = UIOX_TB4_STATE_ERROR; return rc; }
    uiox_tb4_proto_get_domain(&sys->proto);
    sys->state = UIOX_TB4_STATE_READY;
    return 0;
}

void uiox_tb4_subsys_stop(uiox_tb4_subsys_t *sys)
{
    if (!sys) return;
    uiox_tb4_if_stop(&sys->tif);
    sys->state = UIOX_TB4_STATE_OFF;
}

void uiox_tb4_subsys_tick(uiox_tb4_subsys_t *sys, uint32_t now_ms)
{
    if (!sys || sys->state != UIOX_TB4_STATE_READY) return;
    sys->tick_count++;
    sys->uptime_ms += 10u;
    (void)now_ms;

    /* Handle pending hotplug */
    if (sys->tif.hw->pending_irq & NHI_INT_HOTPLUG) {
        sys->tif.hw->pending_irq &= ~NHI_INT_HOTPLUG;
        /* Scan topology */
        int found = uiox_tb4_topo_scan(&sys->topo);
        if (found > 0) {
            uiox_tb4_router_t *r = sys->topo.list;
            fire(sys, UIOX_TB4_EV_DEVICE_CONNECTED, r);
            if (sys->auto_approve) uiox_tb4_subsys_approve(sys, r);
        }
    }

    /* Poll for ICM events */
    uiox_tb4_if_irq_handle(&sys->tif);
}

int uiox_tb4_subsys_approve(uiox_tb4_subsys_t *sys, uiox_tb4_router_t *r)
{
    if (!sys || !r) return -EINVAL;
    int rc = uiox_tb4_proto_approve_dev(&sys->proto, r);
    if (rc == 0 && r->authorised) {
        fire(sys, UIOX_TB4_EV_DEVICE_AUTHORISED, r);
        /* Bring up tunnels */
        uiox_tb4_proto_enable_pcie(&sys->proto, r);
        fire(sys, UIOX_TB4_EV_PCIE_TUNNEL_UP, r);
        uiox_tb4_proto_enable_dp(&sys->proto, r);
        fire(sys, UIOX_TB4_EV_DP_TUNNEL_UP, r);
        uiox_tb4_proto_enable_usb(&sys->proto, r);
        fire(sys, UIOX_TB4_EV_USB_TUNNEL_UP, r);
    } else {
        fire(sys, UIOX_TB4_EV_DEVICE_REJECTED, r);
    }
    return rc;
}

void uiox_tb4_subsys_set_cb(uiox_tb4_subsys_t *sys,
                              uiox_tb4_evt_cb_t cb, void *ctx)
{ if (!sys) return; sys->evt_cb = cb; sys->evt_ctx = ctx; }
