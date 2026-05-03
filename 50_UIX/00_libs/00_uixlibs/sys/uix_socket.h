
#ifndef __SYS_UIX_SOCKET__H
#define __SYS_UIX_SOCKET__H
/*
sys/socket.h 
*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>
#include <stddef.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

/* Basic socket-related types */
typedef unsigned int socklent;
typedef unsigned short safamilyt;

/* Generic socket address */
struct sockaddr {
    safamilyt safamily;
    char sadata[14];
};

/* Scatter/gather I/O vector */
struct iovec {
    void  iovbase;
    sizet iovlen;
};

/* Message header for sendmsg/recvmsg */
struct msghdr {
    void         msgname;
    socklent     msgnamelen;
    struct iovec msgiov;
    sizet        msgiovlen;
    void         msgcontrol;
    sizet        msgcontrollen;
    int           msgflags;
};

/* Ancillary data object */
struct cmsghdr {
    sizet cmsglen;
    int    cmsglevel;
    int    cmsgtype;
};

/* Socket types */
#define SOCKSTREAM    1
#define SOCKDGRAM     2
#define SOCKRAW       3
#define SOCKRDM       4
#define SOCKSEQPACKET 5

/* Socket flags */
#define SOCKCLOEXEC  02000000
#define SOCKNONBLOCK 00004000

/* Address families */
#define AFUNSPEC 0
#define AFUNIX   1
#define AFINET   2
#define AFINET6  10

/* Protocol families */
#define PFUNSPEC AFUNSPEC
#define PFUNIX   AFUNIX
#define PFINET   AFINET
#define PFINET6  AFINET6

/* Shutdown modes */
#define SHUTRD   0
#define SHUTWR   1
#define SHUTRDWR 2

/* Message flags */
#define MSGOOB         0x01
#define MSGPEEK        0x02
#define MSGDONTROUTE   0x04
#define MSGCTRUNC      0x08
#define MSGTRUNC       0x20
#define MSGDONTWAIT    0x40
#define MSGEOR         0x80
#define MSGWAITALL     0x100
#define MSGNOSIGNAL    0x4000

/* Socket option levels */
#define SOLSOCKET 1

/* Common socket options */
#define SOREUSEADDR  2
#define SOTYPE       3
#define SOERROR      4
#define SOBROADCAST  6
#define SOSNDBUF     7
#define SORCVBUF     8
#define SOKEEPALIVE  9
#define SOOOBINLINE  10
#define SOREUSEPORT  15

/**/ Function declarations */
int socket(int domain, int type, int protocol);
int socketpair(int domain, int type, int protocol, int sv[2]);

int bind(int sockfd, const struct sockaddr addr, socklent addrlen);
int connect(int sockfd, const struct sockaddr addr, socklent addrlen);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr addr, socklent addrlen);

ssizet send(int sockfd, const void buf, sizet len, int flags);
ssizet recv(int sockfd, void buf, sizet len, int flags);

ssizet sendto(int sockfd, const void buf, sizet len, int flags,
               const struct sockaddr destaddr, socklent addrlen);
ssizet recvfrom(int sockfd, void buf, sizet len, int flags,
                 struct sockaddr srcaddr, socklent addrlen);

ssizet sendmsg(int sockfd, const struct msghdr msg, int flags);
ssizet recvmsg(int sockfd, struct msghdr msg, int flags);

int getsockname(int sockfd, struct sockaddr addr, socklent addrlen);
int getpeername(int sockfd, struct sockaddr addr, socklent addrlen);

int getsockopt(int sockfd, int level, int optname,
               void optval, socklent optlen);
int setsockopt(int sockfd, int level, int optname,
               const void optval, socklent optlen);

int shutdown(int sockfd, int how);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */

#endif /* End of __SYS_UIX_SOCKET__H */
/* ***This is End of file, there is no more line should be added after this line*** */