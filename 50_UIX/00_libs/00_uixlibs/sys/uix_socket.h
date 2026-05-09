
#ifndef __SYS_UIX_SOCKET__H
#define __SYS_UIX_SOCKET__H
/*
sys/socket.h 
*/
/* This is for only POXIS */

#include "uix_features.h" //?

#include "uix_types.h"

#define UIX_AF_UNSPEC    0
#define UIX_AF_UNIX      1      // UNIX domain sockets
#define UIX_AF_INET      2      // IPv4 address family
#define UIX_AF_INET6     10   // IPv6 address family

#define UIX_SOCK_STREAM    1 // Reliable, ordered byte stream (TCP)
#define UIX_SOCK_DGRAM     2  // Unreliable datagrams (UDP)
#define UIX_SOCK_RAW       3   // Raw network protocol access
#define UIX_SOCK_SEQPACKET 5

#define UIX_SOL_SOCKET   1       // Socket-level options
#define UIX_SO_REUSEADDR 2   // Allows rebinding to address in TIME_WAIT
#define UIX_SO_TYPE      3
#define UIX_SO_ERROR     4
#define UIX_SO_KEEPALIVE 9     // Enable TCP keepalive probes
#define UIX_SO_SNDBUF    7
#define UIX_SO_RCVBUF    8
#define UIX_SO_BROADCAST 6

#define UIX_SHUT_RD   0
#define UIX_SHUT_WR   1
#define UIX_SHUT_RDWR 2

#define UIX_MSG_OOB       0x0001
#define UIX_MSG_PEEK      0x0002      // Read without consuming data
#define UIX_MSG_DONTROUTE 0x0004
#define UIX_MSG_WAITALL   0x0100
#define UIX_MSG_DONTWAIT  0x0040      // Non-blocking send/recv
#define UIX_MSG_NOSIGNAL  0x4000

typedef uix_uint16_t uix_sa_family_t;
typedef uix_uint32_t uix_socklen_t;
typedef uix_uint32_t uix_in_addr_t;
typedef uix_uint16_t uix_in_port_t;

typedef struct uix_sockaddr {
    uix_sa_family_t sa_family;
    char            sa_data[14];
} uix_sockaddr_t;                // Generic socket address

typedef struct uix_in_addr {
    uix_in_addr_t s_addr;
} uix_in_addr_s;

typedef struct uix_sockaddr_in {
    uix_sa_family_t sin_family;
    uix_in_port_t   sin_port;
    uix_in_addr_s   sin_addr;
    unsigned char   sin_zero[8];
} uix_sockaddr_in_t;         // IPv4 socket address

typedef struct uix_sockaddr_un {
    uix_sa_family_t sun_family;
    char            sun_path[108];
} uix_sockaddr_un_t;              // UNIX domain socket address

#define uix_htons(x) ((uix_uint16_t)(((x)>>8)|((x)<<8)))   // Host to network byte order (16-bit)
#define uix_ntohs(x) uix_htons(x)
#define uix_htonl(x) ((((x)>>24)&0xFF)|(((x)>>8)&0xFF00)|\
                       (((x)<<8)&0xFF0000)|(((x)<<24)&0xFF000000U))   // Network to host byte order (32-bit)
#define uix_ntohl(x) uix_htonl(x)

int         uix_socket    (int domain, int type, int protocol);   // Creates socket endpoint
int         uix_bind      (int sockfd, const uix_sockaddr_t *addr,
                            uix_socklen_t addrlen);                 // Associates address with socket
int         uix_listen    (int sockfd, int backlog);              // Marks socket as passive, sets connection queue length
int         uix_accept    (int sockfd, uix_sockaddr_t *addr,
                            uix_socklen_t *addrlen);               // Accepts incoming connection, returns new fd
int         uix_connect   (int sockfd, const uix_sockaddr_t *addr,
                            uix_socklen_t addrlen);                 // Initiates connection to remote address
int         uix_shutdown  (int sockfd, int how);       // Shuts down part of full-duplex connection
uix_ssize_t uix_send      (int sockfd, const void *buf,
                            uix_size_t len, int flags);       // Sends data on connected socket
uix_ssize_t uix_recv      (int sockfd, void *buf,
                            uix_size_t len, int flags);           // Receives data from connected socket
uix_ssize_t uix_sendto    (int sockfd, const void *buf, uix_size_t len,
                            int flags, const uix_sockaddr_t *dest,
                            uix_socklen_t addrlen);                 // Sends datagram to specific address
uix_ssize_t uix_recvfrom  (int sockfd, void *buf, uix_size_t len,
                            int flags, uix_sockaddr_t *src,
                            uix_socklen_t *addrlen);        // Receives datagram with source address
int         uix_getsockopt(int sockfd, int level, int optname,
                            void *optval, uix_socklen_t *optlen); //Gets socket option value    
int         uix_setsockopt(int sockfd, int level, int optname,
                            const void *optval, uix_socklen_t optlen);  // Sets socket option
int         uix_getsockname(int sockfd, uix_sockaddr_t *addr,
                             uix_socklen_t *addrlen);                 // Gets local address of socket
int         uix_getpeername(int sockfd, uix_sockaddr_t *addr,
                             uix_socklen_t *addrlen);   // Gets remote address of connected socket

#endif /* End of __SYS_UIX_SOCKET__H */
/* ***This is End of file, there is no more line should be added after this line*** */
