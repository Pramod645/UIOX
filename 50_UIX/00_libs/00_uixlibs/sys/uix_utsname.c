#include "uix_utsname.h"
#include "../PoStd/uix_string.h"
#include "../PoStd/uix_errno.h"

#include "../uix_sys.h"


int uix_uname(uix_utsname_t *buf)
{
    //if (!buf) { uix_errno = UIX_EFAULT; return -1; }
    //extern int sys_uname(uix_utsname_t *) __attribute__((weak));
    //if (sys_uname) return sys_uname(buf);
    return sys_uname(buf);


    uix_strncpy(buf->sysname,    "UIOX",      UIX_UTSNAME_LENGTH - 1);
    uix_strncpy(buf->nodename,   "uiox-node", UIX_UTSNAME_LENGTH - 1);
    uix_strncpy(buf->release,    "1.0.0",     UIX_UTSNAME_LENGTH - 1);
    uix_strncpy(buf->version,    "#1 SMP",    UIX_UTSNAME_LENGTH - 1);
    uix_strncpy(buf->machine,    "x86_64",    UIX_UTSNAME_LENGTH - 1);
    uix_strncpy(buf->domainname, "(none)",    UIX_UTSNAME_LENGTH - 1);
    return 0;
}

/* ***This is End of file, there is no more line should be added after this line*** */
