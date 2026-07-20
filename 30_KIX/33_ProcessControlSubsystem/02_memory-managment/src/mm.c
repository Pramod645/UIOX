#include "../include/mm.h"
#include <stddef.h>

static uix_uintptr_t g_brk = 0x10000;

#define MAX_REGIONS 16
typedef struct { int in_use; uix_uintptr_t base; uix_size_t size; int prot; } region_t;
static region_t g_regions[MAX_REGIONS];

void *kernel_mmap(void *addr, uix_size_t len, int prot,
                   int flags, int fd, uix_off_t off)
{
    (void)addr;(void)flags;(void)fd;(void)off;
    int i;
    for(i=0;i<MAX_REGIONS;i++) {
        if(!g_regions[i].in_use) {
            g_regions[i].in_use=1;
            g_regions[i].base=(uix_uintptr_t)(0x20000+(i*0x1000));
            g_regions[i].size=len;
            g_regions[i].prot=prot;
            return (void*)g_regions[i].base;
        }
    }
    return (void*)-1;
}

int kernel_munmap(void *addr, uix_size_t len)
{
    (void)len;
    int i;
    for(i=0;i<MAX_REGIONS;i++)
        if(g_regions[i].in_use&&(void*)g_regions[i].base==addr)
            { g_regions[i].in_use=0; return 0; }
    return -1;
}

int kernel_mprotect(void *addr, uix_size_t len, int prot)
{
    (void)len;
    int i;
    for(i=0;i<MAX_REGIONS;i++)
        if(g_regions[i].in_use&&(void*)g_regions[i].base==addr)
            { g_regions[i].prot=prot; return 0; }
    return -1;
}

uix_uintptr_t kernel_brk(uix_uintptr_t nb)
{
    if(nb==0) return g_brk;
    if(nb<MIN_BRK||nb>MAX_BRK) return g_brk;
    nb=(nb+BRK_ALIGN-1)&~(uix_uintptr_t)(BRK_ALIGN-1);
    g_brk=nb;
    return g_brk;
}
