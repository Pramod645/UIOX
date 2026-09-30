/* uiox_bt_proto.h */
#ifndef UIOX_BT_PROTO_H
#define UIOX_BT_PROTO_H
#include "uiox_bt_mgr.h"
#ifdef __cplusplus
extern "C" {
#endif
/* L2CAP channel IDs */
#define L2CAP_CID_SIGNALLING    0x0001u
#define L2CAP_CID_GATT          0x0004u
#define L2CAP_CID_LE_SIGNALLING 0x0005u
#define L2CAP_CID_SM            0x0006u
/* GATT attribute types */
#define GATT_PRIMARY_SERVICE    0x2800u
#define GATT_CHARACTERISTIC     0x2803u
#define GATT_CCC_DESCRIPTOR     0x2902u
/* Common GATT service UUIDs */
#define GATT_SVC_GENERIC_ACCESS 0x1800u
#define GATT_SVC_BATTERY        0x180Fu
#define GATT_SVC_HID            0x1812u
#define GATT_SVC_HEART_RATE     0x180Du
#define GATT_CHAR_BATTERY_LEVEL 0x2A19u
#define GATT_CHAR_DEVICE_NAME   0x2A00u

typedef struct {
    uiox_bt_mgr_t *mgr;
    uint32_t       gatt_handle_count;
} uiox_bt_proto_t;

int  uiox_bt_proto_init         (uiox_bt_proto_t *proto,
                                  uiox_bt_mgr_t *mgr);
int  uiox_bt_proto_gatt_discover(uiox_bt_proto_t *proto,
                                  uint16_t conn_handle);
int  uiox_bt_proto_gatt_write   (uiox_bt_proto_t *proto,
                                  uint16_t conn_handle,
                                  uint16_t attr_handle,
                                  const uint8_t *data, uint16_t len);
int  uiox_bt_proto_gatt_read    (uiox_bt_proto_t *proto,
                                  uint16_t conn_handle,
                                  uint16_t attr_handle,
                                  uint8_t *data_out, uint16_t *len_out);
int  uiox_bt_proto_l2cap_tx     (uiox_bt_proto_t *proto,
                                  uint16_t conn_handle,
                                  uint16_t cid,
                                  const uint8_t *data, uint16_t len);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_BT_PROTO_H */
