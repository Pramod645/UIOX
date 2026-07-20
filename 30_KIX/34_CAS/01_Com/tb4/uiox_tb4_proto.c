/* uiox_tb4_proto.c */
#include "uiox_tb4_proto.h"
#include <string.h>
#include <errno.h>

int uiox_tb4_proto_init(uiox_tb4_proto_t *proto, uiox_tb4_topo_t *topo)
{
    if (!proto || !topo) return -EINVAL;
    memset(proto, 0, sizeof(*proto));
    proto->topo = topo;
    return 0;
}

int uiox_tb4_proto_driver_ready(uiox_tb4_proto_t *proto)
{
    if (!proto) return -EINVAL;
    uint32_t req[4]  = { ICM_OP_DRIVER_READY, 0,0,0 };
    uint32_t resp[8];
    int rc = uiox_tb4_if_icm_cmd(proto->topo->tif,
                                   req, 4u, resp, 8u);
    if (rc >= 0 && (resp[0] & 0xFFu) == ICM_RESP_OK)
        proto->driver_ready = true;
    return rc;
}

int uiox_tb4_proto_get_domain(uiox_tb4_proto_t *proto)
{
    if (!proto) return -EINVAL;
    uint32_t req[2]  = { ICM_OP_GET_DOMAIN_UUID, 0u };
    uint32_t resp[8];
    int rc = uiox_tb4_if_icm_cmd(proto->topo->tif, req, 2u, resp, 8u);
    if (rc >= 4) memcpy(proto->domain_uuid, &resp[1], 16u);
    return rc;
}

int uiox_tb4_proto_approve_dev(uiox_tb4_proto_t *proto,
                                uiox_tb4_router_t *r)
{
    if (!proto || !r) return -EINVAL;
    uint32_t req[4] = {
        ICM_OP_APPROVE_DEVICE,
        r->route_lo,
        (uint32_t)r->route_hi,
        0u
    };
    uint32_t resp[4];
    int rc = uiox_tb4_if_icm_cmd(proto->topo->tif, req, 4u, resp, 4u);
    if (rc >= 0 && (resp[0] & 0xFFu) == ICM_RESP_OK) {
        r->authorised = true;
        proto->approve_count++;
    } else {
        proto->reject_count++;
    }
    return rc;
}

int uiox_tb4_proto_enable_pcie(uiox_tb4_proto_t *proto,
                                uiox_tb4_router_t *r)
{
    if (!proto || !r) return -EINVAL;
    /* Configure PCIe downstream adapter via router config space */
    return uiox_tb4_router_write_cfg(proto->topo, r,
                                      0x10u, 0x00000001u);
}

int uiox_tb4_proto_enable_dp(uiox_tb4_proto_t *proto,
                               uiox_tb4_router_t *r)
{
    if (!proto || !r) return -EINVAL;
    return uiox_tb4_router_write_cfg(proto->topo, r,
                                      0x20u, 0x00000001u);
}

int uiox_tb4_proto_enable_usb(uiox_tb4_proto_t *proto,
                                uiox_tb4_router_t *r)
{
    if (!proto || !r) return -EINVAL;
    return uiox_tb4_router_write_cfg(proto->topo, r,
                                      0x30u, 0x00000001u);
}
