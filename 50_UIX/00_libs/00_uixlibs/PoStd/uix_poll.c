#include "uix_poll.h"
#include "uix_errno.h"
#include "sys/uix_time.h"

int uix_poll(uix_pollfd_t *fds, uix_nfds_t nfds, int timeout)
{
    extern int sys_poll(uix_pollfd_t*,uix_nfds_t,int)
        __attribute__((weak));
    if (sys_poll) return sys_poll(fds, nfds, timeout);

    if (!fds) { uix_errno = UIX_EFAULT; return -1; }
    /* Stub: mark all readable fd 0..2, others as error */
    int count = 0;
    for (uix_nfds_t i = 0; i < nfds; i++) {
        fds[i].revents = 0;
        if (fds[i].fd < 3 && (fds[i].events & UIX_POLLIN)) {
            fds[i].revents = UIX_POLLIN;
            count++;
        }
        if (fds[i].fd < 3 && (fds[i].events & UIX_POLLOUT)) {
            fds[i].revents |= UIX_POLLOUT;
            count++;
        }
    }
    (void)timeout;
    return count;
}

#if 0
int uix_ppoll(uix_pollfd_t *fds, uix_nfds_t nfds,
              const struct uix_timeval_t *tmo_p,
              const uix_size_t *sigmask)
{
    int timeout = -1;
    if (tmo_p)
        // comeback to fix error here timeout = (int)(tmo_p->tv_sec * 1000 + tmo_p->tv_nsec / 1000000);
    (void)sigmask;
    return uix_poll(fds, nfds, timeout);
}
#endif

/* ***This is End of file, there is no more line should be added after this line*** */
