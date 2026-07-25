/**
 * @file    uiox_wifi_sec.h
 * @brief   UIOX WiFi security layer (WPA2/WPA3, CCMP, EAPOL, PMKSA).
 *
 * Implements:
 *   - WPA2-Personal (PSK) 4-way handshake
 *   - WPA3-Personal (SAE) authentication stub
 *   - CCMP (AES-128-CCM) software encrypt/decrypt
 *   - TKIP legacy support (RC4-based, deprecated)
 *   - PMKSA cache (Pairwise Master Key Security Association)
 *   - GTK/PTK key derivation (PRF-512)
 *
 * @date    2026-05-28
 */
//Layer 2b — Security
 #ifndef UIOX_WIFI_SEC_H
 #define UIOX_WIFI_SEC_H
 
 #include "uiox_wifi_hw.h"
 #include "uiox_wifi_buf.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Security suite selectors (IEEE 802.11-2020 Table 9-174)
  * ====================================================================== */
 
 #define UIOX_WIFI_CIPHER_NONE       0x00u
 #define UIOX_WIFI_CIPHER_WEP40      0x01u
 #define UIOX_WIFI_CIPHER_TKIP       0x02u
 #define UIOX_WIFI_CIPHER_CCMP128    0x04u  /**< AES-CCMP-128 (WPA2 default)*/
 #define UIOX_WIFI_CIPHER_CCMP256    0x09u
 #define UIOX_WIFI_CIPHER_GCMP128    0x08u  /**< AES-GCMP-128 (WPA3)        */
 #define UIOX_WIFI_CIPHER_GCMP256    0x0Cu
 
 #define UIOX_WIFI_AKM_PSK           0x02u  /**< WPA2-Personal              */
 #define UIOX_WIFI_AKM_SAE           0x08u  /**< WPA3-Personal              */
 #define UIOX_WIFI_AKM_8021X         0x01u  /**< WPA2-Enterprise            */
 
 /* =========================================================================
  * Key lengths
  * ====================================================================== */
 
 #define UIOX_WIFI_PMK_LEN           32
 #define UIOX_WIFI_PTK_LEN           48    /**< KCK(16) + KEK(16) + TK(16) */
 #define UIOX_WIFI_GTK_MAX_LEN       32
 #define UIOX_WIFI_NONCE_LEN         32
 #define UIOX_WIFI_MIC_LEN           16
 #define UIOX_WIFI_SSID_MAX_LEN      32
 
 /* =========================================================================
  * PTK key structure
  * ====================================================================== */
 
 typedef struct {
     uint8_t kck[16];   /**< Key Confirmation Key                           */
     uint8_t kek[16];   /**< Key Encryption Key                             */
     uint8_t tk [16];   /**< Temporal Key (used for CCMP)                   */
     bool    valid;
 } uiox_wifi_ptk_t;
 
 /* =========================================================================
  * GTK
  * ====================================================================== */
 
 typedef struct {
     uint8_t  key[UIOX_WIFI_GTK_MAX_LEN];
     uint8_t  key_len;
     uint8_t  key_id;  /**< GTK index (1..3)                                */
     uint8_t  cipher;
     bool     valid;
 } uiox_wifi_gtk_t;
 
 /* =========================================================================
  * PMKSA cache entry
  * ====================================================================== */
 
 #define UIOX_WIFI_PMKSA_MAX         4
 #define UIOX_WIFI_PMKID_LEN         16
 
 typedef struct {
     uint8_t         pmk[UIOX_WIFI_PMK_LEN];
     uint8_t         pmkid[UIOX_WIFI_PMKID_LEN];
     uiox_wifi_mac_t bssid;
     uint32_t        created_s;   /**< Creation time (seconds since boot)   */
     uint32_t        lifetime_s;  /**< Validity lifetime (0 = forever)      */
     bool            valid;
 } uiox_wifi_pmksa_t;
 
 /* =========================================================================
  * 4-way handshake state
  * ====================================================================== */
 
 typedef enum {
     UIOX_WIFI_HS_IDLE = 0,
     UIOX_WIFI_HS_MSG1_SENT,
     UIOX_WIFI_HS_MSG2_SENT,
     UIOX_WIFI_HS_MSG3_SENT,
     UIOX_WIFI_HS_COMPLETE,
     UIOX_WIFI_HS_FAILED,
 } uiox_wifi_hs_state_t;
 
 /* =========================================================================
  * Security context
  * ====================================================================== */
 
 typedef struct {
     uint8_t              cipher;
     uint8_t              akm;
     uint8_t              pmk[UIOX_WIFI_PMK_LEN];
     uiox_wifi_ptk_t      ptk;
     uiox_wifi_gtk_t      gtk;
     uiox_wifi_hs_state_t hs_state;
     uiox_wifi_mac_t      bssid;
     uiox_wifi_mac_t      own_mac;
     uint8_t              anonce[UIOX_WIFI_NONCE_LEN];
     uint8_t              snonce[UIOX_WIFI_NONCE_LEN];
     uint64_t             replay_counter;
     uiox_wifi_pmksa_t    pmksa[UIOX_WIFI_PMKSA_MAX];
     uint32_t             tx_pn;   /**< Transmit packet number (CCMP PN)    */
     uint32_t             rx_pn;   /**< Last received PN (replay protection)*/
 } uiox_wifi_sec_t;
 
 /* =========================================================================
  * Security API
  * ====================================================================== */
 
 int  uiox_wifi_sec_init    (uiox_wifi_sec_t *sec,
                              uint8_t cipher, uint8_t akm,
                              const uiox_wifi_mac_t bssid,
                              const uiox_wifi_mac_t own_mac);
 
 /** Derive PMK from passphrase + SSID (PBKDF2-SHA1, 4096 iterations). */
 int  uiox_wifi_sec_derive_pmk(uiox_wifi_sec_t *sec,
                                const char *passphrase,
                                const char *ssid,
                                uint8_t     ssid_len);
 
 /** Derive PTK from PMK + nonces + MACs (PRF-512). */
 int  uiox_wifi_sec_derive_ptk(uiox_wifi_sec_t *sec,
                                const uint8_t *anonce,
                                const uint8_t *snonce);
 
 /** Process incoming EAPOL frame; advance 4-way handshake state. */
 int  uiox_wifi_sec_eapol_rx  (uiox_wifi_sec_t    *sec,
                                uiox_wifi_frame_t  *frame,
                                uiox_wifi_frame_t **reply_out);
 
 /** Encrypt frame payload with CCMP-128 (in-place). */
 int  uiox_wifi_sec_ccmp_enc  (uiox_wifi_sec_t   *sec,
                                uiox_wifi_frame_t *frame);
 
 /** Decrypt and verify CCMP-128 frame (in-place). */
 int  uiox_wifi_sec_ccmp_dec  (uiox_wifi_sec_t   *sec,
                                uiox_wifi_frame_t *frame);
 
 /** Store/refresh a PMKSA entry. */
 void uiox_wifi_sec_pmksa_add (uiox_wifi_sec_t *sec,
                                const uint8_t *pmk,
                                const uiox_wifi_mac_t bssid,
                                uint32_t now_s);
 
 /** Lookup PMKSA for given BSSID. Returns NULL if not found/expired. */
 const uiox_wifi_pmksa_t *uiox_wifi_sec_pmksa_lookup(
     const uiox_wifi_sec_t *sec,
     const uiox_wifi_mac_t bssid,
     uint32_t now_s);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_WIFI_SEC_H */
 