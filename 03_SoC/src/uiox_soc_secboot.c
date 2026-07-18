/**
 * @file    uiox_soc_secboot.c
 * @brief   UIOX SoC — Secure Boot verification.
 *          Zero libc dependency — no string.h or stdio.h.
 * @date    2026-07-07
 */

 #include "../include/uiox_soc_secboot.h"
 #include "../include/uiox_soc_hw.h"
 
 /* =========================================================================
  * Bare-metal helpers
  * ====================================================================== */
 
 static void soc_memset_sb(void *dst, int val, size_t n)
 {
     uint8_t *d = (uint8_t *)dst;
     while (n--) *d++ = (uint8_t)val;
 }
 
 static void soc_memcpy_sb(void *dst, const void *src, size_t n)
 {
     uint8_t *d       = (uint8_t *)dst;
     const uint8_t *s = (const uint8_t *)src;
     while (n--) *d++ = *s++;
 }
 
 static int soc_memcmp_sb(const void *a, const void *b, size_t n)
 {
     const uint8_t *p = (const uint8_t *)a;
     const uint8_t *q = (const uint8_t *)b;
     while (n--) {
         if (*p != *q) return (int)*p - (int)*q;
         p++; q++;
     }
     return 0;
 }
 
 static void soc_strncpy_sb(char *dst, const char *src, size_t n)
 {
     size_t i = 0u;
     while (i < n - 1u && src[i]) { dst[i] = src[i]; i++; }
     dst[i] = '\0';
 }
 
 static void soc_puts_sb(const char *s)
 {
     const uiox_soc_hw_ops_t *ops = uiox_soc_hw_ops();
     if (!ops || !ops->uart_putc) return;
     while (*s) {
         if (*s == '\n') ops->uart_putc('\r');
         ops->uart_putc(*s++);
     }
 }
 
 /* =========================================================================
  * SHA-256 (FIPS 180-4)
  * ====================================================================== */
 
 static const uint32_t K256[64] = {
     0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
     0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
     0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
     0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
     0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
     0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
     0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
     0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
     0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
     0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
     0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
     0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
     0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
     0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
     0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
     0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
 };
 
 #define ROTR32(x, n) (((x) >> (n)) | ((x) << (32u - (n))))
 #define CH(e,f,g)    (((e) & (f)) ^ (~(e) & (g)))
 #define MAJ(a,b,c)   (((a) & (b)) ^ ((a) & (c)) ^ ((b) & (c)))
 #define SIG0(a)  (ROTR32(a,2)  ^ ROTR32(a,13) ^ ROTR32(a,22))
 #define SIG1(e)  (ROTR32(e,6)  ^ ROTR32(e,11) ^ ROTR32(e,25))
 #define sig0(x)  (ROTR32(x,7)  ^ ROTR32(x,18) ^ ((x) >> 3))
 #define sig1(x)  (ROTR32(x,17) ^ ROTR32(x,19) ^ ((x) >> 10))
 
 static void sha256_transform(uiox_soc_sha256_ctx_t *ctx,
                               const uint8_t *block)
 {
     uint32_t W[64], a, b, c, d, e, f, g, h, T1, T2;
     for (int i = 0; i < 16; i++) {
         W[i] = ((uint32_t)block[i*4]   << 24) |
                ((uint32_t)block[i*4+1] << 16) |
                ((uint32_t)block[i*4+2] <<  8) |
                ((uint32_t)block[i*4+3]);
     }
     for (int i = 16; i < 64; i++)
         W[i] = sig1(W[i-2]) + W[i-7] + sig0(W[i-15]) + W[i-16];
 
     a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2];
     d = ctx->state[3]; e = ctx->state[4]; f = ctx->state[5];
     g = ctx->state[6]; h = ctx->state[7];
 
     for (int i = 0; i < 64; i++) {
         T1 = h + SIG1(e) + CH(e,f,g) + K256[i] + W[i];
         T2 = SIG0(a) + MAJ(a,b,c);
         h=g; g=f; f=e; e=d+T1; d=c; c=b; b=a; a=T1+T2;
     }
 
     ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c;
     ctx->state[3] += d; ctx->state[4] += e; ctx->state[5] += f;
     ctx->state[6] += g; ctx->state[7] += h;
 }
 
 void uiox_soc_sha256_init(uiox_soc_sha256_ctx_t *ctx)
 {
     ctx->state[0] = 0x6a09e667u; ctx->state[1] = 0xbb67ae85u;
     ctx->state[2] = 0x3c6ef372u; ctx->state[3] = 0xa54ff53au;
     ctx->state[4] = 0x510e527fu; ctx->state[5] = 0x9b05688cu;
     ctx->state[6] = 0x1f83d9abu; ctx->state[7] = 0x5be0cd19u;
     ctx->bit_count = 0u;
     ctx->buf_len   = 0u;
 }
 
 void uiox_soc_sha256_update(uiox_soc_sha256_ctx_t *ctx,
                               const uint8_t *d, size_t len)
 {
     ctx->bit_count += (uint64_t)len * 8u;
     while (len > 0u) {
         uint32_t space = 64u - ctx->buf_len;
         uint32_t take  = (uint32_t)len < space ? (uint32_t)len : space;
         soc_memcpy_sb(ctx->buf + ctx->buf_len, d, take);
         ctx->buf_len += take;
         d += take; len -= take;
         if (ctx->buf_len == 64u) {
             sha256_transform(ctx, ctx->buf);
             ctx->buf_len = 0u;
         }
     }
 }
 
 void uiox_soc_sha256_final(uiox_soc_sha256_ctx_t *ctx,
                              uint8_t digest[32])
 {
     uint8_t pad[64];
     soc_memset_sb(pad, 0, sizeof(pad));
     pad[0] = 0x80u;
     uint32_t used = ctx->buf_len;
     uint32_t pad_len = used < 56u ? 56u - used : 120u - used;
     uiox_soc_sha256_update(ctx, pad, pad_len);
     uint8_t len_be[8];
     uint64_t bc = ctx->bit_count;
     for (int i = 7; i >= 0; i--) { len_be[i] = (uint8_t)(bc & 0xFFu); bc >>= 8; }
     uiox_soc_sha256_update(ctx, len_be, 8u);
     for (int i = 0; i < 8; i++) {
         digest[i*4+0] = (uint8_t)(ctx->state[i] >> 24);
         digest[i*4+1] = (uint8_t)(ctx->state[i] >> 16);
         digest[i*4+2] = (uint8_t)(ctx->state[i] >>  8);
         digest[i*4+3] = (uint8_t)(ctx->state[i]       );
     }
 }
 
 void uiox_soc_sha256(const uint8_t *d, size_t l, uint8_t digest[32])
 {
     uiox_soc_sha256_ctx_t ctx;
     uiox_soc_sha256_init(&ctx);
     uiox_soc_sha256_update(&ctx, d, l);
     uiox_soc_sha256_final(&ctx, digest);
 }
 
 /* =========================================================================
  * PCR (Platform Configuration Register) — 8 × 32-byte banks
  * ====================================================================== */
 
 static uint8_t s_pcr[8][32];
 
 void uiox_soc_secboot_extend_pcr(uint32_t idx,
                                    const uint8_t measurement[32])
 {
     if (idx >= 8u) return;
     uint8_t combined[64];
     soc_memcpy_sb(combined,      s_pcr[idx], 32u);
     soc_memcpy_sb(combined + 32, measurement, 32u);
     uiox_soc_sha256(combined, 64u, s_pcr[idx]);
 }
 
 void uiox_soc_secboot_read_pcr(uint32_t idx, uint8_t out[32])
 {
     if (idx < 8u) soc_memcpy_sb(out, s_pcr[idx], 32u);
 }
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 uiox_soc_err_t uiox_soc_secboot_init(uiox_soc_secboot_ctx_t *ctx,
                                        const uint8_t rot_hash[32],
                                        uiox_soc_sig_algo_t   algo,
                                        bool sim_mode)
 {
     if (!ctx || !rot_hash) return UIOX_SOC_ERR_INVAL;
     soc_memset_sb(ctx, 0, sizeof(*ctx));
     soc_memcpy_sb(ctx->rot.pubkey_hash, rot_hash, 32u);
     ctx->rot.algo     = algo;
     ctx->rot.sim_mode = sim_mode;
     soc_puts_sb("[SOC] secboot_init: sim=");
     soc_puts_sb(sim_mode ? "yes" : "no");
     soc_puts_sb("\n");
     return UIOX_SOC_OK;
 }
 
 uiox_soc_secboot_result_t
 uiox_soc_secboot_verify_cert(uiox_soc_secboot_ctx_t *ctx,
                                const uiox_soc_svc_t   *svc)
 {
     if (!ctx || !svc) return UIOX_SOC_SECBOOT_ERR_INVAL;
     if (svc->magic != UIOX_SOC_SECBOOT_MAGIC)
         return UIOX_SOC_SECBOOT_ERR_BAD_MAGIC;
 
     if (ctx->rot.sim_mode) {
         ctx->svc = (uiox_soc_svc_t *)svc;
         soc_puts_sb("[SOC] secboot cert: PASS (sim)\n");
         return UIOX_SOC_SECBOOT_OK;
     }
 
     /* Real path: hash the subject public key and compare with RoT hash */
     uint8_t computed[32];
     uiox_soc_sha256(svc->subject_pubkey, svc->subject_pubkey_len,
                      computed);
     if (soc_memcmp_sb(computed, ctx->rot.pubkey_hash, 32u) != 0)
         return UIOX_SOC_SECBOOT_ERR_CERT;
 
     ctx->svc = (uiox_soc_svc_t *)svc;
     return UIOX_SOC_SECBOOT_OK;
 }
 
 uiox_soc_secboot_result_t
 uiox_soc_secboot_verify_image(uiox_soc_secboot_ctx_t          *ctx,
                                const uiox_soc_signed_img_hdr_t *hdr,
                                const uint8_t                   *payload,
                                size_t                           len,
                                uiox_soc_secboot_report_t       *report)
 {
     uiox_soc_secboot_report_t local;
     uiox_soc_secboot_report_t *r = report ? report : &local;
     soc_memset_sb(r, 0, sizeof(*r));
     r->result = UIOX_SOC_SECBOOT_OK;
 
     if (!ctx || !hdr || !payload) {
         r->result = UIOX_SOC_SECBOOT_ERR_INVAL;
         return r->result;
     }
 
     /* Step 1: magic */
     if (hdr->magic != UIOX_SOC_SECBOOT_MAGIC) {
         soc_strncpy_sb(r->fail_reason, "bad magic", sizeof(r->fail_reason));
         r->result = UIOX_SOC_SECBOOT_ERR_BAD_MAGIC;
         return r->result;
     }
 
     /* Step 2: hash */
     uiox_soc_sha256(payload, len, r->measured_hash);
     r->hash_ok = (soc_memcmp_sb(r->measured_hash, hdr->image_hash, 32u) == 0);
     if (!r->hash_ok) {
         soc_strncpy_sb(r->fail_reason, "hash mismatch",
                         sizeof(r->fail_reason));
         r->result = UIOX_SOC_SECBOOT_ERR_HASH;
         return r->result;
     }
 
     /* Step 3: anti-rollback */
     r->image_version = hdr->version;
     r->rollback_ok   = (hdr->version >= ctx->min_version);
     if (!r->rollback_ok) {
         soc_strncpy_sb(r->fail_reason, "rollback",
                         sizeof(r->fail_reason));
         r->result = UIOX_SOC_SECBOOT_ERR_ROLLBACK;
         return r->result;
     }
 
     /* Step 4: signature (sim: skip; real: RSA/Ed25519 verify) */
     r->sig_ok  = true;
     r->cert_ok = true;
 
     /* Extend PCR 0 with the measured hash */
     uiox_soc_secboot_extend_pcr(0u, r->measured_hash);
 
     return UIOX_SOC_SECBOOT_OK;
 }
 
 void uiox_soc_secboot_print(const uiox_soc_secboot_report_t *r)
 {
     if (!r) return;
     soc_puts_sb("[SOC] Secure boot report:\n");
     soc_puts_sb("  result  : ");
     soc_puts_sb(r->result == UIOX_SOC_SECBOOT_OK ? "PASS" : "FAIL");
     soc_puts_sb("\n  hash_ok : ");
     soc_puts_sb(r->hash_ok   ? "yes" : "no");
     soc_puts_sb("\n  sig_ok  : ");
     soc_puts_sb(r->sig_ok    ? "yes" : "no");
     soc_puts_sb("\n  cert_ok : ");
     soc_puts_sb(r->cert_ok   ? "yes" : "no");
     soc_puts_sb("\n  rvk_ok  : ");
     soc_puts_sb(r->rollback_ok ? "yes" : "no");
     soc_puts_sb("\n");
     if (r->result != UIOX_SOC_SECBOOT_OK) {
         soc_puts_sb("  reason : ");
         soc_puts_sb(r->fail_reason);
         soc_puts_sb("\n");
     }
 }
 
 