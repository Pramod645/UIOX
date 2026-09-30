/* uiox_tb4_proto.h */
#ifndef UIOX_TB4_PROTO_H
#define UIOX_TB4_PROTO_H
#include "uiox_tb4_router.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ICM command opcodes */
#define ICM_OP_DRIVER_READY         0x01u
#define ICM_OP_GET_ROUTE            0x03u
#define ICM_OP_APPROVE_DEVICE       0x04u
#define ICM_OP_CHALLENGE_DEVICE     0x05u
#define ICM_OP_ADD_DEVICE_KEY       0x06u
#define ICM_OP_GET_DOMAIN_UUID      0x08u
#define ICM_OP_DISCONNECT_PCIE_PATHS 0x09u
#define ICM_OP_DRIVER_UNLOAD        0x07u

/* ICM response codes */
#define ICM_RESP_OK                 0x00u
#define ICM_RESP_ERROR              0x01u
#define ICM_RESP_AUTH_NEEDED        0x02u

/* XDomain protocol UUIDs */
#define XDOMAIN_UUID_NETWORKING     0x1u
#define XDOMAIN_UUID_FILESYSM       0x2u

typedef struct {
    uiox_tb4_topo_t  *topo;
    uint8_t           domain_uuid[UIOX_TB4_UUID_LEN];
    bool              driver_ready;
    uint32_t          approve_count;
    uint32_t          reject_count;
} uiox_tb4_proto_t;

int  uiox_tb4_proto_init         (uiox_tb4_proto_t *proto,
                                   uiox_tb4_topo_t  *topo);
int  uiox_tb4_proto_driver_ready (uiox_tb4_proto_t *proto);
int  uiox_tb4_proto_get_domain   (uiox_tb4_proto_t *proto);
int  uiox_tb4_proto_approve_dev  (uiox_tb4_proto_t *proto,
                                   uiox_tb4_router_t *r);
int  uiox_tb4_proto_enable_pcie  (uiox_tb4_proto_t *proto,
                                   uiox_tb4_router_t *r);
int  uiox_tb4_proto_enable_dp    (uiox_tb4_proto_t *proto,
                                   uiox_tb4_router_t *r);
int  uiox_tb4_proto_enable_usb   (uiox_tb4_proto_t *proto,
                                   uiox_tb4_router_t *r);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_TB4_PROTO_H */
