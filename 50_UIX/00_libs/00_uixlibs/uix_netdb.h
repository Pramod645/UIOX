
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


/* include/uix_netdb.h */
#ifndef UIX_NETDB_H
#define UIX_NETDB_H

#include "uix_types.h"
#include "uix_socket.h"

/* h_errno values */
#define UIX_HOST_NOT_FOUND 1     // h_errno value — no such host
#define UIX_TRY_AGAIN      2   // h_errno — temporary DNS failure
#define UIX_NO_RECOVERY    3
#define UIX_NO_DATA        4

extern int uix_h_errno;

typedef struct uix_hostent {
    char   *h_name;           // Official host name
    char  **h_aliases;
    int     h_addrtype;
    int     h_length;
    char  **h_addr_list;     // Null-terminated list of addresses
} uix_hostent_t;

typedef struct uix_servent {
    char   *s_name;
    char  **s_aliases;
    int     s_port;
    char   *s_proto;
} uix_servent_t;

typedef struct uix_protoent {
    char   *p_name;
    char  **p_aliases;
    int     p_proto;
} uix_protoent_t;

typedef struct uix_addrinfo {
    int                ai_flags;
    int                ai_family;
    int                ai_socktype;
    int                ai_protocol;
    uix_socklen_t      ai_addrlen;
    uix_sockaddr_t    *ai_addr;
    char              *ai_canonname;
    struct uix_addrinfo *ai_next;
} uix_addrinfo_t;                     // Modern address resolution result

#define UIX_AI_PASSIVE     0x0001     // For server bind — use wildcard address
#define UIX_AI_CANONNAME   0x0002    // Return canonical name
#define UIX_AI_NUMERICHOST 0x0004
#define UIX_AI_NUMERICSERV 0x0008

#define UIX_EAI_AGAIN      -3
#define UIX_EAI_BADFLAGS   -1
#define UIX_EAI_FAIL       -4
#define UIX_EAI_FAMILY     -6
#define UIX_EAI_MEMORY     -10
#define UIX_EAI_NONAME     -2
#define UIX_EAI_SERVICE    -8
#define UIX_EAI_SOCKTYPE   -7

uix_hostent_t *uix_gethostbyname (const char *name);             // DNS lookup by name — obsolete, use getaddrinfo()
uix_hostent_t *uix_gethostbyaddr (const void *addr, uix_socklen_t len, int type);
uix_servent_t *uix_getservbyname (const char *name, const char *proto);
uix_servent_t *uix_getservbyport (int port, const char *proto);
uix_protoent_t *uix_getprotobyname(const char *name);
int            uix_getaddrinfo   (const char *node, const char *service,
                                   const uix_addrinfo_t *hints,
                                   uix_addrinfo_t **res);                 // Modern name resolution — POSIX.1-2001
void           uix_freeaddrinfo  (uix_addrinfo_t *res);                 /// Frees linked list from getaddrinfo()
int            uix_getnameinfo   (const uix_sockaddr_t *sa, uix_socklen_t salen,
                                   char *host, uix_size_t hostlen,
                                   char *serv, uix_size_t servlen, int flags);    // Reverse DNS lookup — POSIX.1-2001
const char    *uix_gai_strerror  (int ecode);                                    /// Returns string description of getaddrinfo error

#endif /* UIX_NETDB_H */



#endif /* End of __NETDB__H */
/* ***This is End of file, there is no more line should be added after this line*** */