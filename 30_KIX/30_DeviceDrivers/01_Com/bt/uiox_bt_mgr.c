/**
 * @file    uiox_bt_mgr.c
 * @brief   UIOX Bluetooth device manager implementation.
 * @date    2026-06-09
 */

 #include "uiox_bt_mgr.h"
 #include <string.h>
 #include <stdio.h>
 #include <errno.h>
 
 int uiox_bt_mgr_init(uiox_bt_mgr_t *mgr, uiox_bt_if_t *bif)
 {
     if (!mgr || !bif) return -EINVAL;
     memset(mgr, 0, sizeof(*mgr));
     mgr->bif = bif;
     return 0;
 }
 
 int uiox_bt_mgr_init_ctrl(uiox_bt_mgr_t *mgr)
 {
     if (!mgr) return -EINVAL;
     uint8_t resp[16];
     int rc;
 
     /* HCI Reset */
     rc = uiox_bt_if_hci_cmd(mgr->bif, HCI_OP_RESET,
                               NULL, 0, resp, sizeof(resp), 5000u);
     if (rc < 0) return rc;
 
     /* Read local BD address */
     rc = uiox_bt_if_hci_cmd(mgr->bif, HCI_OP_READ_BD_ADDR,
                               NULL, 0, resp, sizeof(resp), 3000u);
     if (rc == 0 && mgr->bif->hw)
         memcpy(mgr->local_addr, &resp[0], UIOX_BT_ADDR_LEN);
     memcpy(mgr->bif->hw->bd_addr, mgr->local_addr, UIOX_BT_ADDR_LEN);
 
     /* Read local version */
     rc = uiox_bt_if_hci_cmd(mgr->bif, HCI_OP_READ_LOCAL_VER,
                               NULL, 0, resp, sizeof(resp), 3000u);
 
     /* Enable Simple Pairing */
     uint8_t sp_en = 0x01u;
     uiox_bt_if_hci_cmd(mgr->bif, HCI_OP_WRITE_SIMPLE_PAIR_MODE,
                         &sp_en, 1u, resp, sizeof(resp), 3000u);
 
     return 0;
 }
 
 int uiox_bt_mgr_set_name(uiox_bt_mgr_t *mgr, const char *name)
 {
     if (!mgr || !name) return -EINVAL;
     strncpy(mgr->local_name, name, UIOX_BT_NAME_MAX - 1);
     uint8_t params[248] = {0};
     strncpy((char*)params, name, 247);
     uint8_t resp[4];
     return uiox_bt_if_hci_cmd(mgr->bif, HCI_OP_WRITE_LOCAL_NAME,
                                 params, 248u, resp, sizeof(resp), 3000u);
 }
 
 int uiox_bt_mgr_set_disc(uiox_bt_mgr_t *mgr, bool disc, bool conn)
 {
     if (!mgr) return -EINVAL;
     mgr->discoverable = disc;
     mgr->connectable  = conn;
     uint8_t scan = (disc ? 0x02u : 0x00u) | (conn ? 0x01u : 0x00u);
     uint8_t resp[4];
     return uiox_bt_if_hci_cmd(mgr->bif, HCI_OP_WRITE_SCAN_ENABLE,
                                 &scan, 1u, resp, sizeof(resp), 3000u);
 }
 
 int uiox_bt_mgr_scan_start(uiox_bt_mgr_t *mgr, uint8_t duration_s)
 {
     if (!mgr) return -EINVAL;
     mgr->scanning = true;
     uint8_t params[5] = {
         duration_s, 0u,   /* lap[0..2] = 0x9E8B33 (GIAC) */
         0x33u, 0x8Bu, 0x9Eu
     };
     uint8_t resp[4];
     return uiox_bt_if_hci_cmd(mgr->bif, HCI_OP_INQUIRY,
                                 params, 5u, resp, sizeof(resp), 5000u);
 }
 
 int uiox_bt_mgr_scan_stop(uiox_bt_mgr_t *mgr)
 {
     if (!mgr) return -EINVAL;
     mgr->scanning = false;
     uint8_t resp[4];
     return uiox_bt_if_hci_cmd(mgr->bif, HCI_OP_INQUIRY_CANCEL,
                                 NULL, 0u, resp, sizeof(resp), 3000u);
 }
 
 int uiox_bt_mgr_adv_start(uiox_bt_mgr_t *mgr,
                             const uint8_t *adv_data, uint8_t adv_len)
 {
     if (!mgr) return -EINVAL;
     uint8_t resp[4];
     /* Set ADV parameters: 100ms interval, undirected, non-connectable */
     uint8_t adv_params[15] = {
         0xA0u,0x00u, /* min interval 160 × 0.625ms = 100ms */
         0xA0u,0x00u, /* max interval */
         0x00u,       /* ADV_IND */
         0x00u,       /* own addr type: public */
         0x00u,       /* peer addr type */
         0,0,0,0,0,0, /* peer addr */
         0x07u,       /* channel map: 37,38,39 */
         0x00u,       /* filter policy */
     };
     uiox_bt_if_hci_cmd(mgr->bif, HCI_OP_LE_SET_ADV_PARAMS,
                         adv_params, sizeof(adv_params),
                         resp, sizeof(resp), 3000u);
     /* Set ADV data */
     uint8_t adv_pkt[32] = {0};
     adv_pkt[0] = adv_len & 0x1Fu;
     if (adv_data && adv_len)
         memcpy(&adv_pkt[1], adv_data, adv_len < 31u ? adv_len : 31u);
     uiox_bt_if_hci_cmd(mgr->bif, HCI_OP_LE_SET_ADV_DATA,
                         adv_pkt, 32u, resp, sizeof(resp), 3000u);
     /* Enable advertising */
     uint8_t en = 0x01u;
     int rc = uiox_bt_if_hci_cmd(mgr->bif, HCI_OP_LE_SET_ADV_ENABLE,
                                   &en, 1u, resp, sizeof(resp), 3000u);
     if (rc == 0) mgr->advertising = true;
     return rc;
 }
 
 int uiox_bt_mgr_adv_stop(uiox_bt_mgr_t *mgr)
 {
     if (!mgr) return -EINVAL;
     uint8_t en = 0x00u;
     uint8_t resp[4];
     int rc = uiox_bt_if_hci_cmd(mgr->bif, HCI_OP_LE_SET_ADV_ENABLE,
                                   &en, 1u, resp, sizeof(resp), 3000u);
     if (rc == 0) mgr->advertising = false;
     return rc;
 }
 
 int uiox_bt_mgr_le_scan_start(uiox_bt_mgr_t *mgr,
                                 uint16_t interval, uint16_t window,
                                 bool active)
 {
     if (!mgr) return -EINVAL;
     uint8_t resp[4];
     /* Set scan parameters */
     uint8_t scan_params[7] = {
         active ? 0x01u : 0x00u,
         (uint8_t)(interval & 0xFFu), (uint8_t)(interval >> 8u),
         (uint8_t)(window   & 0xFFu), (uint8_t)(window   >> 8u),
         0x00u,  /* own addr: public */
         0x00u,  /* filter policy: all */
     };
     uiox_bt_if_hci_cmd(mgr->bif, HCI_OP_LE_SET_SCAN_PARAMS,
                         scan_params, sizeof(scan_params),
                         resp, sizeof(resp), 3000u);
     /* Enable scan */
     uint8_t en[2] = { 0x01u, 0x00u }; /* enable, no filter duplicates */
     int rc = uiox_bt_if_hci_cmd(mgr->bif, HCI_OP_LE_SET_SCAN_ENABLE,
                                   en, 2u, resp, sizeof(resp), 3000u);
     if (rc == 0) mgr->scanning = true;
     return rc;
 }
 
 int uiox_bt_mgr_le_scan_stop(uiox_bt_mgr_t *mgr)
 {
     if (!mgr) return -EINVAL;
     uint8_t en[2] = { 0x00u, 0x00u };
     uint8_t resp[4];
     int rc = uiox_bt_if_hci_cmd(mgr->bif, HCI_OP_LE_SET_SCAN_ENABLE,
                                   en, 2u, resp, sizeof(resp), 3000u);
     if (rc == 0) mgr->scanning = false;
     return rc;
 }
 
 int uiox_bt_mgr_connect(uiox_bt_mgr_t *mgr,
                          const uiox_bt_addr_t addr, bool is_ble)
 {
     if (!mgr || !addr) return -EINVAL;
     uint8_t resp[4];
     if (is_ble) {
         uint8_t params[25] = {
             0x60u,0x00u, /* scan interval */
             0x30u,0x00u, /* scan window */
             0x00u,       /* filter policy */
             0x00u,       /* peer addr type: public */
             addr[0],addr[1],addr[2],addr[3],addr[4],addr[5],
             0x00u,       /* own addr type */
             0x18u,0x00u, /* conn interval min */
             0x28u,0x00u, /* conn interval max */
             0x00u,0x00u, /* latency */
             0x48u,0x00u, /* supervision timeout */
             0x00u,0x00u, /* min CE len */
             0x00u,0x00u, /* max CE len */
         };
         return uiox_bt_if_hci_cmd(mgr->bif, HCI_OP_LE_CREATE_CONN,
                                    params, sizeof(params),
                                    resp, sizeof(resp), 15000u);
     }
     return -ENOTSUP;
 }
 
 int uiox_bt_mgr_disconnect(uiox_bt_mgr_t *mgr, uint16_t handle)
 {
     if (!mgr) return -EINVAL;
     uint8_t params[3] = {
         (uint8_t)(handle & 0xFFu),
         (uint8_t)(handle >> 8u),
         0x13u  /* reason: remote user terminated */
     };
     uint8_t resp[4];
     return uiox_bt_if_hci_cmd(mgr->bif, 0x0406u,
                                 params, 3u, resp, sizeof(resp), 5000u);
 }
 
 uiox_bt_remote_dev_t *uiox_bt_mgr_find(uiox_bt_mgr_t *mgr,
                                          const uiox_bt_addr_t addr)
 {
     if (!mgr || !addr) return NULL;
     for (uint8_t i = 0; i < mgr->num_devices; i++)
         if (mgr->devices[i].valid &&
             memcmp(mgr->devices[i].addr, addr, UIOX_BT_ADDR_LEN) == 0)
             return &mgr->devices[i];
     return NULL;
 }
 
 static uiox_bt_remote_dev_t *find_or_add(uiox_bt_mgr_t *mgr,
                                            const uiox_bt_addr_t addr)
 {
     uiox_bt_remote_dev_t *d = uiox_bt_mgr_find(mgr, addr);
     if (d) return d;
     if (mgr->num_devices >= UIOX_BT_MAX_DEVICES) return NULL;
     d = &mgr->devices[mgr->num_devices++];
     memset(d, 0, sizeof(*d));
     memcpy(d->addr, addr, UIOX_BT_ADDR_LEN);
     d->valid = true;
     return d;
 }
 
 void uiox_bt_mgr_process_evt(uiox_bt_mgr_t *mgr,
                                const uiox_bt_pkt_t *evt)
 {
     if (!mgr || !evt || evt->pkt_type != HCI_EVENT_PKT) return;
     const uint8_t *p = evt->data;
     uint8_t ev_code  = p[1];
 
     switch (ev_code) {
     case HCI_EV_INQUIRY_RESULT: {
         uint8_t num = p[3];
         for (uint8_t i = 0; i < num; i++) {
             const uint8_t *d = &p[4 + i*14u];
             uiox_bt_remote_dev_t *r = find_or_add(mgr, d);
             if (r) {
                 r->dev_type  = UIOX_BT_DEV_CLASSIC;
                 r->cod       = (uint32_t)(d[9] | (d[10]<<8u) | (d[11]<<16u));
                 r->rssi      = (int8_t)d[13];
             }
         }
         break;
     }
     case HCI_EV_CONN_COMPLETE: {
         /* status(1) handle(2) addr(6) type(1) enc(1) */
         if (p[3] == 0u) {
             uint16_t h = (uint16_t)(p[4] | (p[5] << 8u));
             uiox_bt_remote_dev_t *r = find_or_add(mgr, &p[6]);
             if (r) { r->handle = h; r->conn_state = UIOX_BT_CONN_CONNECTED; }
         }
         break;
     }
     case HCI_EV_DISCONN_COMPLETE: {
         uint16_t h = (uint16_t)(p[4] | (p[5] << 8u));
         for (uint8_t i = 0; i < mgr->num_devices; i++) {
             if (mgr->devices[i].handle == h) {
                 mgr->devices[i].handle     = 0;
                 mgr->devices[i].conn_state = UIOX_BT_CONN_DISCONNECTED;
             }
         }
         break;
     }
     case HCI_EV_LE_META: {
         uint8_t sub = p[3];
         if (sub == HCI_LE_EV_ADV_REPORT) {
             /* num_reports(1) type(1) addr_type(1) addr(6) data_len(1)... */
             uint8_t num = p[4];
             uint32_t off = 5u;
             for (uint8_t i = 0; i < num; i++) {
                 uint8_t addr_type = p[off+1];
                 const uint8_t *addr = &p[off+2];
                 int8_t rssi = (int8_t)p[off + 9u + p[off+8u]];
                 uiox_bt_remote_dev_t *r = find_or_add(mgr, addr);
                 if (r) {
                     r->dev_type = UIOX_BT_DEV_BLE;
                     r->rssi     = rssi;
                     (void)addr_type;
                 }
                 off += 9u + p[off+8u] + 1u;
             }
         } else if (sub == HCI_LE_EV_CONN_COMPLETE) {
             if (p[4] == 0u) {
                 uint16_t h = (uint16_t)(p[5] | (p[6] << 8u));
                 uiox_bt_remote_dev_t *r = find_or_add(mgr, &p[8]);
                 if (r) { r->handle = h; r->conn_state = UIOX_BT_CONN_CONNECTED; }
             }
         }
         break;
     }
     default: break;
     }
 }
 
 void uiox_bt_mgr_print_devices(const uiox_bt_mgr_t *mgr)
 {
     if (!mgr) return;
     static const char *conn_names[] = {
         "DISCONNECTED","CONNECTING","CONNECTED",
         "PAIRING","PAIRED","DISCONNECTING"
     };
     printf("  Discovered devices (%u):\n", mgr->num_devices);
     for (uint8_t i = 0; i < mgr->num_devices; i++) {
         const uiox_bt_remote_dev_t *d = &mgr->devices[i];
         if (!d->valid) continue;
         printf("    [%u] %02X:%02X:%02X:%02X:%02X:%02X"
                "  rssi=%3d dBm  type=%s  %s  name='%s'\n",
                i,
                d->addr[5],d->addr[4],d->addr[3],
                d->addr[2],d->addr[1],d->addr[0],
                d->rssi,
                (d->dev_type & UIOX_BT_DEV_CLASSIC) ? "Classic" : "BLE",
                conn_names[d->conn_state < 6u ? d->conn_state : 0u],
                d->name[0] ? d->name : "(unknown)");
     }
 }
 