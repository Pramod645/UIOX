#include "uix_ioctl.h"
#include "uix_errno.h"
#include <stdarg.h>

int uix_ioctl(int fd, unsigned long request, ...)
{
    va_list ap;
    va_start(ap, request);
    void *arg = va_arg(ap, void *);
    va_end(ap);

    extern int sys_ioctl(int, unsigned long, void *)
        __attribute__((weak));
    if (sys_ioctl) return sys_ioctl(fd, request, arg);

    switch (request) {
    case UIX_TIOCGWINSZ: {
        uix_winsize_t *ws = (uix_winsize_t *)arg;
        if (ws) { ws->ws_row=24; ws->ws_col=80;
                  ws->ws_xpixel=0; ws->ws_ypixel=0; }
        return 0;
    }
    case UIX_FIONBIO:
        return 0;
    case UIX_FIONREAD: {
        int *n = (int *)arg;
        if (n) *n = 0;
        return 0;
    }
    default:
        uix_errno = UIX_EINVAL;
        return -1;
    }
}
