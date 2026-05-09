#include "uix_socket.h"
#include "uix_errno.h"

int uix_socket(int domain, int type, int protocol)
{
    extern int sys_socket(int,int,int) __attribute__((weak));
    if (sys_socket) return sys_socket(domain, type, protocol);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_bind(int sockfd, const uix_sockaddr_t *addr,
             uix_socklen_t addrlen)
{
    extern int sys_bind(int,const uix_sockaddr_t*,uix_socklen_t)
        __attribute__((weak));
    if (sys_bind) return sys_bind(sockfd, addr, addrlen);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_listen(int sockfd, int backlog)
{
    extern int sys_listen(int,int) __attribute__((weak));
    if (sys_listen) return sys_listen(sockfd, backlog);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_accept(int sockfd, uix_sockaddr_t *addr,
               uix_socklen_t *addrlen)
{
    extern int sys_accept(int,uix_sockaddr_t*,uix_socklen_t*)
        __attribute__((weak));
    if (sys_accept) return sys_accept(sockfd, addr, addrlen);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_connect(int sockfd, const uix_sockaddr_t *addr,
                uix_socklen_t addrlen)
{
    extern int sys_connect(int,const uix_sockaddr_t*,uix_socklen_t)
        __attribute__((weak));
    if (sys_connect) return sys_connect(sockfd, addr, addrlen);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_shutdown(int sockfd, int how)
{
    extern int sys_shutdown(int,int) __attribute__((weak));
    if (sys_shutdown) return sys_shutdown(sockfd, how);
    uix_errno = UIX_ENOSYS; return -1;
}

uix_ssize_t uix_send(int sockfd, const void *buf,
                     uix_size_t len, int flags)
{
    extern long sys_sendto(int,const void*,uix_size_t,int,
                           const uix_sockaddr_t*,uix_socklen_t)
        __attribute__((weak));
    if (sys_sendto)
        return (uix_ssize_t)sys_sendto(sockfd,buf,len,flags,NULL,0);
    uix_errno = UIX_ENOSYS; return -1;
}

uix_ssize_t uix_recv(int sockfd, void *buf,
                     uix_size_t len, int flags)
{
    extern long sys_recvfrom(int,void*,uix_size_t,int,
                             uix_sockaddr_t*,uix_socklen_t*)
        __attribute__((weak));
    if (sys_recvfrom)
        return (uix_ssize_t)sys_recvfrom(sockfd,buf,len,flags,NULL,NULL);
    uix_errno = UIX_ENOSYS; return -1;
}

uix_ssize_t uix_sendto(int sockfd, const void *buf, uix_size_t len,
                       int flags, const uix_sockaddr_t *dest_addr,
                       uix_socklen_t addrlen)
{
    extern long sys_sendto(int,const void*,uix_size_t,int,
                           const uix_sockaddr_t*,uix_socklen_t)
        __attribute__((weak));
    if (sys_sendto)
        return (uix_ssize_t)sys_sendto(sockfd,buf,len,flags,
                                       dest_addr,addrlen);
    uix_errno = UIX_ENOSYS; return -1;
}

uix_ssize_t uix_recvfrom(int sockfd, void *buf, uix_size_t len,
                         int flags, uix_sockaddr_t *src_addr,
                         uix_socklen_t *addrlen)
{
    extern long sys_recvfrom(int,void*,uix_size_t,int,
                             uix_sockaddr_t*,uix_socklen_t*)
        __attribute__((weak));
    if (sys_recvfrom)
        return (uix_ssize_t)sys_recvfrom(sockfd,buf,len,flags,
                                         src_addr,addrlen);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_getsockopt(int sockfd, int level, int optname,
                   void *optval, uix_socklen_t *optlen)
{
    extern int sys_getsockopt(int,int,int,void*,uix_socklen_t*)
        __attribute__((weak));
    if (sys_getsockopt)
        return sys_getsockopt(sockfd,level,optname,optval,optlen);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_setsockopt(int sockfd, int level, int optname,
                   const void *optval, uix_socklen_t optlen)
{
    extern int sys_setsockopt(int,int,int,const void*,uix_socklen_t)
        __attribute__((weak));
    if (sys_setsockopt)
        return sys_setsockopt(sockfd,level,optname,optval,optlen);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_getsockname(int sockfd, uix_sockaddr_t *addr,
                    uix_socklen_t *addrlen)
{
    extern int sys_getsockname(int,uix_sockaddr_t*,uix_socklen_t*)
        __attribute__((weak));
    if (sys_getsockname)
        return sys_getsockname(sockfd, addr, addrlen);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_getpeername(int sockfd, uix_sockaddr_t *addr,
                    uix_socklen_t *addrlen)
{
    extern int sys_getpeername(int,uix_sockaddr_t*,uix_socklen_t*)
        __attribute__((weak));
    if (sys_getpeername)
        return sys_getpeername(sockfd, addr, addrlen);
    uix_errno = UIX_ENOSYS; return -1;
}

/* ***This is End of file, there is no more line should be added after this line*** */
