/**
 * @file    uiox_mouse_device.c
 * @brief   UIOX Mouse device API implementation.
 * @date    2026-06-01
 */

 #include "uiox_mouse_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_mouse_open(uiox_mouse_device_t           *dev,
                      const uiox_mouse_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
 
     int rc = uiox_mouse_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
 
     rc = uiox_mouse_subsys_init(&dev->subsys, p->hw,
                                   p->proto, &p->event_cfg);
     if (rc < 0) return rc;
 
     dev->open = true;
     return 0;
 }
 
 int uiox_mouse_start(uiox_mouse_device_t *dev)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_mouse_subsys_start(&dev->subsys);
 }
 
 void uiox_mouse_stop(uiox_mouse_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_mouse_subsys_stop(&dev->subsys);
 }
 
 void uiox_mouse_close(uiox_mouse_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_mouse_stop(dev);
     uiox_mouse_hw_deinit(dev->hw);
     dev->open = false;
 }
 
 void uiox_mouse_tick(uiox_mouse_device_t *dev, uint32_t now_ms)
 {
     if (!dev || !dev->open) return;
     uiox_mouse_subsys_tick(&dev->subsys, now_ms);
 }
 
 bool uiox_mouse_poll(uiox_mouse_device_t *dev, uiox_mouse_event_t *ev_out)
 {
     if (!dev || !dev->open || !ev_out) return false;
     return uiox_mouse_buf_pop(&dev->subsys.cooked_rb, ev_out);
 }
 
 int uiox_mouse_add_zone(uiox_mouse_device_t     *dev,
                          const uiox_mouse_zone_t *zone)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_mouse_subsys_add_zone(&dev->subsys, zone);
 }
 
 int uiox_mouse_add_callback(uiox_mouse_device_t *dev,
                               uiox_mouse_evt_cb_t  fn,
                               void                *ctx,
                               uint8_t              filter)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_mouse_subsys_add_cb(&dev->subsys, fn, ctx, filter);
 }
 
 void uiox_mouse_cursor(const uiox_mouse_device_t *dev,
                         int32_t *x, int32_t *y)
 {
     if (!dev || !dev->open) return;
     uiox_mouse_subsys_cursor(&dev->subsys, x, y);
 }
 
 void uiox_mouse_warp(uiox_mouse_device_t *dev, int32_t x, int32_t y)
 {
     if (!dev || !dev->open) return;
     uiox_mouse_subsys_warp(&dev->subsys, x, y);
 }
 
 bool uiox_mouse_connected(const uiox_mouse_device_t *dev)
 {
     if (!dev || !dev->open) return false;
     return dev->subsys.state != UIOX_MOUSE_SUBSYS_DISCONNECTED &&
            dev->subsys.state != UIOX_MOUSE_SUBSYS_STOPPED;
 }
 
 void uiox_mouse_print_stats(const uiox_mouse_device_t *dev)
 {
     if (!dev) return;
     const uiox_mouse_subsys_t *s = &dev->subsys;
     printf("  State          : %s\n", uiox_mouse_state_name(s->state));
     printf("  Tick count     : %u\n",   s->tick_count);
     printf("  Total moves    : %llu\n", (unsigned long long)s->total_moves);
     printf("  Total clicks   : %llu\n", (unsigned long long)s->total_clicks);
     printf("  Total dblclicks: %llu\n", (unsigned long long)s->total_dblclicks);
     printf("  Total scrolls  : %llu\n", (unsigned long long)s->total_scrolls);
     printf("  Raw overflow   : %u\n",   s->raw_rb.overflow);
     printf("  Hot zones      : %u\n",   s->zone_count);
     printf("  Callbacks      : %u\n",   s->cb_count);
     int32_t cx = 0, cy = 0;
     uiox_mouse_subsys_cursor(s, &cx, &cy);
     printf("  Cursor pos     : (%d, %d)\n", cx, cy);
     uiox_mouse_if_stats_t is;
     uiox_mouse_if_stats_get(&dev->subsys.mif, &is);
     printf("  Reports RX     : %llu\n", (unsigned long long)is.reports_received);
     printf("  Reports dropped: %llu\n", (unsigned long long)is.reports_dropped);
 }
 
 const char *uiox_mouse_evt_name(uiox_mouse_ev_type_t type)
 {
     switch (type) {
     case UIOX_MOUSE_EV_MOVE:        return "MOVE";
     case UIOX_MOUSE_EV_BTN_PRESS:   return "BTN_PRESS";
     case UIOX_MOUSE_EV_BTN_RELEASE: return "BTN_RELEASE";
     case UIOX_MOUSE_EV_CLICK:       return "CLICK";
     case UIOX_MOUSE_EV_DBLCLICK:    return "DBLCLICK";
     case UIOX_MOUSE_EV_SCROLL_V:    return "SCROLL_V";
     case UIOX_MOUSE_EV_SCROLL_H:    return "SCROLL_H";
     case UIOX_MOUSE_EV_CONNECT:     return "CONNECT";
     case UIOX_MOUSE_EV_DISCONNECT:  return "DISCONNECT";
     case UIOX_MOUSE_EV_GESTURE:     return "GESTURE";
     default:                         return "UNKNOWN";
     }
 }
 
 const char *uiox_mouse_state_name(uiox_mouse_subsys_state_t s)
 {
     switch (s) {
     case UIOX_MOUSE_SUBSYS_STOPPED:      return "STOPPED";
     case UIOX_MOUSE_SUBSYS_RUNNING:      return "RUNNING";
     case UIOX_MOUSE_SUBSYS_DISCONNECTED: return "DISCONNECTED";
     default:                              return "UNKNOWN";
     }
 }
 