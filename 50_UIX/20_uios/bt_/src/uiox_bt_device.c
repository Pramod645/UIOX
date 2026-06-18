/* uiox_bt_device.c */
#include "uiox_bt_device.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>

int uiox_bt_open(uiox_bt_device_t *dev, const uiox_bt_open_params_t *p)
{
    if (!dev||!p||!p->hw||!p->hw_ops) return -EINVAL;
    memset(dev,0,sizeof(*dev));
    dev->hw = p->hw;
    int rc = uiox_bt_hw_init(p->hw, p->hw_ops);
    if (rc<0) return rc;
    rc = uiox_bt_subsys_init(&dev->subsys, p->hw);
    if (rc<0) return rc;
    if (p->evt_cb)
        uiox_bt_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
    dev->open=true;
    return 0;
}
int  uiox_bt_start(uiox_bt_device_t *d)
{ if(!d||!d->open) return -EINVAL;
  return uiox_bt_subsys_start(&d->subsys); }
void uiox_bt_stop(uiox_bt_device_t *d)
{ if(!d||!d->open) return; uiox_bt_subsys_stop(&d->subsys); }
void uiox_bt_close(uiox_bt_device_t *d)
{ if(!d||!d->open) return; uiox_bt_stop(d);
  uiox_bt_hw_deinit(d->hw); d->open=false; }
void uiox_bt_tick(uiox_bt_device_t *d, uint32_t ms)
{ if(!d||!d->open) return; uiox_bt_subsys_tick(&d->subsys,ms); }
int  uiox_bt_set_name(uiox_bt_device_t *d, const char *n)
{ if(!d||!d->open) return -EINVAL;
  return uiox_bt_mgr_set_name(&d->subsys.mgr,n); }
int  uiox_bt_scan_start(uiox_bt_device_t *d, uint8_t dur)
{ if(!d||!d->open) return -EINVAL;
  d->subsys.state = UIOX_BT_SUBSYS_SCANNING;
  return uiox_bt_mgr_scan_start(&d->subsys.mgr,dur); }
int  uiox_bt_scan_stop(uiox_bt_device_t *d)
{ if(!d||!d->open) return -EINVAL;
  d->subsys.state = UIOX_BT_SUBSYS_IDLE;
  return uiox_bt_mgr_scan_stop(&d->subsys.mgr); }
int  uiox_bt_le_scan_start(uiox_bt_device_t *d,
                            uint16_t iv, uint16_t win, bool act)
{ if(!d||!d->open) return -EINVAL;
  d->subsys.state = UIOX_BT_SUBSYS_SCANNING;
  return uiox_bt_mgr_le_scan_start(&d->subsys.mgr,iv,win,act); }
int  uiox_bt_le_scan_stop(uiox_bt_device_t *d)
{ if(!d||!d->open) return -EINVAL;
  d->subsys.state = UIOX_BT_SUBSYS_IDLE;
  return uiox_bt_mgr_le_scan_stop(&d->subsys.mgr); }
int  uiox_bt_adv_start(uiox_bt_device_t *d,
                        const uint8_t *data, uint8_t len)
{ if(!d||!d->open) return -EINVAL;
  d->subsys.state = UIOX_BT_SUBSYS_ADVERTISING;
  return uiox_bt_mgr_adv_start(&d->subsys.mgr,data,len); }
int  uiox_bt_adv_stop(uiox_bt_device_t *d)
{ if(!d||!d->open) return -EINVAL;
  d->subsys.state = UIOX_BT_SUBSYS_IDLE;
  return uiox_bt_mgr_adv_stop(&d->subsys.mgr); }
int  uiox_bt_connect(uiox_bt_device_t *d,
                      const uiox_bt_addr_t addr, bool ble)
{ if(!d||!d->open) return -EINVAL;
  return uiox_bt_mgr_connect(&d->subsys.mgr,addr,ble); }
int  uiox_bt_disconnect(uiox_bt_device_t *d, uint16_t h)
{ if(!d||!d->open) return -EINVAL;
  return uiox_bt_mgr_disconnect(&d->subsys.mgr,h); }
int  uiox_bt_gatt_write(uiox_bt_device_t *d, uint16_t ch,
                         uint16_t ah, const uint8_t *data, uint16_t len)
{ if(!d||!d->open) return -EINVAL;
  return uiox_bt_proto_gatt_write(&d->subsys.proto,ch,ah,data,len); }
int  uiox_bt_acl_send(uiox_bt_device_t *d, uint16_t h,
                       const uint8_t *data, uint16_t len)
{ if(!d||!d->open) return -EINVAL;
  return uiox_bt_if_acl_tx(&d->subsys.bif,h,data,len); }
int  uiox_bt_hci_cmd(uiox_bt_device_t *d, uint16_t op,
                      const uint8_t *p, uint8_t plen,
                      uint8_t *resp, uint8_t rmax, uint32_t ms)
{ if(!d||!d->open) return -EINVAL;
  return uiox_bt_if_hci_cmd(&d->subsys.bif,op,p,plen,resp,rmax,ms); }
uiox_bt_remote_dev_t *uiox_bt_find_device(uiox_bt_device_t *d,
                                           const uiox_bt_addr_t addr)
{ if(!d||!d->open) return NULL;
  return uiox_bt_mgr_find(&d->subsys.mgr,addr); }

void uiox_bt_print_info(const uiox_bt_device_t *d)
{
    if(!d) return;
    const uiox_bt_hw_t *hw = d->hw;
    printf("  BT model       : %s\n", hw->model);
    printf("  BT version     : %s\n", uiox_bt_ver_name(hw->version));
    printf("  FW version     : %s\n", hw->fw_version);
    printf("  BD address     : %02X:%02X:%02X:%02X:%02X:%02X\n",
           hw->bd_addr[5],hw->bd_addr[4],hw->bd_addr[3],
           hw->bd_addr[2],hw->bd_addr[1],hw->bd_addr[0]);
    printf("  Capabilities   : 0x%08X\n", hw->caps);
    printf("  Local name     : %s\n", d->subsys.mgr.local_name);
    printf("  State          : %s\n",
           uiox_bt_state_name(d->subsys.state));
}

void uiox_bt_print_stats(uiox_bt_device_t *d)
{
    if(!d) return;
    printf("  Uptime         : %llu ms\n",
           (unsigned long long)d->subsys.uptime_ms);
    printf("  Tick count     : %u\n",   d->subsys.tick_count);
    uiox_bt_if_stats_t is;
    uiox_bt_if_stats_get(&d->subsys.bif, &is);
    printf("  HCI cmds sent  : %llu\n",(unsigned long long)is.hci_cmds_sent);
    printf("  HCI evts recv  : %llu\n",(unsigned long long)is.hci_events_recv);
    printf("  ACL TX pkts    : %llu\n",(unsigned long long)is.acl_tx_pkts);
    printf("  ACL RX pkts    : %llu\n",(unsigned long long)is.acl_rx_pkts);
    printf("  Bytes TX       : %llu\n",(unsigned long long)is.bytes_tx);
    printf("  Bytes RX       : %llu\n",(unsigned long long)is.bytes_rx);
    printf("  CMD errors     : %u\n",   is.cmd_errors);
    printf("  Comm errors    : %u\n",   is.comm_errors);
    printf("  Devices found  : %u\n",   d->subsys.mgr.num_devices);
}

void uiox_bt_print_devices(const uiox_bt_device_t *d)
{ if(d&&d->open) uiox_bt_mgr_print_devices(&d->subsys.mgr); }

const char *uiox_bt_state_name(uiox_bt_subsys_state_t s)
{
    switch(s){
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
    switch(ev){
    case UIOX_BT_EV_POWERED_ON:   return "POWERED_ON";
    case UIOX_BT_EV_POWERED_OFF:  return "POWERED_OFF";
    case UIOX_BT_EV_SCAN_STARTED: return "SCAN_STARTED";
    case UIOX_BT_EV_SCAN_STOPPED: return "SCAN_STOPPED";
    case UIOX_BT_EV_DEVICE_FOUND: return "DEVICE_FOUND";
    case UIOX_BT_EV_CONNECTED:    return "CONNECTED";
    case UIOX_BT_EV_DISCONNECTED:  return "DISCONNECTED";
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
    switch(v){
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
