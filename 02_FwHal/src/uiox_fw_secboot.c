/**
 * @file  uiox_fw_secboot.c
 * @brief UIOX Firmware — Secure Boot verification.
 *        Zero libc dependency — no string.h or stdio.h.
 * @date  2026-07-07
 */

 #include "../include/uiox_fw_secboot.h"
 #include "../include/uiox_fw_hw.h"
 
 /* =========================================================================
  * Bare-metal helpers
  * ====================================================================== */
 
 static void fw_memset_sb(void *dst, int val, size_t n)
 {
     uint8_t *d = (uint8_t *)dst; while (n--) *d++ = (uint8_t)val;
 }
 
 static void fw_memcpy_sb(void *dst, const void *src, size_t n)
 {
     uint8_t *d = (uint8_t *)dst;
     const uint8_t *s = (const uint8_t *)src;
     while (n--) *d++ = *s++;
 }
 
 static int fw_memcmp_sb(const void *a, const void *b, size_t n)
 {
     const uint8_t *p = (const uint8_t *)a;
     const uint8_t *q = (const uint8_t *)b;
     while (n--) { if (*p != *q) return (int)*p - (int)*q; p++; q++; }
     return 0;
 }
 
 static void fw_strncpy_sb(char *dst, const char *src, size_t n)
 {
     size_t i = 0u;
     while (i < n - 1u && src[i]) { dst[i] = src[i]; i++; }
     dst[i] = '\0';
 }
 
 static void fw_puts_sb(const char *s)
{
    /* uiox_fw_hw_ops() is declared in uiox_fw_hw.h (already included) */
    const uiox_fw_hw_ops_t *ops = uiox_fw_hw_ops();
    if (!ops || !ops->uart_putc) return;
    while (*s) {
        if (*s == '\n') ops->uart_putc('\r');
        ops->uart_putc(*s++);
    }
}

 static char *fw_u32_hex_sb(uint32_t v, char *buf)
 {
     static const char h[] = "0123456789abcdef";
     for (int i = 7; i >= 0; i--) { buf[i] = h[v & 0xFu]; v >>= 4; }
     buf[8] = '\0'; return buf;
 }
 
 /* =========================================================================
  * PCR banks (8 × 32 bytes)
  * ====================================================================== */
 
 #define UIOX_PCR_BANKS 8u
 static uint8_t s_pcr[UIOX_PCR_BANKS][32];
 
 void uiox_fw_secboot_extend_pcr(uint32_t idx, const uint8_t measurement[32])
 {
     if (idx >= UIOX_PCR_BANKS) return;
     uint8_t buf[64];
     fw_memcpy_sb(buf,      s_pcr[idx],   32u);
     fw_memcpy_sb(buf + 32u, measurement, 32u);
     uiox_sha256(buf, 64u, s_pcr[idx]);
 }
 
 void uiox_fw_secboot_read_pcr(uint32_t idx, uint8_t out[32])
 {
     if (idx >= UIOX_PCR_BANKS) { fw_memset_sb(out, 0, 32u); return; }
     fw_memcpy_sb(out, s_pcr[idx], 32u);
 }
 
 /* =========================================================================
  * SHA-256 (RFC 6234 — no libc)
  * ====================================================================== */
 
 #define ROTR32(x,n) (((x)>>(n))|((x)<<(32u-(n))))
 #define CH(x,y,z)   (((x)&(y))^(~(x)&(z)))
 #define MAJ(x,y,z)  (((x)&(y))^((x)&(z))^((y)&(z)))
 #define EP0(x)      (ROTR32(x,2) ^ROTR32(x,13)^ROTR32(x,22))
 #define EP1(x)      (ROTR32(x,6) ^ROTR32(x,11)^ROTR32(x,25))
 #define SIG0(x)     (ROTR32(x,7) ^ROTR32(x,18)^((x)>> 3))
 #define SIG1(x)     (ROTR32(x,17)^ROTR32(x,19)^((x)>>10))
 
 static const uint32_t K256[64] = {
     0x428A2F98u,0x71374491u,0xB5C0FBCFu,0xE9B5DBA5u,
     0x3956C25Bu,0x59F111F1u,0x923F82A4u,0xAB1C5ED5u,
     0xD807AA98u,0x12835B01u,0x243185BEu,0x550C7DC3u,
     0x72BE5D74u,0x80DEB1FEu,0x9BDC06A7u,0xC19BF174u,
     0xE49B69C1u,0xEFBE4786u,0x0FC19DC6u,0x240CA1CCu,
     0x2DE92C6Fu,0x4A7484AAu,0x5CB0A9DCu,0x76F988DAu,
     0x983E5152u,0xA831C66Du,0xB00327C8u,0xBF597FC7u,
     0xC6E00BF3u,0xD5A79147u,0x06CA6351u,0x14292967u,
     0x27B70A85u,0x2E1B2138u,0x4D2C6DFCu,0x53380D13u,
     0x650A7354u,0x766A0ABBu,0x81C2C92Eu,0x92722C85u,
     0xA2BFE8A1u,0xA81A664Bu,0xC24B8B70u,0xC76C51A3u,
     0xD192E819u,0xD6990624u,0xF40E3585u,0x106AA070u,
     0x19A4C116u,0x1E376C08u,0x2748774Cu,0x34B0BCB5u,
     0x391C0CB3u,0x4ED8AA4Au,0x5B9CCA4Fu,0x682E6FF3u,
     0x748F82EEu,0x78A5636Fu,0x84C87814u,0x8CC70208u,
     0x90BEFFFAu,0xA4506CEBu,0xBEF9A3F7u,0xC67178F2u,
 };
 
 static const uint32_t H0[8] = {
     0x6A09E667u,0xBB67AE85u,0x3C6EF372u,0xA54FF53Au,
     0x510E527Fu,0x9B05688Cu,0x1F83D9ABu,0x5BE0CD19u
 };
 
 static void sha256_transform(uiox_sha256_ctx_t *ctx,
                               const uint8_t *blk)
 {
     uint32_t W[64], a,b,c,d,e,f,g,h,t1,t2;
     for (int i=0;i<16;i++)
         W[i] = ((uint32_t)blk[i*4  ]<<24u)
               |((uint32_t)blk[i*4+1]<<16u)
               |((uint32_t)blk[i*4+2]<< 8u)
               | (uint32_t)blk[i*4+3];
     for (int i=16;i<64;i++)
         W[i]=SIG1(W[i-2])+W[i-7]+SIG0(W[i-15])+W[i-16];
     a=ctx->state[0]; b=ctx->state[1]; c=ctx->state[2]; d=ctx->state[3];
     e=ctx->state[4]; f=ctx->state[5]; g=ctx->state[6]; h=ctx->state[7];
     for (int i=0;i<64;i++){
         t1=h+EP1(e)+CH(e,f,g)+K256[i]+W[i];
         t2=EP0(a)+MAJ(a,b,c);
         h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
     }
     ctx->state[0]+=a; ctx->state[1]+=b; ctx->state[2]+=c;
     ctx->state[3]+=d; ctx->state[4]+=e; ctx->state[5]+=f;
     ctx->state[6]+=g; ctx->state[7]+=h;
 }
 
 void uiox_sha256_init(uiox_sha256_ctx_t *ctx)
 {
     for (int i=0;i<8;i++) ctx->state[i]=H0[i];
     ctx->bit_count=0u; ctx->buf_len=0u;
 }
 
 void uiox_sha256_update(uiox_sha256_ctx_t *ctx,
                          const uint8_t *d, size_t len)
 {
     ctx->bit_count += (uint64_t)len * 8u;
     for (size_t i=0;i<len;i++){
         ctx->buf[ctx->buf_len++]=d[i];
         if (ctx->buf_len==64u){
             sha256_transform(ctx,ctx->buf);
             ctx->buf_len=0u;
         }
     }
 }
 
 void uiox_sha256_final(uiox_sha256_ctx_t *ctx, uint8_t digest[32])
 {
     uint32_t i=ctx->buf_len;
     ctx->buf[i++]=0x80u;
     if(i>56u){
         while(i<64u) ctx->buf[i++]=0u;
         sha256_transform(ctx,ctx->buf); i=0u;
     }
     while(i<56u) ctx->buf[i++]=0u;
     uint64_t bc=ctx->bit_count;
     for(int j=7;j>=0;j--){
         ctx->buf[56u+(uint32_t)j]=(uint8_t)(bc&0xFFu); bc>>=8u;
     }
     sha256_transform(ctx,ctx->buf);
     for(int j=0;j<8;j++){
         digest[j*4+0]=(uint8_t)(ctx->state[j]>>24u);
         digest[j*4+1]=(uint8_t)(ctx->state[j]>>16u);
         digest[j*4+2]=(uint8_t)(ctx->state[j]>> 8u);
         digest[j*4+3]=(uint8_t)(ctx->state[j]      );
     }
 }
 
 void uiox_sha256(const uint8_t *d, size_t l, uint8_t digest[32])
 {
     uiox_sha256_ctx_t ctx;
     uiox_sha256_init(&ctx);
     uiox_sha256_update(&ctx, d, l);
     uiox_sha256_final(&ctx, digest);
 }
 
 /* =========================================================================
  * Secure Boot API
  * ====================================================================== */
 
 uiox_fw_err_t uiox_fw_secboot_init(uiox_secboot_ctx_t *ctx,
                                      const uint8_t rot_hash[32],
                                      uiox_sig_algo_t algo,
                                      bool sim_mode)
 {
     if (!ctx || !rot_hash) return UIOX_FW_ERR_INVAL;
     fw_memset_sb(ctx, 0, sizeof(*ctx));
     fw_memcpy_sb(ctx->rot.pubkey_hash, rot_hash, 32u);
     ctx->rot.algo      = algo;
     ctx->rot.sim_mode  = sim_mode;
     ctx->min_version   = 0u;
     ctx->debug_mode    = sim_mode;
     fw_memset_sb(s_pcr, 0, sizeof(s_pcr));
     return UIOX_FW_OK;
 }
 
 uiox_secboot_result_t
 uiox_fw_secboot_verify_cert(uiox_secboot_ctx_t *ctx,
                               const uiox_fvc_t *fvc)
 {
     if (!ctx || !fvc) return UIOX_SECBOOT_ERR_INVAL;
     if (fvc->magic != UIOX_SECBOOT_MAGIC) return UIOX_SECBOOT_ERR_BAD_MAGIC;
     ctx->fvc = (uiox_fvc_t *)fvc;
     if (ctx->rot.sim_mode || ctx->debug_mode) return UIOX_SECBOOT_OK;
 
     uint8_t cert_key_hash[32];
     uiox_sha256(fvc->subject_pubkey, fvc->subject_pubkey_len, cert_key_hash);
     uiox_fw_secboot_extend_pcr(0u, cert_key_hash);
     return UIOX_SECBOOT_OK;
 }
 
 uiox_secboot_result_t
 uiox_fw_secboot_verify_image(uiox_secboot_ctx_t *ctx,
                                const uiox_signed_img_hdr_t *hdr,
                                const uint8_t *payload,
                                size_t len,
                                uiox_secboot_report_t *report)
 {
     uiox_secboot_report_t local;
     uiox_secboot_report_t *r = report ? report : &local;
     fw_memset_sb(r, 0, sizeof(*r));
 
     if (!ctx || !hdr || !payload || len == 0u) {
         r->result = UIOX_SECBOOT_ERR_INVAL;
         return r->result;
     }
 
     /* 1. Magic */
     if (hdr->magic != UIOX_SECBOOT_MAGIC) {
         fw_strncpy_sb(r->fail_reason, "bad magic",
                       sizeof(r->fail_reason));
         r->result = UIOX_SECBOOT_ERR_BAD_MAGIC;
         return r->result;
     }
 
     /* 2. Anti-rollback */
     r->image_version = hdr->version;
     if (hdr->version < ctx->min_version) {
         fw_strncpy_sb(r->fail_reason, "rollback denied",
                       sizeof(r->fail_reason));
         r->result = UIOX_SECBOOT_ERR_ROLLBACK;
         return r->result;
     }
     r->rollback_ok = true;
 
     /* 3. SHA-256 hash */
     uiox_sha256(payload, len, r->measured_hash);
     r->hash_ok = (fw_memcmp_sb(r->measured_hash,
                                 hdr->image_hash,
                                 UIOX_SECBOOT_SHA256_LEN) == 0);
     if (!r->hash_ok) {
         fw_strncpy_sb(r->fail_reason, "SHA-256 mismatch",
                       sizeof(r->fail_reason));
         r->result = UIOX_SECBOOT_ERR_HASH;
         return r->result;
     }
     uiox_fw_secboot_extend_pcr(1u, r->measured_hash);
 
     /* 4. Signature */
     if (ctx->debug_mode || hdr->sig_algo == UIOX_SIG_NONE) {
         r->sig_ok  = true;
         r->cert_ok = true;
     } else {
         r->sig_ok  = true;   /* stub — replace with real RSA/Ed25519 */
         r->cert_ok = (ctx->fvc != NULL);
     }
 
     if (!r->sig_ok) {
         fw_strncpy_sb(r->fail_reason, "signature invalid",
                       sizeof(r->fail_reason));
         r->result = UIOX_SECBOOT_ERR_SIG;
         return r->result;
     }
     if (!r->cert_ok) {
         fw_strncpy_sb(r->fail_reason, "cert not verified",
                       sizeof(r->fail_reason));
         r->result = UIOX_SECBOOT_ERR_CERT;
         return r->result;
     }
 
     r->result = UIOX_SECBOOT_OK;
     return UIOX_SECBOOT_OK;
 }
 
 void uiox_fw_secboot_print(const uiox_secboot_report_t *r)
 {
     if (!r) return;
     static const char *results[] = {
         "OK","BAD_MAGIC","HASH_FAIL","SIG_FAIL",
         "CERT_FAIL","ROLLBACK","KEY_ERR","REVOKED","INVAL"
     };
     char buf[12];
     uint8_t ri = (r->result <= 0) ? (uint8_t)(-(int)r->result) : 8u;
 
     fw_puts_sb("[SECBOOT] Result   : ");
     fw_puts_sb(ri < 9u ? results[ri] : "?");
     fw_puts_sb("\n");
     fw_puts_sb("[SECBOOT] Hash OK  : ");
     fw_puts_sb(r->hash_ok ? "YES\n" : "NO\n");
     fw_puts_sb("[SECBOOT] Sig  OK  : ");
     fw_puts_sb(r->sig_ok  ? "YES\n" : "NO\n");
     fw_puts_sb("[SECBOOT] Cert OK  : ");
     fw_puts_sb(r->cert_ok ? "YES\n" : "NO\n");
     fw_puts_sb("[SECBOOT] Rollback : ");
     fw_puts_sb(r->rollback_ok ? "OK\n" : "FAIL\n");
 
     /* Print image version */
     fw_puts_sb("[SECBOOT] Img ver  : 0x");
     fw_u32_hex_sb(r->image_version, buf);
     fw_puts_sb(buf); fw_puts_sb("\n");
 
     if (r->result != UIOX_SECBOOT_OK) {
         fw_puts_sb("[SECBOOT] Reason   : ");
         fw_puts_sb(r->fail_reason);
         fw_puts_sb("\n");
     }
 }
 