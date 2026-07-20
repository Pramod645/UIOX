/**
 * @file    uiox_wifi_proto.h
 * @brief   UIOX WiFi 802.11 protocol layer.
 *
 * Implements:
 *   - 802.11 MAC frame format (data/mgmt/ctrl)
 *   - Beacon parsing (SSID, capabilities, RSN IE)
 *   - Probe request/response
 *   - Authentication (Open System, SAE stub)
 *   - Association / reassociation
 *   - Disassociation / deauthentication
 *   - MSDU/MPDU encapsulation (LLC/SNAP)
 *   - TSF synchronisation
 *
 * @date    2026-05-28
 */
//Layer 3 — Protocol
 #ifndef UIOX_WIFI_PROTO_H
 #define UIOX_WIFI_PROTO_H
 
 #include "uiox_wifi_if.h"
 #include "uiox_wifi_sec.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * 802.11 Frame Control field definitions
  * ====================================================================== */
 
 #define UIOX_80211_FC_TYPE_MGMT     0x00u
 #define UIOX_80211_FC_TYPE_CTRL     0x04u
 #define UIOX_80211_FC_TYPE_DATA     0x08u
 
 #define UIOX_80211_FC_STYPE_ASSOC_REQ   0x00u
 #define UIOX_80211_FC_STYPE_ASSOC_RSP   0x10u
 #define UIOX_80211_FC_STYPE_PROBE_REQ   0x40u
 #define UIOX_80211_FC_STYPE_PROBE_RSP   0x50u
 #define UIOX_80211_FC_STYPE_BEACON      0x80u
 #define UIOX_80211_FC_STYPE_DISASSOC    0xA0u
 #define UIOX_80211_FC_STYPE_AUTH        0xB0u
 #define UIOX_80211_FC_STYPE_DEAUTH      0xC0u
 #define UIOX_80211_FC_STYPE_DATA        0x00u
 #define UIOX_80211_FC_STYPE_QOS_DATA    0x80u
 
 #define UIOX_80211_FC_TODS              (1u << 8)
 #define UIOX_80211_FC_FROMDS            (1u << 9)
 #define UIOX_80211_FC_PROTECTED         (1u << 14)
 
 /* =========================================================================
  * 802.11 Information Element IDs
  * ====================================================================== */
 
 #define UIOX_IE_SSID                0x00u
 #define UIOX_IE_RATES               0x01u
 #define UIOX_IE_CHANNEL             0x03u
 #define UIOX_IE_RSN                 0x30u
 #define UIOX_IE_EXT_RATES           0x32u
 #define UIOX_IE_HT_CAPS             0x2Du
 #define UIOX_IE_HT_OP               0x3Du
 #define UIOX_IE_VHT_CAPS            0xBFu
 #define UIOX_IE_VENDOR              0xDDu
 
 /* =========================================================================
  * LLC/SNAP header (RFC 1042 encapsulation)
  * ====================================================================== */
 
 #define UIOX_LLC_SNAP_LEN           8
 static const uint8_t UIOX_LLC_SNAP_HDR[UIOX_LLC_SNAP_LEN] =
     { 0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 };
 
 /* =========================================================================
  * Scanned BSS entry
  * ====================================================================== */
 
 #define UIOX_WIFI_SSID_MAX          32
 #define UIOX_WIFI_MAX_BSS           32
 
 typedef struct {
     uiox_wifi_mac_t bssid;
     char            ssid[UIOX_WIFI_SSID_MAX + 1];
     uint8_t         ssid_len;
     uint8_t         channel;
     int8_t          rssi_dbm;
     uint16_t        caps;          /**< 802.11 capability info field       */
     uint8_t         cipher;        /**< UIOX_WIFI_CIPHER_* detected        */
     uint8_t         akm;           /**< UIOX_WIFI_AKM_* detected           */
     uint32_t        beacon_int_tu; /**< Beacon interval (TU = 1024 µs)    */
     uint64_t        tsf;           /**< BSS timestamp (µs)                 */
     bool            valid;
     uint32_t        last_seen_ms;
 } uiox_wifi_bss_t;
 
 /* =========================================================================
  * Connection state machine
  * ====================================================================== */
 
 typedef enum {
     UIOX_WIFI_STATE_IDLE = 0,
     UIOX_WIFI_STATE_SCANNING,
     UIOX_WIFI_STATE_AUTHENTICATING,
     UIOX_WIFI_STATE_ASSOCIATING,
     UIOX_WIFI_STATE_4WAY_HS,
     UIOX_WIFI_STATE_CONNECTED,
     UIOX_WIFI_STATE_DISCONNECTING,
     UIOX_WIFI_STATE_ROAMING,
 } uiox_wifi_conn_state_t;
 
 /* =========================================================================
  * Protocol context
  * ====================================================================== */
 
 typedef struct {
     uiox_wifi_if_t        *wif;
     uiox_wifi_sec_t        sec;
     uiox_wifi_conn_state_t state;
     uiox_wifi_bss_t        bss_cache[UIOX_WIFI_MAX_BSS];
     uint8_t                bss_count;
     uiox_wifi_bss_t        current_bss;
     uint16_t               assoc_id;      /**< AID from AP                */
     uint16_t               seq_num;       /**< MAC sequence number        */
     uint32_t               beacon_miss;   /**< Consecutive beacon misses  */
     uint32_t               scan_timeout_ms;
 } uiox_wifi_proto_t;
 
 /* =========================================================================
  * Protocol API
  * ====================================================================== */
 
 int  uiox_wifi_proto_init      (uiox_wifi_proto_t *proto,
                                  uiox_wifi_if_t    *wif);
 
 /** Start passive/active scan. Results available via bss_cache. */
 int  uiox_wifi_proto_scan      (uiox_wifi_proto_t *proto,
                                  uint32_t timeout_ms);
 
 /** Connect to a BSS (auth → assoc → 4-way HS). */
 int  uiox_wifi_proto_connect   (uiox_wifi_proto_t *proto,
                                  const char        *ssid,
                                  const char        *passphrase,
                                  uint32_t           timeout_ms);
 
 /** Disconnect (send deauth frame). */
 int  uiox_wifi_proto_disconnect(uiox_wifi_proto_t *proto);
 
 /** Send a data MSDU (LLC/SNAP encapsulated). */
 int  uiox_wifi_proto_tx_data   (uiox_wifi_proto_t  *proto,
                                  const uiox_wifi_mac_t dst,
                                  uint16_t            ethertype,
                                  const uint8_t      *payload,
                                  uint16_t            len);
 
 /** Process incoming frame — dispatch mgmt/data. Returns consumed frame. */
 int  uiox_wifi_proto_rx        (uiox_wifi_proto_t  *proto,
                                  uiox_wifi_frame_t  *frame,
                                  uiox_wifi_frame_t **data_out);
 
 /** Periodic tick — beacon watchdog, retransmit, scan timeout. */
 void uiox_wifi_proto_tick      (uiox_wifi_proto_t *proto,
                                  uint32_t           now_ms);
 
 /* -------------------------------------------------------------------------
  * Frame builders (internal, exposed for subsystem use)
  * ---------------------------------------------------------------------- */
 
 uiox_wifi_frame_t *uiox_wifi_proto_build_probe_req(
     uiox_wifi_proto_t *proto, const char *ssid);
 
 uiox_wifi_frame_t *uiox_wifi_proto_build_auth(
     uiox_wifi_proto_t *proto);
 
 uiox_wifi_frame_t *uiox_wifi_proto_build_assoc_req(
     uiox_wifi_proto_t *proto);
 
 uiox_wifi_frame_t *uiox_wifi_proto_build_deauth(
     uiox_wifi_proto_t *proto, uint16_t reason);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_WIFI_PROTO_H */
 
 