#include "uiox_tb4_subsys.h"

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
    if (sys->last_ms && now_ms > sys->last_ms)
        sys->uptime_ms += (uint64_t)(now_ms - sys->last_ms);
    sys->last_ms = now_ms;

    if (sys->tif.hw->pending_irq & NHI_INT_HOTPLUG) {
        sys->tif.hw->pending_irq &= ~NHI_INT_HOTPLUG;

        /* topo_scan APPENDS (router.c:22); reset so this REFRESHES.
         * The real fix belongs in topo_scan — layer 30. */
        sys->topo.num_routers = 0;
        sys->topo.list        = NULL;

        int found = uiox_tb4_topo_scan(&sys->topo);
        if (found > 0) {
            uiox_tb4_router_t *r = sys->topo.list;
            fire(sys, UIOX_TB4_EV_DEVICE_CONNECTED, r);
            if (sys->auto_approve) uiox_tb4_subsys_approve(sys, r);
        }
    }

    uiox_tb4_if_irq_handle(&sys->tif);
}

int uiox_tb4_subsys_approve(uiox_tb4_subsys_t *sys, uiox_tb4_router_t *r)
{
    if (!sys || !r) return -EINVAL;
    int rc = uiox_tb4_proto_approve_dev(&sys->proto, r);
    if (rc == 0 && r->authorised) {
        fire(sys, UIOX_TB4_EV_DEVICE_AUTHORISED, r);
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

uiox_tb4_state_t uiox_tb4_subsys_state(const uiox_tb4_subsys_t *sys)
{
    if (!sys) return UIOX_TB4_STATE_OFF;
    return sys->state;
}

int uiox_tb4_subsys_topo_get(const uiox_tb4_subsys_t *sys,
                             uint8_t *count_out)
{
    if (!sys || !count_out) return -EINVAL;
    *count_out = sys->topo.num_routers;
    return 0;
}

int uiox_tb4_subsys_dev_get(const uiox_tb4_subsys_t *sys,
                            uint8_t index,
                            uiox_tb4_router_t *out)
{
    if (!sys || !out) return -EINVAL;
    if (index >= sys->topo.num_routers) return -EINVAL;

    /* COPIED: the caller must not walk `next` into the live list. */
    *out = sys->topo.routers[index];
    out->next = NULL;
    return 0;
}

int uiox_tb4_subsys_sec_get(const uiox_tb4_subsys_t *sys,
                            uiox_tb4_sec_t *mode_out,
                            bool *auto_approve_out)
{
    if (!sys || !mode_out || !auto_approve_out) return -EINVAL;
    *mode_out         = sys->security;
    *auto_approve_out = sys->auto_approve;
    return 0;
}

void uiox_tb4_subsys_stats_get(const uiox_tb4_subsys_t *sys,
                               uint32_t *ticks_out,
                               uint64_t *uptime_ms_out)
{
    if (!sys) return;
    if (ticks_out)     *ticks_out     = sys->tick_count;
    if (uptime_ms_out) *uptime_ms_out = sys->uptime_ms;
}
