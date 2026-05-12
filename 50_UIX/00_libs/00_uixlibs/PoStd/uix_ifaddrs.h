
#ifndef __UIX_IFADDRS__H
#define __UIX_IFADDRS__H
/*
ifaddrs.h is a modern, POSIX/BSD-derived header that provides an easier, safer API for enumerating network interfaces and 
their addresses, replacing older, ioctl()-based methods involving <net/if.h>.  

It defines getifaddrs() and freeifaddrs(), which let you list network interfaces, IPs, and flags in a 
linked list — no ioctl() buffer management needed.

Overview
• Purpose: Retrieve a linked list of all active network interfaces and their associated addresses.  
• Defined in: <ifaddrs.h>  
• Main function(s):  
  - int getifaddrs(struct ifaddrs *ifap);  
  - void freeifaddrs(struct ifaddrs ifa);  
• Structure provided: struct ifaddrs, which you iterate over to examine each interface.
*/
/* This is for only POXIS */

//#include "uix_features.h"//??

#include "sys/uix_socket.h"

typedef struct uix_ifaddrs {
    struct uix_ifaddrs  *ifa_next;     // Next interface in linked list
    char                *ifa_name;    // Interface name
    unsigned int         ifa_flags;   // Interface flags (IFF_*)
    uix_sockaddr_t      *ifa_addr;    // Interface address
    uix_sockaddr_t      *ifa_netmask;    // Network mask
    uix_sockaddr_t      *ifa_broadaddr;
    void                *ifa_data;
} uix_ifaddrs_t;

int  uix_getifaddrs(uix_ifaddrs_t **ifap);   // Gets list of all interface addresses — POSIX
void uix_freeifaddrs(uix_ifaddrs_t *ifa);    // Frees linked list from getifaddrs()



#endif /* End of __UIX_IFADDRS__H */
/* ***This is End of file, there is no more line should be added after this line*** */
