/**
 * @file  uiox_boot_verify.c
 * @brief UIOX Bootloader — SHA-256 (RFC 6234) and image header check.
 * @date  2026-06-12
 */

 #include "uiox_boot.h"

 /* =========================================================================
  * SHA-256 implementation (RFC 6234)
  * ====================================================================== */
 
 static const uint32_t K[64] = {
     0x428A2F98u, 0x71374491u, 0xB5C0FBCFu, 0xE9B5DBA5u,
     0x3956C25Bu, 0x59F111F1u, 0x923F82A4u, 0xAB1C5ED5u,
     0xD807AA98u, 0x12835B01u, 0x243185BEu, 0x550C7DC3u,
     0x72BE5D74u, 0x80DEB1FEu, 0x9BDC06A7u, 0xC19BF174u,
     0xE49B69C1u, 0xEFBE4786u, 0x0FC19DC6u, 0x240CA1CCu,
     0x2DE92C6Fu, 0x4A7484AAu, 0x5CB0A9DCu, 0x76F988DAu,
     0x983E5152u, 0xA831C66Du, 0xB00327C8u, 0xBF597FC7u,
     0xC6E00BF3u, 0xD5A79147u, 0x06CA6351u, 0x14292967u,
     0x27B70A85u, 0x2E1B2138u, 0x4D2C6DFCu, 0x53380D13u,
     0x650A7354u, 0x766A0ABBu, 0x81C2C92Eu, 0x92722C85u,
     0xA2BFE8A1u, 0xA81A664Bu, 0xC24B8B70u, 0xC76C51A3u,
     0xD192E819u, 0xD6990624u, 0xF40E3585u, 0x106AA070u,
     0x19A4C116u, 0x1E376C08u, 0x2748774Cu, 0x34B0BCB5u,
     0x391C0CB3u, 0x4ED8AA4Au, 0x5B9CCA4Fu, 0x682E6FF3u,
     0x748F82EEu, 0x78A5636Fu, 0x84C87814u, 0x8CC70208u,
     0x90BEFFFAu, 0xA4506CEBu, 0xBEF9A3F7u, 0xC67178F2u
 };
 
 static const uint32_t H0[8] = {
     0x6A09E667u, 0xBB67AE85u, 0x3C6EF372u, 0xA54FF53Au,
     0x510E527Fu, 0x9B05688Cu, 0x1F83D9ABu, 0x5BE0CD19u
 };
 
 #define ROTR32(x,n) (((x) >> (n)) | ((x) << (32u - (n))))
 #define CH(x,y,z)   (((x) & (y)) ^ (~(x) & (z)))
 #define MAJ(x,y,z)  (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
 #define EP0(x)      (ROTR32(x,2)  ^ ROTR32(x,13) ^ ROTR32(x,22))
 #define EP1(x)      (ROTR32(x,6)  ^ ROTR32(x,11) ^ ROTR32(x,25))
 #define SIG0(x)     (ROTR32(x,7)  ^ ROTR32(x,18) ^ ((x) >> 3))
 #define SIG1(x)     (ROTR32(x,17) ^ ROTR32(x,19) ^ ((x) >> 10))
 
 static void sha256_transform(uiox_sha256_ctx_t *ctx,
                               const uint8_t data[64])
 {
     uint32_t W[64], a, b, c, d, e, f, g, h, t1, t2;
     for (int i = 0; i < 16; i++)
         W[i] = ((uint32_t)data[i*4]   << 24u)
              | ((uint32_t)data[i*4+1] << 16u)
              | ((uint32_t)data[i*4+2] <<  8u)
              |  (uint32_t)data[i*4+3];
     for (int i = 16; i < 64; i++)
         W[i] = SIG1(W[i-2]) + W[i-7] + SIG0(W[i-15]) + W[i-16];
 
     a = ctx->state[0]; b = ctx->state[1];
     c = ctx->state[2]; d = ctx->state[3];
     e = ctx->state[4]; f = ctx->state[5];
     g = ctx->state[6]; h = ctx->state[7];
 
     for (int i = 0; i < 64; i++) {
         t1 = h + EP1(e) + CH(e,f,g) + K[i] + W[i];
         t2 = EP0(a) + MAJ(a,b,c);
         h = g; g = f; f = e; e = d + t1;
         d = c; c = b; b = a; a = t1 + t2;
     }
     ctx->state[0] += a; ctx->state[1] += b;
     ctx->state[2] += c; ctx->state[3] += d;
     ctx->state[4] += e; ctx->state[5] += f;
     ctx->state[6] += g; ctx->state[7] += h;
 }
 
 void uiox_sha256_init(uiox_sha256_ctx_t *ctx)
 {
     for (int i = 0; i < 8; i++) ctx->state[i] = H0[i];
     ctx->bit_count = 0u;
     ctx->buf_len   = 0u;
 }
 
 void uiox_sha256_update(uiox_sha256_ctx_t *ctx,
                          const uint8_t *data, size_t len)
 {
     ctx->bit_count += (uint64_t)len * 8u;
     for (size_t i = 0u; i < len; i++) {
         ctx->buf[ctx->buf_len++] = data[i];
         if (ctx->buf_len == 64u) {
             sha256_transform(ctx, ctx->buf);
             ctx->buf_len = 0u;
         }
     }
 }
 
 void uiox_sha256_final(uiox_sha256_ctx_t *ctx, uint8_t digest[32])
 {
     uint32_t i = ctx->buf_len;
     ctx->buf[i++] = 0x80u;
     if (i > 56u) {
         while (i < 64u) ctx->buf[i++] = 0u;
         sha256_transform(ctx, ctx->buf);
         i = 0u;
     }
     while (i < 56u) ctx->buf[i++] = 0u;
     /* Append bit count big-endian */
     uint64_t bc = ctx->bit_count;
     for (int j = 7; j >= 0; j--) {
         ctx->buf[56u + (uint32_t)j] = (uint8_t)(bc & 0xFFu);
         bc >>= 8u;
     }
     sha256_transform(ctx, ctx->buf);
     for (int j = 0; j < 8; j++) {
         digest[j*4+0] = (uint8_t)(ctx->state[j] >> 24u);
         digest[j*4+1] = (uint8_t)(ctx->state[j] >> 16u);
         digest[j*4+2] = (uint8_t)(ctx->state[j] >>  8u);
         digest[j*4+3] = (uint8_t)(ctx->state[j]        );
     }
 }
 
 void uiox_sha256(const uint8_t *data, size_t len, uint8_t digest[32])
 {
     uiox_sha256_ctx_t ctx;
     uiox_sha256_init(&ctx);
     uiox_sha256_update(&ctx, data, len);
     uiox_sha256_final(&ctx, digest);
 }
 
 /* =========================================================================
  * Image header verification
  * ====================================================================== */
 
 uiox_boot_err_t uiox_boot_verify_image(const uiox_image_hdr_t *hdr,
                                          const uint8_t *payload,
                                          size_t pay_len,
                                          uiox_arch_t expected_arch)
 {
     if (!hdr || !payload) return UIOX_BOOT_ERR_INVAL;
 
     /* Magic check */
     if (hdr->magic != UIOX_IMAGE_MAGIC) {
         BOOT_ERR("bad image magic: 0x%08x", hdr->magic);
         return UIOX_BOOT_ERR_BADMAGIC;
     }
     /* Version check */
     if (hdr->version != UIOX_IMAGE_HDR_VERSION) {
         BOOT_ERR("unsupported image version: %u", hdr->version);
         return UIOX_BOOT_ERR_BADMAGIC;
     }
     /* Architecture match */
     if (hdr->arch != (uint32_t)expected_arch) {
         BOOT_ERR("arch mismatch: hdr=%u expected=%u",
                  hdr->arch, (uint32_t)expected_arch);
         return UIOX_BOOT_ERR_INVAL;
     }
     /* Size sanity */
     if (hdr->image_size == 0u || hdr->image_size != (uint64_t)pay_len) {
         BOOT_ERR("image size mismatch: hdr=%llu payload=%llu",
                  (unsigned long long)hdr->image_size,
                  (unsigned long long)pay_len);
         return UIOX_BOOT_ERR_INVAL;
     }
     /* SHA-256 check */
     uint8_t digest[32];
     uiox_sha256(payload, pay_len, digest);
     if (uiox_boot_memcmp(digest, hdr->sha256, 32u) != 0) {
         BOOT_ERR("SHA-256 mismatch — image corrupted");
         return UIOX_BOOT_ERR_BADCSUM;
     }
     return UIOX_BOOT_OK;
 }
  