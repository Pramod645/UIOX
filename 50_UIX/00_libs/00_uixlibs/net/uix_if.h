
#ifndef __NET_UIX_IF__H
#define __NET_UIX_IF__H
/*
<net/if.h> is another key POSIX/BSD networking header.  
It defines the network interface structures and constants used to query or manipulate network adapters (like eth0, lo, etc.).  
Developers use it in conjunction with <sys/ioctl.h> and <netinet/in.h> when, for example, listing interface names or addresses.

Overview
• Purpose: manage or identify network interfaces (e.g., Ethernet, Wi-Fi, loopback).  
• Defines: struct ifreq, struct ifconf, interface flags, and constants such as IFFUP, IFFBROADCAST, etc.  
• Common uses:
  - Getting the list of interfaces.  
  - Checking if an interface is active (IFFUP).  
  - Fetching interface IP addresses.  
  - Setting network parameters (in privileged programs).
*/
/* This is for only POXIS */

#include "uix_features.h" //??

#include "sys/uix_types.h"
#include "sys/uix_socket.h"

#define UIX_IF_NAMESIZE   16                // Max interface name length including null
#define UIX_IFNAMSIZ      UIX_IF_NAMESIZE

/* Interface flags */
#define UIX_IFF_UP        0x0001            // Interface is up
#define UIX_IFF_BROADCAST 0x0002          // Loopback interface
#define UIX_IFF_LOOPBACK  0x0008
#define UIX_IFF_POINTOPOINT 0x0010
#define UIX_IFF_RUNNING   0x0040
#define UIX_IFF_PROMISC   0x0100      // Promiscuous mode — receives all packets
#define UIX_IFF_MULTICAST 0x1000         // Supports multicast

typedef struct uix_ifreq {
    char               ifr_name[UIX_IFNAMSIZ];
    union {
        uix_sockaddr_t ifru_addr;
        uix_sockaddr_t ifru_dstaddr;
        uix_sockaddr_t ifru_broadaddr;
        short          ifru_flags;
        int            ifru_metric;
        int            ifru_mtu;
        int            ifru_ifindex;
    } ifr_ifru;
} uix_ifreq_t;    // Interface request structure for ioctl()

#define ifr_addr      ifr_ifru.ifru_addr
#define ifr_flags     ifr_ifru.ifru_flags
#define ifr_mtu       ifr_ifru.ifru_mtu
#define ifr_ifindex   ifr_ifru.ifru_ifindex

unsigned int uix_if_nametoindex(const char *ifname);                 // Returns interface index by name — POSIX
char        *uix_if_indextoname(unsigned int ifindex, char *ifname);  // Returns interface name by index — POSIX



#endif /* End of __NET_UIX_IF__H */
/* ***This is End of file, there is no more line should be added after this line*** */
