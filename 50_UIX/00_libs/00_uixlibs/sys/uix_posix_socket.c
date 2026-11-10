/**
 * @file  uix_posix_socket.c
 * @brief UIOX POSIX — socket syscall implementations.
 *        socket, bind, connect, listen, accept, send, recv,
 *        sendto, recvfrom, sendmsg, recvmsg, setsockopt,
 *        getsockopt, getpeername, getsockname, shutdown.
 */

 #include "uix_sys.h"
 #include "uix_socket.h"
 #include "../PoStd/uix_errno.h"
 
 static inline long _ret(long r)
 {
     if (r < 0L) { uix_errno = (int)(-r); return -1L; }
     return r;
 }
 
 int uix_socket(int domain, int type, int protocol)
 {
     return (int)_ret(my_syscall3(SYS_SOCKET,
                                    (long)domain,(long)type,(long)protocol));
 }
 
 int uix_bind(int sockfd, const struct uix_sockaddr *addr, uix_socklen_t addrlen)
 {
     return (int)_ret(my_syscall3(SYS_BIND,
                                    (long)sockfd,(long)addr,(long)addrlen));
 }
 
 int uix_connect(int sockfd, const struct uix_sockaddr *addr, uix_socklen_t addrlen)
 {
     return (int)_ret(my_syscall3(SYS_CONNECT,
                                    (long)sockfd,(long)addr,(long)addrlen));
 }
 
 int uix_listen(int sockfd, int backlog)
 {
     return (int)_ret(my_syscall2(SYS_LISTEN,(long)sockfd,(long)backlog));
 }
 
 int uix_accept(int sockfd, struct uix_sockaddr *addr, uix_socklen_t *addrlen)
 {
     return (int)_ret(my_syscall3(SYS_ACCEPT,
                                    (long)sockfd,(long)addr,(long)addrlen));
 }
 
 int uix_accept4(int sockfd, struct uix_sockaddr *addr,
                  uix_socklen_t *addrlen, int flags)
 {
     return (int)_ret(my_syscall4(SYS_ACCEPT4,
                       (long)sockfd,(long)addr,(long)addrlen,(long)flags));
 }
 
 uix_ssize_t uix_send(int sockfd, const void *buf, uix_size_t len, int flags)
 {
     return (uix_ssize_t)_ret(my_syscall4(SYS_SEND,
                           (long)sockfd,(long)buf,(long)len,(long)flags));
 }
 
 uix_ssize_t uix_recv(int sockfd, void *buf, uix_size_t len, int flags)
 {
     return (uix_ssize_t)_ret(my_syscall4(SYS_RECV,
                           (long)sockfd,(long)buf,(long)len,(long)flags));
 }
 
 uix_ssize_t uix_sendto(int sockfd, const void *buf, uix_size_t len, int flags,
                     const struct uix_sockaddr *dest_addr, uix_socklen_t addrlen)
 {
     return (uix_ssize_t)_ret(my_syscall6(SYS_SENDTO,
                           (long)sockfd,(long)buf,(long)len,(long)flags,
                           (long)dest_addr,(long)addrlen));
 }
 
 uix_ssize_t uix_recvfrom(int sockfd, void *buf, uix_size_t len, int flags,
                       struct uix_sockaddr *src_addr, uix_socklen_t *addrlen)
 {
     return (uix_ssize_t)_ret(my_syscall6(SYS_RECVFROM,
                           (long)sockfd,(long)buf,(long)len,(long)flags,
                           (long)src_addr,(long)addrlen));
 }
 
 uix_ssize_t uix_sendmsg(int sockfd, const void *msg, int flags)
 {
     return (uix_ssize_t)_ret(my_syscall3(SYS_SENDMSG,
                                        (long)sockfd,(long)msg,(long)flags));
 }
 
 uix_ssize_t uix_recvmsg(int sockfd, void *msg, int flags)
 {
     return (uix_ssize_t)_ret(my_syscall3(SYS_RECVMSG,
                                        (long)sockfd,(long)msg,(long)flags));
 }
 
 int uix_setsockopt(int sockfd, int level, int optname,
                     const void *optval, uix_socklen_t optlen)
 {
     return (int)_ret(my_syscall5(SYS_SETSOCKOPT,
                       (long)sockfd,(long)level,(long)optname,
                       (long)optval,(long)optlen));
 }
 
 int uix_getsockopt(int sockfd, int level, int optname,
                     void *optval, uix_socklen_t *optlen)
 {
     return (int)_ret(my_syscall5(SYS_GETSOCKOPT,
                       (long)sockfd,(long)level,(long)optname,
                       (long)optval,(long)optlen));
 }
 
 int uix_getpeername(int sockfd, struct uix_sockaddr *addr, uix_socklen_t *addrlen)
 {
     return (int)_ret(my_syscall3(SYS_GETPEERNAME,
                                    (long)sockfd,(long)addr,(long)addrlen));
 }
 
 int uix_getsockname(int sockfd, struct uix_sockaddr *addr, uix_socklen_t *addrlen)
 {
     return (int)_ret(my_syscall3(SYS_GETSOCKNAME,
                                    (long)sockfd,(long)addr,(long)addrlen));
 }
 
 int uix_shutdown(int sockfd, int how)
 {
     return (int)_ret(my_syscall2(SYS_SHUTDOWN,(long)sockfd,(long)how));
 }
 