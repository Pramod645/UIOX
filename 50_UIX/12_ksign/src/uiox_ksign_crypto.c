/**
 * @file  uiox_ksign_crypto.c
 * @brief UIOX Signed Kernel — SHA-256, SHA-384, HMAC, RSA/ECDSA stubs.
 *        Zero libc — all primitives inline.
 * @date  2026-07-07
 */

 #include "../include/uiox_ksign_crypto.h"

 /* ── No-libc helpers ────────────────────────────────────────── */
 
 static void ks_memset(void *d, int v, size_t n)
 { uint8_t *p=(uint8_t*)d; while(n--)*p++=(uint8_t)v; }
 static void ks_memcpy(void *d, const void *s, size_t n)
 { uint8_t *dp=(uint8_t*)d; const uint8_t *sp=(const uint8_t*)s; while(n--)*dp++=*sp++; }
 
 /* =========================================================================
  * SHA-256 (identical to uiox_fw_secboot.c — shared algorithm)
  * ====================================================================== */
 
 #define RR32(x,n) (((x)>>(n))|((x)<<(32u-(n))))
 #define SCH(x,y,z) (((x)&(y))^(~(x)&(z)))
 #define SMAJ(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))
 #define SEP0(x) (RR32(x,2) ^RR32(x,13)^RR32(x,22))
 #define SEP1(x) (RR32(x,6) ^RR32(x,11)^RR32(x,25))
 #define SS0(x)  (RR32(x,7) ^RR32(x,18)^((x)>> 3))
 #define SS1(x)  (RR32(x,17)^RR32(x,19)^((x)>>10))
 
 static const uint32_t KS_K256[64] = {
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
 static const uint32_t KS_H0_256[8] = {
     0x6A09E667u,0xBB67AE85u,0x3C6EF372u,0xA54FF53Au,
     0x510E527Fu,0x9B05688Cu,0x1F83D9ABu,0x5BE0CD19u
 };
 
 static void ks_sha256_transform(uiox_ks_sha256_ctx_t *c, const uint8_t *blk)
 {
     uint32_t W[64],a,b,cd,d,e,f,g,h,t1,t2;
     for(int i=0;i<16;i++)
         W[i]=((uint32_t)blk[i*4]<<24)|((uint32_t)blk[i*4+1]<<16)
              |((uint32_t)blk[i*4+2]<<8)|(uint32_t)blk[i*4+3];
     for(int i=16;i<64;i++)
         W[i]=SS1(W[i-2])+W[i-7]+SS0(W[i-15])+W[i-16];
     a=c->state[0];b=c->state[1];cd=c->state[2];d=c->state[3];
     e=c->state[4];f=c->state[5];g=c->state[6];h=c->state[7];
     for(int i=0;i<64;i++){
         t1=h+SEP1(e)+SCH(e,f,g)+KS_K256[i]+W[i];
         t2=SEP0(a)+SMAJ(a,b,cd);
         h=g;g=f;f=e;e=d+t1;d=cd;cd=b;b=a;a=t1+t2;
     }
     c->state[0]+=a;c->state[1]+=b;c->state[2]+=cd;c->state[3]+=d;
     c->state[4]+=e;c->state[5]+=f;c->state[6]+=g;c->state[7]+=h;
 }
 
 void uiox_ks_sha256_init(uiox_ks_sha256_ctx_t *ctx)
 { for(int i=0;i<8;i++)ctx->state[i]=KS_H0_256[i];
   ctx->bit_count=0u;ctx->buf_len=0u; }
 
 void uiox_ks_sha256_update(uiox_ks_sha256_ctx_t *ctx,
                              const uint8_t *d, size_t len)
 {
     ctx->bit_count+=(uint64_t)len*8u;
     for(size_t i=0;i<len;i++){
         ctx->buf[ctx->buf_len++]=d[i];
         if(ctx->buf_len==64u){ks_sha256_transform(ctx,ctx->buf);ctx->buf_len=0u;}
     }
 }
 
 void uiox_ks_sha256_final(uiox_ks_sha256_ctx_t *ctx,
                             uint8_t digest[UIOX_KS_SHA256_LEN])
 {
     uint32_t i=ctx->buf_len;
     ctx->buf[i++]=0x80u;
     if(i>56u){while(i<64u)ctx->buf[i++]=0u;ks_sha256_transform(ctx,ctx->buf);i=0u;}
     while(i<56u)ctx->buf[i++]=0u;
     uint64_t bc=ctx->bit_count;
     for(int j=7;j>=0;j--){ctx->buf[56u+(uint32_t)j]=(uint8_t)(bc&0xFFu);bc>>=8u;}
     ks_sha256_transform(ctx,ctx->buf);
     for(int j=0;j<8;j++){
         digest[j*4+0]=(uint8_t)(ctx->state[j]>>24u);
         digest[j*4+1]=(uint8_t)(ctx->state[j]>>16u);
         digest[j*4+2]=(uint8_t)(ctx->state[j]>> 8u);
         digest[j*4+3]=(uint8_t)(ctx->state[j]     );
     }
 }
 
 void uiox_ks_sha256(const uint8_t *d, size_t l, uint8_t digest[32])
 {
     uiox_ks_sha256_ctx_t ctx;
     uiox_ks_sha256_init(&ctx);
     uiox_ks_sha256_update(&ctx,d,l);
     uiox_ks_sha256_final(&ctx,digest);
 }
 
 /* =========================================================================
  * SHA-384 (truncated SHA-512)
  * ====================================================================== */
 
 static const uint64_t KS_K384[80] = {
     0x428A2F98D728AE22ULL,0x7137449123EF65CDULL,0xB5C0FBCFEC4D3B2FULL,
     0xE9B5DBA58189DBBCULL,0x3956C25BF348B538ULL,0x59F111F1B605D019ULL,
     0x923F82A4AF194F9BULL,0xAB1C5ED5DA6D8118ULL,0xD807AA98A3030242ULL,
     0x12835B0145706FBEULL,0x243185BE4EE4B28CULL,0x550C7DC3D5FFB4E2ULL,
     0x72BE5D74F27B896FULL,0x80DEB1FE3B1696B1ULL,0x9BDC06A725C71235ULL,
     0xC19BF174CF692694ULL,0xE49B69C19EF14AD2ULL,0xEFBE4786384F25E3ULL,
     0x0FC19DC68B8CD5B5ULL,0x240CA1CC77AC9C65ULL,0x2DE92C6F592B0275ULL,
     0x4A7484AA6EA6E483ULL,0x5CB0A9DCBD41FBD4ULL,0x76F988DA831153B5ULL,
     0x983E5152EE66DFABULL,0xA831C66D2DB43210ULL,0xB00327C898FB213FULL,
     0xBF597FC7BEEF0EE4ULL,0xC6E00BF33DA88FC2ULL,0xD5A79147930AA725ULL,
     0x06CA6351E003826FULL,0x142929670A0E6E70ULL,0x27B70A8546D22FFCULL,
     0x2E1B21385C26C926ULL,0x4D2C6DFC5AC42AEDULL,0x53380D139D95B3DFULL,
     0x650A73548BAF63DEULL,0x766A0ABB3C77B2A8ULL,0x81C2C92E47EDAEE6ULL,
     0x92722C851482353BULL,0xA2BFE8A14CF10364ULL,0xA81A664BBC423001ULL,
     0xC24B8B70D0F89791ULL,0xC76C51A30654BE30ULL,0xD192E819D6EF5218ULL,
     0xD69906245565A910ULL,0xF40E35855771202AULL,0x106AA07032BBD1B8ULL,
     0x19A4C116B8D2D0C8ULL,0x1E376C085141AB53ULL,0x2748774CDF8EEB99ULL,
     0x34B0BCB5E19B48A8ULL,0x391C0CB3C5C95A63ULL,0x4ED8AA4AE3418ACBULL,
     0x5B9CCA4F7763E373ULL,0x682E6FF3D6B2B8A3ULL,0x748F82EE5DEFB2FCULL,
     0x78A5636F43172F60ULL,0x84C87814A1F0AB72ULL,0x8CC702081A6439ECULL,
     0x90BEFFFA23631E28ULL,0xA4506CEBDE82BDE9ULL,0xBEF9A3F7B2C67915ULL,
     0xC67178F2E372532BULL,0xCA273ECEEA26619CULL,0xD186B8C721C0C207ULL,
     0xEADA7DD6CDE0EB1EULL,0xF57D4F7FEE6ED178ULL,0x06F067AA72176FBAULL,
     0x0A637DC5A2C898A6ULL,0x113F9804BEF90DAEULL,0x1B710B35131C471BULL,
     0x28DB77F523047D84ULL,0x32CAAB7B40C72493ULL,0x3C9EBE0A15C9BEBCULL,
     0x431D67C49C100D4CULL,0x4CC5D4BECB3E42B6ULL,0x597F299CFC657E2AULL,
     0x5FCB6FAB3AD6FAECULL,0x6C44198C4A475817ULL,
 };
 static const uint64_t KS_H0_384[8] = {
     0xCBBB9D5DC1059ED8ULL,0x629A292A367CD507ULL,
     0x9159015A3070DD17ULL,0x152FECD8F70E5939ULL,
     0x67332667FFC00B31ULL,0x8EB44A8768581511ULL,
     0xDB0C2E0D64F98FA7ULL,0x47B5481DBEFA4FA4ULL
 };
 
 #define RR64(x,n) (((x)>>(n))|((x)<<(64u-(n))))
 #define SCH64(x,y,z) (((x)&(y))^(~(x)&(z)))
 #define SMAJ64(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))
 #define SEP0_64(x) (RR64(x,28)^RR64(x,34)^RR64(x,39))
 #define SEP1_64(x) (RR64(x,14)^RR64(x,18)^RR64(x,41))
 #define SS0_64(x)  (RR64(x, 1)^RR64(x, 8)^((x)>> 7))
 #define SS1_64(x)  (RR64(x,19)^RR64(x,61)^((x)>> 6))
 
 static void ks_sha384_transform(uiox_ks_sha384_ctx_t *c, const uint8_t *blk)
 {
     uint64_t W[80],a,b,cd,d,e,f,g,h,t1,t2;
     for(int i=0;i<16;i++){
         W[i]=0;
         for(int j=0;j<8;j++) W[i]=(W[i]<<8)|blk[i*8+j];
     }
     for(int i=16;i<80;i++)
         W[i]=SS1_64(W[i-2])+W[i-7]+SS0_64(W[i-15])+W[i-16];
     a=c->state[0];b=c->state[1];cd=c->state[2];d=c->state[3];
     e=c->state[4];f=c->state[5];g=c->state[6];h=c->state[7];
     for(int i=0;i<80;i++){
         t1=h+SEP1_64(e)+SCH64(e,f,g)+KS_K384[i]+W[i];
         t2=SEP0_64(a)+SMAJ64(a,b,cd);
         h=g;g=f;f=e;e=d+t1;d=cd;cd=b;b=a;a=t1+t2;
     }
     c->state[0]+=a;c->state[1]+=b;c->state[2]+=cd;c->state[3]+=d;
     c->state[4]+=e;c->state[5]+=f;c->state[6]+=g;c->state[7]+=h;
 }
 
 void uiox_ks_sha384_init(uiox_ks_sha384_ctx_t *ctx)
 { for(int i=0;i<8;i++)ctx->state[i]=KS_H0_384[i];
   ctx->bit_count_lo=0;ctx->bit_count_hi=0;ctx->buf_len=0; }
 
 void uiox_ks_sha384_update(uiox_ks_sha384_ctx_t *ctx,
                              const uint8_t *d, size_t len)
 {
     uint64_t old=ctx->bit_count_lo;
     ctx->bit_count_lo+=(uint64_t)len*8u;
     if(ctx->bit_count_lo<old)ctx->bit_count_hi++;
     for(size_t i=0;i<len;i++){
         ctx->buf[ctx->buf_len++]=d[i];
         if(ctx->buf_len==128u){ks_sha384_transform(ctx,ctx->buf);ctx->buf_len=0;}
     }
 }
 
 void uiox_ks_sha384_final(uiox_ks_sha384_ctx_t *ctx,
                             uint8_t digest[UIOX_KS_SHA384_LEN])
 {
     uint32_t i=ctx->buf_len;
     ctx->buf[i++]=0x80u;
     if(i>112u){while(i<128u)ctx->buf[i++]=0u;ks_sha384_transform(ctx,ctx->buf);i=0;}
     while(i<112u)ctx->buf[i++]=0u;
     uint64_t bclo=ctx->bit_count_lo,bchi=ctx->bit_count_hi;
     for(int j=7;j>=0;j--){ctx->buf[112+(uint32_t)j]=(uint8_t)(bchi&0xFF);bchi>>=8;}
     for(int j=7;j>=0;j--){ctx->buf[120+(uint32_t)j]=(uint8_t)(bclo&0xFF);bclo>>=8;}
     ks_sha384_transform(ctx,ctx->buf);
     for(int j=0;j<6;j++) /* only first 6 × 8 bytes = 48 bytes for SHA-384 */
         for(int k=7;k>=0;k--)
             digest[j*8+(7-k)]=(uint8_t)(ctx->state[j]>>((uint64_t)k*8u));
 }
 
 void uiox_ks_sha384(const uint8_t *d, size_t l, uint8_t digest[48])
 {
     uiox_ks_sha384_ctx_t ctx;
     uiox_ks_sha384_init(&ctx);
     uiox_ks_sha384_update(&ctx,d,l);
     uiox_ks_sha384_final(&ctx,digest);
 }
 
 /* =========================================================================
  * HMAC-SHA256
  * ====================================================================== */
 
 void uiox_ks_hmac_sha256(const uint8_t *key, size_t klen,
                            const uint8_t *data, size_t dlen,
                            uint8_t mac[UIOX_KS_SHA256_LEN])
 {
     uint8_t k[64], ipad[64], opad[64];
     ks_memset(k, 0, 64);
     if (klen > 64u) {
         uiox_ks_sha256(key, klen, k);
     } else {
         ks_memcpy(k, key, klen);
     }
     for (int i=0;i<64;i++) { ipad[i]=(uint8_t)(k[i]^0x36u); opad[i]=(uint8_t)(k[i]^0x5Cu); }
 
     uiox_ks_sha256_ctx_t ctx;
     uint8_t inner[UIOX_KS_SHA256_LEN];
     uiox_ks_sha256_init(&ctx);
     uiox_ks_sha256_update(&ctx, ipad, 64u);
     uiox_ks_sha256_update(&ctx, data, dlen);
     uiox_ks_sha256_final(&ctx, inner);
 
     uiox_ks_sha256_init(&ctx);
     uiox_ks_sha256_update(&ctx, opad, 64u);
     uiox_ks_sha256_update(&ctx, inner, UIOX_KS_SHA256_LEN);
     uiox_ks_sha256_final(&ctx, mac);
 }
 
 /* =========================================================================
  * RSA-PKCS#1 v1.5 verification stub
  * Production: replace with real modular exponentiation.
  * ====================================================================== */
 
 uiox_ks_err_t uiox_ks_rsa_verify(const uiox_ks_rsa_pubkey_t *key,
                                     const uint8_t *sig, uint32_t sig_len,
                                     const uint8_t digest[UIOX_KS_SHA256_LEN])
 {
     if (!key || !sig || sig_len == 0u || !digest) return UIOX_KS_ERR_INVAL;
     /*
      * Real implementation:
      *   1. m = sig^e mod n  (modular exponentiation)
      *   2. Remove PKCS#1 v1.5 padding: 0x00 0x01 0xFF... 0x00 DigestInfo
      *   3. Compare DigestInfo SHA-256 OID + digest against @digest
      *
      * Simulation: check that the last 32 bytes of sig match digest
      * XOR'd with a known test pattern (non-trivial to forge but not secure).
      */
     if (sig_len < UIOX_KS_SHA256_LEN) return UIOX_KS_ERR_SIG;
 
     /* In sim mode: accept if sig ends with SHA-256(digest || "UIOX") */
     uint8_t expected[UIOX_KS_SHA256_LEN];
     static const uint8_t salt[] = { 'U','I','O','X','_','S','I','G' };
     uiox_ks_sha256_ctx_t ctx;
     uiox_ks_sha256_init(&ctx);
     uiox_ks_sha256_update(&ctx, digest, UIOX_KS_SHA256_LEN);
     uiox_ks_sha256_update(&ctx, salt, 8u);
     uiox_ks_sha256_final(&ctx, expected);
 
     /* Constant-time compare of last 32 bytes of signature */
     return (uiox_ks_ct_memcmp(sig + sig_len - UIOX_KS_SHA256_LEN,
                                 expected, UIOX_KS_SHA256_LEN) == 0)
            ? UIOX_KS_OK : UIOX_KS_ERR_SIG;
 }
 
 /* =========================================================================
  * ECDSA-P256 stub
  * ====================================================================== */
 
 uiox_ks_err_t uiox_ks_ecdsa_verify(const uiox_ks_ecdsa_pubkey_t *key,
                                       const uint8_t sig[UIOX_KS_ECDSA_SIG_LEN],
                                       const uint8_t digest[UIOX_KS_SHA256_LEN])
 {
     if (!key || !sig || !digest) return UIOX_KS_ERR_INVAL;
     /* Sim: same salt-hash trick as RSA stub */
     uint8_t expected[UIOX_KS_SHA256_LEN];
     static const uint8_t salt[] = { 'U','I','O','X','_','E','C' };
     uiox_ks_sha256_ctx_t ctx;
     uiox_ks_sha256_init(&ctx);
     uiox_ks_sha256_update(&ctx, digest, UIOX_KS_SHA256_LEN);
     uiox_ks_sha256_update(&ctx, salt, 7u);
     uiox_ks_sha256_final(&ctx, expected);
     return (uiox_ks_ct_memcmp(sig, expected, UIOX_KS_SHA256_LEN) == 0)
            ? UIOX_KS_OK : UIOX_KS_ERR_SIG;
 }
 
 /* =========================================================================
  * Constant-time compare
  * ====================================================================== */
 
 int uiox_ks_ct_memcmp(const uint8_t *a, const uint8_t *b, size_t len)
 {
     uint8_t diff = 0u;
     for (size_t i=0;i<len;i++) diff |= (a[i] ^ b[i]);
     return (diff != 0u) ? 1 : 0;
 }
 
 /* =========================================================================
  * Secure memory wipe
  * ====================================================================== */
 
 void uiox_ks_memzero(volatile void *ptr, size_t len)
 {
     volatile uint8_t *p = (volatile uint8_t *)ptr;
     while (len--) *p++ = 0u;
 }
 