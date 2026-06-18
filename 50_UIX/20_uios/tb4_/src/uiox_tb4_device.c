/* uiox_tb4_device.c */
#include "uiox_tb4_device.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>

int uiox_tb4_open(uiox_tb4_device_t *dev, const uiox_tb4_open_params_t *p)
{
    if (!dev||!p||!p->hw||!p->hw_ops) return -EINVAL;
    memset(dev, 0, sizeof(*dev));
    dev->hw = p->hw;
    int rc = uiox_tb4_hw_init(p->hw, p->hw_ops);
    if (rc<0) return rc;
    rc = uiox_tb4_subsys_init(&dev->subsys, p->hw, p->security);
    if (rc<0) return rc;
    if (p->evt_cb)
        uiox_tb4_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
    dev->open=true;
    return 0;
}
int  uiox_tb4_start(uiox_tb4_device_t *d)
{ if(!d||!d->open) return -EINVAL;
  return uiox_tb4_subsys_start(&d->subsys); }
void uiox_tb4_stop(uiox_tb4_device_t *d)
{ if(!d||!d->open) return; uiox_tb4_subsys_stop(&d->subsys); }
void uiox_tb4_close(uiox_tb4_device_t *d)
{ if(!d||!d->open) return; uiox_tb4_stop(d);
  uiox_tb4_hw_deinit(d->hw); d->open=false; }
void uiox_tb4_tick(uiox_tb4_device_t *d, uint32_t ms)
{ if(!d||!d->open) return; uiox_tb4_subsys_tick(&d->subsys,ms); }
int  uiox_tb4_scan(uiox_tb4_device_t *d)
{ if(!d||!d->open) return -EINVAL;
  return uiox_tb4_topo_scan(&d->subsys.topo); }
int  uiox_tb4_approve(uiox_tb4_device_t *d, uiox_tb4_router_t *r)
{ if(!d||!d->open) return -EINVAL;
  return uiox_tb4_subsys_approve(&d->subsys,r); }
uiox_tb4_router_t *uiox_tb4_get_router(uiox_tb4_device_t *d,
                                         uint8_t hi, uint32_t lo)
{ if(!d||!d->open) return NULL;
  return uiox_tb4_topo_find(&d->subsys.topo,hi,lo); }

int uiox_tb4_send(uiox_tb4_device_t *d, const void *data, uint32_t len)
{
    if(!d||!d->open||!data||!len) return -EINVAL;
    uiox_tb4_frame_t *f = uiox_tb4_buf_alloc_tx();
    if(!f) return -ENOMEM;
    if(len > f->capacity) len = f->capacity;
    memcpy(f->data, data, len);
    f->len = len;
    int rc = uiox_tb4_if_tx(&d->subsys.tif, f);
    uiox_tb4_buf_free(f);
    return rc;
}

void uiox_tb4_print_info(const uiox_tb4_device_t *d)
{
    if(!d) return;
    const uiox_tb4_hw_t *hw = d->hw;
    printf("  Model          : %s\n", hw->model);
    printf("  Version        : %s\n", uiox_tb4_ver_name(hw->version));
    printf("  Security       : %s\n",
           uiox_tb4_sec_name(d->subsys.security));
    printf("  Capabilities   : 0x%08X\n", hw->caps);
    printf("  Ports          : %u\n", hw->num_ports);
    printf("  State          : %s\n",
           uiox_tb4_state_name(d->subsys.state));
    printf("  ICM ready      : %s\n", hw->icm_ready?"YES":"NO");
    printf("  Auto-approve   : %s\n",
           d->subsys.auto_approve?"YES":"NO");
    printf("  Domain UUID    : %02X%02X%02X%02X-...\n",
           d->subsys.proto.domain_uuid[0],
           d->subsys.proto.domain_uuid[1],
           d->subsys.proto.domain_uuid[2],
           d->subsys.proto.domain_uuid[3]);
}

void uiox_tb4_print_stats(uiox_tb4_device_t *d)
{
    if(!d) return;
    const uiox_tb4_subsys_t *s = &d->subsys;
    printf("  Uptime         : %llu ms\n",(unsigned long long)s->uptime_ms);
    printf("  Tick count     : %u\n", s->tick_count);
    printf("  Routers found  : %u\n", s->topo.num_routers);
    printf("  Approved       : %u\n", s->proto.approve_count);
    printf("  Rejected       : %u\n", s->proto.reject_count);
    uiox_tb4_if_stats_t is;
    uiox_tb4_if_stats_get(&d->subsys.tif, &is);
    printf("  Frames TX      : %llu\n",(unsigned long long)is.frames_tx);
    printf("  Frames RX      : %llu\n",(unsigned long long)is.frames_rx);
    printf("  Bytes TX       : %llu\n",(unsigned long long)is.bytes_tx);
    printf("  Bytes RX       : %llu\n",(unsigned long long)is.bytes_rx);
    printf("  ICM sent       : %u\n",  is.icm_msgs_sent);
    printf("  ICM recv       : %u\n",  is.icm_msgs_recv);
    printf("  Hotplug events : %u\n",  is.hotplug_events);
    printf("  Errors         : %u\n",  is.errors);
    printf("  TX buf free    : %u / %u\n",
           uiox_tb4_buf_tx_free(), UIOX_TB4_TX_POOL_SIZE);
    printf("  RX buf free    : %u / %u\n",
           uiox_tb4_buf_rx_free(), UIOX_TB4_RX_POOL_SIZE);
}

void uiox_tb4_print_topo(const uiox_tb4_device_t *d)
{ if(d&&d->open) uiox_tb4_topo_print(&d->subsys.topo); }

const char *uiox_tb4_state_name(uiox_tb4_state_t s)
{
    switch(s){
    case UIOX_TB4_STATE_OFF:  return "OFF";
    case UIOX_TB4_STATE_INIT: return "INIT";
    case UIOX_TB4_STATE_READY:return "READY";
    case UIOX_TB4_STATE_ERROR:return "ERROR";
    default:                   return "UNKNOWN";
    }
}
const char *uiox_tb4_ev_name(uiox_tb4_ev_t ev)
{
    switch(ev){
    case UIOX_TB4_EV_DEVICE_CONNECTED:   return "DEVICE_CONNECTED";
    case UIOX_TB4_EV_DEVICE_DISCONNECTED:return "DEVICE_DISCONNECTED";
    case UIOX_TB4_EV_DEVICE_AUTHORISED:  return "DEVICE_AUTHORISED";
    case UIOX_TB4_EV_DEVICE_REJECTED:    return "DEVICE_REJECTED";
    case UIOX_TB4_EV_PCIE_TUNNEL_UP:     return "PCIE_TUNNEL_UP";
    case UIOX_TB4_EV_DP_TUNNEL_UP:       return "DP_TUNNEL_UP";
    case UIOX_TB4_EV_USB_TUNNEL_UP:      return "USB_TUNNEL_UP";
    case UIOX_TB4_EV_TUNNEL_DOWN:        return "TUNNEL_DOWN";
    case UIOX_TB4_EV_SECURITY_VIOLATION: return "SECURITY_VIOLATION";
    case UIOX_TB4_EV_ERROR:              return "ERROR";
    default:                              return "UNKNOWN";
    }
}
const char *uiox_tb4_ver_name(uiox_tb4_ver_t v)
{
    switch(v){
    case UIOX_TB4_VER_TB3:     return "Thunderbolt 3";
    case UIOX_TB4_VER_TB4:     return "Thunderbolt 4";
    case UIOX_TB4_VER_USB4_V1: return "USB4 v1.0";
    case UIOX_TB4_VER_USB4_V2: return "USB4 v2.0";
    default:                    return "Unknown";
    }
}
const char *uiox_tb4_sec_name(uiox_tb4_sec_t s)
{
    switch(s){
    case UIOX_TB4_SEC_NONE:           return "None (open)";
    case UIOX_TB4_SEC_USER_AUTH:      return "User auth";
    case UIOX_TB4_SEC_SECURE_CONNECT: return "Secure Connect";
    case UIOX_TB4_SEC_DP_ONLY:        return "Display-only";
    case UIOX_TB4_SEC_USB_ONLY:       return "USB-only";
    default:                           return "Unknown";
    }
}
