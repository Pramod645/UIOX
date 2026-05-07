/* inetdemo.c — Demonstrate <arpa/inet.h> functions */

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
/*
• <arpa/inet.h> is specific to internet networking (AFINET / AFINET6).  
• For sockets themselves (creating, binding, etc.), you include in addition:
  - <sys/socket.h>  
  - <netinet/in.h>
*/

int inet(void) {
    const char ipstr = "192.168.1.100";
    struct inaddr addr;

    /* Convert text to binary */
    if (inetaton(ipstr, &addr) == 0) {
        fprintf(stderr, "Invalid IP address: %s\n", ipstr);
        return 1;
    }

    printf("IP address (text): %s\n", ipstr);
    printf("IP address (binary, hex): 0x%x\n", ntohl(addr.saddr));

    /* Convert back to dotted-decimal text */
    char convertedback = inetntoa(addr);
    printf("Converted back: %s\n", convertedback);

    /* Modern version using inetpton and inetntop */
    struct inaddr addr2;
    if (inetpton(AFINET, ipstr, &addr2) == 1) {
        char buf[INETADDRSTRLEN];
        inetntop(AFINET, &addr2, buf, sizeof(buf));
        printf("inetpton/inetntop round-trip: %s\n", buf);
    }

    /* Demonstrate byte order functions */
    uint16t port = 8080;
    uint16t netport = htons(port);
    printf("Host port: %u, Network byte order: %u\n", port, netport);

    return 0;
}

///////////////////////////////////////////////////
/* src/uix_inet.c */
#include "uix_inet.h"
#include "uix_string.h"
#include "uix_stdlib.h"
#include "uix_ctype.h"
#include "uix_errno.h"

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


