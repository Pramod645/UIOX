#ifndef UIX_POLL_H
#define UIX_POLL_H

#include "uix_types.h"

#define UIX_POLLIN    0x0001     // Data available to read
#define UIX_POLLPRI   0x0002
#define UIX_POLLOUT   0x0004      // Writing will not block
#define UIX_POLLERR   0x0008     // Error condition — output only
#define UIX_POLLHUP   0x0010     // Hang up — output only, peer closed
#define UIX_POLLNVAL  0x0020     // Invalid fd — output only
#define UIX_POLLRDNORM 0x0040
#define UIX_POLLWRNORM 0x0100

typedef struct uix_pollfd {
    int   fd;                    // File descriptor to monitor
    short events;               // Events to watch for
    short revents;          /// Events that occurred
} uix_pollfd_t;

typedef unsigned int uix_nfds_t;

int uix_poll (uix_pollfd_t *fds, uix_nfds_t nfds, int timeout);     // Waits for events on multiple fds — POSIX.1-2001
int uix_ppoll(uix_pollfd_t *fds, uix_nfds_t nfds,
              const uix_timespec_t *tmo, const uix_sigset_t *sigmask); // Like poll() with nanosecond timeout and signal mask — Linux/glibc

#endif /* UIX_POLL_H */
