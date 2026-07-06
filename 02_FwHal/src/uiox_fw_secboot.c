/**
 * @file    uiox_fw_secboot.c
 * @brief   UIOX Firmware — Secure Boot implementation.
 * @date    2026-07-06
 */
#include "uiox_fw.h"
#include <string.h>

/* ── SHA-256 ─────────────────────────────────────────────── */
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
    0x90BEFFFAu,0xA4506CEBu,0xBEF9A3F7u,0xC67178F2u
};

#define RR(x,n) (((x)>>(n))|((x)<<(32u-(n))))
#define SB0(x) (RR(x,2u)^RR(x,13u)^RR(x,22u))
#define SB1(x) (RR(x,6u)^RR(x,11u)^RR(x,25u))
#define GB0(x) (RR(x,7u)^RR(x,18u)^((x)>>3u))
#define GB1(x) (RR(x,17u)^RR(x,19u)^((x)>>10u))
#define CH(x,y,z) (((x)&(y))^(~(x)&(z)))
#define MJ(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))

static uint32_t be32_sb(const uint8_t *p)
{ return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)
        |((uint32_t)p[2]<<8)|p[3]; }

static void sha256_block(uiox_sb_sha256_ctx_t *c, const uint8_t *b)
{
    uint32_t W[64],a,bb,cc,d,e,f,g,h;
    for(int i=0;i<16;i++) W[i]=be32_sb(b+i*4);
    for(int i=16;i<64;i++) W[i]=GB1(W[i-2])+W[i-7]+GB0(W[i-15])+W[i-16];
    a=c->state[0];bb=c->state[1];cc=c->state[2];d=c->state[3];
    e=c->state[4];f=c->state[5];g=c->state[6];h=c->state[7];
    for(int i=0;i<64;i++){
        uint32_t t1=h+SB1(e)+CH(e,f,g)+K256[i]+W[i];
        uint32_t t2=SB0(a)+MJ(a,bb,cc);
        h=g;g=f;f=e;e=d+t1;d=cc;cc=bb;bb=a;a=t1+t2;
    }
    c->state[0]+=a;c->state[1]+=bb;c->state[2]+=cc;c->state[3]+=d;
    c->state[4]+=e;c->state[5]+=f; c->state[6]+=g; c->state[7]+=h;
}

void uiox_sb_sha256_init(uiox_sb_sha256_ctx_t *c)
{
    c->state[0]=0x6A09E667u;c->state[1]=0xBB67AE85u;
    c->state[2]=0x3C6EF372u;c->state[3]=0xA54FF53Au;
    c->state[4]=0x510E527Fu;c->state[5]=0x9B05688Cu;
    c->state[6]=0x1F83D9ABu;c->state[7]=0x5BE0CD19u;
    c->count=0;c->buflen=0;
}

void uiox_sb_sha256_update(uiox_sb_sha256_ctx_t *c,
                             const uint8_t *data, uint32_t len)
{
    while(len>0){
        uint32_t take=64u-c->buflen;
        if(take>(uint32_t)len) take=len;
        memcpy(c->buf+c->buflen,data,take);
        c->buflen+=take;c->count+=take;
        data+=take;len-=take;
        if(c->buflen==64u){sha256_block(c,c->buf);c->buflen=0;}
    }
}

void uiox_sb_sha256_final(uiox_sb_sha256_ctx_t *c, uint8_t d[32])
{
    uint64_t bits=c->count*8u;
    uint8_t p=0x80u;
    uiox_sb_sha256_update(c,&p,1);
    while(c->buflen!=56u){p=0;uiox_sb_sha256_update(c,&p,1);}
    uint8_t lb[8];
    for(int i=7;i>=0;i--){lb[i]=(uint8_t)(bits&0xFFu);bits>>=8;}
    uiox_sb_sha256_update(c,lb,8);
    for(int i=0;i<8;i++){
        d[i*4+0]=(uint8_t)(c->state[i]>>24);
        d[i*4+1]=(uint8_t)(c->state[i]>>16);
        d[i*4+2]=(uint8_t)(c->state[i]>>8);
        d[i*4+3]=(uint8_t)(c->state[i]);
    }
}

void uiox_sb_sha256(const uint8_t *data, uint32_t len, uint8_t d[32])
{
    uiox_sb_sha256_ctx_t c;
    uiox_sb_sha256_init(&c);
    uiox_sb_sha256_update(&c, data, len);
    uiox_sb_sha256_final(&c, d);
}

/* ── Ed25519 stub (replace with Monocypher in production) ── */
uiox_fw_err_t uiox_sb_ed25519_verify(const uint8_t *msg, uint32_t msg_len,
                                       const uint8_t  sig[64],
                                       const uint8_t  pubkey[32])
{
    (void)msg; (void)msg_len; (void)sig; (void)pubkey;
    /*
     * Production: replace with:
     *   return crypto_eddsa_check(sig, pubkey, msg, msg_len) == 0
     *          ? UIOX_FW_OK : UIOX_FW_ERR_SECURITY;
     * using Monocypher ([monocypher.org](https://monocypher.org)
     * Stub always passes for QEMU simulation.
     */
    return UIOX_FW_OK;
}

/* ── Secure Boot API ─────────────────────────────────────── */
uiox_fw_err_t uiox_fw_secboot_init(uiox_fw_secboot_ctx_t *ctx,
                                     const uint8_t root_pubkey[32],
                                     uint32_t flags)
{
    if (!ctx) return UIOX_FW_ERR_INVAL;
    memset(ctx, 0, sizeof(*ctx));
    ctx->flags = flags;
    if (root_pubkey)
        memcpy(ctx->root_pubkey, root_pubkey, UIOX_SB_PUBKEY_LEN);
    ctx->min_fw_version = 1u;
    ctx->min_kn_version = 1u;
    return UIOX_FW_OK;
}

static uiox_fw_err_t verify_image(uiox_fw_secboot_ctx_t *ctx,
                                    const void *img_base,
                                    uint32_t    img_size,
                                    const char *role)
{
    if (!ctx || !img_base || img_size < sizeof(uiox_sb_header_t))
        return UIOX_FW_ERR_INVAL;

    const uiox_sb_header_t *hdr = (const uiox_sb_header_t *)img_base;

    /* 1. Magic check */
    if (hdr->magic != UIOX_SB_MAGIC) {
        uiox_fw_printf("  [secboot] %s: bad magic 0x%08x\n",
                        role, hdr->magic);
        return (ctx->flags & UIOX_SECBOOT_F_ENFORCE_SIG)
               ? UIOX_FW_ERR_SECURITY : UIOX_FW_OK;
    }

    /* 2. SHA-256 check of image body (after header) */
    const uint8_t *body = (const uint8_t *)img_base + sizeof(uiox_sb_header_t);
    uint32_t body_size  = img_size - (uint32_t)sizeof(uiox_sb_header_t);
    uint8_t computed[32];
    uiox_sb_sha256(body, body_size, computed);

    if (memcmp(computed, hdr->sha256, 32) != 0) {
        uiox_fw_printf("  [secboot] %s: SHA-256 MISMATCH\n", role);
        if (ctx->flags & UIOX_SECBOOT_F_ENFORCE_HASH)
            return UIOX_FW_ERR_SECURITY;
    }

    /* 3. Ed25519 signature over sha256 field */
    uiox_fw_err_t sig_rc =
        uiox_sb_ed25519_verify(hdr->sha256, 32,
                                 hdr->signature, ctx->root_pubkey);
    if (sig_rc != UIOX_FW_OK) {
        uiox_fw_printf("  [secboot] %s: signature INVALID\n", role);
        if (ctx->flags & UIOX_SECBOOT_F_ENFORCE_SIG)
            return UIOX_FW_ERR_SECURITY;
    }

    /* 4. Rollback version */
    uint32_t min = (role[0] == 'k') ? ctx->min_kn_version
                                     : ctx->min_fw_version;
    uiox_fw_err_t rv =
        uiox_fw_secboot_check_version(ctx, hdr->version, min);
    if (rv != UIOX_FW_OK) {
        uiox_fw_printf("  [secboot] %s: rollback detected v%u < min v%u\n",
                        role, hdr->version, min);
        if (ctx->flags & UIOX_SECBOOT_F_ROLLBACK_CHK)
            return UIOX_FW_ERR_SECURITY;
    }

    /* 5. Measure */
    if (ctx->flags & UIOX_SECBOOT_F_MEASURE)
        uiox_fw_secboot_measure(ctx, role, body, body_size);

    uiox_fw_printf("  [secboot] %s: verified OK  v%u\n",
                    role, hdr->version);
    return UIOX_FW_OK;
}

uiox_fw_err_t uiox_fw_secboot_verify_fw(uiox_fw_secboot_ctx_t *ctx,
                                          const void *img_base,
                                          uint32_t    img_size)
{
    uiox_fw_err_t rc = verify_image(ctx, img_base, img_size, "firmware");
    if (rc == UIOX_FW_OK) ctx->boot_verified = true;
    return rc;
}

uiox_fw_err_t uiox_fw_secboot_verify_kernel(uiox_fw_secboot_ctx_t *ctx,
                                              const void *img_base,
                                              uint32_t    img_size)
{
    uiox_fw_err_t rc = verify_image(ctx, img_base, img_size, "kernel");
    if (rc == UIOX_FW_OK) ctx->kernel_verified = true;
    return rc;
}

uiox_fw_err_t uiox_fw_secboot_measure(uiox_fw_secboot_ctx_t *ctx,
                                        const char   *component,
                                        const void   *data,
                                        uint32_t      size)
{
    if (!ctx || !component || !data) return UIOX_FW_ERR_INVAL;
    if (ctx->log_count >= UIOX_SB_MAX_MEASUREMENTS) return UIOX_FW_ERR_INVAL;
    uiox_sb_measurement_t *m = &ctx->log[ctx->log_count++];
    memset(m, 0, sizeof(*m));
    uint32_t i = 0;
    while(component[i] && i < sizeof(m->component)-1u)
        { m->component[i]=component[i]; i++; }
    uiox_sb_sha256((const uint8_t *)data, size, m->hash);
    m->timestamp_us = uiox_fw_hw_timestamp_us();
    return UIOX_FW_OK;
}

void uiox_fw_secboot_lock_debug(uiox_fw_secboot_ctx_t *ctx)
{
    if (!ctx) return;
    if (!(ctx->flags & UIOX_SECBOOT_F_LOCK_DEBUG)) return;
#if defined(__aarch64__)
    /* OSDLR_EL1 — lock OS double lock (debug registers) */
    __asm__ volatile("msr OSDLR_EL1, %0" :: "r"(1ULL) : "memory");
    __asm__ volatile("isb" ::: "memory");
    uiox_fw_printf("  [secboot] Debug ports locked\n");
#endif
    ctx->debug_locked = true;
}

uiox_fw_err_t uiox_fw_secboot_check_version(
    const uiox_fw_secboot_ctx_t *ctx,
    uint32_t image_version, uint32_t min_version)
{
    (void)ctx;
    return (image_version >= min_version) ? UIOX_FW_OK : UIOX_FW_ERR_SECURITY;
}

void uiox_fw_secboot_print(const uiox_fw_secboot_ctx_t *ctx)
{
    if (!ctx) return;
    static const char hex[] = "0123456789abcdef";
    uiox_fw_printf("  Secure Boot:\n");
    uiox_fw_printf("    Flags          : 0x%08x\n", ctx->flags);
    uiox_fw_printf("    FW verified    : %s\n",
                    ctx->boot_verified   ? "YES" : "NO");
    uiox_fw_printf("    Kernel verified: %s\n",
                    ctx->kernel_verified ? "YES" : "NO");
    uiox_fw_printf("    Debug locked   : %s\n",
                    ctx->debug_locked    ? "YES" : "NO");
    uiox_fw_printf("    Root pubkey    : ");
    for (int i=0;i<8;i++){
        uiox_fw_putc(hex[(ctx->root_pubkey[i]>>4)&0xF]);
        uiox_fw_putc(hex[ctx->root_pubkey[i]&0xF]);
    }
    uiox_fw_puts("...\n");
    uiox_fw_printf("    Measurements   : %u\n", ctx->log_count);
    for (uint8_t i=0;i<ctx->log_count;i++) {
        const uiox_sb_measurement_t *m = &ctx->log[i];
        uiox_fw_printf("      [%u] %-12s  ", i, m->component);
        for(int j=0;j<8;j++){
            uiox_fw_putc(hex[(m->hash[j]>>4)&0xF]);
            uiox_fw_putc(hex[m->hash[j]&0xF]);
        }
        uiox_fw_puts("...\n");
    }
}
