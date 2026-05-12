/******************************** uix_if.c ********************************************/
#include "uix_if.h"
#include "../uix_string.h"
#include "../uix_errno.h"

unsigned int uix_if_nametoindex(const char *ifname)
{
    if (!ifname) { uix_errno = UIX_EINVAL; return 0; }
    /* Stub: loopback = 1 */
    if (uix_strcmp(ifname, "lo") == 0) return 1;
    uix_errno = UIX_ENXIO; return 0;
}

char *uix_if_indextoname(unsigned int idx, char *ifname)
{
    if (!ifname) { uix_errno = UIX_EINVAL; return NULL; }
    if (idx == 1) { uix_strcpy(ifname, "lo"); return ifname; }
    uix_errno = UIX_ENXIO; return NULL;
}

/* ***This is End of file, there is no more line should be added after this line*** */
