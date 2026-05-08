
#ifndef __ARPA_INET__H
#define __ARPA_INET__H
/*
• arpa/inet.h, this one is very common in network programming.
• <arpa/inet.h> is a POSIX and BSD networking header.  
• It provides functions for converting Internet addresses between binary (used by the kernel) and textual (human-readable) forms.  
• It’s part of the Berkeley Sockets API and used with headers like <netinet/in.h> and <sys/socket.h>.
*/
/* This is for only POXIS */

#include "features.h"

#include <netinet/in.h>   / for struct inaddr, inportt, etc. /
#include <sys/types.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

/* Convert values between host and network byte order */
uint32t htonl(uint32t hostlong);
uint16t htons(uint16t hostshort);
uint32t ntohl(uint32t netlong);
uint16t ntohs(uint16t netshort);

/* Convert IPv4 addresses between text and binary form */
inaddrt inetaddr(const char cp);
char inetntoa(struct inaddr in);

/* Reentrant / modern versions (preferred) */
int inetaton(const char cp, struct inaddr inp);
const char inetntop(int af, const void src, char dst, socklent size);
int inetpton(int af, const char src, void dst);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */



/* include/uix_inet.h */
#ifndef UIX_INET_H
#define UIX_INET_H

#include "uix_types.h"
#include "uix_socket.h"

/* inet_addr / inet_ntoa / inet_pton / inet_ntop */
uix_in_addr_t  uix_inet_addr   (const char *cp);    // Converts dotted IPv4 string to binary — returns INADDR_NONE on error
char          *uix_inet_ntoa   (uix_in_addr_s addr); // Converts in_addr to dotted string — uses static buffer (not reentrant)
int            uix_inet_pton   (int af, const char *src, void *dst); // Converts presentation to network — POSIX.1-2001, supports IPv4/IPv6
const char    *uix_inet_ntop   (int af, const void *src,
                                 char *dst, uix_socklen_t size); // Converts network to presentation — thread-safe, caller provides buffer
int            uix_inet_aton   (const char *cp, uix_in_addr_s *inp);  // Converts dotted IPv4 to in_addr — returns 1 on success

#define UIX_INET_ADDRSTRLEN  16             // Max length of IPv4 address string including null
#define UIX_INET6_ADDRSTRLEN 46

#define UIX_INADDR_ANY       ((uix_in_addr_t)0x00000000)          // Binds to all local interfaces (0.0.0.0)
#define UIX_INADDR_BROADCAST ((uix_in_addr_t)0xffffffff)
#define UIX_INADDR_LOOPBACK  ((uix_in_addr_t)0x7f000001)        // Loopback address (127.0.0.1)
#define UIX_INADDR_NONE      ((uix_in_addr_t)0xffffffff)

#endif /* UIX_INET_H */


#endif /* End of __ARPA_INET__H */
/* ***This is End of file, there is no more line should be added after this line*** */