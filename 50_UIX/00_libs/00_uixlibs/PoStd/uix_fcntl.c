#include "uix_fcntl.h"
#include "uix_errno.h"
#include "../sys/uix_stat.h"
//#include "../sys/uix_types.h"
#include "uix_stdarg.h"

#if STUB
#include "../uix_sys.h"
#else
#include "../../../40_SystemCallInterface/uix_sys.h"
#endif

int uix_open(const char *path, int flags, ...)
{
    uix_mode_t mode = 0;
    if (flags & UIX_O_CREAT) {
        uix_va_list ap; uix_va_start(ap, flags);
        mode = uix_va_arg(ap, uix_mode_t);
        uix_va_end(ap);
    }
    //extern int sys_open(const char *, int, uix_mode_t)
    //    __attribute__((weak));
    if (SYS_OPEN) return sys_open(path, flags, mode);
    uix_errno = UIX_ENOENT;
    return -1;

}

int uix_creat(const char *path, uix_mode_t mode)
{
    return uix_open(path, UIX_O_CREAT | UIX_O_WRONLY | UIX_O_TRUNC,
                    mode);
}

int uix_fcntl(int fd, int cmd, ...)
{
    uix_va_list ap; uix_va_start(ap, cmd);
    int arg = uix_va_arg(ap, int);
    uix_va_end(ap);

    //extern int sys_fcntl(int, int, int) __attribute__((weak));
    if (SYS_FCNTL) return sys_fcntl(fd, cmd, arg);

    switch (cmd) {
    case UIX_F_GETFD: return 0;
    case UIX_F_SETFD: return 0;
    case UIX_F_GETFL: return 0;
    case UIX_F_SETFL: return 0;
    default: uix_errno = UIX_EINVAL; return -1;
    }
}

/* ***This is End of file, there is no more line should be added after this line*** */
