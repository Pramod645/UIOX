/*********************  uix_utime.c *******************************************/
#include "uix_utime.h"
#include "uix_errno.h"

#include "../uix_sys.h"

int uix_utime(const char *path, const uix_utimbuf_t *times)
{
    //extern int sys_utime(const char*, const uix_utimbuf_t*)
    //    __attribute__((weak));
    if (SYS_UTIME) return sys_utime(path, times);
    uix_errno = UIX_ENOSYS; return -1;

}

/* ***This is End of file, there is no more line should be added after this line*** */
