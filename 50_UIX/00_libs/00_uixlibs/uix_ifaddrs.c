#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int ifaddrs(void) {
    struct ifaddrs ifaddr, ifa;
    char addrbuf[INET6ADDRSTRLEN];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return 1;
    }

    printf("Network interfaces and addresses:\n");
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifanext) {
        if (ifa->ifaaddr == NULL)
            continue;

        int family = ifa->ifaaddr->safamily;
        if (family == AFINET || family == AFINET6) {
            void addrptr;
            const char version;

            if (family == AFINET) {
                addrptr = &((struct sockaddrin )ifa->ifaaddr)->sinaddr;
                version = "IPv4";
            } else {
                addrptr = &((struct sockaddrin6 )ifa->ifaaddr)->sin6addr;
                version = "IPv6";
            }

            inetntop(family, addrptr, addrbuf, sizeof(addrbuf));
            printf("%-8s %5s  %s\n", ifa->ifaname, version, addrbuf);
        }
    }

    freeifaddrs(ifaddr);
    return 0;
}
/*
Network interfaces and addresses:
lo        IPv4  127.0.0.1
lo        IPv6  ::1
eth0      IPv4  192.168.1.10
eth0      IPv6  fe80::a00:27ff:fe33:7a11
`

Notes
• ifaddrs vs. net/if.h:  
  - net/if.h + ioctl(SIOCGIFCONF)  → low-level, manual buffer management.  
  - ifaddrs.h + getifaddrs() → automatically allocates a linked list and is much simpler.

• IPv4 and IPv6 support: Built-in and automatic.

• Cleaning up: Always call freeifaddrs() to release memory allocated by getifaddrs().

*/

//////////////////////////////
/* src/uix_ifaddrs.c */
#include "uix_ifaddrs.h"
#include "uix_stdlib.h"
#include "uix_string.h"
#include "uix_errno.h"
#include "uix_inet.h"
#include "uix_if.h"

int uix_getifaddrs(uix_ifaddrs_t **ifap)
{
    if (!ifap) { uix_errno = UIX_EFAULT; return -1; }
    extern int sys_getifaddrs(uix_ifaddrs_t**) __attribute__((weak));
    if (sys_getifaddrs) return sys_getifaddrs(ifap);

    /* Stub: return loopback only */
    uix_ifaddrs_t       *ifa  = (uix_ifaddrs_t*)uix_malloc(sizeof(*ifa));
    uix_sockaddr_in_t   *addr = (uix_sockaddr_in_t*)uix_malloc(sizeof(*addr));
    if (!ifa||!addr) { uix_free(ifa); uix_free(addr);
                       uix_errno=UIX_ENOMEM; return -1; }

    uix_memset(addr, 0, sizeof(*addr));
    addr->sin_family   = UIX_AF_INET;
    addr->sin_addr.s_addr = uix_htonl(0x7F000001);

    uix_memset(ifa, 0, sizeof(*ifa));
    ifa->ifa_name  = uix_strdup("lo");
    ifa->ifa_flags = UIX_IFF_UP | UIX_IFF_LOOPBACK;
    ifa->ifa_addr  = (uix_sockaddr_t*)addr;
    ifa->ifa_next  = NULL;
    *ifap = ifa;
    return 0;
}

void uix_freeifaddrs(uix_ifaddrs_t *ifa)
{
    while (ifa) {
        uix_ifaddrs_t *next = ifa->ifa_next;
        uix_free(ifa->ifa_name);
        uix_free(ifa->ifa_addr);
        uix_free(ifa->ifa_netmask);
        uix_free(ifa);
        ifa = next;
    }
}


