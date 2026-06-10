/* uiox_tb4_router.h */
#ifndef UIOX_TB4_ROUTER_H
#define UIOX_TB4_ROUTER_H
#include "uiox_tb4_if.h"
#ifdef __cplusplus
extern "C" {
#endif

#define UIOX_TB4_MAX_ADAPTERS       20
#define UIOX_TB4_ROUTE_BITS         64

typedef enum {
    UIOX_TB4_ADAPTER_NONE = 0,
    UIOX_TB4_ADAPTER_LANE,      /**< Thunderbolt lane adapter            */
    UIOX_TB4_ADAPTER_PCIE_DN,   /**< PCIe downstream adapter             */
    UIOX_TB4_ADAPTER_PCIE_UP,   /**< PCIe upstream adapter               */
    UIOX_TB4_ADAPTER_DP_IN,     /**< DisplayPort IN adapter              */
    UIOX_TB4_ADAPTER_DP_OUT,    /**< DisplayPort OUT adapter             */
    UIOX_TB4_ADAPTER_USB3,      /**< USB 3.x adapter                    */
} uiox_tb4_adapter_type_t;

typedef struct {
    uint8_t                  id;
    uiox_tb4_adapter_type_t  type;
    bool                     active;
    uint32_t                 cap;
} uiox_tb4_adapter_t;

typedef struct uiox_tb4_router {
    uint8_t              route_hi;   /**< Upper 8 bits of route string    */
    uint32_t             route_lo;   /**< Lower 32 bits of route string   */
    uint8_t              depth;      /**< Hop count from host             */
    uint8_t              num_adapters;
    uiox_tb4_adapter_t   adapters[UIOX_TB4_MAX_ADAPTERS];
    char                 vendor[32];
    char                 model[32];
    uint8_t              uuid[UIOX_TB4_UUID_LEN];
    bool                 authorised;
    bool                 active;
    struct uiox_tb4_router *next;
} uiox_tb4_router_t;

typedef struct {
    uiox_tb4_if_t     *tif;
    uiox_tb4_router_t  routers[UIOX_TB4_MAX_ROUTERS];
    uint8_t            num_routers;
    uiox_tb4_router_t *list;     /**< Linked list of active routers       */
} uiox_tb4_topo_t;

int  uiox_tb4_topo_init       (uiox_tb4_topo_t *topo, uiox_tb4_if_t *tif);
int  uiox_tb4_topo_scan       (uiox_tb4_topo_t *topo);
uiox_tb4_router_t *uiox_tb4_topo_find(uiox_tb4_topo_t *topo,
                                       uint8_t route_hi,
                                       uint32_t route_lo);
int  uiox_tb4_router_read_cfg (uiox_tb4_topo_t *topo,
                                uiox_tb4_router_t *r,
                                uint32_t offset, uint32_t *val);
int  uiox_tb4_router_write_cfg(uiox_tb4_topo_t *topo,
                                uiox_tb4_router_t *r,
                                uint32_t offset, uint32_t val);
void uiox_tb4_topo_print      (const uiox_tb4_topo_t *topo);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_TB4_ROUTER_H */
