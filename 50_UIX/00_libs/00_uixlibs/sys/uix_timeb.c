/****************    uix_timeb.c ******************************************/
#include "uix_timeb.h"
#include "uix_time.h"

#include "../PoStd/uix_string.h"

int uix_ftime(uix_timeb_t *tp)
{
    if (!tp) return -1;
    uix_timeval_t tv;
    uix_gettimeofday(&tv, NULL);
    tp->time     = tv.tv_sec;
    tp->millitm  = (unsigned short)(tv.tv_usec / 1000);
    tp->timezone = 0;
    tp->dstflag  = 0;
    return 0;
}

/* ***This is End of file, there is no more line should be added after this line*** */
