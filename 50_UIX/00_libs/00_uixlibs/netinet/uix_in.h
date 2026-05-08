
#ifndef __NETINET_IN__H
#define __NETINET_IN__H
/*
<netinet/in.h>, this one’s central to all TCP/IP network programming on POSIX systems.  
and its contents, and an example of how it’s used.

Overview
• <netinet/in.h> defines the Internet address family (AFINET, AFINET6), the network byte order types like struct sockaddrin, and constants for IP protocols and ports.  
• It’s part of the BSD sockets API, and used alongside:
  - <sys/socket.h> for socket functions, and  
  - <arpa/inet.h> for address conversion functions.
*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>
#include <stdint.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

/* Address families */
#define AFINET      2     / IPv4 Internet protocols /
#define AFINET6     10    / IPv6 Internet protocols /

/* Protocols */
#define IPPROTOIP     0   / Dummy protocol for IP /
#define IPPROTOTCP    6   / TCP /
#define IPPROTOUDP    17  / UDP /

/* Special address structures */

/* Internet address */
struct inaddr {
    uint32t saddr;       /* Internet address (big-endian / network order) */
};

/* Socket address, Internet style */
struct sockaddrin {
    safamilyt    sinfamily;  // Address family (AFINET) /
    inportt      sinport;    // Port number (network byte order) /
    struct inaddr sinaddr;    // Internet address /
    uint8t        sinzero[8]; // Padding to match struct sockaddr size /
};

/* IPv6 address and socket structures (simplified) */
struct in6addr {
    unsigned char s6addr[16];
};

struct sockaddrin6 {
    safamilyt     sin6family;   // AFINET6 /
    inportt       sin6port;     // Port number /
    uint32t        sin6flowinfo; // IPv6 flow information /
    struct in6addr sin6addr;     // IPv6 address /
    uint32t        sin6scopeid; // Scope ID /
};

/* Well-known port numbers */
#define INADDRANY       ((inaddrt)0x00000000)
#define INADDRLOOPBACK  ((inaddrt)0x7f000001)
#define INADDRBROADCAST ((inaddrt)0xffffffff)
#define INADDRNONE      ((inaddrt)0xffffffff)

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */


/* include/uix_in.h */
#ifndef UIX_IN_H
#define UIX_IN_H

#include "uix_types.h"

/* IP protocols */
#define UIX_IPPROTO_IP    0
#define UIX_IPPROTO_ICMP  1       // ICMP protocol number
#define UIX_IPPROTO_TCP   6       // TCP protocol number (well-known)
#define UIX_IPPROTO_UDP   17      // UDP protocol number
#define UIX_IPPROTO_RAW   255

/* IP socket options */
#define UIX_IP_TOS         1          // Type-of-service socket option
#define UIX_IP_TTL         2        // Time-to-live socket option
#define UIX_IP_HDRINCL     3
#define UIX_IP_OPTIONS     4 
#define UIX_IP_MULTICAST_TTL  33
#define UIX_IP_MULTICAST_LOOP 34
#define UIX_IP_ADD_MEMBERSHIP 35       // Join multicast group
#define UIX_IP_DROP_MEMBERSHIP 36

/* IPv6 options */
#define UIX_IPV6_V6ONLY    26             // Restrict IPv6 socket to IPv6 only
#define UIX_IPV6_UNICAST_HOPS 16
#define UIX_IPV6_MULTICAST_HOPS 18

typedef struct uix_ip_mreq {
    uix_in_addr_s imr_multiaddr;
    uix_in_addr_s imr_interface;
} uix_ip_mreq_t;                // Multicast group membership structure

/* TCP options */
#define UIX_TCP_NODELAY   1     // Disable Nagle algorithm
#define UIX_TCP_KEEPIDLE  4     // Seconds before first keepalive probe
#define UIX_TCP_KEEPINTVL 5
#define UIX_TCP_KEEPCNT   6
#define UIX_SOL_TCP       6

/* UDP */
#define UIX_SOL_UDP       17

#endif /* UIX_IN_H */



#endif /* End of __NETINET_IN__H */
/* ***This is End of file, there is no more line should be added after this line*** */