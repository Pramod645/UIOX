/*
 * uiox_boot_verify.c - SHA-256 + UIOX image header verification.
 */
#include "uiox_boot_verify.h"
#include "uiox_boot_console.h"
#include "uiox_boot_mem.h"
#include <string.h>

/* -- SHA-256 constants -------------------------------------- */
static const uboot_u32_t K[64] = {
    0x428A2F98,0x71374491,0xB5C0FBCF,0xE9B5DBA5,
    0x3956C25B,0x59F111F1,0x923F82A4,0xAB1C5ED5,
    0xD807AA98,0x12835B01,0x243185BE,0x550C7DC3,
    0x72BE5D74,0x80DEB1FE,0x9BDC06A7,0xC19BF174,
    0xE49B69C1,0xEFBE4786,0x0FC19DC6,0x240CA1CC,
    0x2DE92C6F,0x4A7484AA,0x5CB0A9DC,0x76F988DA,
    0x983E5152,0xA831C66D,0xB00327C8,0xBF597FC7,
    0xC6E00BF3,0xD5A79147,0x06CA6351,0x14292967,
    0x27B70A85,0x2E1B2138,0x4D2C6DFC,0x53380D13,
    0x650A7354,0x766A0ABB,0x81C2C92E,0x92722C85,
    0xA2BFE8A1,0xA81A664B,0xC24B8B70,0xC76C51A3,
    0xD192E819,0xD6990624,0xF40E3585,0x106AA070,
    0x19A4C116,0x1E376C08,0x2748774C,0x34B0BCB5,
    0x391C0CB3,0x4ED8AA4A,0x5B9CCA4F,0x682E6FF3,
    0x748F82EE,0x78A5636F,0x84C87814,0x8CC70208,
    0x90BEFFFA,0xA4506CEB,0xBEF9A3F7,0xC67178F2
};

#define ROR32(x,n) (((x)>>(n))|((x)<<(32-(n))))
#define S0(x) (ROR32(x,2) ^ ROR32(x,13) ^ ROR32(x,22))
#define S1(x) (ROR32(x,6) ^ ROR32(x,11) ^ ROR32(x,25))
#define G0(x) (ROR32(x,7) ^ ROR32(x,18) ^ ((x)>>3))
#define G1(x) (ROR32(x,17)^ ROR32(x,19) ^ ((x)>>10))
#define CH(x,y,z) (((x)&(y))^(~(x)&(z)))
#define MAJ(x,y,z)(((x)&(y))^((x)&(z))^((y)&(z)))

static uboot_u32_t be32(const uboot_u8_t *p)
{
    return ((uboot_u32_t)p[0]<<24)|((uboot_u32_t)p[1]<<16)
          |((uboot_u32_t)p[2]<<8) | p[3];
}

static void sha256_process(uboot_sha256_ctx_t *ctx,
                             const uboot_u8_t blk[64])
{
    uboot_u32_t W[64], a,b,c,d,e,f,g,h;
    for (int i=0;i<16;i++) W[i]=be32(blk+i*4);
    for (int i=16;i<64;i++)
        W[i]=G1(W[i-2])+W[i-7]+G0(W[i-15])+W[i-16];
    a=ctx->state[0]; b=ctx->state[1]; c=ctx->state[2]; d=ctx->state[3];
    e=ctx->state[4]; f=ctx->state[5]; g=ctx->state[6]; h=ctx->state[7];
    for (int i=0;i<64;i++) {
        uboot_u32_t t1=h+S1(e)+CH(e,f,g)+K[i]+W[i];
        uboot_u32_t t2=S0(a)+MAJ(a,b,c);
        h=g; g=f; f=e; e=d+t1;
        d=c; c=b; b=a; a=t1+t2;
    }
    ctx->state[0]+=a; ctx->state[1]+=b; ctx->state[2]+=c;
    ctx->state[3]+=d; ctx->state[4]+=e; ctx->state[5]+=f;
    ctx->state[6]+=g; ctx->state[7]+=h;
}

void uboot_sha256_init(uboot_sha256_ctx_t *ctx)
{
    ctx->state[0]=0x6A09E667; ctx->state[1]=0xBB67AE85;
    ctx->state[2]=0x3C6EF372; ctx->state[3]=0xA54FF53A;
    ctx->state[4]=0x510E527F; ctx->state[5]=0x9B05688C;
    ctx->state[6]=0x1F83D9AB; ctx->state[7]=0x5BE0CD19;
    ctx->count=0; ctx->buflen=0;
}

void uboot_sha256_update(uboot_sha256_ctx_t *ctx,
                          const uboot_u8_t *data, uboot_size_t len)
{
    while (len > 0) {
        uboot_u32_t take = 64u - ctx->buflen;
        if ((uboot_size_t)take > len) take = (uboot_u32_t)len;
        uboot_memcpy(ctx->buf + ctx->buflen, data, take);
        ctx->buflen += take;
        ctx->count  += take;
        data += take; len -= take;
        if (ctx->buflen == 64) {
            sha256_process(ctx, ctx->buf);
            ctx->buflen = 0;
        }
    }
}

void uboot_sha256_final(uboot_sha256_ctx_t *ctx, uboot_u8_t digest[32])
{
    uboot_u64_t bitlen = ctx->count * 8u;
    uboot_u8_t pad = 0x80;
    uboot_sha256_update(ctx, &pad, 1);
    while (ctx->buflen != 56)
        { pad=0; uboot_sha256_update(ctx,&pad,1); }
    uboot_u8_t len_be[8];
    for (int i=7;i>=0;i--) { len_be[i]=(uboot_u8_t)(bitlen&0xFF); bitlen>>=8; }
    uboot_sha256_update(ctx, len_be, 8);
    for (int i=0;i<8;i++) {
        digest[i*4+0]=(uboot_u8_t)(ctx->state[i]>>24);
        digest[i*4+1]=(uboot_u8_t)(ctx->state[i]>>16);
        digest[i*4+2]=(uboot_u8_t)(ctx->state[i]>>8);
        digest[i*4+3]=(uboot_u8_t)(ctx->state[i]);
    }
}

void uboot_sha256(const uboot_u8_t *data, uboot_size_t len,
                   uboot_u8_t digest[32])
{
    uboot_sha256_ctx_t ctx;
    uboot_sha256_init(&ctx);
    uboot_sha256_update(&ctx, data, len);
    uboot_sha256_final(&ctx, digest);
}

int uboot_verify_image(const void *img, uboot_size_t size)
{
    if (size < sizeof(uiox_kimg_hdr_t)) return UBOOT_EBADIMG;
    const uiox_kimg_hdr_t *hdr = (const uiox_kimg_hdr_t *)img;
    if (hdr->magic != UIOX_KIMG_MAGIC) return UBOOT_EBADIMG;
    /* compute SHA-256 over image body (after header)            */
    const uboot_u8_t *body = (const uboot_u8_t *)img
                              + sizeof(uiox_kimg_hdr_t);
    uboot_size_t body_len  = size - sizeof(uiox_kimg_hdr_t);
    uboot_u8_t  digest[32];
    uboot_sha256(body, body_len, digest);
    if (uboot_memcmp(digest, hdr->sha256, 32) != 0)
        return UBOOT_EVERIFY;
    return UBOOT_OK;
}

void uboot_verify_print(const uiox_kimg_hdr_t *hdr, int ok)
{
    uboot_printf("  Image magic   : ");
    uboot_puthex32(hdr->magic);
    uboot_printf("\r\n  Arch          : ");
    uboot_puthex32(hdr->arch);
    uboot_printf("\r\n  Load addr     : ");
    uboot_puthex64(hdr->load_addr);
    uboot_printf("\r\n  Entry point   : ");
    uboot_puthex64(hdr->entry_point);
    uboot_printf("\r\n  Image size    : %u bytes\r\n",
        (uboot_u32_t)hdr->image_size);
    uboot_printf("  SHA-256       : %s\r\n", ok ? "PASS" : "FAIL");
}
