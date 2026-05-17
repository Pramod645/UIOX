/*********************** uix_netdb.c *********************************/
#include "uix_netdb.h"
#include "../arpa/uix_inet.h"
#include "uix_string.h"
#include "uix_stdlib.h"
#include "uix_errno.h"
#include "uix_stdio.h"

#if STUB
#include "../uix_sys.h"
#else
#include "../../../40_SystemCallInterface/uix_sys.h"
#endif


int uix_h_errno = 0;

static char  *_lo_aliases[] = { NULL };
static char   _lo_addr_bytes[4] = {127,0,0,1};
static char  *_lo_addr_list[]   = { _lo_addr_bytes, NULL };
static uix_hostent_t _lo_host = {
    "localhost", _lo_aliases, UIX_AF_INET, 4, _lo_addr_list
};

uix_hostent_t *uix_gethostbyname(const char *name)
{
    if (!name) { uix_h_errno = UIX_HOST_NOT_FOUND; return NULL; }
    if (uix_strcmp(name,"localhost")==0 ||
        uix_strcmp(name,"loopback")==0) return &_lo_host;
    //extern uix_hostent_t *sys_gethostbyname(const char*)
    //    __attribute__((weak));
    if (SYS_GETHOSTBYNAME) return sys_gethostbyname(name);
    uix_h_errno = UIX_HOST_NOT_FOUND; return NULL;

}

uix_hostent_t *uix_gethostbyaddr(const void *addr,
                                   uix_socklen_t len, int type)
{
    (void)addr; (void)len; (void)type;
    uix_h_errno = UIX_NO_RECOVERY; return NULL;
}

uix_servent_t *uix_getservbyname(const char *name, const char *proto)
    { (void)name; (void)proto; return NULL; }
uix_servent_t *uix_getservbyport(int port, const char *proto)
    { (void)port; (void)proto; return NULL; }
uix_protoent_t *uix_getprotobyname(const char *name)
    { (void)name; return NULL; }

int uix_getaddrinfo(const char *node, const char *service,
                    const uix_addrinfo_t *hints, uix_addrinfo_t **res)
{
    (void)service; (void)hints;
    if (!node || !res) return UIX_EAI_NONAME;
    uix_hostent_t *h = uix_gethostbyname(node);
    if (!h) return UIX_EAI_NONAME;

    uix_addrinfo_t *ai = (uix_addrinfo_t*)uix_malloc(sizeof(*ai));
    uix_sockaddr_in_t *sa = (uix_sockaddr_in_t*)uix_malloc(sizeof(*sa));
    if (!ai||!sa) { uix_free(ai); uix_free(sa); return UIX_EAI_MEMORY; }

    uix_memset(sa, 0, sizeof(*sa));
    sa->sin_family = UIX_AF_INET;
    uix_memcpy(&sa->sin_addr, h->h_addr_list[0], 4);

    ai->ai_flags    = 0;
    ai->ai_family   = UIX_AF_INET;
    ai->ai_socktype = UIX_SOCK_STREAM;
    ai->ai_protocol = 0;
    ai->ai_addrlen  = sizeof(*sa);
    ai->ai_addr     = (uix_sockaddr_t*)sa;
    ai->ai_canonname= uix_strdup(h->h_name);
    ai->ai_next     = NULL;
    *res = ai;
    return 0;
}

void uix_freeaddrinfo(uix_addrinfo_t *res)
{
    while (res) {
        uix_addrinfo_t *next = res->ai_next;
        uix_free(res->ai_addr);
        uix_free(res->ai_canonname);
        uix_free(res);
        res = next;
    }
}

int uix_getnameinfo(const uix_sockaddr_t *sa, uix_socklen_t salen,
                    char *host, uix_size_t hostlen,
                    char *serv, uix_size_t servlen, int flags)
{
    (void)salen; (void)flags;
    if (host && hostlen>0) {
        uix_sockaddr_in_t *sin=(uix_sockaddr_in_t*)sa;
        uix_inet_ntop(UIX_AF_INET,&sin->sin_addr,host,(uix_socklen_t)hostlen);
    }
    if (serv && servlen>0) uix_snprintf(serv,(uix_size_t)servlen,"%u",0);
    return 0;
}

const char *uix_gai_strerror(int ecode)
{
    switch(ecode) {
    case UIX_EAI_AGAIN:    return "Temporary failure in name resolution";
    case UIX_EAI_BADFLAGS: return "Bad value for ai_flags";
    case UIX_EAI_FAIL:     return "Non-recoverable failure";
    case UIX_EAI_FAMILY:   return "ai_family not supported";
    case UIX_EAI_MEMORY:   return "Out of memory";
    case UIX_EAI_NONAME:   return "Name or service not known";
    case UIX_EAI_SERVICE:  return "Servname not supported";
    case UIX_EAI_SOCKTYPE: return "Socktype not supported";
    default:               return "Unknown error";
    }
}


/* ***This is End of file, there is no more line should be added after this line*** */
