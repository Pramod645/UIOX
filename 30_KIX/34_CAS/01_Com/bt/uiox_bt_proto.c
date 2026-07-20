/* uiox_bt_proto.c */
#include "uiox_bt_proto.h"
#include <string.h>
#include <errno.h>

int uiox_bt_proto_init(uiox_bt_proto_t *proto, uiox_bt_mgr_t *mgr)
{
    if (!proto || !mgr) return -EINVAL;
    memset(proto, 0, sizeof(*proto));
    proto->mgr = mgr;
    return 0;
}

int uiox_bt_proto_gatt_discover(uiox_bt_proto_t *proto,
                                  uint16_t conn_handle)
{
    if (!proto) return -EINVAL;
    /* ATT Read By Group Type Request: discover primary services */
    uint8_t attdata[] = {
        0x10,           /* ATT_READ_BY_GROUP_TYPE_REQ */
        0x01,0x00,      /* start handle */
        0xFF,0xFF,      /* end handle */
        0x00,0x28       /* primary service UUID (little-endian) */
    };
    return uiox_bt_proto_l2cap_tx(proto, conn_handle,
                                   L2CAP_CID_GATT,
                                   attdata, sizeof(attdata));
}

int uiox_bt_proto_gatt_write(uiox_bt_proto_t *proto,
                               uint16_t conn_handle,
                               uint16_t attr_handle,
                               const uint8_t *data, uint16_t len)
{
    if (!proto || !data) return -EINVAL;
    uint8_t pkt[64];
    pkt[0] = 0x12u; /* ATT_WRITE_REQ */
    pkt[1] = (uint8_t)(attr_handle & 0xFFu);
    pkt[2] = (uint8_t)(attr_handle >> 8u);
    uint16_t copy = len < 61u ? len : 61u;
    memcpy(&pkt[3], data, copy);
    return uiox_bt_proto_l2cap_tx(proto, conn_handle,
                                   L2CAP_CID_GATT, pkt, (uint16_t)(3u+copy));
}

int uiox_bt_proto_gatt_read(uiox_bt_proto_t *proto,
                              uint16_t conn_handle,
                              uint16_t attr_handle,
                              uint8_t *data_out, uint16_t *len_out)
{
    if (!proto || !data_out || !len_out) return -EINVAL;
    uint8_t pkt[3];
    pkt[0] = 0x0Au; /* ATT_READ_REQ */
    pkt[1] = (uint8_t)(attr_handle & 0xFFu);
    pkt[2] = (uint8_t)(attr_handle >> 8u);
    *len_out = 0;
    return uiox_bt_proto_l2cap_tx(proto, conn_handle,
                                   L2CAP_CID_GATT, pkt, 3u);
}

int uiox_bt_proto_l2cap_tx(uiox_bt_proto_t *proto,
                             uint16_t conn_handle,
                             uint16_t cid,
                             const uint8_t *data, uint16_t len)
{
    if (!proto || !data) return -EINVAL;
    uint8_t pkt[UIOX_BT_PKT_MAX_LEN];
    /* L2CAP header: len(2) + CID(2) */
    pkt[0] = (uint8_t)(len & 0xFFu);
    pkt[1] = (uint8_t)(len >> 8u);
    pkt[2] = (uint8_t)(cid & 0xFFu);
    pkt[3] = (uint8_t)(cid >> 8u);
    uint16_t copy = len < (UIOX_BT_PKT_MAX_LEN - 4u) ?
                    len : (UIOX_BT_PKT_MAX_LEN - 4u);
    memcpy(&pkt[4], data, copy);
    return uiox_bt_if_acl_tx(proto->mgr->bif,
                              conn_handle, pkt, (uint16_t)(4u + copy));
}
