
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

#endif /* End of __ARPA_INET__H */
/* ***This is End of file, there is no more line should be added after this line*** */