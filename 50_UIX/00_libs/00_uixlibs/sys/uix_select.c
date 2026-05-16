#include "uix_select.h"
#include "../PoStd/uix_string.h"
#include "../PoStd/uix_errno.h"


#include "../uix_sys.h"


int uix_select(int nfds, uix_fd_set *readfds, uix_fd_set *writefds,
               uix_fd_set *exceptfds, uix_timeval_t *timeout)
{
    //extern int sys_select(int, uix_fd_set*, uix_fd_set*,
    //                      uix_fd_set*, uix_timeval_t*)
    //    __attribute__((weak));
    if (SYS_SELECT)
        return sys_select(nfds,readfds,writefds,exceptfds,timeout);

    (void)timeout;
    int count = 0;
    /* Stub: mark stdin readable, stdout/stderr writable */
    for (int fd = 0; fd < nfds; fd++) {
        if (readfds  && UIX_FD_ISSET(fd, readfds)  && fd == 0)
            count++;
        else if (readfds && UIX_FD_ISSET(fd, readfds))
            UIX_FD_CLR(fd, readfds);

        if (writefds && UIX_FD_ISSET(fd, writefds) && fd < 3)
            count++;
        else if (writefds && UIX_FD_ISSET(fd, writefds))
            UIX_FD_CLR(fd, writefds);

        if (exceptfds && UIX_FD_ISSET(fd, exceptfds))
            UIX_FD_CLR(fd, exceptfds);
    }
    return count;
}

int uix_pselect(int nfds, uix_fd_set *readfds, uix_fd_set *writefds,
                uix_fd_set *exceptfds, const uix_timespec_t *timeout,
                const uix_sigset_t *sigmask)
{
    uix_timeval_t tv;
    uix_timeval_t *tvp = NULL;
    if (timeout) {
        tv.tv_sec  = timeout->tv_sec;
        tv.tv_usec = timeout->tv_nsec / 1000;
        tvp = &tv;
    }
    (void)sigmask;
    return uix_select(nfds, readfds, writefds, exceptfds, tvp);
}


/* ***This is End of file, there is no more line should be added after this line*** */
