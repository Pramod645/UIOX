#include "uix_resource.h"
#include "../PoStd/uix_errno.h"
#include "../PoStd/uix_string.h"

static uix_rlimit_t rlimits[10] = {
    [UIX_RLIMIT_CPU]    = { UIX_RLIM_INFINITY, UIX_RLIM_INFINITY },
    [UIX_RLIMIT_FSIZE]  = { UIX_RLIM_INFINITY, UIX_RLIM_INFINITY },
    [UIX_RLIMIT_DATA]   = { UIX_RLIM_INFINITY, UIX_RLIM_INFINITY },
    [UIX_RLIMIT_STACK]  = { 8*1024*1024,        UIX_RLIM_INFINITY },
    [UIX_RLIMIT_CORE]   = { 0,                  UIX_RLIM_INFINITY },
    [UIX_RLIMIT_NOFILE] = { 128,                4096              },
    [UIX_RLIMIT_NPROC]  = { 64,                 1024              },
    [UIX_RLIMIT_AS]     = { UIX_RLIM_INFINITY, UIX_RLIM_INFINITY },
};

int uix_getrlimit(int resource, uix_rlimit_t *rlim)
{
    if (resource < 0 || resource >= 10)
        { uix_errno = UIX_EINVAL; return -1; }
    if (!rlim) { uix_errno = UIX_EFAULT; return -1; }
    *rlim = rlimits[resource];
    return 0;
}

int uix_setrlimit(int resource, const uix_rlimit_t *rlim)
{
    if (resource < 0 || resource >= 10)
        { uix_errno = UIX_EINVAL; return -1; }
    if (!rlim) { uix_errno = UIX_EFAULT; return -1; }
    if (rlim->rlim_cur > rlim->rlim_max)
        { uix_errno = UIX_EINVAL; return -1; }
    rlimits[resource] = *rlim;
    return 0;
}

int uix_getrusage(int who, uix_rusage_t *usage)
{
    if (!usage) { uix_errno = UIX_EFAULT; return -1; }
    uix_memset(usage, 0, sizeof(*usage));
    (void)who;
    return 0;
}

int uix_getpriority(int which, int who)
    { (void)which; (void)who; return 0; }
int uix_setpriority(int which, int who, int prio)
    { (void)which; (void)who; (void)prio; return 0; }

/* ***This is End of file, there is no more line should be added after this line*** */
