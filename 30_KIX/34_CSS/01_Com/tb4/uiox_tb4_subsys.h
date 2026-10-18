/* uiox_tb4_subsys.h */
#ifndef UIOX_TB4_SUBSYS_H
#define UIOX_TB4_SUBSYS_H
#include "uiox_tb4_proto.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UIOX_TB4_EV_DEVICE_CONNECTED = 0,
    UIOX_TB4_EV_DEVICE_DISCONNECTED,
    UIOX_TB4_EV_DEVICE_AUTHORISED,
    UIOX_TB4_EV_DEVICE_REJECTED,
    UIOX_TB4_EV_PCIE_TUNNEL_UP,
    UIOX_TB4_EV_DP_TUNNEL_UP,
    UIOX_TB4_EV_USB_TUNNEL_UP,
    UIOX_TB4_EV_TUNNEL_DOWN,
    UIOX_TB4_EV_SECURITY_VIOLATION,
    UIOX_TB4_EV_ERROR,
} uiox_tb4_ev_t;

typedef void (*uiox_tb4_evt_cb_t)(uiox_tb4_ev_t ev,
                                    uiox_tb4_router_t *router, void *ctx);

typedef enum {
    UIOX_TB4_STATE_OFF = 0,
    UIOX_TB4_STATE_INIT,
    UIOX_TB4_STATE_READY,
    UIOX_TB4_STATE_ERROR,
} uiox_tb4_state_t;

typedef struct {
    uiox_tb4_if_t       tif;
    uiox_tb4_topo_t     topo;
    uiox_tb4_proto_t    proto;
    uiox_tb4_state_t    state;
    uiox_tb4_sec_t      security;
    bool                auto_approve;
    uiox_tb4_evt_cb_t   evt_cb;
    void               *evt_ctx;
    uint32_t            tick_count;
    uint32_t            last_ms;      /* previous now_ms, for uptime */
    uint64_t            uptime_ms;
} uiox_tb4_subsys_t;

int  uiox_tb4_subsys_init    (uiox_tb4_subsys_t *sys,
                               uiox_tb4_hw_t *hw,
                               uiox_tb4_sec_t security);
int  uiox_tb4_subsys_start   (uiox_tb4_subsys_t *sys);
void uiox_tb4_subsys_stop    (uiox_tb4_subsys_t *sys);
void uiox_tb4_subsys_tick    (uiox_tb4_subsys_t *sys, uint32_t now_ms);
void uiox_tb4_subsys_set_cb  (uiox_tb4_subsys_t *sys,
                               uiox_tb4_evt_cb_t cb, void *ctx);
int  uiox_tb4_subsys_approve (uiox_tb4_subsys_t *sys,
                               uiox_tb4_router_t *r);

                               // added now
uiox_tb4_state_t uiox_tb4_subsys_state (const uiox_tb4_subsys_t *sys);
int  uiox_tb4_subsys_topo_get  (const uiox_tb4_subsys_t *sys,
                                uint8_t *count_out);
int  uiox_tb4_subsys_dev_get   (const uiox_tb4_subsys_t *sys,
                                uint8_t index,
                                uiox_tb4_router_t *out);
int  uiox_tb4_subsys_sec_get   (const uiox_tb4_subsys_t *sys,
                                uiox_tb4_sec_t *mode_out,
                                bool *auto_approve_out);
void uiox_tb4_subsys_stats_get (const uiox_tb4_subsys_t *sys,
                                uint32_t *ticks_out,
                                uint64_t *uptime_ms_out);

int  uiox_tb4_dev_bind (uiox_tb4_subsys_t *sys);

int  uiox_tb4_dev_bind (uiox_tb4_subsys_t *sys);

/* Instantiate, register and start this class in one call.  Defined in
 * uiox_tb4_ops.c, where the subsystem is file-static. */
int  uiox_tb4_probe (uiox_tb4_hw_t *hw, uiox_tb4_sec_t security);

int uiox_tb4_bringup (void);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_TB4_SUBSYS_H */
