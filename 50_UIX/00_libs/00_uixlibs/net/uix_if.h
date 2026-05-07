
#ifndef __NET_IF__H
#define __NET_IF__H
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

#include "features.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

 #define IFNAMSIZ 16   /* Maximum interface name length (including '\0') */

/* Interface request structure for ioctl calls */
struct ifreq {
    char ifrname[IFNAMSIZ];   / Interface name, e.g. "eth0" /

    union {
        struct sockaddr ifruaddr;      / IP address /
        struct sockaddr ifrudstaddr;   / P2P destination address /
        struct sockaddr ifrubroadaddr; / Broadcast address /
        short           ifruflags;     / Interface flags /
        int             ifruivalue;    / Simple integer values /
        int             ifrumtu;       / MTU size /
        void           ifrudata;      / Pointer to arbitrary data /
    } ifrifru;
};

/* Helper macros to access the union members */
#define ifraddr       ifrifru.ifruaddr
#define ifrdstaddr    ifrifru.ifrudstaddr
#define ifrbroadaddr  ifrifru.ifrubroadaddr
#define ifrflags      ifrifru.ifruflags
#define ifrmtu        ifrifru.ifrumtu
#define ifrdata       ifrifru.ifrudata

/* used with SIOCGIFCONF to fetch all interface names */
struct ifconf {
    int ifclen;                   /* Size of buffer */
    union {
        char ifcubuf;            /* Buffer containing ifreq structures */
        struct ifreq ifcureq;    /* Pointer to array of ifreq structures */
    } ifcifcu;
};
#define ifcbuf ifcifcu.ifcubuf
#define ifcreq ifcifcu.ifcureq

/* Interface flags */
#define IFFUP          0x1     // Interface is up /
#define IFFBROADCAST   0x2     // Supports broadcast /
#define IFFLOOPBACK    0x8     // Is a loopback net /
#define IFFRUNNING     0x40    // Interface operational /
#define IFFMULTICAST   0x1000  // Supports multicast /

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */


/* include/uix_if.h */
#ifndef UIX_IF_H
#define UIX_IF_H

#include "uix_types.h"
#include "uix_socket.h"

#define UIX_IF_NAMESIZE   16
#define UIX_IFNAMSIZ      UIX_IF_NAMESIZE

/* Interface flags */
#define UIX_IFF_UP        0x0001
#define UIX_IFF_BROADCAST 0x0002
#define UIX_IFF_LOOPBACK  0x0008
#define UIX_IFF_POINTOPOINT 0x0010
#define UIX_IFF_RUNNING   0x0040
#define UIX_IFF_PROMISC   0x0100
#define UIX_IFF_MULTICAST 0x1000

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
} uix_ifreq_t;

#define ifr_addr      ifr_ifru.ifru_addr
#define ifr_flags     ifr_ifru.ifru_flags
#define ifr_mtu       ifr_ifru.ifru_mtu
#define ifr_ifindex   ifr_ifru.ifru_ifindex

unsigned int uix_if_nametoindex(const char *ifname);
char        *uix_if_indextoname(unsigned int ifindex, char *ifname);

#endif /* UIX_IF_H */




#endif /* End of __NET_IF__H */
/* ***This is End of file, there is no more line should be added after this line*** */