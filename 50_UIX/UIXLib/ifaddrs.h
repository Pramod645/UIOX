
#ifndef __IFADDRS__H
#define __IFADDRS__H
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

#include "features.h"

#include <sys/socket.h>
#include <net/if.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

struct ifaddrs {
    struct ifaddrs  ifanext;    // Pointer to next structure /
    char            ifaname;    // Interface name (e.g., "eth0") /
    unsigned int     ifaflags;   // Flags from <net/if.h> (IFFUP, etc.) /
    struct sockaddr ifaaddr;    // Address of interface /
    struct sockaddr ifanetmask; // Netmask of interface /
    struct sockaddr ifabroadaddr; // Broadcast address (if applicable) /
    struct sockaddr ifadstaddr; // Destination address (P2P interfaces) /
    void            ifadata;    // Per-interface data (if any) /
};

// Functions to allocate and free the list /
int  getifaddrs(struct ifaddrs ifap);
void freeifaddrs(struct ifaddrs ifa);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */

#endif /* End of __IFADDRS__H */
/* ***This is End of file, there is no more line should be added after this line*** */