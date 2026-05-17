/***********************  uix_ifaddrs.c *******************************/
#include "uix_ifaddrs.h"
#include "uix_stdlib.h"
#include "uix_string.h"
#include "uix_errno.h"
#include "../arpa/uix_inet.h"
#include "../net/uix_if.h"

#if STUB
#include "../uix_sys.h"
#else
#include "../../../40_SystemCallInterface/uix_sys.h"
#endif


int uix_getifaddrs(uix_ifaddrs_t **ifap)
{
    if (!ifap) { uix_errno = UIX_EFAULT; return -1; }
    //extern int sys_getifaddrs(uix_ifaddrs_t**) __attribute__((weak));
    if (SYS_GETIFADDRS) return sys_getifaddrs(ifap);
    #ifndef __has_syscall_getifaddrs
        return sys_getifaddrs(ifap);
    #else
        /* stub: return loopback only (existing code) */
    #endif

    /* Stub: return loopback only */
    uix_ifaddrs_t       *ifa  = (uix_ifaddrs_t*)uix_malloc(sizeof(*ifa));
    uix_sockaddr_in_t   *addr = (uix_sockaddr_in_t*)uix_malloc(sizeof(*addr));
    if (!ifa||!addr) { uix_free(ifa); uix_free(addr);
                       uix_errno=UIX_ENOMEM; return -1; }

    uix_memset(addr, 0, sizeof(*addr));
    addr->sin_family   = UIX_AF_INET;
    //addr->sin_addr.s_addr = uix_htonl(0x7F000001);
    addr->sin_addr.s_addr = (((unsigned int)0x7F000001U >> 24) & 0xFFU)
                       | (((unsigned int)0x7F000001U >>  8) & 0xFF00U)
                       | (((unsigned int)0x7F000001U <<  8) & 0xFF0000U)
                       | (((unsigned int)0x7F000001U << 24) & 0xFF000000U);

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

/* ***This is End of file, there is no more line should be added after this line*** */
