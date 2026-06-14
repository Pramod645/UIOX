/*
 * uiox_boot_mem.c  —  Memory map + bump allocator.
 */
#include "uiox_boot_mem.h"
#include "uiox_boot_console.h"

/* ── bump allocator ─────────────────────────────────────── */
static uboot_addr_t g_base=0;
static uboot_size_t g_size=0, g_used=0;

void uboot_heap_init(uboot_addr_t b,uboot_size_t s)
{g_base=b;g_size=s;g_used=0;}

void *uboot_alloc(uboot_size_t bytes, uboot_size_t align)
{
    if(!align) align=8u;
    uboot_size_t off=(g_used+align-1u)&~(align-1u);
    if(off+bytes>g_size) return NULL;
    g_used=off+bytes;
    return (void*)(uboot_addr_t)(g_base+off);
}
void uboot_heap_reset(void){g_used=0;}
uboot_size_t uboot_heap_used(void){return g_used;}

/* ── libc stubs ─────────────────────────────────────────── */
void uboot_memset(void *d,uboot_u8_t v,uboot_size_t n)
{uboot_u8_t *p=(uboot_u8_t*)d;while(n--)  *p++=v;}
void uboot_memcpy(void *d,const void *s,uboot_size_t n)
{uboot_u8_t *dd=(uboot_u8_t*)d;const uboot_u8_t *ss=(const uboot_u8_t*)s;
while(n--)*dd++=*ss++;}
int uboot_memcmp(const void *a,const void *b,uboot_size_t n)
{const uboot_u8_t*pa=(const uboot_u8_t*)a,*pb=(const uboot_u8_t*)b;
while(n--){if(*pa!=*pb)return(*pa<*pb)?-1:1;pa++;pb++;}return 0;}

/* ── arch probes (QEMU defaults) ────────────────────────── */
int uboot_mem_probe_arm64(uboot_mem_map_t *m, uboot_u64_t dtb)
{
    (void)dtb;
    uboot_memset(m,0,sizeof(*m));
    m->regions[0]=(uboot_mem_region_t){0x40000000ULL,0x04000000ULL,UBOOT_MEM_USABLE};
    m->count=1; m->total_usable=0x04000000ULL;
    return UBOOT_OK;
}
int uboot_mem_probe_arm32(uboot_mem_map_t *m, uboot_u32_t *atags)
{
    (void)atags;
    uboot_memset(m,0,sizeof(*m));
    m->regions[0]=(uboot_mem_region_t){0x00100000UL,0x00F00000UL,UBOOT_MEM_USABLE};
    m->count=1; m->total_usable=0x00F00000UL;
    return UBOOT_OK;
}
int uboot_mem_probe_x86(uboot_mem_map_t *m, uboot_u32_t e820)
{
    (void)e820;
    uboot_memset(m,0,sizeof(*m));
    m->regions[0]=(uboot_mem_region_t){0x00100000ULL,0x03F00000ULL,UBOOT_MEM_USABLE};
    m->regions[1]=(uboot_mem_region_t){0xFEC00000ULL,0x00400000ULL,UBOOT_MEM_MMIO};
    m->count=2; m->total_usable=0x03F00000ULL;
    return UBOOT_OK;
}

static const char *mtype(uboot_mem_type_t t)
{
    switch(t){case UBOOT_MEM_USABLE:return"USABLE";
              case UBOOT_MEM_RESERVED:return"RESERVED";
              case UBOOT_MEM_MMIO:return"MMIO";
              default:return"OTHER";}
}

void uboot_mem_print(const uboot_mem_map_t *m)
{
    uboot_printf("  Memory map (%u regions):\r\n",m->count);
    for(uboot_u32_t i=0;i<m->count;i++){
        const uboot_mem_region_t *r=&m->regions[i];
        uboot_puts("    base="); uboot_puthex64(r->base);
        uboot_puts("  size=");   uboot_puthex64(r->size);
        uboot_puts("  ");        uboot_puts(mtype(r->type));
        uboot_puts("\r\n");
    }
    uboot_printf("  Usable: %u MB\r\n",
        (uboot_u32_t)(m->total_usable>>20));
}
