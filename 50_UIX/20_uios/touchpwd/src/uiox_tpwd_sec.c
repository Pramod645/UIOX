/**
 * @file    uiox_tpwd_sec.c
 * @brief   UIOX Touch-Password security engine implementation.
 * @date    2026-06-01
 */

 #include "uiox_tpwd_sec.h"
 #include <string.h>
 #include <errno.h>
 
 /* =========================================================================
  * Minimal SHA-256 (FIPS 180-4)
  * Replace with mbedTLS / WolfSSL / hardware AES in production.
  * ====================================================================== */
 
 #define SHA256_DIGEST_LEN   32u
 #define SHA256_BLOCK_LEN    64u
 
 typedef struct {
     uint32_t state[8];
     uint8_t  buf[64];
     uint64_t bits;
     uint8_t  cnt;
 } sha256_ctx_t;
 
 static const uint32_t K[64] = {
     0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,
     0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
     0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,
     0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
     0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,
     0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
     0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,
     0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
     0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,
     0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
     0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,
     0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
     0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,
     0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
     0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,
     0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
 };
 
 static inline uint32_t rotr32(uint32_t v, int n)
 { return (v >> n) | (v << (32 - n)); }
 
 static void sha256_compress(sha256_ctx_t *ctx)
 {
     uint32_t w[64];
     const uint8_t *b = ctx->buf;
     for (int i = 0; i < 16; i++)
         w[i] = ((uint32_t)b[i*4]<<24)|((uint32_t)b[i*4+1]<<16)|
                ((uint32_t)b[i*4+2]<<8)|(uint32_t)b[i*4+3];
     for (int i = 16; i < 64; i++) {
         uint32_t s0 = rotr32(w[i-15],7)^rotr32(w[i-15],18)^(w[i-15]>>3u);
         uint32_t s1 = rotr32(w[i-2],17)^rotr32(w[i-2],19)^(w[i-2]>>10u);
         w[i] = w[i-16] + s0 + w[i-7] + s1;
     }
     uint32_t a=ctx->state[0],b2=ctx->state[1],c=ctx->state[2],
              d=ctx->state[3],e=ctx->state[4],f=ctx->state[5],
              g=ctx->state[6],h=ctx->state[7];
     for (int i = 0; i < 64; i++) {
         uint32_t S1   = rotr32(e,6)^rotr32(e,11)^rotr32(e,25);
         uint32_t ch   = (e&f)^((~e)&g);
         uint32_t tmp1 = h + S1 + ch + K[i] + w[i];
         uint32_t S0   = rotr32(a,2)^rotr32(a,13)^rotr32(a,22);
         uint32_t maj  = (a&b2)^(a&c)^(b2&c);
         uint32_t tmp2 = S0 + maj;
         h=g; g=f; f=e; e=d+tmp1;
         d=c; c=b2; b2=a; a=tmp1+tmp2;
     }
     ctx->state[0]+=a; ctx->state[1]+=b2; ctx->state[2]+=c;
     ctx->state[3]+=d; ctx->state[4]+=e; ctx->state[5]+=f;
     ctx->state[6]+=g; ctx->state[7]+=h;
 }
 
 static void sha256_init(sha256_ctx_t *ctx)
 {
     ctx->state[0]=0x6a09e667u; ctx->state[1]=0xbb67ae85u;
     ctx->state[2]=0x3c6ef372u; ctx->state[3]=0xa54ff53au;
     ctx->state[4]=0x510e527fu; ctx->state[5]=0x9b05688cu;
     ctx->state[6]=0x1f83d9abu; ctx->state[7]=0x5be0cd19u;
     ctx->bits=0; ctx->cnt=0;
 }
 
 static void sha256_update(sha256_ctx_t *ctx,
                            const uint8_t *data, size_t len)
 {
     for (size_t i = 0; i < len; i++) {
         ctx->buf[ctx->cnt++] = data[i];
         ctx->bits += 8;
         if (ctx->cnt == 64) { sha256_compress(ctx); ctx->cnt = 0; }
     }
 }
 
 static void sha256_final(sha256_ctx_t *ctx, uint8_t *out)
 {
     ctx->buf[ctx->cnt++] = 0x80u;
     if (ctx->cnt > 56) {
         while (ctx->cnt < 64) ctx->buf[ctx->cnt++] = 0;
         sha256_compress(ctx); ctx->cnt = 0;
     }
     while (ctx->cnt < 56) ctx->buf[ctx->cnt++] = 0;
     for (int i = 7; i >= 0; i--)
         ctx->buf[56+(7-i)] = (uint8_t)(ctx->bits >> (i*8));
     sha256_compress(ctx);
     for (int i = 0; i < 8; i++) {
         out[i*4]   = (uint8_t)(ctx->state[i]>>24);
         out[i*4+1] = (uint8_t)(ctx->state[i]>>16);
         out[i*4+2] = (uint8_t)(ctx->state[i]>>8);
         out[i*4+3] = (uint8_t)(ctx->state[i]);
     }
 }
 
 /* HMAC-SHA256 */
 static void hmac_sha256(const uint8_t *key, size_t klen,
                          const uint8_t *msg, size_t mlen,
                          uint8_t *out)
 {
     uint8_t ipad[64], opad[64], k[SHA256_DIGEST_LEN];
     if (klen > 64) {
         sha256_ctx_t c; sha256_init(&c);
         sha256_update(&c, key, klen); sha256_final(&c, k);
         key = k; klen = SHA256_DIGEST_LEN;
     }
     for (int i = 0; i < 64; i++) {
         uint8_t b = (i < (int)klen) ? key[i] : 0u;
         ipad[i] = b ^ 0x36u;
         opad[i] = b ^ 0x5Cu;
     }
     sha256_ctx_t c;
     sha256_init(&c);
     sha256_update(&c, ipad, 64);
     sha256_update(&c, msg, mlen);
     sha256_final(&c, out);
     sha256_init(&c);
     sha256_update(&c, opad, 64);
     sha256_update(&c, out, SHA256_DIGEST_LEN);
     sha256_final(&c, out);
 }
 
 /* PBKDF2-HMAC-SHA256 */
 static void pbkdf2_sha256(const uint8_t *pass, size_t plen,
                            const uint8_t *salt, size_t slen,
                            uint32_t iters,
                            uint8_t *out, uint16_t out_len)
 {
     uint8_t  U[SHA256_DIGEST_LEN], T[SHA256_DIGEST_LEN];
     uint8_t  s[64];
     uint16_t opos = 0;
     for (uint32_t block = 1; opos < out_len; block++) {
         if (slen < 60) {
             memcpy(s, salt, slen);
             s[slen]   = (uint8_t)(block >> 24);
             s[slen+1] = (uint8_t)(block >> 16);
             s[slen+2] = (uint8_t)(block >>  8);
             s[slen+3] = (uint8_t)(block);
             hmac_sha256(pass, plen, s, slen + 4, U);
         } else {
             hmac_sha256(pass, plen, salt, slen, U);
         }
         memcpy(T, U, SHA256_DIGEST_LEN);
         for (uint32_t i = 1; i < iters; i++) {
             hmac_sha256(pass, plen, U, SHA256_DIGEST_LEN, U);
             for (int j = 0; j < (int)SHA256_DIGEST_LEN; j++) T[j] ^= U[j];
         }
         uint16_t cp = (uint16_t)(out_len - opos < SHA256_DIGEST_LEN ?
                                   out_len - opos : SHA256_DIGEST_LEN);
         memcpy(out + opos, T, cp);
         opos += cp;
     }
 }
 
 /* =========================================================================
  * LFSR-based PRNG for salt (replace with TRNG on target hardware)
  * ====================================================================== */
 
 static uint32_t lfsr_next(uint32_t *state)
 {
     *state ^= *state << 13u;
     *state ^= *state >> 17u;
     *state ^= *state <<  5u;
     return *state;
 }
 
 /* =========================================================================
  * Public Security API
  * ====================================================================== */
 
 int uiox_tpwd_sec_init(uiox_tpwd_sec_t *sec, uint32_t rng_seed)
 {
     if (!sec) return -EINVAL;
     memset(sec, 0, sizeof(*sec));
     sec->rng_state = rng_seed ? rng_seed : 0xDEADBEEFu;
     return 0;
 }
 
 void uiox_tpwd_sec_gen_salt(uiox_tpwd_sec_t *sec,
                               uint8_t *salt, uint8_t len)
 {
     if (!sec || !salt) return;
     for (uint8_t i = 0; i < len; i += 4) {
         uint32_t r = lfsr_next(&sec->rng_state);
         for (int j = 0; j < 4 && (i+j) < len; j++)
             salt[i+j] = (uint8_t)(r >> (j*8));
     }
 }
 
 void uiox_tpwd_sec_hash(const uint8_t *data, uint16_t data_len,
                          const uint8_t *salt,
                          uint8_t *hash_out)
 {
     if (!data || !salt || !hash_out) return;
     pbkdf2_sha256(data, data_len, salt, UIOX_TPWD_SALT_LEN,
                   UIOX_TPWD_SEC_PBKDF2_ITER,
                   hash_out, UIOX_TPWD_HASH_LEN);
 }
 
 bool uiox_tpwd_sec_compare(const uint8_t *a, const uint8_t *b, uint16_t len)
 {
     if (!a || !b) return false;
     volatile uint8_t diff = 0;
     for (uint16_t i = 0; i < len; i++) diff |= a[i] ^ b[i];
     return diff == 0u;
 }
 
 int uiox_tpwd_sec_enrol(uiox_tpwd_sec_t *sec,
                          const char *id,
                          const uint8_t *data, uint16_t data_len)
 {
     if (!sec || !id || !data || !data_len) return -EINVAL;
     if (sec->record_count >= UIOX_TPWD_SEC_MAX_STORED) return -ENOSPC;
 
     /* Check for duplicate ID */
     for (uint8_t i = 0; i < sec->record_count; i++)
         if (sec->records[i].valid &&
             strncmp(sec->records[i].id, id, UIOX_TPWD_SEC_ID_LEN) == 0)
             return -EEXIST;
 
     uiox_tpwd_sec_record_t *r = &sec->records[sec->record_count++];
     memset(r, 0, sizeof(*r));
     strncpy(r->id, id, UIOX_TPWD_SEC_ID_LEN - 1);
     uiox_tpwd_sec_gen_salt(sec, r->salt, UIOX_TPWD_SALT_LEN);
     uiox_tpwd_sec_hash(data, data_len, r->salt, r->hash);
     r->valid    = true;
     r->attempts = 0;
     return 0;
 }
 
 int uiox_tpwd_sec_verify(uiox_tpwd_sec_t *sec,
                           const char *id,
                           const uint8_t *data, uint16_t data_len,
                           uint32_t now_s)
 {
     if (!sec || !id || !data) return -EINVAL;
 
     /* Global lockout */
     if (sec->global_lockout_until_s && now_s < sec->global_lockout_until_s)
         return -EPERM;
 
     /* Find record */
     uiox_tpwd_sec_record_t *r = NULL;
     for (uint8_t i = 0; i < sec->record_count; i++) {
         if (sec->records[i].valid &&
             strncmp(sec->records[i].id, id, UIOX_TPWD_SEC_ID_LEN) == 0) {
             r = &sec->records[i];
             break;
         }
     }
     if (!r) return -ENOENT;
 
     /* Per-record lockout */
     if (r->lockout_until_s && now_s < r->lockout_until_s) return -EPERM;
 
     /* Hash the candidate */
     uint8_t candidate[UIOX_TPWD_HASH_LEN];
     uiox_tpwd_sec_hash(data, data_len, r->salt, candidate);
     bool match = uiox_tpwd_sec_compare(candidate, r->hash,
                                         UIOX_TPWD_HASH_LEN);
     uiox_tpwd_sec_zero(candidate, sizeof(candidate));
 
     if (match) {
         r->attempts = 0;
         r->lockout_until_s = 0;
         return 0;
     }
 
     /* Failed attempt */
     r->attempts++;
     sec->global_failures++;
     if (r->attempts >= UIOX_TPWD_SEC_MAX_ATTEMPTS)
         r->lockout_until_s = now_s + UIOX_TPWD_SEC_LOCKOUT_S;
     if (sec->global_failures >= UIOX_TPWD_SEC_MAX_ATTEMPTS * 2u)
         sec->global_lockout_until_s = now_s + UIOX_TPWD_SEC_LOCKOUT_S * 2u;
 
     return -EACCES;
 }
 
 int uiox_tpwd_sec_delete(uiox_tpwd_sec_t *sec, const char *id)
 {
     if (!sec || !id) return -EINVAL;
     for (uint8_t i = 0; i < sec->record_count; i++) {
         if (sec->records[i].valid &&
             strncmp(sec->records[i].id, id, UIOX_TPWD_SEC_ID_LEN) == 0) {
             uiox_tpwd_sec_zero(&sec->records[i],
                                 sizeof(uiox_tpwd_sec_record_t));
             sec->records[i].valid = false;
             return 0;
         }
     }
     return -ENOENT;
 }
 
 bool uiox_tpwd_sec_is_locked(const uiox_tpwd_sec_t *sec,
                                const char *id, uint32_t now_s)
 {
     if (!sec || !id) return true;
     if (sec->global_lockout_until_s && now_s < sec->global_lockout_until_s)
         return true;
     for (uint8_t i = 0; i < sec->record_count; i++) {
         if (sec->records[i].valid &&
             strncmp(sec->records[i].id, id, UIOX_TPWD_SEC_ID_LEN) == 0)
             return (sec->records[i].lockout_until_s &&
                     now_s < sec->records[i].lockout_until_s);
     }
     return false;
 }
 
 void uiox_tpwd_sec_gen_token(uiox_tpwd_sec_t *sec,
                                uint32_t valid_for_s, uint32_t now_s)
 {
     if (!sec) return;
     for (uint8_t i = 0; i < UIOX_TPWD_SEC_TOKEN_LEN; i += 4) {
         uint32_t r = lfsr_next(&sec->rng_state);
         for (int j = 0; j < 4 && (i+j) < UIOX_TPWD_SEC_TOKEN_LEN; j++)
             sec->session_token[i+j] = (uint8_t)(r >> (j*8));
     }
     sec->session_valid      = true;
     sec->session_expires_s  = now_s + valid_for_s;
 }
 
 bool uiox_tpwd_sec_token_valid(const uiox_tpwd_sec_t *sec, uint32_t now_s)
 {
     return sec && sec->session_valid && now_s < sec->session_expires_s;
 }
 
 void uiox_tpwd_sec_logout(uiox_tpwd_sec_t *sec)
 {
     if (!sec) return;
     uiox_tpwd_sec_zero(sec->session_token, UIOX_TPWD_SEC_TOKEN_LEN);
     sec->session_valid     = false;
     sec->session_expires_s = 0;
 }
 
 void uiox_tpwd_sec_zero(volatile void *buf, size_t len)
 {
     volatile uint8_t *p = (volatile uint8_t *)buf;
     while (len--) *p++ = 0u;
 }
 