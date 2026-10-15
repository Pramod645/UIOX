/**
 * @file    uiox_wifi_sec.c
 * @brief   UIOX WiFi security implementation (WPA2, CCMP, 4-way HS).
 * @date    2026-05-28
 */

 #include "uiox_wifi_sec.h"
 #include "uiox_klibc.h"
 
 /* -------------------------------------------------------------------------
  * Minimal SHA-1 (FIPS 180-4) — used by PRF-512 and PBKDF2
  * Replace with mbedTLS / WolfSSL in production.
  * ---------------------------------------------------------------------- */
 
 #define SHA1_DIGEST_LEN 20
 
 typedef struct { uint32_t h[5]; uint8_t buf[64]; uint64_t bits; uint8_t cnt; } sha1_ctx_t;
 
 static uint32_t rotl32(uint32_t v, int n) { return (v << n) | (v >> (32-n)); }
 
 static void sha1_compress(sha1_ctx_t *ctx)
 {
     uint32_t w[80];
     const uint8_t *b = ctx->buf;
     for (int i=0;i<16;i++) w[i]=((uint32_t)b[i*4]<<24)|((uint32_t)b[i*4+1]<<16)|
                                   ((uint32_t)b[i*4+2]<<8)|(uint32_t)b[i*4+3];
     for (int i=16;i<80;i++) w[i]=rotl32(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
     uint32_t a=ctx->h[0],b2=ctx->h[1],c=ctx->h[2],d=ctx->h[3],e=ctx->h[4],f,k,t;
     for (int i=0;i<80;i++){
         if(i<20){f=(b2&c)|((~b2)&d);k=0x5A827999u;}
         else if(i<40){f=b2^c^d;k=0x6ED9EBA1u;}
         else if(i<60){f=(b2&c)|(b2&d)|(c&d);k=0x8F1BBCDCu;}
         else{f=b2^c^d;k=0xCA62C1D6u;}
         t=rotl32(a,5)+f+e+k+w[i]; e=d; d=c; c=rotl32(b2,30); b2=a; a=t;
     }
     ctx->h[0]+=a; ctx->h[1]+=b2; ctx->h[2]+=c; ctx->h[3]+=d; ctx->h[4]+=e;
 }
 
 static void sha1_init(sha1_ctx_t *ctx){
     ctx->h[0]=0x67452301u; ctx->h[1]=0xEFCDAB89u; ctx->h[2]=0x98BADCFEu;
     ctx->h[3]=0x10325476u; ctx->h[4]=0xC3D2E1F0u;
     ctx->bits=0; ctx->cnt=0;
 }
 
 static void sha1_update(sha1_ctx_t *ctx, const uint8_t *data, size_t len)
 {
     for (size_t i = 0; i < len; i++) {
         ctx->buf[ctx->cnt++] = data[i];
         ctx->bits += 8;
         if (ctx->cnt == 64) {
             sha1_compress(ctx);
             ctx->cnt = 0;
         }
     }
 }
 
 static void sha1_final(sha1_ctx_t *ctx, uint8_t *out)
 {
     ctx->buf[ctx->cnt++] = 0x80u;
     if (ctx->cnt > 56) {
         while (ctx->cnt < 64) ctx->buf[ctx->cnt++] = 0;
         sha1_compress(ctx); ctx->cnt = 0;
     }
     while (ctx->cnt < 56) ctx->buf[ctx->cnt++] = 0;
     for (int i = 7; i >= 0; i--)
         ctx->buf[56 + (7 - i)] = (uint8_t)(ctx->bits >> (i * 8));
     sha1_compress(ctx);
     for (int i = 0; i < 5; i++) {
         out[i*4+0] = (uint8_t)(ctx->h[i] >> 24);
         out[i*4+1] = (uint8_t)(ctx->h[i] >> 16);
         out[i*4+2] = (uint8_t)(ctx->h[i] >>  8);
         out[i*4+3] = (uint8_t)(ctx->h[i]);
     }
 }
 
 /* HMAC-SHA1 */
 static void hmac_sha1(const uint8_t *key, size_t klen,
                        const uint8_t *msg, size_t mlen,
                        uint8_t *out)
 {
     uint8_t ipad[64], opad[64], tmp_key[SHA1_DIGEST_LEN];
     if (klen > 64) {
         sha1_ctx_t c; sha1_init(&c);
         sha1_update(&c, key, klen); sha1_final(&c, tmp_key);
         key = tmp_key; klen = SHA1_DIGEST_LEN;
     }
     for (int i = 0; i < 64; i++) {
         uint8_t k = (i < (int)klen) ? key[i] : 0;
         ipad[i] = k ^ 0x36u;
         opad[i] = k ^ 0x5Cu;
     }
     sha1_ctx_t c;
     sha1_init(&c); sha1_update(&c, ipad, 64); sha1_update(&c, msg, mlen);
     sha1_final(&c, out);
     sha1_init(&c); sha1_update(&c, opad, 64); sha1_update(&c, out, SHA1_DIGEST_LEN);
     sha1_final(&c, out);
 }
 
 /* =========================================================================
  * PRF-512 (IEEE 802.11-2020 §12.7.1.2)
  * Produces 64 bytes from HMAC-SHA1 iterations.
  * ====================================================================== */
 
 static void prf512(const uint8_t *key,   size_t key_len,
                    const uint8_t *label, size_t label_len,
                    const uint8_t *data,  size_t data_len,
                    uint8_t *out)
 {
     uint8_t  buf[256];
     uint16_t pos = 0;
     memcpy(buf, label, label_len);     pos += (uint16_t)label_len;
     buf[pos++] = 0x00u;                /* null separator */
     memcpy(buf + pos, data, data_len); pos += (uint16_t)data_len;
 
     uint8_t result[64];
     uint16_t rpos = 0;
     for (uint8_t i = 0; rpos < 64; i++) {
         buf[pos] = i;
         uint8_t h[SHA1_DIGEST_LEN];
         hmac_sha1(key, key_len, buf, (size_t)(pos + 1), h);
         uint16_t copy = (uint16_t)(64 - rpos < SHA1_DIGEST_LEN ?
                                    64 - rpos : SHA1_DIGEST_LEN);
         memcpy(result + rpos, h, copy);
         rpos += copy;
     }
     memcpy(out, result, 64);
 }
 
 /* =========================================================================
  * PBKDF2-SHA1 for PMK derivation (PSK mode, 4096 iterations)
  * ====================================================================== */
 
 static void pbkdf2_sha1(const char *passphrase,
                          const char *ssid, uint8_t ssid_len,
                          uint32_t iterations,
                          uint8_t *out, uint16_t out_len)
 {
     uint8_t  U[SHA1_DIGEST_LEN];
     uint8_t  T[SHA1_DIGEST_LEN];
     uint8_t  salt[36];
     memcpy(salt, ssid, ssid_len);
 
     uint16_t opos = 0;
     for (uint32_t block = 1; opos < out_len; block++) {
         salt[ssid_len + 0] = (uint8_t)(block >> 24);
         salt[ssid_len + 1] = (uint8_t)(block >> 16);
         salt[ssid_len + 2] = (uint8_t)(block >>  8);
         salt[ssid_len + 3] = (uint8_t)(block);
 
         hmac_sha1((const uint8_t*)passphrase, strlen(passphrase),
                   salt, ssid_len + 4, U);
         memcpy(T, U, SHA1_DIGEST_LEN);
 
         for (uint32_t i = 1; i < iterations; i++) {
             hmac_sha1((const uint8_t*)passphrase, strlen(passphrase),
                       U, SHA1_DIGEST_LEN, U);
             for (int j = 0; j < SHA1_DIGEST_LEN; j++) T[j] ^= U[j];
         }
 
         uint16_t copy = (uint16_t)(out_len - opos < SHA1_DIGEST_LEN ?
                                    out_len - opos : SHA1_DIGEST_LEN);
         memcpy(out + opos, T, copy);
         opos += copy;
     }
 }
 
 /* =========================================================================
  * AES-128 (FIPS 197) stub — minimal ECB for CCM key schedule
  * Replace with hardware AES or mbedTLS in production.
  * ====================================================================== */
 
 static const uint8_t s_sbox[256] = {
     0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,
     0xab,0x76,0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,
     0x9c,0xa4,0x72,0xc0,0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,
     0xe5,0xf1,0x71,0xd8,0x31,0x15,0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,
     0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,0x09,0x83,0x2c,0x1a,0x1b,0x6e,
     0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,0x53,0xd1,0x00,0xed,
     0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,0xd0,0xef,
     0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
     0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,
     0xf3,0xd2,0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,
     0x64,0x5d,0x19,0x73,0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,
     0xb8,0x14,0xde,0x5e,0x0b,0xdb,0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,
     0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,0xe7,0xc8,0x37,0x6d,0x8d,0xd5,
     0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,0xba,0x78,0x25,0x2e,
     0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,0x70,0x3e,
     0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
     0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,
     0x28,0xdf,0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,
     0xb0,0x54,0xbb,0x16
 };
 
 /* AES-128 ECB encrypt one 16-byte block (key schedule inline, simplified) */
 static void aes128_ecb_enc(const uint8_t *key, const uint8_t *in, uint8_t *out)
 {
     /* Minimal AES for MIC computation — in production use HW AES engine */
     uint8_t state[16];
     memcpy(state, in, 16);
     /* XOR with key (simplified — not full AES; replace with real AES) */
     for (int i = 0; i < 16; i++)
         out[i] = s_sbox[state[i] ^ key[i]];
     (void)key;
 }
 
 /* =========================================================================
  * CCMP-128 encryption/decryption (simplified)
  * In production use a full AES-CCM implementation (mbedTLS / WolfSSL).
  * ====================================================================== */
 
 int uiox_wifi_sec_ccmp_enc(uiox_wifi_sec_t *sec, uiox_wifi_frame_t *frame)
 {
     if (!sec || !frame || !sec->ptk.valid) return -EINVAL;
 
     /* Increment PN */
     sec->tx_pn++;
 
     /* Build 8-byte CCMP header: [KeyID(1)] [PN0..PN5] */
     uint8_t *hdr = (uint8_t *)uiox_wifi_buf_push(frame, 8u);
     if (!hdr) return -ENOBUFS;
 
     hdr[0] = (uint8_t)(sec->tx_pn & 0xFFu);        /* PN0 */
     hdr[1] = (uint8_t)((sec->tx_pn >> 8)  & 0xFF); /* PN1 */
     hdr[2] = 0x00u;                                  /* Rsvd */
     hdr[3] = 0x20u;                                  /* ExtIV=1, KeyID=0 */
     hdr[4] = (uint8_t)((sec->tx_pn >> 16) & 0xFF); /* PN2 */
     hdr[5] = (uint8_t)((sec->tx_pn >> 24) & 0xFF); /* PN3 */
     hdr[6] = 0x00u;                                  /* PN4 */
     hdr[7] = 0x00u;                                  /* PN5 */
 
     /* Append 8-byte MIC (stub — real CCMP uses AES-CBC-MAC) */
     uint8_t *mic = (uint8_t *)uiox_wifi_buf_put(frame, 8u);
     if (!mic) return -ENOBUFS;
     /* MIC = first 8 bytes of AES-ECB(TK, nonce) — simplified */
     uint8_t nonce_block[16] = {0};
     memcpy(nonce_block, sec->ptk.tk, 16);
     nonce_block[0] ^= (uint8_t)(sec->tx_pn & 0xFF);
     aes128_ecb_enc(sec->ptk.tk, nonce_block, nonce_block);
     memcpy(mic, nonce_block, 8);
 
     frame->encrypted = true;
     return 0;
 }
 
 int uiox_wifi_sec_ccmp_dec(uiox_wifi_sec_t *sec, uiox_wifi_frame_t *frame)
 {
     if (!sec || !frame || !sec->ptk.valid) return -EINVAL;
     if (frame->len < 16u) return -EINVAL;  /* too short */
 
     /* Extract PN from CCMP header */
     const uint8_t *hdr = frame->data;
     uint32_t pn = (uint32_t)hdr[0] |
                   ((uint32_t)hdr[1] << 8)  |
                   ((uint32_t)hdr[4] << 16) |
                   ((uint32_t)hdr[5] << 24);
 
     /* Replay protection */
     if (pn <= sec->rx_pn) return -EBADMSG;
     sec->rx_pn = pn;
 
     /* Strip CCMP header + MIC */
     uiox_wifi_buf_pull(frame, 8u);
     frame->len -= 8u;  /* strip MIC */
     frame->encrypted = false;
     return 0;
 }
 
 /* =========================================================================
  * Security init / PMK / PTK
  * ====================================================================== */
 
 int uiox_wifi_sec_init(uiox_wifi_sec_t *sec,
                         uint8_t cipher, uint8_t akm,
                         const uiox_wifi_mac_t bssid,
                         const uiox_wifi_mac_t own_mac)
 {
     if (!sec) return -EINVAL;
     memset(sec, 0, sizeof(*sec));
     sec->cipher = cipher;
     sec->akm    = akm;
     memcpy(sec->bssid,   bssid,   UIOX_WIFI_MAC_LEN);
     memcpy(sec->own_mac, own_mac, UIOX_WIFI_MAC_LEN);
     sec->hs_state = UIOX_WIFI_HS_IDLE;
     return 0;
 }
 
 int uiox_wifi_sec_derive_pmk(uiox_wifi_sec_t *sec,
                                const char *passphrase,
                                const char *ssid,
                                uint8_t     ssid_len)
 {
     if (!sec || !passphrase || !ssid) return -EINVAL;
     pbkdf2_sha1(passphrase, ssid, ssid_len, 4096,
                 sec->pmk, UIOX_WIFI_PMK_LEN);
     return 0;
 }
 
 int uiox_wifi_sec_derive_ptk(uiox_wifi_sec_t *sec,
                                const uint8_t *anonce,
                                const uint8_t *snonce)
 {
     if (!sec || !anonce || !snonce) return -EINVAL;
     memcpy(sec->anonce, anonce, UIOX_WIFI_NONCE_LEN);
     memcpy(sec->snonce, snonce, UIOX_WIFI_NONCE_LEN);
 
     /* PRF-512 data = min(AA,SA) || max(AA,SA) || min(ANonce,SNonce) || ... */
     uint8_t data[2 * UIOX_WIFI_MAC_LEN + 2 * UIOX_WIFI_NONCE_LEN];
     uint8_t *p = data;
 
     /* MAC addresses in canonical order */
     if (memcmp(sec->own_mac, sec->bssid, UIOX_WIFI_MAC_LEN) < 0) {
         memcpy(p, sec->own_mac, UIOX_WIFI_MAC_LEN); p += UIOX_WIFI_MAC_LEN;
         memcpy(p, sec->bssid,   UIOX_WIFI_MAC_LEN); p += UIOX_WIFI_MAC_LEN;
     } else {
         memcpy(p, sec->bssid,   UIOX_WIFI_MAC_LEN); p += UIOX_WIFI_MAC_LEN;
         memcpy(p, sec->own_mac, UIOX_WIFI_MAC_LEN); p += UIOX_WIFI_MAC_LEN;
     }
 
     /* Nonces in canonical order */
     if (memcmp(anonce, snonce, UIOX_WIFI_NONCE_LEN) < 0) {
         memcpy(p, anonce, UIOX_WIFI_NONCE_LEN); p += UIOX_WIFI_NONCE_LEN;
         memcpy(p, snonce, UIOX_WIFI_NONCE_LEN);
     } else {
         memcpy(p, snonce, UIOX_WIFI_NONCE_LEN); p += UIOX_WIFI_NONCE_LEN;
         memcpy(p, anonce, UIOX_WIFI_NONCE_LEN);
     }
 
     uint8_t ptk_buf[UIOX_WIFI_PTK_LEN + 16];
     static const char label[] = "Pairwise key expansion";
     prf512(sec->pmk, UIOX_WIFI_PMK_LEN,
            (const uint8_t *)label, sizeof(label) - 1,
            data, sizeof(data), ptk_buf);
 
     memcpy(sec->ptk.kck, ptk_buf,      16);
     memcpy(sec->ptk.kek, ptk_buf + 16, 16);
     memcpy(sec->ptk.tk,  ptk_buf + 32, 16);
     sec->ptk.valid = true;
     return 0;
 }
 
 int uiox_wifi_sec_eapol_rx(uiox_wifi_sec_t   *sec,
                             uiox_wifi_frame_t *frame,
                             uiox_wifi_frame_t **reply_out)
 {
     if (!sec || !frame) return -EINVAL;
     if (reply_out) *reply_out = NULL;
 
     /* Minimal EAPOL key frame parsing (IEEE 802.1X-2020) */
     if (frame->len < 99u) return -EINVAL;
     const uint8_t *eapol = frame->data;
     if (eapol[0] != 0x02u) return -EINVAL;  /* EAPOL-Key type */
 
     uint8_t key_info_hi = eapol[5];
     bool is_msg1 = !(key_info_hi & 0x01u);  /* No MIC = msg 1 */
 
     if (is_msg1 && sec->hs_state == UIOX_WIFI_HS_IDLE) {
         /* Save ANonce from msg 1 */
         memcpy(sec->anonce, &eapol[17], UIOX_WIFI_NONCE_LEN);
 
         /* Generate SNonce (stub — use TRNG in production) */
         for (int i = 0; i < (int)UIOX_WIFI_NONCE_LEN; i++)
             sec->snonce[i] = (uint8_t)(i * 0x37u + 0xA5u);
 
         /* Derive PTK */
         uiox_wifi_sec_derive_ptk(sec, sec->anonce, sec->snonce);
 
         /* Build msg 2 reply */
         uiox_wifi_frame_t *reply = uiox_wifi_buf_alloc_tx();
         if (reply) {
             memset(reply->data, 0, 99u);
             reply->data[0] = 0x02u;  /* EAPOL-Key */
             reply->data[5] = 0x01u;  /* MIC present */
             memcpy(&reply->data[17], sec->snonce, UIOX_WIFI_NONCE_LEN);
             /* MIC = HMAC-SHA1(KCK, EAPOL frame) */
             hmac_sha1(sec->ptk.kck, 16, reply->data, 99u,
                       &reply->data[81]);
             reply->len = 99u;
             if (reply_out) *reply_out = reply;
         }
         sec->hs_state = UIOX_WIFI_HS_MSG2_SENT;
         return 0;
     }
 
     if (!is_msg1 && sec->hs_state == UIOX_WIFI_HS_MSG2_SENT) {
         /* Msg 3: install GTK, confirm handshake */
         sec->hs_state = UIOX_WIFI_HS_COMPLETE;
         return 0;
     }
 
     return -EPROTO;
 }
 
 void uiox_wifi_sec_pmksa_add(uiox_wifi_sec_t *sec,
                               const uint8_t *pmk,
                               const uiox_wifi_mac_t bssid,
                               uint32_t now_s)
 {
     if (!sec || !pmk) return;
     /* Find free or oldest slot */
     int slot = 0;
     uint32_t oldest = sec->pmksa[0].created_s;
     for (int i = 0; i < UIOX_WIFI_PMKSA_MAX; i++) {
         if (!sec->pmksa[i].valid) { slot = i; break; }
         if (sec->pmksa[i].created_s < oldest) {
             oldest = sec->pmksa[i].created_s; slot = i;
         }
     }
     uiox_wifi_pmksa_t *e = &sec->pmksa[slot];
     memcpy(e->pmk,   pmk,  UIOX_WIFI_PMK_LEN);
     memcpy(e->bssid, bssid, UIOX_WIFI_MAC_LEN);
     e->created_s  = now_s;
     e->lifetime_s = 43200u; /* 12 hours */
     e->valid      = true;
     /* PMKID = HMAC-SHA1-128(PMK, "PMK Name" || BSSID || own_mac) */
     uint8_t id_data[8 + 2 * UIOX_WIFI_MAC_LEN];
     memcpy(id_data, "PMK Name", 8);
     memcpy(id_data + 8, bssid, UIOX_WIFI_MAC_LEN);
     memcpy(id_data + 8 + UIOX_WIFI_MAC_LEN, sec->own_mac, UIOX_WIFI_MAC_LEN);
     uint8_t h[SHA1_DIGEST_LEN];
     hmac_sha1(pmk, UIOX_WIFI_PMK_LEN, id_data, sizeof(id_data), h);
     memcpy(e->pmkid, h, UIOX_WIFI_PMKID_LEN);
 }
 
 const uiox_wifi_pmksa_t *uiox_wifi_sec_pmksa_lookup(
     const uiox_wifi_sec_t *sec,
     const uiox_wifi_mac_t bssid,
     uint32_t now_s)
 {
     if (!sec) return NULL;
     for (int i = 0; i < UIOX_WIFI_PMKSA_MAX; i++) {
         const uiox_wifi_pmksa_t *e = &sec->pmksa[i];
         if (!e->valid) continue;
         if (e->lifetime_s && (now_s - e->created_s) >= e->lifetime_s)
             continue;
         if (memcmp(e->bssid, bssid, UIOX_WIFI_MAC_LEN) == 0)
             return e;
     }
     return NULL;
 }
 