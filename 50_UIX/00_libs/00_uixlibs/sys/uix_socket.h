
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


#ifndef UIX_SOCKET_H
#define UIX_SOCKET_H

#include "uix_types.h"

#define UIX_AF_UNSPEC    0
#define UIX_AF_UNIX      1
#define UIX_AF_INET      2
#define UIX_AF_INET6     10

#define UIX_SOCK_STREAM    1
#define UIX_SOCK_DGRAM     2
#define UIX_SOCK_RAW       3
#define UIX_SOCK_SEQPACKET 5

#define UIX_SOL_SOCKET   1
#define UIX_SO_REUSEADDR 2
#define UIX_SO_TYPE      3
#define UIX_SO_ERROR     4
#define UIX_SO_KEEPALIVE 9
#define UIX_SO_SNDBUF    7
#define UIX_SO_RCVBUF    8
#define UIX_SO_BROADCAST 6

#define UIX_SHUT_RD   0
#define UIX_SHUT_WR   1
#define UIX_SHUT_RDWR 2

#define UIX_MSG_OOB       0x0001
#define UIX_MSG_PEEK      0x0002
#define UIX_MSG_DONTROUTE 0x0004
#define UIX_MSG_WAITALL   0x0100
#define UIX_MSG_DONTWAIT  0x0040
#define UIX_MSG_NOSIGNAL  0x4000

typedef uix_uint16_t uix_sa_family_t;
typedef uix_uint32_t uix_socklen_t;
typedef uix_uint32_t uix_in_addr_t;
typedef uix_uint16_t uix_in_port_t;

typedef struct uix_sockaddr {
    uix_sa_family_t sa_family;
    char            sa_data[14];
} uix_sockaddr_t;

typedef struct uix_in_addr {
    uix_in_addr_t s_addr;
} uix_in_addr_s;

typedef struct uix_sockaddr_in {
    uix_sa_family_t sin_family;
    uix_in_port_t   sin_port;
    uix_in_addr_s   sin_addr;
    unsigned char   sin_zero[8];
} uix_sockaddr_in_t;

typedef struct uix_sockaddr_un {
    uix_sa_family_t sun_family;
    char            sun_path[108];
} uix_sockaddr_un_t;

#define uix_htons(x) ((uix_uint16_t)(((x)>>8)|((x)<<8)))
#define uix_ntohs(x) uix_htons(x)
#define uix_htonl(x) ((((x)>>24)&0xFF)|(((x)>>8)&0xFF00)|\
                       (((x)<<8)&0xFF0000)|(((x)<<24)&0xFF000000U))
#define uix_ntohl(x) uix_htonl(x)

int         uix_socket    (int domain, int type, int protocol);
int         uix_bind      (int sockfd, const uix_sockaddr_t *addr,
                            uix_socklen_t addrlen);
int         uix_listen    (int sockfd, int backlog);
int         uix_accept    (int sockfd, uix_sockaddr_t *addr,
                            uix_socklen_t *addrlen);
int         uix_connect   (int sockfd, const uix_sockaddr_t *addr,
                            uix_socklen_t addrlen);
int         uix_shutdown  (int sockfd, int how);
uix_ssize_t uix_send      (int sockfd, const void *buf,
                            uix_size_t len, int flags);
uix_ssize_t uix_recv      (int sockfd, void *buf,
                            uix_size_t len, int flags);
uix_ssize_t uix_sendto    (int sockfd, const void *buf, uix_size_t len,
                            int flags, const uix_sockaddr_t *dest,
                            uix_socklen_t addrlen);
uix_ssize_t uix_recvfrom  (int sockfd, void *buf, uix_size_t len,
                            int flags, uix_sockaddr_t *src,
                            uix_socklen_t *addrlen);
int         uix_getsockopt(int sockfd, int level, int optname,
                            void *optval, uix_socklen_t *optlen);
int         uix_setsockopt(int sockfd, int level, int optname,
                            const void *optval, uix_socklen_t optlen);
int         uix_getsockname(int sockfd, uix_sockaddr_t *addr,
                             uix_socklen_t *addrlen);
int         uix_getpeername(int sockfd, uix_sockaddr_t *addr,
                             uix_socklen_t *addrlen);

#endif /* UIX_SOCKET_H */


#endif /* End of __SYS_UIX_SOCKET__H */
/* ***This is End of file, there is no more line should be added after this line*** */