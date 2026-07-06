/**
 * @file  uiox_fw_secboot.c
 * @brief UIOX Firmware — Secure Boot verification implementation.
 * @date  2026-07-06
 */
#include "uiox_fw.h"
#include "uiox_fw_secboot.h"
#include <string.h>

/* =========================================================================
 * SHA-256 implementation (NIST FIPS 180-4)
 * No division operators — safe on ARM32 bare-metal.
 * ====================================================================== */
static const uint32_t s_K[64] = {
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
    0x90BEFFFAu,0xA4506CEBu,0xBEF9A3F7u,0xC67178F2u
};

#define ROR32(x,n) (((x)>>(n))|((x)<<(32u-(n))))
#define SHA_S0(x)  (ROR32(x,2u)^ROR32(x,13u)^ROR32(x,22u))
#define SHA_S1(x)  (ROR32(x,6u)^ROR32(x,11u)^ROR32(x,25u))
#define SHA_G0(x)  (ROR32(x,7u)^ROR32(x,18u)^((x)>>3u))
#define SHA_G1(x)  (ROR32(x,17u)^ROR32(x,19u)^((x)>>10u))
#define SHA_CH(x,y,z) (((x)&(y))^(~(x)&(z)))
#define SHA_MJ(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))

static uint32_t sha_be32(const uint8_t *p)
{
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)
          |((uint32_t)p[2]<<8)|p[3];
}

static void sha256_block(uiox_fw_sha256_ctx_t *c, const uint8_t *b)
{
    uint32_t W[64], a,bb,cc,d,e,f,g,h;
    for (int i=0;i<16;i++) W[i]=sha_be32(b+i*4);
    for (int i=16;i<64;i++)
        W[i]=SHA_G1(W[i-2])+W[i-7]+SHA_G0(W[i-15])+W[i-16];
    a=c->state[0];bb=c->state[1];cc=c->state[2];d=c->state[3];
    e=c->state[4];f=c->state[5];g=c->state[6];h=c->state[7];
    for (int i=0;i<64;i++){
        uint32_t t1=h+SHA_S1(e)+SHA_CH(e,f,g)+s_K[i]+W[i];
        uint32_t t2=SHA_S0(a)+SHA_MJ(a,bb,cc);
        h=g;g=f;f=e;e=d+t1;d=cc;cc=bb;bb=a;a=t1+t2;
    }
    c->state[0]+=a;c->state[1]+=bb;c->state[2]+=cc;c->state[3]+=d;
    c->state[4]+=e;c->state[5]+=f; c->state[6]+=g; c->state[7]+=h;
}

void uiox_fw_sha256_init(uiox_fw_sha256_ctx_t *c)
{
    c->state[0]=0x6A09E667u;c->state[1]=0xBB67AE85u;
    c->state[2]=0x3C6EF372u;c->state[3]=0xA54FF53Au;
    c->state[4]=0x510E527Fu;c->state[5]=0x9B05688Cu;
    c->state[6]=0x1F83D9ABu;c->state[7]=0x5BE0CD19u;
    c->count=0;c->buflen=0;
}

void uiox_fw_sha256_update(uiox_fw_sha256_ctx_t *c,
                             const uint8_t *data, size_t len)
{
    while (len > 0u) {
        uint32_t take = 64u - c->buflen;
        if ((size_t)take > len) take = (uint32_t)len;
        memcpy(c->buf + c->buflen, data, take);
        c->buflen += take; c->count  += take;
        data += take; len -= take;
        if (c->buflen == 64u) {
            sha256_block(c, c->buf);
            c->buflen = 0u;
        }
    }
}

void uiox_fw_sha256_final(uiox_fw_sha256_ctx_t *c, uint8_t d[32])
{
    uint64_t bits = c->count * 8u;
    uint8_t  p    = 0x80u;
    uiox_fw_sha256_update(c, &p, 1u);
    while (c->buflen != 56u) { p=0u; uiox_fw_sha256_update(c,&p,1u); }
    uint8_t lb[8];
    for (int i=7;i>=0;i--){ lb[i]=(uint8_t)(bits&0xFFu); bits>>=8u; }
    uiox_fw_sha256_update(c, lb, 8u);
    for (int i=0;i<8;i++){
        d[i*4+0]=(uint8_t)(c->state[i]>>24u);
        d[i*4+1]=(uint8_t)(c->state[i]>>16u);
        d[i*4+2]=(uint8_t)(c->state[i]>>8u);
        d[i*4+3]=(uint8_t)(c->state[i]);
    }
}

void uiox_fw_sha256(const uint8_t *data, size_t len, uint8_t digest[32])
{
    uiox_fw_sha256_ctx_t c;
    uiox_fw_sha256_init(&c);
    uiox_fw_sha256_update(&c, data, len);
    uiox_fw_sha256_final(&c, digest);
}

void uiox_fw_sha256_hex(const uint8_t *data, size_t len, char hex[65])
{
    static const char h[] = "0123456789abcdef";
    uint8_t d[32];
    uiox_fw_sha256(data, len, d);
    for (int i=0;i<32;i++){
        hex[i*2+0] = h[(d[i]>>4)&0xFu];
        hex[i*2+1] = h[d[i]&0xFu];
    }
    hex[64] = '\0';
}

/* =========================================================================
 * Ed25519 verification stub
 * Replace with Monocypher crypto_eddsa_check() for production.
 * ====================================================================== */
static uiox_fw_err_t ed25519_verify(const uint8_t *msg,   size_t len,
                                      const uint8_t sig[64],
                                      const uint8_t pub[32])
{
    /* Stub: accepts any signature in LEVEL_SIGN mode.
     * Replace with: return monocypher_ed25519_check(sig,pub,msg,len)==0
     *               ? UIOX_FW_OK : UIOX_FW_ERR_SECBOOT; */
    (void)msg; (void)len; (void)sig; (void)pub;
    return UIOX_FW_OK;
}

/* =========================================================================
 * Public API
 * ====================================================================== */
uiox_fw_err_t uiox_fw_secboot_init(uiox_fw_secboot_ctx_t *ctx,
                                     uiox_secboot_level_t   level)
{
    if (!ctx) return UIOX_FW_ERR_INVAL;
    memset(ctx, 0, sizeof(*ctx));
    ctx->level    = level;
    ctx->verified = false;
    return UIOX_FW_OK;
}

uiox_fw_err_t uiox_fw_secboot_add_key(uiox_fw_secboot_ctx_t *ctx,
                                         const uint8_t pubkey[32],
                                         const char   *name)
{
    if (!ctx || !pubkey) return UIOX_FW_ERR_INVAL;
    if (ctx->num_keys >= UIOX_SECBOOT_MAX_KEYS) return UIOX_FW_ERR_FULL;
    uiox_fw_trusted_key_t *k = &ctx->keys[ctx->num_keys++];
    memcpy(k->pubkey, pubkey, UIOX_SECBOOT_PUBKEY_LEN);
    if (name) strncpy(k->name, name, sizeof(k->name) - 1u);
    k->valid = true;
    return UIOX_FW_OK;
}

uiox_fw_err_t uiox_fw_secboot_verify(uiox_fw_secboot_ctx_t *ctx,
                                        const void *img, size_t size)
{
    if (!ctx || !img) return UIOX_FW_ERR_INVAL;
    ctx->verified = false;

    /* Level OFF: skip all checks */
    if (ctx->level == UIOX_SECBOOT_LEVEL_OFF) {
        ctx->verified = true;
        strncpy(ctx->fail_reason, "verification disabled",
                sizeof(ctx->fail_reason) - 1u);
        return UIOX_FW_OK;
    }

    if (size < sizeof(uiox_fw_img_hdr_t)) {
        strncpy(ctx->fail_reason, "image too small for header",
                sizeof(ctx->fail_reason) - 1u);
        return UIOX_FW_ERR_SECBOOT;
    }

    const uiox_fw_img_hdr_t *hdr = (const uiox_fw_img_hdr_t *)img;

    /* Check magic */
    if (hdr->magic != UIOX_SECBOOT_IMG_MAGIC) {
        strncpy(ctx->fail_reason, "bad image magic",
                sizeof(ctx->fail_reason) - 1u);
        return UIOX_FW_ERR_SECBOOT;
    }
    if (hdr->header_version != 1u) {
        strncpy(ctx->fail_reason, "unsupported header version",
                sizeof(ctx->fail_reason) - 1u);
        return UIOX_FW_ERR_SECBOOT;
    }

    /* SHA-256 integrity check */
    const uint8_t *body     = (const uint8_t *)img + sizeof(*hdr);
    size_t         body_len = size - sizeof(*hdr);

    if ((uint64_t)size != hdr->image_size) {
        strncpy(ctx->fail_reason, "image_size header mismatch",
                sizeof(ctx->fail_reason) - 1u);
        return UIOX_FW_ERR_SECBOOT;
    }

    uint8_t digest[32];
    uiox_fw_sha256(body, body_len, digest);

    if (memcmp(digest, hdr->sha256, 32u) != 0) {
        strncpy(ctx->fail_reason, "SHA-256 digest mismatch",
                sizeof(ctx->fail_reason) - 1u);
        return UIOX_FW_ERR_SECBOOT;
    }

    /* Ed25519 signature check */
    if (ctx->level >= UIOX_SECBOOT_LEVEL_SIGN) {
        uiox_fw_err_t sig_rc = UIOX_FW_ERR_SECBOOT;
        for (uint8_t k = 0; k < ctx->num_keys; k++) {
            if (!ctx->keys[k].valid) continue;
            sig_rc = ed25519_verify(digest, 32u,
                                     hdr->signature,
                                     ctx->keys[k].pubkey);
            if (sig_rc == UIOX_FW_OK) break;
        }
        /* Also try the key embedded in the header itself */
        if (sig_rc != UIOX_FW_OK) {
            sig_rc = ed25519_verify(digest, 32u,
                                     hdr->signature,
                                     hdr->signer_key);
        }
        if (sig_rc != UIOX_FW_OK) {
            strncpy(ctx->fail_reason,
                    "Ed25519 signature verification failed",
                    sizeof(ctx->fail_reason) - 1u);
            return UIOX_FW_ERR_SECBOOT;
        }
    }

    /* TPM PCR measurement (level MEASURED) */
    if (ctx->level >= UIOX_SECBOOT_LEVEL_MEASURED) {
        /* Extend PCR[8] = SHA-256(PCR[8] || digest) */
        uint8_t pcr_data[64];
        memcpy(pcr_data,      ctx->measured_pcr, 32u);
        memcpy(pcr_data + 32, digest,             32u);
        uiox_fw_sha256(pcr_data, 64u, ctx->measured_pcr);
        /* Real impl: call TPM2_PCR_Extend(8, digest) via SPI/I2C */
    }

    ctx->verified = true;
    return UIOX_FW_OK;
}

void uiox_fw_secboot_print(const uiox_fw_secboot_ctx_t *ctx)
{
    if (!ctx) return;
    static const char *level_names[] = {
        "OFF", "HASH", "SIGN", "MEASURED"
    };
    uiox_fw_printf("  SecureBoot level  : %s\n",
                   ctx->level <= 3u ? level_names[ctx->level] : "?");
    uiox_fw_printf("  Verification      : %s\n",
                   ctx->verified ? "PASS" : "FAIL");
    if (!ctx->verified && ctx->fail_reason[0])
        uiox_fw_printf("  Fail reason       : %s\n", ctx->fail_reason);
    uiox_fw_printf("  Trusted keys      : %u\n", ctx->num_keys);
    for (uint8_t i = 0; i < ctx->num_keys; i++)
        if (ctx->keys[i].valid)
            uiox_fw_printf("    [%u] %s\n", i, ctx->keys[i].name);
}
