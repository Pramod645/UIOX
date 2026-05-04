#ifndef UIX_POLL_H
#define UIX_POLL_H

#include "uix_types.h"

#define UIX_POLLIN    0x0001
#define UIX_POLLPRI   0x0002
#define UIX_POLLOUT   0x0004
#define UIX_POLLERR   0x0008
#define UIX_POLLHUP   0x0010
#define UIX_POLLNVAL  0x0020
#define UIX_POLLRDNORM 0x0040
#define UIX_POLLWRNORM 0x0100

typedef struct uix_pollfd {
    int   fd;
    short events;
    short revents;
} uix_pollfd_t;

typedef unsigned int uix_nfds_t;

int uix_poll (uix_pollfd_t *fds, uix_nfds_t nfds, int timeout);
int uix_ppoll(uix_pollfd_t *fds, uix_nfds_t nfds,
              const uix_timespec_t *tmo, const uix_sigset_t *sigmask);

#endif /* UIX_POLL_H */
