/* uiox_tb4_device.h */
#ifndef UIOX_TB4_DEVICE_H
#define UIOX_TB4_DEVICE_H
#include "uiox_tb4_subsys.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uiox_tb4_hw_t            *hw;
    const uiox_tb4_hw_ops_t  *hw_ops;
    uiox_tb4_sec_t            security;
    uiox_tb4_evt_cb_t         evt_cb;
    void                     *evt_ctx;
} uiox_tb4_open_params_t;
typedef struct {
    uiox_tb4_subsys_t  subsys;
    uiox_tb4_hw_t     *hw;
    bool               open;
} uiox_tb4_device_t;

int  uiox_tb4_open       (uiox_tb4_device_t           *dev,
                           const uiox_tb4_open_params_t *p);
int  uiox_tb4_start      (uiox_tb4_device_t *dev);
void uiox_tb4_stop       (uiox_tb4_device_t *dev);
void uiox_tb4_close      (uiox_tb4_device_t *dev);
void uiox_tb4_tick       (uiox_tb4_device_t *dev, uint32_t now_ms);
int  uiox_tb4_scan       (uiox_tb4_device_t *dev);
int  uiox_tb4_approve    (uiox_tb4_device_t *dev, uiox_tb4_router_t *r);
int  uiox_tb4_send       (uiox_tb4_device_t *dev,
                           const void *data, uint32_t len);
uiox_tb4_router_t *uiox_tb4_get_router(uiox_tb4_device_t *dev,
                                        uint8_t route_hi,
                                        uint32_t route_lo);
void uiox_tb4_print_info (const uiox_tb4_device_t *dev);
void uiox_tb4_print_stats(uiox_tb4_device_t *dev);
void uiox_tb4_print_topo (const uiox_tb4_device_t *dev);
const char *uiox_tb4_state_name(uiox_tb4_state_t s);
const char *uiox_tb4_ev_name   (uiox_tb4_ev_t ev);
const char *uiox_tb4_ver_name  (uiox_tb4_ver_t v);
const char *uiox_tb4_sec_name  (uiox_tb4_sec_t s);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_TB4_DEVICE_H */
