/**
 * @file    uiox_wifi_proto.c
 * @brief   UIOX WiFi 802.11 protocol layer implementation.
 * @date    2026-05-28
 */

 #include "uiox_wifi_proto.h"
 #include <string.h>
 #include <errno.h>
 
 /* =========================================================================
  * MAC frame header builder helpers
  * ====================================================================== */
 
 static uint16_t next_seq(uiox_wifi_proto_t *proto)
 {
     proto->seq_num = (proto->seq_num + 1) & 0x0FFFu;
     return (uint16_t)(proto->seq_num << 4u);
 }
 
 /* Build a minimal 802.11 MAC header onto a frame */
 static int build_mac_hdr(uiox_wifi_frame_t *f,
                           uint16_t fc,
                           const uiox_wifi_mac_t dst,
                           const uiox_wifi_mac_t src,
                           const uiox_wifi_mac_t bssid,
                           uint16_t seq)
 {
     /* 802.11 MAC header = 24 bytes (no QoS) */
     uint8_t *hdr = (uint8_t *)uiox_wifi_buf_push(f, 24u);
     if (!hdr) return -ENOBUFS;
 
     hdr[0] = (uint8_t)(fc & 0xFFu);
     hdr[1] = (uint8_t)(fc >> 8u);
     hdr[2] = 0x00u; hdr[3] = 0x00u; /* Duration */
     memcpy(&hdr[4],  dst,   UIOX_WIFI_MAC_LEN);
     memcpy(&hdr[10], src,   UIOX_WIFI_MAC_LEN);
     memcpy(&hdr[16], bssid, UIOX_WIFI_MAC_LEN);
     hdr[22] = (uint8_t)(seq & 0xFFu);
     hdr[23] = (uint8_t)(seq >> 8u);
     return 0;
 }
 
 /* =========================================================================
  * Init
  * ====================================================================== */
 
 int uiox_wifi_proto_init(uiox_wifi_proto_t *proto, uiox_wifi_if_t *wif)
 {
     if (!proto || !wif) return -EINVAL;
     memset(proto, 0, sizeof(*proto));
     proto->wif   = wif;
     proto->state = UIOX_WIFI_STATE_IDLE;
     return 0;
 }
 
 /* =========================================================================
  * Frame builders
  * ====================================================================== */
 
 uiox_wifi_frame_t *uiox_wifi_proto_build_probe_req(
     uiox_wifi_proto_t *proto, const char *ssid)
 {
     uiox_wifi_frame_t *f = uiox_wifi_buf_alloc_tx();
     if (!f) return NULL;
 
     uint8_t ssid_len = ssid ? (uint8_t)strlen(ssid) : 0u;
 
     /* SSID IE */
     uint8_t *body = (uint8_t *)uiox_wifi_buf_put(f, (uint16_t)(2u + ssid_len));
     if (!body) { uiox_wifi_buf_free(f); return NULL; }
     body[0] = UIOX_IE_SSID;
     body[1] = ssid_len;
     if (ssid_len) memcpy(&body[2], ssid, ssid_len);
 
     /* Supported rates IE */
     uint8_t *rates = (uint8_t *)uiox_wifi_buf_put(f, 10u);
     if (rates) {
         rates[0] = UIOX_IE_RATES; rates[1] = 8;
         rates[2]=0x82; rates[3]=0x84; rates[4]=0x8B; rates[5]=0x96;
         rates[6]=0x24; rates[7]=0x30; rates[8]=0x48; rates[9]=0x6C;
     }
 
     static const uiox_wifi_mac_t broadcast = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
     uint16_t fc = (uint16_t)(UIOX_80211_FC_TYPE_MGMT | UIOX_80211_FC_STYPE_PROBE_REQ);
     build_mac_hdr(f, fc, broadcast, proto->wif->hw->mac_addr,
                   broadcast, next_seq(proto));
     f->type = UIOX_WIFI_FRAME_MGMT;
     return f;
 }
 
 uiox_wifi_frame_t *uiox_wifi_proto_build_auth(uiox_wifi_proto_t *proto)
 {
     uiox_wifi_frame_t *f = uiox_wifi_buf_alloc_tx();
     if (!f) return NULL;
 
     /* Auth body: algorithm=Open(0), seq=1, status=0 */
     uint8_t *body = (uint8_t *)uiox_wifi_buf_put(f, 6u);
     if (!body) { uiox_wifi_buf_free(f); return NULL; }
     body[0]=0x00; body[1]=0x00; /* Open System */
     body[2]=0x01; body[3]=0x00; /* Sequence 1 */
     body[4]=0x00; body[5]=0x00; /* Status 0 = success */
 
     uint16_t fc = (uint16_t)(UIOX_80211_FC_TYPE_MGMT | UIOX_80211_FC_STYPE_AUTH);
     build_mac_hdr(f, fc, proto->current_bss.bssid,
                   proto->wif->hw->mac_addr,
                   proto->current_bss.bssid, next_seq(proto));
     f->type = UIOX_WIFI_FRAME_MGMT;
     return f;
 }
 
 uiox_wifi_frame_t *uiox_wifi_proto_build_assoc_req(uiox_wifi_proto_t *proto)
 {
     uiox_wifi_frame_t *f = uiox_wifi_buf_alloc_tx();
     if (!f) return NULL;
 
     /* Capability info + listen interval */
     uint8_t *fixed = (uint8_t *)uiox_wifi_buf_put(f, 4u);
     if (!fixed) { uiox_wifi_buf_free(f); return NULL; }
     fixed[0]=0x31; fixed[1]=0x04; /* caps: ESS, Privacy, ShortPreamble, ShortSlot */
     fixed[2]=0x0A; fixed[3]=0x00; /* listen interval = 10 beacon periods */
 
     /* SSID IE */
     uint8_t slen = proto->current_bss.ssid_len;
     uint8_t *ssid_ie = (uint8_t *)uiox_wifi_buf_put(f, (uint16_t)(2u + slen));
     if (ssid_ie) {
         ssid_ie[0] = UIOX_IE_SSID; ssid_ie[1] = slen;
         memcpy(&ssid_ie[2], proto->current_bss.ssid, slen);
     }
 
     uint16_t fc = (uint16_t)(UIOX_80211_FC_TYPE_MGMT | UIOX_80211_FC_STYPE_ASSOC_REQ);
     build_mac_hdr(f, fc, proto->current_bss.bssid,
                   proto->wif->hw->mac_addr,
                   proto->current_bss.bssid, next_seq(proto));
     f->type = UIOX_WIFI_FRAME_MGMT;
     return f;
 }
 
 uiox_wifi_frame_t *uiox_wifi_proto_build_deauth(uiox_wifi_proto_t *proto,
                                                   uint16_t reason)
 {
     uiox_wifi_frame_t *f = uiox_wifi_buf_alloc_tx();
     if (!f) return NULL;
     uint8_t *body = (uint8_t *)uiox_wifi_buf_put(f, 2u);
     if (body) { body[0]=(uint8_t)(reason&0xFF); body[1]=(uint8_t)(reason>>8); }
     uint16_t fc = (uint16_t)(UIOX_80211_FC_TYPE_MGMT | UIOX_80211_FC_STYPE_DEAUTH);
     build_mac_hdr(f, fc, proto->current_bss.bssid,
                   proto->wif->hw->mac_addr,
                   proto->current_bss.bssid, next_seq(proto));
     f->type = UIOX_WIFI_FRAME_MGMT;
     return f;
 }
 
 /* =========================================================================
  * Beacon / probe response parser
  * ====================================================================== */
 
 static void parse_beacon(uiox_wifi_proto_t *proto,
                           const uiox_wifi_frame_t *f)
 {
     if (f->len < 24u + 12u) return;   /* MAC hdr + fixed params */
 
     /* Find a free or matching BSS slot */
     const uiox_wifi_mac_t *bssid =
         (const uiox_wifi_mac_t *)(f->data + 16u);  /* addr3 = BSSID */
 
     uiox_wifi_bss_t *slot = NULL;
     for (int i = 0; i < UIOX_WIFI_MAX_BSS; i++) {
         if (!proto->bss_cache[i].valid) { slot = &proto->bss_cache[i]; break; }
         if (memcmp(proto->bss_cache[i].bssid, *bssid, UIOX_WIFI_MAC_LEN) == 0) {
             slot = &proto->bss_cache[i]; break;
         }
     }
     if (!slot) slot = &proto->bss_cache[0];  /* overwrite oldest */
 
     memset(slot, 0, sizeof(*slot));
     memcpy(slot->bssid, *bssid, UIOX_WIFI_MAC_LEN);
     slot->rssi_dbm = f->rssi_dbm;
 
     /* Parse fixed params: TSF(8) + BeaconInt(2) + CapInfo(2) */
     const uint8_t *fixed = f->data + 24u;
     slot->tsf          = (uint64_t)fixed[0] | ((uint64_t)fixed[1]<<8);
     slot->beacon_int_tu= (uint32_t)(fixed[8] | (fixed[9]<<8));
     slot->caps         = (uint16_t)(fixed[10] | (fixed[11]<<8));
 
     /* Parse information elements */
     const uint8_t *ie = fixed + 12u;
     const uint8_t *end = f->data + f->len;
     while (ie + 2 <= end) {
         uint8_t id  = ie[0];
         uint8_t len = ie[1];
         if (ie + 2 + len > end) break;
         switch (id) {
         case UIOX_IE_SSID:
             slot->ssid_len = len < UIOX_WIFI_SSID_MAX ? len : UIOX_WIFI_SSID_MAX;
             memcpy(slot->ssid, ie + 2, slot->ssid_len);
             slot->ssid[slot->ssid_len] = '\0';
             break;
         case UIOX_IE_CHANNEL:
             if (len >= 1) slot->channel = ie[2];
             break;
         case UIOX_IE_RSN:
             /* Minimal RSN IE parse for cipher/AKM */
             if (len >= 8) {
                 slot->cipher = ie[9];   /* pairwise cipher suite selector */
                 slot->akm    = ie[15];  /* AKM suite selector */
             }
             break;
         default: break;
         }
         ie += 2u + len;
     }
     slot->valid = true;
     if (proto->bss_count < UIOX_WIFI_MAX_BSS) proto->bss_count++;
 }
 
 /* =========================================================================
  * Scan
  * ====================================================================== */
 
 int uiox_wifi_proto_scan(uiox_wifi_proto_t *proto, uint32_t timeout_ms)
 {
     if (!proto) return -EINVAL;
     proto->state     = UIOX_WIFI_STATE_SCANNING;
     proto->bss_count = 0;
 
     /* Send probe request on current channel */
     uiox_wifi_frame_t *probe = uiox_wifi_proto_build_probe_req(proto, NULL);
     if (probe) {
         uiox_wifi_if_tx(proto->wif, probe, UIOX_WIFI_AC_BE);
         uiox_wifi_if_tx_flush(proto->wif);
     }
 
     /* Collect beacons/probe responses during timeout window */
     uint32_t waited = 0;
     while (waited < timeout_ms) {
         uiox_wifi_frame_t *rx = uiox_wifi_if_rx(proto->wif);
         if (rx) {
             if (rx->type == UIOX_WIFI_FRAME_MGMT &&
                 rx->len >= 2u) {
                 uint16_t stype = (uint16_t)(rx->data[0] & 0xF0u);
                 if (stype == 0x80u || stype == 0x50u)  /* beacon/probe rsp */
                     parse_beacon(proto, rx);
             }
             uiox_wifi_buf_free(rx);
         }
         /* uiox_os_sleep_ms(1); */
         waited++;
     }
 
     proto->state = UIOX_WIFI_STATE_IDLE;
     return proto->bss_count > 0 ? (int)proto->bss_count : 0;
 }
 
 /* =========================================================================
  * Connect
  * ====================================================================== */
 
 int uiox_wifi_proto_connect(uiox_wifi_proto_t *proto,
                              const char *ssid,
                              const char *passphrase,
                              uint32_t timeout_ms)
 {
     if (!proto || !ssid || !passphrase) return -EINVAL;
 
     /* Find target BSS */
     uiox_wifi_bss_t *target = NULL;
     for (int i = 0; i < proto->bss_count; i++) {
         if (proto->bss_cache[i].valid &&
             strcmp(proto->bss_cache[i].ssid, ssid) == 0) {
             target = &proto->bss_cache[i];
             break;
         }
     }
     if (!target) return -ENOENT;
 
     memcpy(&proto->current_bss, target, sizeof(*target));
 
     /* Init security */
     uiox_wifi_sec_init(&proto->sec, target->cipher, target->akm,
                         target->bssid, proto->wif->hw->mac_addr);
     uiox_wifi_sec_derive_pmk(&proto->sec, passphrase,
                               ssid, (uint8_t)strlen(ssid));
 
     /* Set channel */
     uiox_wifi_channel_t ch = {
         .channel    = target->channel,
         .freq_mhz   = (target->channel <= 14u) ?
                       (uint32_t)(2407u + target->channel * 5u) :
                       (uint32_t)(5000u + target->channel * 5u),
         .bw         = UIOX_WIFI_BW_20MHZ,
         .tx_power_dbm = -1,
         .is_5ghz    = (target->channel > 14u),
     };
     uiox_wifi_hw_set_channel(proto->wif->hw, &ch);
 
     /* Authentication */
     proto->state = UIOX_WIFI_STATE_AUTHENTICATING;
     uiox_wifi_frame_t *auth = uiox_wifi_proto_build_auth(proto);
     if (auth) {
         uiox_wifi_if_tx(proto->wif, auth, UIOX_WIFI_AC_VO);
         uiox_wifi_if_tx_flush(proto->wif);
     }
 
     /* Wait for auth response */
     uint32_t waited = 0;
     bool auth_ok = false;
     while (waited < timeout_ms && !auth_ok) {
         uiox_wifi_frame_t *rx = uiox_wifi_if_rx(proto->wif);
         if (rx) {
             if (rx->len >= 2u) {
                 uint16_t stype = (uint16_t)(rx->data[0] & 0xF0u);
                 if (stype == 0xB0u) auth_ok = true;  /* Auth response */
             }
             uiox_wifi_buf_free(rx);
         }
         waited++;
     }
     if (!auth_ok) { proto->state = UIOX_WIFI_STATE_IDLE; return -ETIMEDOUT; }
 
     /* Association */
     proto->state = UIOX_WIFI_STATE_ASSOCIATING;
     uiox_wifi_frame_t *assoc = uiox_wifi_proto_build_assoc_req(proto);
     if (assoc) {
         uiox_wifi_if_tx(proto->wif, assoc, UIOX_WIFI_AC_VO);
         uiox_wifi_if_tx_flush(proto->wif);
     }
 
     /* Wait for assoc response */
     waited = 0; bool assoc_ok = false;
     while (waited < timeout_ms && !assoc_ok) {
         uiox_wifi_frame_t *rx = uiox_wifi_if_rx(proto->wif);
         if (rx) {
             if (rx->len >= 2u) {
                 uint16_t stype = (uint16_t)(rx->data[0] & 0xF0u);
                 if (stype == 0x10u) {  /* Assoc response */
                     assoc_ok = true;
                     if (rx->len >= 28u)
                         proto->assoc_id = (uint16_t)(rx->data[26] | (rx->data[27] << 8));
                 }
             }
             uiox_wifi_buf_free(rx);
         }
         waited++;
     }
     if (!assoc_ok) { proto->state = UIOX_WIFI_STATE_IDLE; return -ETIMEDOUT; }
 
     /* 4-way handshake */
     proto->state = UIOX_WIFI_STATE_4WAY_HS;
     waited = 0; bool hs_ok = false;
     while (waited < timeout_ms && !hs_ok) {
         uiox_wifi_frame_t *rx = uiox_wifi_if_rx(proto->wif);
         if (rx) {
             uiox_wifi_frame_t *reply = NULL;
             int rc = uiox_wifi_sec_eapol_rx(&proto->sec, rx, &reply);
             if (reply) {
                 uiox_wifi_if_tx(proto->wif, reply, UIOX_WIFI_AC_VO);
                 uiox_wifi_if_tx_flush(proto->wif);
             }
             if (rc == 0 && proto->sec.hs_state == UIOX_WIFI_HS_COMPLETE)
                 hs_ok = true;
             uiox_wifi_buf_free(rx);
         }
         waited++;
     }
     if (!hs_ok) { proto->state = UIOX_WIFI_STATE_IDLE; return -ETIMEDOUT; }
 
     proto->wif->hw->associated = true;
     proto->state = UIOX_WIFI_STATE_CONNECTED;
     return 0;
 }
 
 /* =========================================================================
  * Disconnect
  * ====================================================================== */
 
 int uiox_wifi_proto_disconnect(uiox_wifi_proto_t *proto)
 {
     if (!proto) return -EINVAL;
     proto->state = UIOX_WIFI_STATE_DISCONNECTING;
     uiox_wifi_frame_t *deauth = uiox_wifi_proto_build_deauth(proto, 3u);
     if (deauth) {
         uiox_wifi_if_tx(proto->wif, deauth, UIOX_WIFI_AC_VO);
         uiox_wifi_if_tx_flush(proto->wif);
     }
     proto->wif->hw->associated = false;
     proto->state = UIOX_WIFI_STATE_IDLE;
     return 0;
 }
 
 /* =========================================================================
  * Data TX — LLC/SNAP encapsulated
  * ====================================================================== */
 
 int uiox_wifi_proto_tx_data(uiox_wifi_proto_t *proto,
                              const uiox_wifi_mac_t dst,
                              uint16_t ethertype,
                              const uint8_t *payload,
                              uint16_t len)
 {
     if (!proto || !payload) return -EINVAL;
     if (proto->state != UIOX_WIFI_STATE_CONNECTED) return -ENETDOWN;
 
     uiox_wifi_frame_t *f = uiox_wifi_buf_alloc_tx();
     if (!f) return -ENOMEM;
 
     /* Payload */
     void *p = uiox_wifi_buf_put(f, len);
     if (!p) { uiox_wifi_buf_free(f); return -ENOBUFS; }
     memcpy(p, payload, len);
 
    /* LLC/SNAP + ethertype */
    uint8_t snap_hdr[8];
    memcpy(snap_hdr, UIOX_LLC_SNAP_HDR, 6u);
    snap_hdr[6] = (uint8_t)(ethertype >> 8u);
    snap_hdr[7] = (uint8_t)(ethertype & 0xFFu);
    uint8_t *snap = (uint8_t *)uiox_wifi_buf_push(f, UIOX_LLC_SNAP_LEN);
    if (!snap) { uiox_wifi_buf_free(f); return -ENOBUFS; }
    memcpy(snap, snap_hdr, UIOX_LLC_SNAP_LEN);

    /* CCMP encrypt if security active */
    if (proto->sec.ptk.valid)
        uiox_wifi_sec_ccmp_enc(&proto->sec, f);

    /* 802.11 QoS Data MAC header */
    uint8_t *qhdr = (uint8_t *)uiox_wifi_buf_push(f, 26u);
    if (!qhdr) { uiox_wifi_buf_free(f); return -ENOBUFS; }
    uint16_t fc = (uint16_t)(UIOX_80211_FC_TYPE_DATA     |
                              UIOX_80211_FC_STYPE_QOS_DATA |
                              UIOX_80211_FC_TODS           |
                              (proto->sec.ptk.valid ? UIOX_80211_FC_PROTECTED : 0u));
    qhdr[0] = (uint8_t)(fc & 0xFFu);
    qhdr[1] = (uint8_t)(fc >> 8u);
    qhdr[2] = qhdr[3] = 0u; /* duration */
    memcpy(&qhdr[4],  proto->current_bss.bssid, UIOX_WIFI_MAC_LEN); /* RA */
    memcpy(&qhdr[10], proto->wif->hw->mac_addr,  UIOX_WIFI_MAC_LEN); /* TA */
    memcpy(&qhdr[16], dst,                        UIOX_WIFI_MAC_LEN); /* DA */
    uint16_t seq = next_seq(proto);
    qhdr[22] = (uint8_t)(seq & 0xFFu);
    qhdr[23] = (uint8_t)(seq >> 8u);
    qhdr[24] = 0x00u; /* QoS control lo */
    qhdr[25] = 0x00u; /* QoS control hi */

    f->type = UIOX_WIFI_FRAME_DATA;
    f->ac   = UIOX_WIFI_AC_BE;
    return uiox_wifi_if_tx(proto->wif, f, UIOX_WIFI_AC_BE);
}

/* =========================================================================
 * RX dispatch
 * ====================================================================== */

int uiox_wifi_proto_rx(uiox_wifi_proto_t  *proto,
                        uiox_wifi_frame_t  *frame,
                        uiox_wifi_frame_t **data_out)
{
    if (!proto || !frame) return -EINVAL;
    if (data_out) *data_out = NULL;
    if (frame->len < 2u) return -EINVAL;

    uint8_t  type  = (uint8_t)(frame->data[0] & 0x0Cu);
    uint8_t  stype = (uint8_t)(frame->data[0] & 0xF0u);
    uint16_t fc    = (uint16_t)(frame->data[0] | (frame->data[1] << 8u));

    if (type == UIOX_80211_FC_TYPE_MGMT) {
        if (stype == 0x80u || stype == 0x50u)
            parse_beacon(proto, frame);
        return 0;
    }

    if (type == UIOX_80211_FC_TYPE_DATA) {
        /* Decrypt if protected */
        if (fc & UIOX_80211_FC_PROTECTED)
            uiox_wifi_sec_ccmp_dec(&proto->sec, frame);

        /* Strip MAC header (24 or 26 bytes for QoS) */
        uint16_t mac_hdr_len = (stype == 0x80u) ? 26u : 24u;
        if (frame->len <= mac_hdr_len + UIOX_LLC_SNAP_LEN) return 0;
        uiox_wifi_buf_pull(frame, mac_hdr_len + UIOX_LLC_SNAP_LEN);

        if (data_out) *data_out = frame;
        return 1;
    }

    return 0;
}

void uiox_wifi_proto_tick(uiox_wifi_proto_t *proto, uint32_t now_ms)
{
    if (!proto) return;
    if (proto->state != UIOX_WIFI_STATE_CONNECTED) return;

    /* Beacon watchdog — if >10 beacons missed, trigger roam */
    if (proto->beacon_miss > 10u) {
        proto->state = UIOX_WIFI_STATE_IDLE;
        proto->wif->hw->associated = false;
    }
    (void)now_ms;
}
 