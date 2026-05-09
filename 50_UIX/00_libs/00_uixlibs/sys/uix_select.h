
#ifndef __SYS_UIX_SELECT__H
#define __SYS_UIX_SELECT__H
/*
sys/select.h 
*/
/* This is for only POXIS */

#include "uix_features.h" //?

#include "uix_types.h"
#include "uix_time.h"
#include "uix_signal.h"
#include "uix_string.h"

#define UIX_FD_SETSIZE 1024   // Maximum file descriptors monitored by select()

typedef struct {
    uix_uint64_t fds_bits[UIX_FD_SETSIZE / 64];
} uix_fd_set;     // Bit array of file descriptors — 64-bit words

#define UIX_FD_ZERO(s)     uix_memset((s), 0, sizeof(uix_fd_set))          // Clears all bits in set
#define UIX_FD_SET(fd,s)   ((s)->fds_bits[(fd)/64] |=  (1ULL<<((fd)%64))) // Sets bit for fd
#define UIX_FD_CLR(fd,s)   ((s)->fds_bits[(fd)/64] &= ~(1ULL<<((fd)%64)))  // Clears bit for fd
#define UIX_FD_ISSET(fd,s) (!!((s)->fds_bits[(fd)/64] & (1ULL<<((fd)%64))))  // Tests if fd is set

int uix_select (int nfds, uix_fd_set *rfds, uix_fd_set *wfds,
                uix_fd_set *efds, uix_timeval_t *timeout);          // Monitors multiple fds for readability/writability — POSIX
int uix_pselect(int nfds, uix_fd_set *rfds, uix_fd_set *wfds,
                uix_fd_set *efds, const uix_timespec_t *timeout,
                const uix_sigset_t *sigmask);                      // Like select() but uses nanosecond timeout and signal mask — POSIX.1-2001



#endif /* End of __SYS_UIX_SELECT__H */
/* ***This is End of file, there is no more line should be added after this line*** */
