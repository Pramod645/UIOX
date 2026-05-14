/*************************  uix_inet.c ******************************/
#include "uix_inet.h"
#include "../PoStd/uix_string.h"
#include "../PoStd/uix_stdlib.h"
#include "../PoStd/uix_ctype.h"
#include "../PoStd/uix_errno.h"

#include "../PoStd/uix_stdio.h"

uix_in_addr_t uix_inet_addr(const char *cp)
{
    uix_in_addr_s addr;
    if (uix_inet_aton(cp, &addr) == 0) return UIX_INADDR_NONE;
    return addr.s_addr;
}

int uix_inet_aton(const char *cp, uix_in_addr_s *inp)
{
    uix_uint32_t parts[4]; int n=0;
    while (*cp) {
        if (n>3) return 0;
        if (!uix_isdigit((unsigned char)*cp)) return 0;
        unsigned long v=uix_strtoul(cp,(char**)&cp,10);
        if (v>255) return 0;
        parts[n++]=(uix_uint32_t)v;
        if (*cp=='.') cp++; else if (*cp!='\0') return 0;
    }
    if (n!=4) return 0;
    inp->s_addr = uix_htonl(
        (parts[0]<<24)|(parts[1]<<16)|(parts[2]<<8)|parts[3]);
    return 1;
}

static char _ntoa_buf[16];
char *uix_inet_ntoa(uix_in_addr_s addr)
{
    uix_uint32_t a = uix_ntohl(addr.s_addr);
    uix_snprintf(_ntoa_buf, sizeof(_ntoa_buf), "%u.%u.%u.%u",
                 (a>>24)&0xFF,(a>>16)&0xFF,(a>>8)&0xFF,a&0xFF);
    return _ntoa_buf;
}

int uix_inet_pton(int af, const char *src, void *dst)
{
    if (af==UIX_AF_INET) {
        uix_in_addr_s a;
        if (!uix_inet_aton(src,&a)) return 0;
        uix_memcpy(dst,&a,4); return 1;
    }
    uix_errno=UIX_ENOSYS; return -1;
}

const char *uix_inet_ntop(int af, const void *src,
                           char *dst, uix_socklen_t sz)
{
    if (af==UIX_AF_INET) {
        uix_in_addr_s a;
        uix_memcpy(&a, src, 4);
        uix_uint32_t h=uix_ntohl(a.s_addr);
        uix_snprintf(dst, sz, "%u.%u.%u.%u",
                     (h>>24)&0xFF,(h>>16)&0xFF,(h>>8)&0xFF,h&0xFF);
        return dst;
    }
    uix_errno=UIX_ENOSYS; return NULL;
}

/* ***This is End of file, there is no more line should be added after this line*** */
