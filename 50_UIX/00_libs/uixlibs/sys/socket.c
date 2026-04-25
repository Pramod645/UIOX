#include "sys/socket.h"

#include <unistd.h>
#include <sys/syscall.h>

int socket(int domain, int type, int protocol)
{
    return syscall(SYSsocket, domain, type, protocol);
}

int socketpair(int domain, int type, int protocol, int sv[2])
{
    return syscall(SYSsocketpair, domain, type, protocol, sv);
}

int bind(int sockfd, const struct sockaddr addr, socklent addrlen)
{
    return syscall(SYSbind, sockfd, addr, addrlen);
}

int connect(int sockfd, const struct sockaddr addr, socklent addrlen)
{
    return syscall(SYSconnect, sockfd, addr, addrlen);
}

int listen(int sockfd, int backlog)
{
    return syscall(SYSlisten, sockfd, backlog);
}

int accept(int sockfd, struct sockaddr addr, socklent addrlen)
{
    return syscall(SYSaccept, sockfd, addr, addrlen);
}

ssizet send(int sockfd, const void buf, sizet len, int flags)
{
    return syscall(SYSsendto, sockfd, buf, len, flags, 0, 0);
}

ssizet recv(int sockfd, void buf, sizet len, int flags)
{
    return syscall(SYSrecvfrom, sockfd, buf, len, flags, 0, 0);
}

ssizet sendto(int sockfd, const void buf, sizet len, int flags,
               const struct sockaddr destaddr, socklent addrlen)
{
    return syscall(SYSsendto, sockfd, buf, len, flags, destaddr, addrlen);
}

ssizet recvfrom(int sockfd, void buf, sizet len, int flags,
                 struct sockaddr srcaddr, socklent addrlen)
{
    return syscall(SYSrecvfrom, sockfd, buf, len, flags, srcaddr, addrlen);
}

ssizet sendmsg(int sockfd, const struct msghdr msg, int flags)
{
    return syscall(SYSsendmsg, sockfd, msg, flags);
}

ssizet recvmsg(int sockfd, struct msghdr msg, int flags)
{
    return syscall(SYSrecvmsg, sockfd, msg, flags);
}

int getsockname(int sockfd, struct sockaddr addr, socklent addrlen)
{
    return syscall(SYSgetsockname, sockfd, addr, addrlen);
}

int getpeername(int sockfd, struct sockaddr addr, socklent addrlen)
{
    return syscall(SYSgetpeername, sockfd, addr, addrlen);
}

int getsockopt(int sockfd, int level, int optname,
               void optval, socklent optlen)
{
    return syscall(SYSgetsockopt, sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname,
               const void optval, socklent optlen)
{
    return syscall(SYSsetsockopt, sockfd, level, optname, optval, optlen);
}

int shutdown(int sockfd, int how)
{
    return syscall(SYSshutdown, sockfd, how);
}
`
/*
Important caveats

This is a minimal educational version. Real Linux sys/socket.h is more complicated:

• many constants live in architecture-specific internal headers
• some ABIs historically used socketcall
• ancillary data macros are normally also defined:
  - CMSGDATA
  - CMSGFIRSTHDR
  - CMSGNXTHDR
  - CMSGSPACE
  - CMSGLEN
• accept4() is commonly present
• sockaddrstorage is usually defined too
• MSG, SO, and AF* sets are much larger in real libc

Also, some of the numeric values here are Linux-like, but if you need ABI-accurate definitions for a real target, 
they should match your exact libc/kernel target.


*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

int socketlib(void)
{
    int fd = socket(AFINET, SOCK_STREAM, 0);
    if (fd < 0)
        return 1;

    close(fd);
    return 0;
}
