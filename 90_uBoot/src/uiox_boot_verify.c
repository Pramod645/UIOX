/*
 * uiox_boot_verify.c  —  SHA-256 + kernel image header check.
 */
#include "uiox_boot_verify.h"
#include "uiox_boot_console.h"
#include "uiox_boot_mem.h"

/* SHA-256 round constants */
static const uboot_u32_t K[64]={
0x428A2F98,0x71374491,0xB5C0FBCF,0xE9B5DBA5,0x3956C25B,0x59F111F1,
0x923F82A4,0xAB1C5ED5,0xD807AA98,0x12835B01,0x243185BE,0x550C7DC3,
0x72BE5D74,0x80DEB1FE,0x9BDC06A7,0xC19BF174,0xE49B69C1,0xEFBE4786,
0x0FC19DC6,0x240CA1CC,0x2DE92C6F,0x4A7484AA,0x5CB0A9DC,0x76F988DA,
0x983E5152,0xA831C66D,0xB00327C8,0xBF597FC7,0xC6E00BF3,0xD5A79147,
0x06CA6351,0x14292967,0x27B70A85,0x2E1B2138,0x4D2C6DFC,0x53380D13,
0x650A7354,0x766A0ABB,0x81C2C92E,0x92722C85,0xA2BFE8A1,0xA81A664B,
0xC24B8B70,0xC76C51A3,0xD192E819,0xD6990624,0xF40E3585,0x106AA070,
0x19A4C116,0x1E376C08,0x2748774C,0x34B0BCB5,0x391C0CB3,0x4ED8AA4A,
0x5B9CCA4F,0x682E6FF3,0x748F82EE,0x78A5636F,0x84C87814,0x8CC70208,
0x90BEFFFA,0xA4506CEB,0xBEF9A3F7,0xC67178F2};

#define R(x,n) (((x)>>(n))|((x)<<(32-(n))))
#define S0(x) (R(x,2)^R(x,13)^R(x,22))
#define S1(x) (R(x,6)^R(x,11)^R(x,25))
#define G0(x) (R(x,7)^R(x,18)^((x)>>3))
#define G1(x) (R(x,17)^R(x,19)^((x)>>10))
#define CH(x,y,z) (((x)&(y))^(~(x)&(z)))
#define MJ(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))

static uboot_u32_t be32(const uboot_u8_t *p)
{return((uboot_u32_t)p[0]<<24)|((uboot_u32_t)p[1]<<16)
      |((uboot_u32_t)p[2]<<8)|p[3];}

static void sha256_block(uboot_sha256_ctx_t *c, const uboot_u8_t *b)
{
    uboot_u32_t W[64],a,bb,cc,d,e,f,g,h;
    for(int i=0;i<16;i++) W[i]=be32(b+i*4);
    for(int i=16;i<64;i++) W[i]=G1(W[i-2])+W[i-7]+G0(W[i-15])+W[i-16];
    a=c->state[0];bb=c->state[1];cc=c->state[2];d=c->state[3];
    e=c->state[4];f=c->state[5];g=c->state[6];h=c->state[7];
    for(int i=0;i<64;i++){
        uboot_u32_t t1=h+S1(e)+CH(e,f,g)+K[i]+W[i];
        uboot_u32_t t2=S0(a)+MJ(a,bb,cc);
        h=g;g=f;f=e;e=d+t1;d=cc;cc=bb;bb=a;a=t1+t2;
    }
    c->state[0]+=a;c->state[1]+=bb;c->state[2]+=cc;c->state[3]+=d;
    c->state[4]+=e;c->state[5]+=f; c->state[6]+=g; c->state[7]+=h;
}

void uboot_sha256_init(uboot_sha256_ctx_t *c)
{
    c->state[0]=0x6A09E667;c->state[1]=0xBB67AE85;
    c->state[2]=0x3C6EF372;c->state[3]=0xA54FF53A;
    c->state[4]=0x510E527F;c->state[5]=0x9B05688C;
    c->state[6]=0x1F83D9AB;c->state[7]=0x5BE0CD19;
    c->count=0;c->buflen=0;
}

void uboot_sha256_update(uboot_sha256_ctx_t *c,
                          const uboot_u8_t *data, uboot_size_t len)
{
    while(len>0){
        uboot_u32_t take=64u-c->buflen;
        if((uboot_size_t)take>len) take=(uboot_u32_t)len;
        uboot_memcpy(c->buf+c->buflen,data,take);
        c->buflen+=take; c->count+=take;
        data+=take; len-=take;
        if(c->buflen==64){sha256_block(c,c->buf);c->buflen=0;}
    }
}

void uboot_sha256_final(uboot_sha256_ctx_t *c, uboot_u8_t d[32])
{
    uboot_u64_t bits=c->count*8u;
    uboot_u8_t p=0x80;
    uboot_sha256_update(c,&p,1);
    while(c->buflen!=56){p=0;uboot_sha256_update(c,&p,1);}
    uboot_u8_t lb[8];
    for(int i=7;i>=0;i--){lb[i]=(uboot_u8_t)(bits&0xFF);bits>>=8;}
    uboot_sha256_update(c,lb,8);
    for(int i=0;i<8;i++){
        d[i*4+0]=(uboot_u8_t)(c->state[i]>>24);
        d[i*4+1]=(uboot_u8_t)(c->state[i]>>16);
        d[i*4+2]=(uboot_u8_t)(c->state[i]>>8);
        d[i*4+3]=(uboot_u8_t)(c->state[i]);
    }
}

void uboot_sha256(const uboot_u8_t *data,uboot_size_t len,
                   uboot_u8_t digest[32])
{uboot_sha256_ctx_t c;uboot_sha256_init(&c);
uboot_sha256_update(&c,data,len);uboot_sha256_final(&c,digest);}

int uboot_verify_image(const void *img, uboot_size_t size)
{
    if(size<sizeof(uiox_kimg_hdr_t)) return UBOOT_EBADIMG;
    const uiox_kimg_hdr_t *h=(const uiox_kimg_hdr_t*)img;
    if(h->magic!=UIOX_KIMG_MAGIC) return UBOOT_EBADIMG;
    const uboot_u8_t *body=(const uboot_u8_t*)img+sizeof(uiox_kimg_hdr_t);
    uboot_size_t blen=size-sizeof(uiox_kimg_hdr_t);
    uboot_u8_t dig[32];
    uboot_sha256(body,blen,dig);
    if(uboot_memcmp(dig,h->sha256,32)!=0) return UBOOT_EVERIFY;
    return UBOOT_OK;
}

void uboot_verify_print(const uiox_kimg_hdr_t *h, int ok)
{
    uboot_puts("  Magic   : "); uboot_puthex32(h->magic); uboot_puts("\r\n");
    uboot_puts("  Arch    : "); uboot_puthex32(h->arch);  uboot_puts("\r\n");
    uboot_puts("  Load    : "); uboot_puthex64(h->load_addr);  uboot_puts("\r\n");
    uboot_puts("  Entry   : "); uboot_puthex64(h->entry_point); uboot_puts("\r\n");
    uboot_printf("  Size    : %u bytes\r\n",(uboot_u32_t)h->image_size);
    uboot_printf("  SHA-256 : %s\r\n", ok ? "PASS" : "FAIL");
}
