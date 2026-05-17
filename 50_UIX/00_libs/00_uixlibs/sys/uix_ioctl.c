#include "uix_ioctl.h"
#include "../PoStd/uix_errno.h"
#include "../PoStd/uix_stdarg.h"

#if STUB
#include "../uix_sys.h"
#else
#include "../../../40_SystemCallInterface/uix_sys.h"
#endif

int uix_ioctl(int fd, unsigned long request, ...)
{
    uix_va_list ap;
    uix_va_start(ap, request);
    void *arg = uix_va_arg(ap, void *);
    uix_va_end(ap);

    //extern int sys_ioctl(int, unsigned long, void *)
    //    __attribute__((weak));
    if (SYS_IOCTL) return sys_ioctl(fd, request, arg);

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

/* ***This is End of file, there is no more line should be added after this line*** */
