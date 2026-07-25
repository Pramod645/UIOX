/**
 * @file    uiox_bt_mgr.h
 * @brief   UIOX Bluetooth device manager (scan, pair, connect).
 * @date    2026-06-09
 */

 #ifndef UIOX_BT_MGR_H
 #define UIOX_BT_MGR_H
 
 #include "uiox_bt_if.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_BT_MAX_DEVICES     16
 #define UIOX_BT_NAME_MAX        64
 #define UIOX_BT_CONN_MAX        7   /**< Max simultaneous connections     */
 
 /* =========================================================================
  * Device type flags
  * ====================================================================== */
 
 #define UIOX_BT_DEV_CLASSIC     (1u << 0)
 #define UIOX_BT_DEV_BLE         (1u << 1)
 #define UIOX_BT_DEV_DUAL_MODE   (UIOX_BT_DEV_CLASSIC | UIOX_BT_DEV_BLE)
 
 /* =========================================================================
  * Connection state
  * ====================================================================== */
 
 typedef enum {
     UIOX_BT_CONN_DISCONNECTED = 0,
     UIOX_BT_CONN_CONNECTING,
     UIOX_BT_CONN_CONNECTED,
     UIOX_BT_CONN_PAIRING,
     UIOX_BT_CONN_PAIRED,
     UIOX_BT_CONN_DISCONNECTING,
 } uiox_bt_conn_state_t;
 
 /* =========================================================================
  * Remote device record
  * ====================================================================== */
 
 typedef struct {
     uiox_bt_addr_t      addr;
     char                name[UIOX_BT_NAME_MAX];
     uint32_t            cod;         /**< Class of Device (classic BT)    */
     uint16_t            appearance;  /**< BLE appearance value            */
     int8_t              rssi;        /**< Last seen RSSI (dBm)            */
     uint8_t             dev_type;    /**< UIOX_BT_DEV_* flags             */
     uiox_bt_conn_state_t conn_state;
     uint16_t            handle;      /**< Connection handle (0=none)      */
     bool                paired;
     bool                trusted;
     uint8_t             ltk[16];     /**< Long-term key (BLE security)    */
     bool                ltk_valid;
     bool                valid;
 } uiox_bt_remote_dev_t;
 
 /* =========================================================================
  * Device manager context
  * ====================================================================== */
 
 typedef struct {
     uiox_bt_if_t           *bif;
     uiox_bt_remote_dev_t    devices[UIOX_BT_MAX_DEVICES];
     uint8_t                 num_devices;
     bool                    scanning;
     bool                    advertising;
     bool                    discoverable;
     bool                    connectable;
     char                    local_name[UIOX_BT_NAME_MAX];
     uiox_bt_addr_t          local_addr;
 } uiox_bt_mgr_t;
 
 /* =========================================================================
  * Manager API
  * ====================================================================== */
 
 int  uiox_bt_mgr_init        (uiox_bt_mgr_t *mgr, uiox_bt_if_t *bif);
 int  uiox_bt_mgr_init_ctrl   (uiox_bt_mgr_t *mgr);
 int  uiox_bt_mgr_set_name    (uiox_bt_mgr_t *mgr, const char *name);
 int  uiox_bt_mgr_set_disc    (uiox_bt_mgr_t *mgr, bool disc, bool conn);
 
 /** Start classic BT inquiry scan. */
 int  uiox_bt_mgr_scan_start  (uiox_bt_mgr_t *mgr, uint8_t duration_s);
 int  uiox_bt_mgr_scan_stop   (uiox_bt_mgr_t *mgr);
 
 /** Start BLE advertising. */
 int  uiox_bt_mgr_adv_start   (uiox_bt_mgr_t *mgr,
                                const uint8_t *adv_data, uint8_t adv_len);
 int  uiox_bt_mgr_adv_stop    (uiox_bt_mgr_t *mgr);
 
 /** Start BLE scan. */
 int  uiox_bt_mgr_le_scan_start(uiox_bt_mgr_t *mgr,
                                 uint16_t interval, uint16_t window,
                                 bool active);
 int  uiox_bt_mgr_le_scan_stop (uiox_bt_mgr_t *mgr);
 
 /** Connect to a device. */
 int  uiox_bt_mgr_connect     (uiox_bt_mgr_t *mgr,
                                const uiox_bt_addr_t addr,
                                bool is_ble);
 int  uiox_bt_mgr_disconnect  (uiox_bt_mgr_t *mgr, uint16_t handle);
 
 /** Process incoming HCI events and update device table. */
 void uiox_bt_mgr_process_evt (uiox_bt_mgr_t *mgr,
                                const uiox_bt_pkt_t *evt);
 
 uiox_bt_remote_dev_t *uiox_bt_mgr_find(uiox_bt_mgr_t *mgr,
                                          const uiox_bt_addr_t addr);
 void uiox_bt_mgr_print_devices(const uiox_bt_mgr_t *mgr);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BT_MGR_H */
 