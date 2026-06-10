/* uiox_tb4_router.c */
#include "uiox_tb4_router.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

int uiox_tb4_topo_init(uiox_tb4_topo_t *topo, uiox_tb4_if_t *tif)
{
    if (!topo || !tif) return -EINVAL;
    memset(topo, 0, sizeof(*topo));
    topo->tif  = tif;
    topo->list = NULL;
    return 0;
}

int uiox_tb4_topo_scan(uiox_tb4_topo_t *topo)
{
    if (!topo) return -EINVAL;
    /* ICM GET_ROUTE command: opcode 0x03 */
    uint32_t req[2] = { 0x00000003u, 0x00000000u };
    uint32_t resp[16];
    int rc = uiox_tb4_if_icm_cmd(topo->tif, req, 2u, resp, 16u);
    if (rc < 0) return rc;

    /* In a real implementation parse ICM response to build router list.
     * Here stub a single downstream router at route 0x01.              */
    if (topo->num_routers < UIOX_TB4_MAX_ROUTERS) {
        uiox_tb4_router_t *r = &topo->routers[topo->num_routers++];
        memset(r, 0, sizeof(*r));
        r->route_hi  = 0u;
        r->route_lo  = 0x01u;
        r->depth     = 1u;
        r->active    = true;
        r->authorised= false;
        strncpy(r->vendor, "Generic", 31);
        strncpy(r->model,  "TB4 Device", 31);
        /* Add lane adapter */
        r->adapters[0].id   = 1u;
        r->adapters[0].type = UIOX_TB4_ADAPTER_LANE;
        r->adapters[0].active = true;
        r->num_adapters = 1u;
        r->next   = topo->list;
        topo->list= r;
    }
    return (int)topo->num_routers;
}

uiox_tb4_router_t *uiox_tb4_topo_find(uiox_tb4_topo_t *topo,
                                        uint8_t route_hi,
                                        uint32_t route_lo)
{
    if (!topo) return NULL;
    for (uiox_tb4_router_t *r = topo->list; r; r = r->next)
        if (r->route_hi == route_hi && r->route_lo == route_lo)
            return r;
    return NULL;
}

int uiox_tb4_router_read_cfg(uiox_tb4_topo_t *topo,
                               uiox_tb4_router_t *r,
                               uint32_t offset, uint32_t *val)
{
    if (!topo || !r || !val) return -EINVAL;
    const uiox_tb4_hw_ops_t *ops =
        (const uiox_tb4_hw_ops_t *)topo->tif->hw->priv;
    if (!ops || !ops->cfg_read) return -ENOSYS;
    return ops->cfg_read(topo->tif->hw, r->route_hi, r->route_lo,
                          offset, val);
}

int uiox_tb4_router_write_cfg(uiox_tb4_topo_t *topo,
                                uiox_tb4_router_t *r,
                                uint32_t offset, uint32_t val)
{
    if (!topo || !r) return -EINVAL;
    const uiox_tb4_hw_ops_t *ops =
        (const uiox_tb4_hw_ops_t *)topo->tif->hw->priv;
    if (!ops || !ops->cfg_write) return -ENOSYS;
    return ops->cfg_write(topo->tif->hw, r->route_hi, r->route_lo,
                           offset, val);
}

void uiox_tb4_topo_print(const uiox_tb4_topo_t *topo)
{
    if (!topo) return;
    printf("  TB4 Topology (%u routers):\n", topo->num_routers);
    for (const uiox_tb4_router_t *r = topo->list; r; r = r->next) {
        printf("    Route=%02X:%08X  depth=%u  %s %s  auth=%s\n",
               r->route_hi, r->route_lo, r->depth,
               r->vendor, r->model,
               r->authorised ? "YES" : "NO");
        for (uint8_t a = 0; a < r->num_adapters; a++) {
            static const char *anames[] = {
                "NONE","LANE","PCIe-DN","PCIe-UP",
                "DP-IN","DP-OUT","USB3"
            };
            const uiox_tb4_adapter_t *ad = &r->adapters[a];
            uint8_t t = ad->type < 7u ? ad->type : 0u;
            printf("      Adapter[%u] type=%-8s %s\n",
                   ad->id, anames[t],
                   ad->active ? "active" : "inactive");
        }
    }
}
