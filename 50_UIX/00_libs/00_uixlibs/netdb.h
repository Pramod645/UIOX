
#ifndef __NETDB__H
#define __NETDB__H
/*
netdb.h> is one of the fundamental network database headers in POSIX and the BSD sockets API.

It defines functions and data structures for working with hostnames, network names, protocols, and services, 
including the gethostbyname() and getaddrinfo() interfaces that resolve hostnames to IP addresses.

Overview
• Purpose: provide access to the system’s network database (hosts, networks, protocols, and services).
• Key structures:  
  - struct hostent — hostname to address mapping.  
  - struct servent — service names and ports (getservbyname).  
  - struct protoent — protocol info (getprotobyname).
• Modern API: getaddrinfo() and getnameinfo() supersede the older gethostbyname() and gethostbyaddr() for thread safety and IPv6 support.

*/
/* This is for only POXIS */

#include "features.h"

#include <netinet/in.h>
#include <sys/socket.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

// Error codes for getaddrinfo() /
#define EAIAGAIN    1   // Temporary failure in name resolution /
#define EAIBADFLAGS 2
#define EAIFAIL     3
#define EAIFAMILY   4
#define EAINONAME   5
#define EAISERVICE  6
#define EAISOCKTYPE 7
#define EAIMEMORY   8

// Legacy structures (for gethostbyname, etc.) /
struct hostent {
    char  hname;       // Official name of host /
    char haliases;    // Alias list /
    int    haddrtype;   // Address type (AFINET, etc.) /
    int    hlength;     // Length of addresses in bytes /
    char haddrlist;  // List of addresses /
};
// for backward compatibility /
#define haddr haddrlist[0]

struct servent {
    char  sname;     // Official service name /
    char *saliases;  // Alias list /
    int    sport;     // Port number (network byte order) /
    char  sproto;    // Protocol /
};

struct protoent {
    char  pname;     // Official protocol name /
    char paliases;  // Aliases /
    int    pproto;    // Protocol number /
};

// Function prototypes /
struct hostent  gethostbyname(const char name);
struct hostent  gethostbyaddr(const void addr, socklent len, int type);
struct servent  getservbyname(const char name, const char proto);
struct protoent getprotobyname(const char name);

// Modern functions (thread-safe and reentrant) /
int getaddrinfo(const char node, const char service,
                const struct addrinfo hints, struct addrinfo res);
void freeaddrinfo(struct addrinfo res);
const char gaistrerror(int errcode);


#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */

#endif /* End of __NETDB__H */
/* ***This is End of file, there is no more line should be added after this line*** */